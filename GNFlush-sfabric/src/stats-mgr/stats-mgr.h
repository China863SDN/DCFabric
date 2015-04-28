/******************************************************************************
*                                                                             *
*   File Name   : stats-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef STATS_MGR_H_
#define STATS_MGR_H_

#include "gnflush-types.h"

#define MAX_RECORED 10

extern UINT4 g_switch_bandwidth;
extern UINT4 g_stats_mgr_interval;

void of10_proc_port_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts);
void of13_proc_port_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts);

void of10_proc_flow_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts);
void of13_proc_flow_stats(gn_switch_t *sw, UINT1 *stats, UINT2 counts);

INT4 init_stats_mgr();
void fini_stats_mgr();

#endif /* STATS_MGR_H_ */
