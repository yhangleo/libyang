cmake_minimum_required(VERSION 2.8.12)
project(libyang C)

include(GNUInstallDirs)

set(LIBYANG_DESCRIPTION "libyang is YANG data modelling language parser and toolkit written (and providing API) in C.")

# Correct RPATH usage on OS X
set(CMAKE_MACOSX_RPATH TRUE)

# set version
set(LIBYANG_MAJOR_VERSION 0)
set(LIBYANG_MINOR_VERSION 14)
set(LIBYANG_MICRO_VERSION 50)
set(LIBYANG_VERSION ${LIBYANG_MAJOR_VERSION}.${LIBYANG_MINOR_VERSION}.${LIBYANG_MICRO_VERSION})
set(LIBYANG_SOVERSION ${LIBYANG_MAJOR_VERSION}.${LIBYANG_MINOR_VERSION})

# set default build type if not specified by user
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE debug)
endif()

set(CMAKE_C_FLAGS         "${CMAKE_C_FLAGS} -Wall -Wextra")
set(CMAKE_C_FLAGS_RELEASE "-O2 -DNDEBUG")
set(CMAKE_C_FLAGS_PACKAGE "-g -O2 -DNDEBUG")
set(CMAKE_C_FLAGS_DEBUG   "-g -O0")

# options
if((CMAKE_BUILD_TYPE STREQUAL debug) OR (CMAKE_BUILD_TYPE STREQUAL Package))
    option(ENABLE_BUILD_TESTS "Build tests" ON)
    option(ENABLE_VALGRIND_TESTS "Build tests with valgrind" ON)
else()
    option(ENABLE_BUILD_TESTS "Build tests" OFF)
    option(ENABLE_VALGRIND_TESTS "Build tests with valgrind" OFF)
endif()
option(ENABLE_CALLGRIND_TESTS "Build performance tests to be run with callgrind" OFF)
option(ENABLE_LATEST_REVISIONS "Enable reusing of latest revisions of schemas" ON)
option(ENABLE_CACHE "Enable data caching for schemas (time-efficient at the cost of increased space-complexity)" OFF)

if (ENABLE_LATEST_REVISIONS)
    set(ENABLE_LATEST_REVISIONS_MACRO "/**\n * @brief Latest revisions of loaded schemas will be reused.\n */\n#define LY_ENABLED_LATEST_REVISIONS")
else()
    set(ENABLE_LATEST_REVISIONS_MACRO "/**\n * @brief Latest revisions of loaded schemas will not be reused and always reloaded.\n */")
endif()

if(ENABLE_CACHE)
    set(ENABLE_CACHE_MACRO "/**\n * @brief Cache of some temporary information will be used.\n */\n#define LY_ENABLED_CACHE")
else()
    set(ENABLE_CACHE_MACRO "/**\n * @brief Cache of some temporary information will not be used.\n */")
endif()

include_directories(${PROJECT_BINARY_DIR}/src ${PROJECT_SOURCE_DIR}/src)
configure_file(${PROJECT_SOURCE_DIR}/src/libyang.h.in ${PROJECT_BINARY_DIR}/src/libyang.h @ONLY)

if(PLUGINS_DIR)
    set(LIBYANG_EXT_PLUGINS_DIR ${PLUGINS_DIR})
else()
    set(LIBYANG_EXT_PLUGINS_DIR ${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/libyang)
endif()
configure_file(${PROJECT_SOURCE_DIR}/src/extensions_config.h.in ${PROJECT_BINARY_DIR}/src/extensions_config.h)

# include custom Modules
set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/CMakeModules/")

# setup bindings
set(GEN_LANGUAGE_BINDINGS 0 CACHE BOOL "Enable language bindings generation.")
set(GEN_CPP_BINDINGS 1 CACHE BOOL "Enable C++ bindings.")
# Python bindings depend on C++ bindings because of SWIG
set(GEN_PYTHON_BINDINGS 1 CACHE BOOL "Enable Python bindings.")
set(GEN_PYTHON_VERSION "3" CACHE STRING "Python version")
set(GEN_JAVASCRIPT_BINDINGS 0 CACHE BOOL "Enable JavaScript bindings.")

find_package(FLEX)
find_package(BISON)
find_program (DEB_BUILDER NAMES debuild)
find_program (RPM_BUILDER NAMES rpmbuild)
find_program (SED_TOOL NAMES sed)

if(NOT BISON_FOUND)
    message(WARNING "Missing Bison.\nYou won't be able to generate source codes from changed flex/bison files.\nCompiling libyang should still works fine.")
elseif (NOT FLEX_FOUND)
    message(WARNING "Missing Flex.\nYou won't be able to generate source codes from changed flex/bison files.\nCompiling libyang should still works fine.")
else()
    if (BISON_VERSION VERSION_LESS 3)
        set(EMPTYDIR "")
    else ()
        set(EMPTYDIR "%empty")
    endif()
    configure_file(${PROJECT_SOURCE_DIR}/src/yang.y.in ${PROJECT_BINARY_DIR}/src/yang.y)
    add_custom_target(bison
        WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}/src
        COMMAND bison -l -o parser_yang_bis.c --defines=parser_yang_bis.h ${PROJECT_BINARY_DIR}/src/yang.y
        COMMAND flex -8 -L -o parser_yang_lex.c --header-file=parser_yang_lex.h yang.l
        COMMAND ${SED_TOOL} -i 's/int yychar\;/int yychar\;\\nchar *s = NULL, *tmp_s = NULL, *ext_name = NULL\;\\nstruct lys_module *trg = NULL\;\\nstruct lys_node *tpdf_parent = NULL, *data_node = NULL\;\\nstruct lys_ext_instance_complex *ext_instance = NULL\;\\nint is_ext_instance\;\\nvoid *actual = NULL\;\\nenum yytokentype backup_type, actual_type = MODULE_KEYWORD\;\\nint64_t cnt_val = 0\;\\nint is_value = 0\;\\nvoid *yang_type = NULL\;/' parser_yang_bis.c
    )
endif()

if (NOT DEFINED ENV{TRAVIS_BRANCH})
    execute_process(COMMAND "git" "rev-parse" "--abbrev-ref" "HEAD"
                    OUTPUT_VARIABLE GIT_BRANCH
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    ERROR_QUIET
                )
    if (NOT GIT_BRANCH)
        set(ENV{TRAVIS_BRANCH} "master")
    else()
        if (GIT_BRANCH MATCHES "master|devel")
            set(ENV{TRAVIS_BRANCH} ${GIT_BRANCH})
        else()
            set(ENV{TRAVIS_BRANCH} "master")
        endif()
    endif()
    set(GIT_BRANCH $ENV{TRAVIS_BRANCH})
endif()

if ($ENV{TRAVIS_BRANCH} STREQUAL "master")
    set(PACKAGE_NAME "libyang")
    set(BRANCH "master")
    set(BUILD_TYPE "Package")
    set(CONFLICT_PACKAGE_NAME "libyang-experimental")
else ()
    set(PACKAGE_NAME "libyang-experimental")
    set(BRANCH "devel")
    set(BUILD_TYPE "debug")
    set(CONFLICT_PACKAGE_NAME "libyang")
endif()
# change version in config files
configure_file(${PROJECT_SOURCE_DIR}/packages/libyang.spec.in ${PROJECT_BINARY_DIR}/build-packages/libyang.spec)
configure_file(${PROJECT_SOURCE_DIR}/packages/libyang.dsc.in ${PROJECT_BINARY_DIR}/build-packages/libyang.dsc)
configure_file(${PROJECT_SOURCE_DIR}/packages/debian.control.in ${PROJECT_BINARY_DIR}/build-packages/debian.control @ONLY)
configure_file(${PROJECT_SOURCE_DIR}/packages/debian.rules.in ${PROJECT_BINARY_DIR}/build-packages/debian.rules)

if (NOT DEB_BUILDER)
    message(WARNING "Missing tools (devscripts, debhelper package) for building deb package.\nYou won't be able to generate deb package from source code.\nCompiling libyang should still works fine.")
else ()
    # target for local build deb package
    add_custom_target(build-deb
                      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                      COMMAND build-packages/local-deb.sh
    )
    configure_file(${PROJECT_SOURCE_DIR}/packages/local-deb.sh.in ${PROJECT_BINARY_DIR}/build-packages/local-deb.sh @ONLY)
endif()

if (NOT RPM_BUILDER)
    message(WARNING "Missing tools (rpm package) for building rpm package. \nYou won't be able to generate rpm package from source code.\nCompiling libyang should still works fine.")
else ()
    # target for local build rpm package
    string(REPLACE ${PROJECT_SOURCE_DIR} "." EXCLUDE_BUILD_DIR ${PROJECT_BINARY_DIR})
    add_custom_target(build-rpm
                      WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
                      COMMAND build-packages/local-rpm.sh
    )
    configure_file(${PROJECT_SOURCE_DIR}/packages/local-rpm.sh.in ${PROJECT_BINARY_DIR}/build-packages/local-rpm.sh @ONLY)
endif()

# check the supported platform
if(NOT UNIX)
    message(FATAL_ERROR "Only *nix like systems are supported.")
endif()

set(libsrc
    src/common.c
    src/context.c
    src/log.c
    src/dict.c
    src/resolve.c
    src/validation.c
    src/xml.c
    src/parser.c
    src/parser_yin.c
    src/parser_xml.c
    src/parser_json.c
    src/parser_yang_bis.c
    src/parser_yang_lex.c
    src/parser_yang.c
    src/tree_schema.c
    src/tree_data.c
    src/extensions.c
    src/printer.c
    src/xpath.c
    src/printer_yang.c
    src/printer_yin.c
    src/printer_xml.c
    src/printer_tree.c
    src/printer_info.c
    src/printer_json.c
    src/yang_types.c)

set(lintsrc
    tools/lint/main.c
    tools/lint/main_ni.c
    tools/lint/commands.c
    tools/lint/completion.c
    tools/lint/configuration.c
    linenoise/linenoise.c)

set(resrc
    tools/re/main.c)

set(yang2yinsrc
    tools/yang2yin/main.c)

set(headers
    src/tree_schema.h
    src/tree_data.h
    src/extensions.h
    src/xml.h
    src/dict.h)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
add_library(yangobj OBJECT ${libsrc})
add_library(yang SHARED $<TARGET_OBJECTS:yangobj>)
set_target_properties(yang PROPERTIES VERSION ${LIBYANG_VERSION} SOVERSION ${LIBYANG_SOVERSION})
set_target_properties(yang PROPERTIES COMPILE_FLAGS "-fvisibility=hidden")

# link math
target_link_libraries(yang m)

#link dl
target_link_libraries(yang ${CMAKE_DL_LIBS})

# find pthreads
set(CMAKE_THREAD_PREFER_PTHREAD TRUE)
find_package(Threads REQUIRED)
target_link_libraries(yang ${CMAKE_THREAD_LIBS_INIT})

# find PCRE library
find_package(PCRE REQUIRED)
include_directories(${PCRE_INCLUDE_DIRS})
target_link_libraries(yang ${PCRE_LIBRARIES})

install(TARGETS yang DESTINATION ${CMAKE_INSTALL_LIBDIR})
install(FILES ${headers} ${PROJECT_BINARY_DIR}/src/libyang.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/libyang)

find_package(PkgConfig)
if(PKG_CONFIG_FOUND)
    # generate and install pkg-config file
    configure_file("libyang.pc.in" "libyang.pc" @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/libyang.pc" DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
    # check that pkg-config includes the used path
    execute_process(COMMAND ${PKG_CONFIG_EXECUTABLE} --variable pc_path pkg-config RESULT_VARIABLE RETURN OUTPUT_VARIABLE PC_PATH ERROR_QUIET)
    if(RETURN EQUAL 0)
        string(REGEX MATCH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig" SUBSTR "${PC_PATH}")
        string(LENGTH "${SUBSTR}" SUBSTR_LEN)
        if(SUBSTR_LEN EQUAL 0)
            message(WARNING "pkg-config will not detect the new package after installation, adjust PKG_CONFIG_PATH using \"export PKG_CONFIG_PATH=\${PKG_CONFIG_PATH}:${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}/pkgconfig\".")
        endif()
    endif()
endif()

# generate doxygen documentation for libyang API
find_package(Doxygen)
if(DOXYGEN_FOUND)
    set(DOXYGEN_SKIP_DOT TRUE)
    add_custom_target(doc
            COMMAND ${DOXYGEN_EXECUTABLE} ${CMAKE_BINARY_DIR}/Doxyfile
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})
    configure_file(Doxyfile.in Doxyfile)
endif()

# clean cmake cache
add_custom_target(cclean
        COMMAND make clean
        COMMAND find . -iname '*cmake*' -not -name CMakeLists.txt -not -path './CMakeModules*' -exec rm -rf {} +
        COMMAND rm -rf Makefile Doxyfile
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR})

# YANG extensions plugins
add_subdirectory(src/extensions)

# yanglint
add_executable(yanglint ${lintsrc})
target_link_libraries(yanglint yang)
install(TARGETS yanglint DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES ${PROJECT_SOURCE_DIR}/tools/lint/yanglint.1 DESTINATION ${CMAKE_INSTALL_MANDIR}/man1)

#yangre
add_executable(yangre ${resrc})
target_link_libraries(yangre yang)
install(TARGETS yangre DESTINATION ${CMAKE_INSTALL_BINDIR})

# yang2yin
add_executable(yang2yin ${yang2yinsrc})

if(ENABLE_VALGRIND_TESTS)
    set(ENABLE_BUILD_TESTS ON)
endif()

if(ENABLE_BUILD_TESTS)
    find_package(CMocka 1.0.0)
    if(CMOCKA_FOUND)
        enable_testing()
        add_subdirectory(tests)
    endif(CMOCKA_FOUND)
endif(ENABLE_BUILD_TESTS)

if(GEN_LANGUAGE_BINDINGS AND GEN_CPP_BINDINGS)
    add_subdirectory(swig)
endif()

if(GEN_LANGUAGE_BINDINGS AND GEN_JAVASCRIPT_BINDINGS)
    include(swig/javascript/CMakeLists.txt)
endif()
