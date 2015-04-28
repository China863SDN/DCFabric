/******************************************************************************
*                                                                             *
*   File Name   : meter-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-10           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef METER_MGR_H_
#define METER_MGR_H_

#include "gnflush-types.h"

INT4 add_meter_entry(gn_switch_t *sw, gn_meter_t *meter);
INT4 modify_meter_entry(gn_switch_t *sw, gn_meter_t *meter);
INT4 delete_meter_entry(gn_switch_t *sw, gn_meter_t *meter);

void clear_meter_entries(gn_switch_t *sw);

INT4 init_meter_mgr();
void fini_meter_mgr();

#endif /* METER_MGR_H_ */
