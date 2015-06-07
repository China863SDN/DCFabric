/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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

/******************************************************************************
*                                                                             *
*   File Name   : l2.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-3           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "l2.h"
#include "timer.h"
#include "../topo-mgr/topo-mgr.h"
#include "../flow-mgr/flow-mgr.h"
#include "../tenant-mgr/tenant-mgr.h"
#include "gn_inet.h"
#include "mod-types.h"
#include "openflow-10.h"
#include "openflow-13.h"

static UINT2 g_l2_flow_entry_idle_time = 0;
static UINT2 g_l2_flow_entry_hard_time = 0;

static INT4 l2_install_flow(gn_switch_t *sw, UINT4 inport, UINT4 outport, UINT1 *dst_mac)
{
    flow_mod_req_info_t flow_mod_req;
    gn_flow_t *flow = NULL;
    gn_instruction_actions_t *instruction = NULL;
    gn_action_output_t *action_outport = NULL;

    memset(&sw->flowmod_helper, 0, sizeof(gn_flowmod_helper_t));
    flow = &sw->flowmod_helper.flow;
    strncpy(flow->creater, FLOW_L2_CREATER, sizeof(FLOW_L2_CREATER));
    flow->create_time = g_cur_sys_time.tv_sec;
    flow->table_id = 0;
    flow->idle_timeout = g_l2_flow_entry_idle_time;
    flow->hard_timeout = g_l2_flow_entry_hard_time;
    flow->priority = 1;
    flow->match.type = OFPMT_OXM;

    flow->match.oxm_fields.in_port = inport;
    flow->match.oxm_fields.mask |= (1 << OFPXMT_OFB_IN_PORT);

    memcpy(flow->match.oxm_fields.eth_dst, dst_mac, 6);
    flow->match.oxm_fields.mask |= (1 << OFPXMT_OFB_ETH_DST);

    instruction = &sw->flowmod_helper.instruction;
    instruction->type = OFPIT_APPLY_ACTIONS;
    instruction->next = flow->instructions;
    flow->instructions = (gn_instruction_t *)instruction;

    action_outport = &sw->flowmod_helper.action_output;
    action_outport->port = outport;
    action_outport->next = NULL;
    instruction->actions = (gn_action_t *)action_outport;

    flow_mod_req.xid = 0;
    flow_mod_req.buffer_id = 0xffffffff;
    flow_mod_req.out_port = 0xffffffff;
    flow_mod_req.out_group = 0xffffffff;
    flow_mod_req.command = OFPFC_ADD;
    flow_mod_req.flags = OFPFF13_SEND_FLOW_REM;
    flow_mod_req.flow = flow;

//        add_flow_entry(sw, flow);
    if(sw->ofp_version == OFP10_VERSION)
    {
        return sw->msg_driver.msg_handler[OFPT_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    }
    else
    {
        return sw->msg_driver.msg_handler[OFPT13_FLOW_MOD](sw, (UINT1 *)&flow_mod_req);
    }
}

void l2_flowmod_chain(UINT8 src_topo_id, UINT8 dst_topo_id, mac_user_t *user_src, mac_user_t *user_dst)
{
//    printf("%s  %d, %d\n", FN, src_topo_id, dst_topo_id);
//    UINT1 dpid[8];
    gn_switch_t *sw_pre = NULL;
    UINT4 port_pre = user_src->port;
    UINT4 port_nxt = 0;
    INT4 id_tmp = 0;

    if((src_topo_id < 0) || (dst_topo_id < 0))
    {
        goto END;
    }

    id_tmp = g_short_path[src_topo_id][dst_topo_id];
    if(NO_PATH == id_tmp)
    {
        goto END;
    }

    id_tmp = g_short_path[dst_topo_id][src_topo_id];
    if(NO_PATH == id_tmp)
    {
        goto END;
    }

    id_tmp = src_topo_id;
    do
    {
        src_topo_id = id_tmp;
        id_tmp = g_short_path[src_topo_id][dst_topo_id];
        if(NO_PATH == id_tmp)
        {
            goto END;
        }

        //��ǰ������
        sw_pre = g_adac_matrix.sw[src_topo_id][id_tmp];
        if((NULL == sw_pre) || (0 == sw_pre->state))
        {
            goto END;
        }

        //��ǰ����������
        port_nxt = g_adac_matrix.src_port[src_topo_id][id_tmp];

        //�����ڽ������·�˫������
        l2_install_flow(sw_pre, port_nxt, port_pre, user_src->mac);
        l2_install_flow(sw_pre, port_pre, port_nxt, user_dst->mac);
        port_pre = g_adac_matrix.src_port[id_tmp][src_topo_id];

    }while (dst_topo_id != g_short_path[src_topo_id][dst_topo_id]);

    //flow mod in the last sw
    sw_pre = user_dst->sw;
    port_nxt = user_dst->port;
    l2_install_flow(sw_pre, port_nxt, port_pre, user_src->mac);
    l2_install_flow(sw_pre, port_pre, port_nxt, user_dst->mac);

END:
    return;
}

void l2_proc(gn_switch_t *sw, mac_user_t *user_src, mac_user_t *user_dst, packet_in_info_t *packet_in_info)
{
    packout_req_info_t pakout_req;
    UINT4 outport = OFPP13_FLOOD;
    if(NULL != user_dst)
    {
        //����Ƿ�����ͬһ�⻧
        if(user_dst->tenant_id != user_src->tenant_id)
        {
            tenant_send_flow_mod_l2(user_src->sw, user_src->mac, user_dst->mac, OFPFC_ADD);
            return;
        }

        //users at the same switch
        if(user_src->sw->index == user_dst->sw->index)
        {
            UINT4 port_pre = user_src->port;
            UINT4 port_nxt = user_dst->port;
            l2_install_flow(user_src->sw, port_nxt, port_pre, user_src->mac);
            l2_install_flow(user_src->sw, port_pre, port_nxt, user_dst->mac);
        }
        else
        {
            l2_flowmod_chain(user_src->sw->index, user_dst->sw->index, user_src, user_dst);
        }
    }

    pakout_req.buffer_id = 0xffffffff;
    pakout_req.inport = packet_in_info->inport;
    pakout_req.outport = outport;
    pakout_req.max_len = 0xff;
    pakout_req.xid = packet_in_info->xid;
    pakout_req.data_len = packet_in_info->data_len;
    pakout_req.data = packet_in_info->data;

    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT_PACKET_OUT](sw, (UINT1 *)&pakout_req);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.msg_handler[OFPT13_PACKET_OUT](sw, (UINT1 *)&pakout_req);
    }
}

INT4 init_l2()
{
    INT1 *value = NULL;

    value = get_value(g_controller_configure, "[controller]", "l2_flow_entry_idle_time");
    g_l2_flow_entry_idle_time = (NULL == value) ? 200 : atoi(value);

    value = get_value(g_controller_configure, "[controller]", "l2_flow_entry_hard_time");
    g_l2_flow_entry_hard_time = (NULL == value) ? 200 : atoi(value);

    return GN_OK;
}

void fini_l2()
{
    return;
}
