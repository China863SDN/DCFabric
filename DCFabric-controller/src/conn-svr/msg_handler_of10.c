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
*   File Name   : msg_handler_of10.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-13           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "msg_handler.h"
#include "conn-svr.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "gn_inet.h"
#include "forward-mgr.h"
#include "../stats-mgr/stats-mgr.h"
#include "../flow-mgr/flow-mgr.h"
#include "openstack/openstack_host.h"
#include "fabric_flows.h"

convertter_t of10_convertter;
msg_handler_t of10_message_handler[OFP10_MAX_MSG];


static gn_port_t *of10_port_convertter(UINT1 *of_port, gn_port_t *new_port)
{
    const struct ofp_phy_port *opp = (struct ofp_phy_port *)of_port;
    memset(new_port, 0, sizeof(gn_port_t));

    new_port->port_no = ntohs(opp->port_no);
    new_port->config = ntohl(opp->config);
    new_port->state = ntohl(opp->state);
    new_port->curr = ntohl(opp->curr);
    new_port->advertised = ntohl(opp->advertised);
    new_port->supported = ntohl(opp->supported);
    new_port->peer = ntohl(opp->peer);
    new_port->config = ntohl(opp->config);
    new_port->state = ntohl(opp->state);

    memcpy(new_port->name, opp->name, OFP_MAX_PORT_NAME_LEN);
    memcpy(new_port->hw_addr, opp->hw_addr, OFP_ETH_ALEN);

    return new_port;
}

static UINT1 of10_oxm_convertter(UINT1 *of_match, gn_oxm_t *gn_oxm)
{
    const struct ofp_match *opf = (struct ofp_match *)of_match;

    UINT4 wildcards = htonl(opf->wildcards);

    memset(gn_oxm, 0x0, sizeof(gn_oxm_t));

    if(!(wildcards & OFPFW_IN_PORT))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IN_PORT);
        gn_oxm->in_port = ntohs(opf->in_port);
    }

    if(!(wildcards & OFPFW_DL_DST))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_DST);
        memcpy(gn_oxm->eth_dst, opf->dl_dst, OFP_ETH_ALEN);
    }

    if(!(wildcards & OFPFW_DL_SRC))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_SRC);
        memcpy(gn_oxm->eth_src, opf->dl_src, OFP_ETH_ALEN);
    }

    if(!(wildcards & OFPFW_DL_TYPE))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_ETH_TYPE);
        gn_oxm->eth_type = ntohs(opf->dl_type);
    }

    if(!(wildcards & OFPFW_DL_VLAN))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_VLAN_VID);
        gn_oxm->vlan_vid = ntohs(opf->dl_vlan);
    }

    if(!(wildcards & OFPFW_DL_VLAN_PCP))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_VLAN_PCP);
        gn_oxm->vlan_pcp = opf->dl_vlan_pcp;
    }

    if(!(wildcards & OFPFW_NW_TOS))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_DSCP);
        gn_oxm->ip_dscp = opf->nw_tos;
    }

    if(!(wildcards & OFPFW_NW_PROTO))
    {
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IP_PROTO);
        gn_oxm->ip_proto = opf->nw_proto;

        if(gn_oxm->ip_proto == IPPROTO_TCP)
        {
            if(!(wildcards & OFPFW_TP_SRC))
            {
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_TCP_SRC);
                gn_oxm->tcp_src = ntohs(opf->tp_src);
            }

            if(!(wildcards & OFPFW_TP_SRC))
            {
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_TCP_DST);
                gn_oxm->tcp_dst = ntohs(opf->tp_dst);
            }
        }
        else if(gn_oxm->ip_proto == IPPROTO_UDP)
        {
            if(!(wildcards & OFPFW_TP_SRC))
            {
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_UDP_SRC);
                gn_oxm->udp_src = ntohs(opf->tp_src);
            }

            if(!(wildcards & OFPFW_TP_SRC))
            {
                gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_UDP_DST);
                gn_oxm->udp_dst = ntohs(opf->tp_dst);
            }
        }
    }

    if(!(wildcards & OFPFW_NW_SRC_MASK))
    {
        gn_oxm->ipv4_src = ntohl(opf->nw_src);
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC);
//        if(wildcards & (mask_set << OFPXMT_OFB_IPV4_SRC_PREFIX))
//        {
//
//        }
    }

    if(!(wildcards & OFPFW_NW_DST_MASK))
    {
        gn_oxm->ipv4_dst = ntohl(opf->nw_dst);
        gn_oxm->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST);
//        if(wildcards & (mask_set << OFPXMT_OFB_IPV4_SRC_PREFIX))
//        {
//
//        }
    }

    return 0;
}

convertter_t of10_convertter =
{
    .port_convertter = of10_port_convertter,
    .oxm_convertter = of10_oxm_convertter
};

static INT4 of10_msg_hello(gn_switch_t *sw, UINT1 *of_msg)
{
    if (of_msg)
	{
    	sw->msg_driver.msg_handler[OFPT_FEATURES_REQUEST](sw, of_msg);
	}
	else
	{
		UINT2 total_len = sizeof(struct ofp_hello);
	    init_sendbuff(sw, OFP10_VERSION, OFPT_HELLO, total_len, 0);
	    send_of_msg(sw, total_len);
	}

    return GN_OK;
}

static INT4 of10_msg_error(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp_error_msg *ofp_err = (struct ofp_error_msg *)of_msg;

    printf("%s: switch 0x%llx sent error type %hu code %hu     ", FN,
               sw->dpid, ntohs(ofp_err->type), ntohs(ofp_err->code));

    switch(ntohs(ofp_err->type))
    {
        case OFPET_HELLO_FAILED:
             printf("Hello failed");
             break;

        case OFPET_BAD_REQUEST:
             {
                 printf("Bad request");
                 break;
             }

        case OFPET_BAD_ACTION:
             printf("Bad action");
             break;

        case OFPET_FLOW_MOD_FAILED:
             printf("Flow mod failed");
             break;

        case OFPET_PORT_MOD_FAILED:
             printf("Port mod failed");
             break;

        case OFPET_QUEUE_OP_FAILED:
             printf("Queue operation failed");
             break;

        default:
            break;
    }

    return GN_OK;
}

INT4 of10_msg_echo_request(gn_switch_t *sw, UINT1 *of_msg)
{
    if (NULL == of_msg) 
    {
        UINT2 total_len = sizeof(struct ofp_header);
        init_sendbuff(sw, OFP10_VERSION, OFPT_ECHO_REQUEST, total_len, DEFAULT_TRANSACTION_XID);
        return send_of_msg(sw, total_len);
    }
    else
    {
        return sw->msg_driver.msg_handler[OFPT_ECHO_REPLY](sw, of_msg);
    }
}

static INT4 of10_msg_echo_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    //收到心跳响应
    if (DEFAULT_TRANSACTION_XID == ((struct ofp_header*)of_msg)->xid) 
    {
        pthread_mutex_lock(&sw->sock_state_mutex);
        sw->sock_state = 1;
        pthread_mutex_unlock(&sw->sock_state_mutex);
        return GN_OK;
    }
    else
    {
        UINT2 total_len = sizeof(struct ofp_header);
        init_sendbuff(sw, OFP10_VERSION, OFPT_ECHO_REPLY, total_len, 0);
        return send_of_msg(sw, total_len);
    }
}


static INT4 of10_msg_vendor(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of10_msg_features_request(gn_switch_t *sw, UINT1 *of_msg)
{
    UINT2 total_len = sizeof(struct ofp_header);
    init_sendbuff(sw, OFP10_VERSION, OFPT_FEATURES_REQUEST, total_len, 0);
    send_of_msg(sw, total_len);
    return GN_OK;
}

static INT4 of10_msg_features_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    stats_req_info_t stats_req_info;
    INT4 n_ports, i, port = 0;
    gn_port_t new_sw_ports;

    struct ofp_switch_features *osf = (struct ofp_switch_features *)of_msg;

    n_ports = (ntohs(osf->header.length) - offsetof(struct ofp_switch_features, ports)) / sizeof(struct ofp_phy_port);

    sw->dpid = gn_ntohll(osf->datapath_id);
    sw->n_buffers = ntohl(osf->n_buffers);
    sw->n_tables = osf->n_tables;
    sw->capabilities = ntohl(osf->capabilities);

    sw->msg_driver.msg_handler[OFPT_SET_CONFIG](sw, of_msg);
    sw->msg_driver.msg_handler[OFPT_GET_CONFIG_REQUEST](sw, of_msg);

    stats_req_info.flags = 0;
    stats_req_info.xid = 0;
    stats_req_info.type = OFPST_DESC;
    sw->msg_driver.msg_handler[OFPT_STATS_REQUEST](sw, (UINT1 *)&stats_req_info);

    stats_req_info.type = OFPST_AGGREGATE;
    sw->msg_driver.msg_handler[OFPT_STATS_REQUEST](sw, (UINT1 *)&stats_req_info);

    if(n_ports == 0)
    {
        return GN_ERR;
    }

    if(n_ports > MAX_PORTS)
    {
        n_ports = MAX_PORTS;
    }

    port = 0;
    for(i=0; i <n_ports; i++)
    {
        (sw->msg_driver.convertter->port_convertter)((UINT1 *)&osf->ports[i], &new_sw_ports);

        //默认最大速率1000Mbps
        new_sw_ports.stats.max_speed = 1000000;  //1073741824 = 1024^3, 1048576 = 1024^2
        if (new_sw_ports.port_no == OFPP_LOCAL)
        {
            sw->lo_port = new_sw_ports;
            continue;
        }

        sw->ports[port] = new_sw_ports;
        port++;
    }

    sw->n_ports = port; //不包括lo

    {
        UINT1 dpid[8];
        ulli64_to_uc8(sw->dpid, dpid);
        LOG_PROC("INFO", "New Openflow10 switch [%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x] connected: ip[%s:%d], dpid:%llu", dpid[0],
                dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7], inet_htoa(ntohl(sw->sw_ip)), ntohs(sw->sw_port), sw->dpid);
    }
    return GN_OK;
}

static INT4 of10_msg_get_config_request(gn_switch_t *sw, UINT1 *of_msg)
{
    UINT2 total_len = sizeof(struct ofp_header);
    init_sendbuff(sw, OFP10_VERSION, OFPT_GET_CONFIG_REQUEST, total_len, 0);
    send_of_msg(sw, total_len);
    return GN_OK;
}

static INT4 of10_msg_get_config_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    sw->msg_driver.msg_handler[OFPT_BARRIER_REQUEST](sw, of_msg);
    return GN_OK;
}

static INT4 of10_msg_set_config(gn_switch_t *sw, UINT1 *of_msg)
{
    UINT2 total_len = sizeof(struct ofp10_switch_config);
    UINT1 * data = init_sendbuff(sw, OFP10_VERSION, OFPT_SET_CONFIG, total_len, 0);

    struct ofp10_switch_config *of_config = (struct ofp10_switch_config*)data;
    of_config->flags = htons(0x03);
    of_config->miss_send_len = htons(1500);    /* Max bytes of new flow that datapath should send to the controller. */

    send_of_msg(sw, total_len);
    return GN_OK;
}

static INT4 of10_msg_packet_in(gn_switch_t *sw, UINT1 *of_msg)
{
    packet_in_info_t packet_in_info;
    struct ofp_packet_in *of_pkt = (struct ofp_packet_in *)of_msg;

    packet_in_info.xid = ntohl(of_pkt->header.xid);
    packet_in_info.buffer_id = ntohl(of_pkt->buffer_id);
    packet_in_info.inport = (UINT4)ntohs(of_pkt->in_port);
    packet_in_info.data_len = ntohs(of_pkt->header.length) - offsetof(struct ofp_packet_in, data);
    packet_in_info.data = of_pkt->data;
    return packet_in_process(sw, &packet_in_info);
}

static INT4 of10_msg_flow_removed(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp_flow_removed *ofp_fr = (struct ofp_flow_removed *)of_msg;
    gn_flow_t flow;

    flow.priority = ntohs(ofp_fr->priority);
    flow.table_id = 0;
    sw->msg_driver.convertter->oxm_convertter((UINT1 *)&(ofp_fr->match), &(flow.match.oxm_fields));

    return flow_entry_timeout(sw, &flow);
}

static INT4 of10_msg_port_status(gn_switch_t *sw, UINT1 *of_msg)
{
    INT4 port;
    gn_port_t new_sw_ports;
    struct ofp_port_status *ops = (struct ofp_port_status *)of_msg;

    if (ops->reason == OFPPR_ADD)
    {
        LOG_PROC("INFO", "New port found: %s", ops->desc.name);
        of10_port_convertter((UINT1 *)&ops->desc, &new_sw_ports);

        set_fabric_host_port_portno(new_sw_ports.hw_addr, new_sw_ports.port_no);

        //默认最大速率1000Mbps
        new_sw_ports.stats.max_speed = 1000000;  //1073741824 = 1024^3, 1048576 = 1024^2
        sw->ports[sw->n_ports] = new_sw_ports;
        sw->n_ports++;

        return GN_OK;
    }
    else if (ops->reason == OFPPR_DELETE)
    {
        LOG_PROC("INFO", "Port deleted: %s", ops->desc.name);
    }
    else if (ops->reason == OFPPR_MODIFY)
    {
        LOG_PROC("INFO", "Port state change: %s[new state: %d]", ops->desc.name, ntohl(ops->desc.state));
    }

    //删除目的转发口down掉的流表
//    l2_del_flowentry_by_portno(sw, ntohl(ops->desc.port_no));

    //更新拓扑
    for (port = 0; port < sw->n_ports; port++)
    {
        if (sw->ports[port].port_no == ntohl(ops->desc.port_no))
        {
            sw->ports[port].state = ntohl(ops->desc.state);
            free(sw->neighbor[port]);
            sw->neighbor[port] = NULL;
            break;
        }
    }

    return GN_OK;
}

static INT4 of10_msg_packet_out(gn_switch_t *sw, UINT1 *pktout_req)
{
    packout_req_info_t *packout_req_info = (packout_req_info_t *)pktout_req;
    UINT2 total_len = sizeof(struct ofp_packet_out) + sizeof(struct ofp_action_output);
    if (packout_req_info->buffer_id == OFP_NO_BUFFER)
    {
        total_len += packout_req_info->data_len;
    }

    UINT1 *data = init_sendbuff(sw, OFP10_VERSION, OFPT_PACKET_OUT, total_len, packout_req_info->xid);

    struct ofp_packet_out *of10_out = (struct ofp_packet_out*)data;
    of10_out->buffer_id   = htonl(packout_req_info->buffer_id);
    of10_out->in_port     = htons(packout_req_info->inport);
    of10_out->actions_len = htons(sizeof(struct ofp_action_output));

    struct ofp_action_output *ofp10_act = (struct ofp_action_output *)of10_out->actions;
    ofp10_act->type    = htons(OFPAT_OUTPUT);
    ofp10_act->len     = htons(sizeof(struct ofp_action_output));
    ofp10_act->port    = htons(packout_req_info->outport);
    ofp10_act->max_len = htons(packout_req_info->max_len);

    if (packout_req_info->buffer_id == OFP_NO_BUFFER)
    {
        memcpy(of10_out->data + sizeof(struct ofp_action_output), packout_req_info->data, packout_req_info->data_len);
    }

    return send_of_msg(sw, total_len);
}

static void of10_add_match(struct ofp_match *match, gn_oxm_t *oxm_fields)
{
    match->wildcards |= ((MASK_SET << 22) - 1);
    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PORT))
    {
        match->wildcards &= ~(OFPFW_IN_PORT);
        match->in_port = htons(oxm_fields->in_port);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_DST))
    {
        match->wildcards &= ~(OFPFW_DL_DST);
        memcpy(match->dl_dst, oxm_fields->eth_dst, OFP_ETH_ALEN);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_SRC))
    {
        match->wildcards &= ~(OFPFW_DL_SRC);
        memcpy(match->dl_src, oxm_fields->eth_src, OFP_ETH_ALEN);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_TYPE))
    {
        match->wildcards &= ~(OFPFW_DL_TYPE);
        match->dl_type = htons(oxm_fields->eth_type);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_VID))
    {
        match->wildcards &= ~(OFPFW_DL_VLAN);
        match->dl_vlan = htons(oxm_fields->vlan_vid);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_PCP))
    {
        match->wildcards &= ~(OFPFW_DL_VLAN_PCP);
        match->dl_vlan_pcp = oxm_fields->vlan_pcp;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_DSCP))
    {
        match->wildcards &= ~(OFPFW_NW_TOS);
        match->nw_tos = oxm_fields->ip_dscp;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_PROTO))
    {
        match->wildcards &= ~(OFPFW_NW_PROTO);
        match->nw_proto = oxm_fields->ip_proto;
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC))
    {
        match->nw_src = htonl(oxm_fields->ipv4_src);
        match->wildcards &= ~(OFPFW_NW_SRC_MASK);
//        if(oxm_fields->mask & (mask_set << OFPXMT_OFB_IPV4_SRC_PREFIX))
//        {
//
//        }
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST))
    {
        match->nw_dst = htonl(oxm_fields->ipv4_dst);
        match->wildcards &= ~(OFPFW_NW_DST_MASK);
//        if(oxm_fields->mask & (mask_set << OFPXMT_OFB_IPV4_SRC_PREFIX))
//        {
//
//        }
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_SRC))
    {
        match->wildcards &= ~(OFPFW_TP_SRC);
        match->tp_src = htons(oxm_fields->tcp_src);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_DST))
    {
        match->wildcards &= ~(OFPFW_TP_DST);
        match->tp_dst = htons(oxm_fields->tcp_dst);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_SRC))
    {
        match->wildcards &= ~(OFPFW_TP_SRC);
        match->tp_src = htons(oxm_fields->tcp_src);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_DST))
    {
        match->wildcards &= ~(OFPFW_TP_DST);
        match->tp_dst = htons(oxm_fields->tcp_dst);
    }

    match->wildcards = htonl(match->wildcards);
}

static UINT2 of10_add_action(UINT1 *buf, gn_action_t *action)
{
    UINT2 action_len = 0;
    UINT1 *act = buf;
    gn_action_t *p_action = action;

    while(p_action)
    {
        switch(p_action->type)
        {
            case OFPAT13_OUTPUT:
            {
                gn_action_output_t *p_action_output = (gn_action_output_t *)p_action;
                struct ofp_action_output *of_action_out = (struct ofp_action_output *)act;
                of_action_out->type = htons(OFPAT_OUTPUT);
                of_action_out->len = htons(8);
                of_action_out->port = htons(p_action_output->port);
                of_action_out->max_len = htons(p_action_output->max_len);
                action_len += 8;
                act += 8;
                break;
            }
            case OFPAT13_SET_FIELD:
            {
                gn_action_set_field_t *p_action_set_field = (gn_action_set_field_t *)p_action;
                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_VLAN_VID))
                {
                    struct ofp_action_vlan_vid *p_action_vlan_vid = (struct ofp_action_vlan_vid *)act;
                    p_action_vlan_vid->type = htons(OFPAT_SET_VLAN_VID);
                    p_action_vlan_vid->len = htons(8);
                    p_action_vlan_vid->vlan_vid = htons(p_action_set_field->oxm_fields.vlan_vid);
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_VLAN_PCP))
                {
                    struct ofp_action_vlan_pcp *p_action_vlan_pcp = (struct ofp_action_vlan_pcp *)act;
                    p_action_vlan_pcp->type = htons(OFPAT_SET_VLAN_PCP);
                    p_action_vlan_pcp->len = htons(8);
                    p_action_vlan_pcp->vlan_pcp = htons(p_action_set_field->oxm_fields.vlan_pcp);
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_ETH_SRC))
                {
                    struct ofp_action_dl_addr *p_action_dl_addr = (struct ofp_action_dl_addr *)act;
                    p_action_dl_addr->type = htons(OFPAT_SET_DL_SRC);
                    p_action_dl_addr->len = htons(16);
                    memcpy(p_action_dl_addr->dl_addr, p_action_set_field->oxm_fields.eth_src, 6);
                    action_len += 16;
                    act += 16;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_ETH_DST))
                {
                    struct ofp_action_dl_addr *p_action_dl_addr = (struct ofp_action_dl_addr *)act;
                    p_action_dl_addr->type = htons(OFPAT_SET_DL_DST);
                    p_action_dl_addr->len = htons(16);
                    memcpy(p_action_dl_addr->dl_addr, p_action_set_field->oxm_fields.eth_dst, 6);
                    action_len += 16;
                    act += 16;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC))
                {
                    struct ofp_action_nw_addr *p_action_nw_addr = (struct ofp_action_nw_addr *)act;
                    p_action_nw_addr->type = htons(OFPAT_SET_NW_SRC);
                    p_action_nw_addr->len = htons(8);
                    p_action_nw_addr->nw_addr = htonl(p_action_set_field->oxm_fields.ipv4_src);
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_IPV4_DST))
                {
                    struct ofp_action_nw_addr *p_action_nw_addr = (struct ofp_action_nw_addr *)act;
                    p_action_nw_addr->type = htons(OFPAT_SET_NW_DST);
                    p_action_nw_addr->len = htons(8);
                    p_action_nw_addr->nw_addr = htonl(p_action_set_field->oxm_fields.ipv4_dst);
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_IP_DSCP))
                {
                    struct ofp_action_nw_tos *p_action_nw_tos = (struct ofp_action_nw_tos *)act;
                    p_action_nw_tos->type = htons(OFPAT_SET_NW_DST);
                    p_action_nw_tos->len = htons(8);
                    p_action_nw_tos->nw_tos = p_action_set_field->oxm_fields.ip_dscp;
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_TCP_SRC))
                {
                    struct ofp_action_tp_port *p_action_tp_port = (struct ofp_action_tp_port *)act;
                    p_action_tp_port->type = htons(OFPAT_SET_TP_SRC);
                    p_action_tp_port->len = htons(8);
                    p_action_tp_port->tp_port = htons(p_action_set_field->oxm_fields.tcp_src);
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_TCP_DST))
                {
                    struct ofp_action_tp_port *p_action_tp_port = (struct ofp_action_tp_port *)act;
                    p_action_tp_port->type = htons(OFPAT_SET_TP_DST);
                    p_action_tp_port->len = htons(8);
                    p_action_tp_port->tp_port = htons(p_action_set_field->oxm_fields.tcp_dst);
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_UDP_SRC))
                {
                    struct ofp_action_tp_port *p_action_tp_port = (struct ofp_action_tp_port *)act;
                    p_action_tp_port->type = htons(OFPAT_SET_TP_SRC);
                    p_action_tp_port->len = htons(8);
                    p_action_tp_port->tp_port = htons(p_action_set_field->oxm_fields.udp_src);
                    action_len += 8;
                    act += 8;
                }

                if(p_action_set_field->oxm_fields.mask & (MASK_SET << OFPXMT_OFB_UDP_DST))
                {
                    struct ofp_action_header *p_action_header = (struct ofp_action_header *)act;
                    p_action_header->type = htons(OFPAT_STRIP_VLAN);
                    p_action_header->len = htons(8);
                    action_len += 8;
                    act += 8;
                }
                break;
            }
            case OFPAT13_SET_QUEUE:
            {
                break;
            }
            case OFPAT13_POP_VLAN:
            {

                break;
            }
            default:break;
        }
        p_action = p_action->next;
    }

    return action_len;
}

static INT4 of10_msg_flow_mod(gn_switch_t *sw, UINT1 *flowmod_req)
{
    flow_mod_req_info_t *mod_info = (flow_mod_req_info_t *)flowmod_req;
    UINT2 total_len = sizeof(struct ofp_flow_mod);
    UINT1 *data = init_sendbuff(sw, OFP10_VERSION, OFPT_FLOW_MOD, total_len, mod_info->xid);
    struct ofp_flow_mod *ofm = (struct ofp_flow_mod *)data;

    ofm->cookie = 0x0;
    ofm->command = htons(mod_info->command);
    ofm->idle_timeout = htons(mod_info->flow->idle_timeout);
    ofm->hard_timeout = htons(mod_info->flow->hard_timeout);
    ofm->priority = htons(mod_info->flow->priority);
    ofm->buffer_id = htonl(mod_info->buffer_id);                //default 0xffffffff
    ofm->out_port = htons(mod_info->out_port);                  //default 0x0
    ofm->flags = htons(mod_info->flags);                        //default OFPFF_SEND_FLOW_REM

    of10_add_match(&(ofm->match), &(mod_info->flow->match.oxm_fields));
    if(mod_info->flow->instructions)
    {
        total_len += of10_add_action((UINT1 *)&(ofm->actions), ((gn_instruction_actions_t *)(mod_info->flow->instructions))->actions);
    }
    ofm->header.length = htons(total_len);

    if(mod_info->command == OFPFC_ADD)
    {
    	add_flow_entry(sw,mod_info->flow);
    }
    else if(mod_info->command == OFPFC_DELETE)
    {
		clean_flow_entry(sw,mod_info->flow);
    }

    return send_of_msg(sw, total_len);
}

static INT4 of10_msg_port_mod(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of10_msg_stats_request(gn_switch_t *sw, UINT1 *stats_req)
{
    stats_req_info_t *stats_req_info = (stats_req_info_t *)stats_req;

    UINT2 total_len  = sizeof(struct ofp_stats_request);
    UINT1 *data = init_sendbuff(sw, OFP10_VERSION, OFPT_STATS_REQUEST, total_len, stats_req_info->xid);

    struct ofp_stats_request *ofp_sr = (struct ofp_stats_request *)data;
    ofp_sr->type  = htons(stats_req_info->type);
    ofp_sr->flags = htons(stats_req_info->flags);

    switch(stats_req_info->type)
    {
        case OFPST_AGGREGATE:
        {
            struct ofp_aggregate_stats_request *ofp_asr =
                    (struct ofp_aggregate_stats_request *) ofp_sr->body;
            memset(&ofp_asr->match, 0x0, sizeof(struct ofp_match));
            ofp_asr->match.wildcards = 0xffffffff;
            ofp_asr->match.dl_vlan = htons(0xffff);
            ofp_asr->table_id = 0xff;
            ofp_asr->out_port = htons(0xffff);

            total_len += sizeof(struct ofp_aggregate_stats_request);
            break;
        }
        case OFPST_FLOW:
        {
            flow_stats_req_data_t *flow_stats_req_data = (flow_stats_req_data_t *)(stats_req_info->data);
            struct ofp_flow_stats_request *ofp_flow = (struct ofp_flow_stats_request *)ofp_sr->body;

            ofp_flow->table_id = flow_stats_req_data->table_id;
            ofp_flow->pad = 0x0;
            ofp_flow->out_port = htons(flow_stats_req_data->out_port);

            memset(&(ofp_flow->match), 0, sizeof(struct ofp_match));
            ofp_flow->match.wildcards   = 0xffffffff;
            ofp_flow->match.dl_vlan     = htons(0xffff);

            total_len += sizeof(struct ofp_flow_stats_request);
            break;
        }
        case OFPST_PORT:
        {
            port_stats_req_data_t *port_stats_req_data = (port_stats_req_data_t *)(stats_req_info->data);
            struct ofp_port_stats_request *ofp_port = (struct ofp_port_stats_request *)ofp_sr->body;

            ofp_port->port_no = htons(port_stats_req_data->port_no);
            memset(ofp_port->pad, 0x0, 6);
            total_len += sizeof(struct ofp_port_stats_request);
            break;
        }
        default:
        {
             break;
        }
    }

    ofp_sr->header.length = htons(total_len);
    return send_of_msg(sw, total_len);
}

static INT4 of10_msg_stats_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp_stats_reply *ofp_sr = (struct ofp_stats_reply *)of_msg;

    switch (ntohs(ofp_sr->type))
    {
        case OFPST_PORT:
        {
            of10_proc_port_stats(sw, ofp_sr->body, (ntohs(ofp_sr->header.length)
                    - sizeof(struct ofp_stats_reply))/sizeof(struct ofp_port_stats));
            break;
        }
        case OFPST_FLOW:
        {
            of10_proc_flow_stats(sw, ofp_sr->body,(ntohs(ofp_sr->header.length)
                    - sizeof(struct ofp_stats_reply))/sizeof(struct ofp_flow_stats));
            break;
        }
        case OFPST_DESC:
        {
            struct ofp_desc_stats *switch_desc = (struct ofp_desc_stats *)(ofp_sr->body);
            memcpy(&sw->sw_desc, switch_desc, sizeof(struct ofp_desc_stats));
            break;
        }
        case OFPST_AGGREGATE:
        {
            break;
        }
        default:
        {
            printf("%s: Unhandled stats reply 0x%x", FN, ntohs(ofp_sr->type));
            break;
        }
    }

    return GN_OK;
}

static INT4 of10_msg_barrier_request(gn_switch_t *sw, UINT1 *of_msg)
{
    UINT2 total_len = sizeof(struct ofp_header);
    init_sendbuff(sw, OFP10_VERSION, OFPT_BARRIER_REQUEST, total_len, 0);

    return send_of_msg(sw, total_len);
}

static INT4 of10_msg_barrier_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    //delete all flow entries.
	install_delete_fabric_flow(sw);
    return GN_OK;
}

static INT4 of10_msg_queue_get_config_request(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

static INT4 of10_msg_queue_get_config_reply(gn_switch_t *sw, UINT1 *of_msg)
{
    //todo
    return GN_OK;
}

msg_handler_t of10_message_handler[OFP10_MAX_MSG] =
{
    of10_msg_hello,
    of10_msg_error,
    of10_msg_echo_request,
    of10_msg_echo_reply,
    of10_msg_vendor,
    of10_msg_features_request,
    of10_msg_features_reply,
    of10_msg_get_config_request,
    of10_msg_get_config_reply,
    of10_msg_set_config,
    of10_msg_packet_in,
    of10_msg_flow_removed,
    of10_msg_port_status,
    of10_msg_packet_out,
    of10_msg_flow_mod,
    of10_msg_port_mod,
    of10_msg_stats_request,
    of10_msg_stats_reply,
    of10_msg_barrier_request,
    of10_msg_barrier_reply,
    of10_msg_queue_get_config_request,
    of10_msg_queue_get_config_reply
};
