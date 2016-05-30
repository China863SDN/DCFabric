/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

/*
 * fabric_state.c
 *
 *  Created on: Jan 18, 2016
 *  Author: BNC zgzhao
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 */

#include "fabric_stats.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "timer.h"

const UINT4 FABRIC_STATS_XID = 0xFFFFFFFF;

stats_fabric_flow_t *g_fabric_flow_list = NULL;
sem_t stats_fabric_flow_sem;

void init_fabric_stats()
{
    g_fabric_flow_list = NULL;
	sem_init(&stats_fabric_flow_sem, 0, 0);
}


void fini_fabric_stats()
{
    clear_fabric_stats();
    sem_destroy(&stats_fabric_flow_sem);
}


void clear_fabric_stats()
{
    gn_flow_t *flow = NULL;
    stats_fabric_flow_t *stats = NULL;
    while (NULL != g_fabric_flow_list)
    {
        stats = g_fabric_flow_list;
        while (NULL != stats->flow)
        {
            flow = stats->flow;
            stats->flow = stats->flow->next;
            gn_free((void **)(&flow));
        }

        g_fabric_flow_list = g_fabric_flow_list->next;
        gn_free((void **)(&stats));
    }
    
    g_fabric_flow_list = NULL;
}


stats_fabric_flow_t *query_fabric_all_flow_entries()
{
    if (NULL == g_server.switches)
    {
        return NULL;
    }
    
    gn_switch_t *sw = NULL;
    UINT4 i = 0;
    for (; i < g_server.max_switch; i++)
    {
        if (1 == g_server.switches[i].state)
        {
            sw = &g_server.switches[i];
            query_fabric_flow_entries_by_switch(sw, OFPG_ANY, OFPP13_ANY, OFPTT_ALL);
        }
    }

    return g_fabric_flow_list;
}


stats_fabric_flow_t *query_fabric_flow_entries_by_switch(gn_switch_t *sw, UINT4 group, UINT4 port, UINT1 id)
{

    flow_stats_req_data_t stats_req;
    stats_req.out_group = group;
    stats_req.out_port = port;
    stats_req.table_id = id;

    stats_req_info_t req;
    req.type = OFPMP_FLOW;
    req.flags = 0;
    req.data = (UINT1 *)&stats_req;
    req.xid = FABRIC_STATS_XID;
    sw->msg_driver.msg_handler[OFPT13_MULTIPART_REQUEST](sw, (UINT1 *)&req);
    
    //wait responses, timeout is 10s
    struct timeval val;
    gettimeofday(&val, NULL);
    struct timespec spec;
    spec.tv_sec = val.tv_sec + 10;
    spec.tv_nsec = val.tv_usec * 1000;
    sem_timedwait(&stats_fabric_flow_sem, &spec);

    return g_fabric_flow_list;
}


void update_fabric_flow_entries(gn_switch_t *sw, UINT1 *entry, UINT2 length, UINT2 flag, UINT4 xid)
{
    if (xid != FABRIC_STATS_XID)
    {
        return;
    }

    struct ofp13_flow_stats *flow_stats = (struct ofp13_flow_stats*)entry;
    UINT2 pos = 0;
    gn_flow_t *flow_entry = NULL;
    gn_flow_t *list = NULL;
    while (pos < length)
    {   
        flow_entry = (gn_flow_t*)gn_malloc(sizeof(gn_flow_t));
        flow_entry->priority = ntohs(flow_stats->priority);
        flow_entry->table_id = flow_stats->table_id;
        flow_entry->idle_timeout = ntohs(flow_stats->idle_timeout);
        flow_entry->hard_timeout = ntohs(flow_stats->hard_timeout);
        flow_entry->stats.timestamp = g_cur_sys_time.tv_sec;
        flow_entry->stats.duration_sec = ntohl(flow_stats->duration_sec);
        flow_entry->stats.byte_count = gn_ntohll(flow_stats->byte_count);
        flow_entry->stats.packet_count = gn_ntohll(flow_stats->packet_count);

        //match
        UINT2 oxm_tot_len = ntohs(flow_stats->match.length);
        UINT2 oxm_len = 4;
        UINT1 *oxm = (UINT1 *)(flow_stats->match.oxm_fields);
        UINT2 oxm_size = 0;
        while(ALIGN_8(oxm_len) < oxm_tot_len)
        {
            sw->msg_driver.convertter->oxm_convertter((UINT1 *)oxm, &(flow_entry->match.oxm_fields));
            oxm_size = sizeof(struct ofp_oxm_header) + ((struct ofp_oxm_header *)oxm)->length;
            oxm += oxm_size;
            oxm_len += oxm_size;
        }
        
        //vlan_vid´¦Àí
        if (flow_entry->match.oxm_fields.vlan_vid & OFPVID_PRESENT)
        {
            flow_entry->match.oxm_fields.vlan_vid ^= OFPVID_PRESENT;
        }

        pos += ntohs(flow_stats->length);
        flow_stats = (struct ofp13_flow_stats*)(entry + pos);

        flow_entry->next = list;
        list= flow_entry;
        
    }

    stats_fabric_flow_t *tmp = (stats_fabric_flow_t *)gn_malloc(sizeof(stats_fabric_flow_t));
    tmp->sw = sw;
    tmp->flow = list;
    tmp->next = g_fabric_flow_list;
    g_fabric_flow_list = tmp;

    //response has been all arrived
    if (0 == flag)
    {
        sem_post(&stats_fabric_flow_sem);
    }
}
