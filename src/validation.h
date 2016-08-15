/**
 * @file validation.h
 * @author Radek Krejci <rkrejci@cesnet.cz>
 * @brief Data tree validation for libyang
 *
 * Copyright (c) 2015 CESNET, z.s.p.o.
 *
 * This source code is licensed under BSD 3-Clause License (the "License").
 * You may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef LY_VALIDATION_H_
#define LY_VALIDATION_H_

#include "libyang.h"
#include "resolve.h"
#include "tree_data.h"

/**
 * @brief Check, that the data node of the given schema node can even appear in a data tree.
 *
 * Checks included:
 * - data node is not disabled via if-features
 * - data node's when-stmt condition - if false, EXIT_FAILURE is returned and ly_vecode is set to LYE_NOCOND,
 * - data node is not status in case of edit-config content (options includes LYD_OPT_EDIT)
 * - data node is in correct place (options includes LYD_OPT_RPC or LYD_OPT_RPCREPLY), since elements order matters
 *   in RPCs and RPC replies.
 *
 * @param[in] node Data tree node to be checked.
 * @param[in] options Parser options, see @ref parseroptions.
 * @param[out] unres Structure to store unresolved items into. Can not be NULL.
 * @return EXIT_SUCCESS or EXIT_FAILURE with ly_errno set.
 */
int lyv_data_context(const struct lyd_node *node, int options, struct unres_data *unres);

/**
 * @brief Validate if the node's content is valid in the context it is placed.
 *
 * Expects that the node is already interconnected to the target tree and all its children
 * are already resolved. All currently connected siblings are included to the tests.
 *
 * @param[in] node Data tree node to be checked.
 * @param[in] options Parser options, see @ref parseroptions.
 * @param[out] unres Structure to store unresolved items into. Cannot be NULL.
 * @return EXIT_SUCCESS or EXIT_FAILURE with set ly_errno. If EXIT_FAILURE is returned
 * but ly_errno is not set, the issue was internally resolved and caller is supposed to
 * unlink and free the node and continue;
 */
int lyv_data_content(struct lyd_node *node, int options, struct unres_data *unres);

/**
 * @brief check for list/leaflist uniqueness.
 *
 * Function is used by lyv_data_context for inner lists/leaflists. Due to optimization, the function
 * is used separatedly for the top-level lists/leaflists.
 *
 * @param[in] node List/leaflist node to be checked.
 * @param[in] start First sibling of the \p node for searching for other instances of the same list/leaflist.
 *                  Used for optimization, but can be NULL and the first sibling will be found.
 */
int lyv_data_unique(struct lyd_node *node, struct lyd_node *start);

/**
 * @brief Validate if the \p node has a sibling from another choice's case. It can report an error or automatically
 * remove the nodes from other case than \p node.
 *
 * @param[in] node Data tree node to be checked.
 * @param[in] schemanode Alternative to \p node (node is preferred), schema of the (potential) node
 * @param[in] first_sibling The first sibling of the node where the searching will always start.
 * @param[in] autodelete Flag to select if the conflicting nodes are supposed to be removed or reported
 * @param[in] nodel Exception for autodelete, if the \p nodel node would be removed, error is reported instead.
 * @return EXIT_SUCCESS or EXIT_FAILURE with set ly_errno.
 */
int lyv_multicases(struct lyd_node *node, struct lys_node *schemanode, struct lyd_node *first_sibling, int autodelete,
                   struct lyd_node *nodel);

#endif /* LY_VALIDATION_H_ */
