/******************************************************************************
 *                                                                             *
 *   File Name   : group-mgr.h           *
 *   Author      : greenet Administrator           *
 *   Create Date : 2015-3-10           *
 *   Version     : 1.0           *
 *   Function    : .           *
 *                                                                             *
 ******************************************************************************/

#ifndef GROUP_MGR_H_
#define GROUP_MGR_H_

#include "gnflush-types.h"

INT4 add_group_entry(gn_switch_t *sw, gn_group_t *group);
INT4 modify_group_entry(gn_switch_t *sw, gn_group_t *group);
INT4 delete_group_entry(gn_switch_t *sw, gn_group_t *group);

void clear_group_entries(gn_switch_t *sw);
void gn_group_free(gn_group_t *group);

INT4 init_group_mgr();
void fini_group_mgr();
#endif /* GROUP_MGR_H_ */
