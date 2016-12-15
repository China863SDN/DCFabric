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
 *   File Name   : json_server.c           *
 *   Author      : greenet Administrator           *
 *   Create Date : 2015-2-28           *
 *   Version     : 1.0           *
 *   Function    : .           *
 *                                                                             *
 ******************************************************************************/

#include "json_server.h"
#include "gnflush-types.h"
#include "mod-types.h"
#include "gn_inet.h"
#include "timer.h"
#include "../conn-svr/conn-svr.h"
#include "../flow-mgr/flow-mgr.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "../cluster-mgr/hbase_client.h"
#include "forward-mgr.h"
#include "../tenant-mgr/tenant-mgr.h"
#include "../stats-mgr/stats-mgr.h"
#include "../meter-mgr/meter-mgr.h"
#include "../group-mgr/group-mgr.h"
#include "../topo-mgr/topo-mgr.h"
#include "../user-mgr/user-mgr.h"
#include "../ovsdb/ovsdb.h"
#include "error_info.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "fabric_impl.h"
#include "openstack_app.h"
#include "fabric_openstack_external.h"
#include "fabric_openstack_nat.h"
#include "../inc/fabric/fabric_flows.h"
#include "../inc/fabric/fabric_stats.h"
#include "debug_svr.h"
#include "openstack-server.h"


extern UINT4 g_openstack_on;

//save flows recved from rest client
flow_entry_json_t *g_flow_entry_json_list = NULL;
UINT g_flow_entry_json_length = 0;


//从url中解析附带的参数
static void get_url_argument(const char *url, key_value_t *arg)
{
    char *tmp = NULL;
    char *arg_str = strstr(url, arg->key);

    if (NULL == arg_str)
    {
        return;
    }

    tmp = strchr(arg_str, '=');
    if (NULL == tmp)
    {
        return;
    }

    arg_str = tmp + 1;
    tmp = strchr(arg_str, '&');
    if (NULL == tmp)
    {
        arg->value = strdup(arg_str);
    }
    else
    {
        INT4 arg_len = tmp - arg_str;
        arg->value = (INT1 *) gn_malloc(arg_len + 1);
        strncpy(arg->value, arg_str, arg_len);
    }
}

//根据错误码，返回对应的json信息
INT1 *json_to_reply(json_t *obj, INT4 code)
{
    INT1 *reply = NULL;
    INT1 json_tmp[32];
    json_t *key, *value;

    if (NULL == obj)
    {
        obj = json_new_object();
    }

    key = json_new_string("retCode");
    sprintf(json_tmp, "%d", code);
    value = json_new_number(json_tmp);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("retMsg");
    value = json_new_string(get_error_msg(code));
    json_insert_child(key, value);
    json_insert_child(obj, key);

    json_tree_to_string(obj, &reply);
    json_free_value(&obj);

    //LOG_PROC("INFO", "Reply: %s", reply);
    return reply;
}


/****************************************************
 * get load status
 ****************************************************/
INT4 get_load_stats(double speed)
{
	if (speed < 25) {
		return LOAD_IDLE;
	}
	else if (speed < 50) {
		return LOAD_LIGHT;
	}
	else if (speed < 75) {
		return LOAD_BUSY;
	}
	else if (speed < 100) {
		return LOAD_HEAVY;
	}
	else {
		return LOAD_FULL;
	}

	return LOAD_IDLE;
}

/****************************************************
 * switch
 ****************************************************/
static INT1 *get_switch_info(const const INT1 *url, json_t *root)
{
    UINT1 dpid[8];
    INT1 json_tmp[1024];
    UINT4 i_sw = 0, i_port = 0;
    gn_switch_t *sw = NULL;
    gn_port_t *sw_port = NULL;
    json_t *Obj, *key, *value, *sw_array, *sw_obj, *port_array, *port_obj;

    Obj = json_new_object();

    //switches
    key = json_new_string("switchInfo");
    sw_array = json_new_array();
    json_insert_child(key, sw_array);
    json_insert_child(Obj, key);

    for (; i_sw < g_server.max_switch; i_sw++)
    {
        sw = &g_server.switches[i_sw];
        if (sw->state == 1)
        {
            sw_obj = json_new_object();
            json_insert_child(sw_array, sw_obj);

            ulli64_to_uc8(sw->dpid, dpid);
            sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                    dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                    dpid[7]);
            key = json_new_string("DPID");
            value = json_new_string(json_tmp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //ip:port
            sprintf(json_tmp, "%s:%d", inet_htoa(ntohl(sw->sw_ip)),
                    ntohs(sw->sw_port));
            key = json_new_string("inetAddr");
            value = json_new_string(json_tmp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //mfr desc
            key = json_new_string("mfrDesc");
            value = json_new_string(sw->sw_desc.mfr_desc);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //hardware desc
            key = json_new_string("hwDesc");
            value = json_new_string(sw->sw_desc.hw_desc);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //software desc
            key = json_new_string("swDesc");
            value = json_new_string(sw->sw_desc.sw_desc);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //serial number
            key = json_new_string("serialNum");
            value = json_new_string(sw->sw_desc.serial_num);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //datapath desc
            key = json_new_string("dpDesc");
            value = json_new_string(sw->sw_desc.dp_desc);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //buffers
            key = json_new_string("buffers");
            sprintf(json_tmp, "%d", sw->n_buffers);
            value = json_new_number(json_tmp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //ports
            key = json_new_string("ports");
            port_array = json_new_array();
            json_insert_child(key, port_array);
            json_insert_child(sw_obj, key);

            for (i_port = 0; i_port <= sw->n_ports; i_port++)
            {
                sw_port = &(sw->ports[i_port]);
                if (i_port == sw->n_ports)
                {
                    sw_port = &sw->lo_port;
                }

                port_obj = json_new_object();
                json_insert_child(port_array, port_obj);

                //name
                key = json_new_string("name");
                value = json_new_string(sw_port->name);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //state
                key = json_new_string("state");
                sprintf(json_tmp, "%d", sw_port->state);
                value = json_new_number(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //mac
                key = json_new_string("hwAddr");
                sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
                        sw_port->hw_addr[0], sw_port->hw_addr[1],
                        sw_port->hw_addr[2], sw_port->hw_addr[3],
                        sw_port->hw_addr[4], sw_port->hw_addr[5]);
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //port_no
                key = json_new_string("portNo");
                sprintf(json_tmp, "%x", sw_port->port_no);
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //config
                key = json_new_string("config");
                sprintf(json_tmp, "%d", sw_port->config);
                value = json_new_number(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //currentFeatures
                key = json_new_string("currentFeatures");
                sprintf(json_tmp, "%d", sw_port->curr);
                value = json_new_number(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //advertisedFeatures
                key = json_new_string("advertisedFeatures");
                sprintf(json_tmp, "%d", sw_port->advertised);
                value = json_new_number(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //supportedFeatures
                key = json_new_string("supportedFeatures");
                sprintf(json_tmp, "%d", sw_port->supported);
                value = json_new_number(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);

                //peerFeatures
                key = json_new_string("peerFeatures");
                sprintf(json_tmp, "%d", sw_port->peer);
                value = json_new_number(json_tmp);
                json_insert_child(key, value);
                json_insert_child(port_obj, key);
            }

            //openflow
            key = json_new_string("openflow");
            if (sw->ofp_version == 0x01)
            {
                value = json_new_string("of1.0");
            }
            else
            {
                value = json_new_string("of1.3");
            }
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            //connected since
            key = json_new_string("connectedSince");
            sprintf(json_tmp, "%llu", sw->connected_since);
            value = json_new_number(json_tmp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);
        }
    }

    return json_to_reply(Obj, GN_OK);
}

/****************************************************
 * topology
 ****************************************************/
static INT1 *get_topo_link(const const INT1 *url, json_t *root)
{
    json_t *Obj, *key, *value, *link_array, *neighbor_array, *link_obj,
            *neighbor_obj;
    UINT4 i, port;
    gn_switch_t *sw = NULL;
    UINT1 dpid[8];
    INT1 json_temp[1024];

    Obj = json_new_object();
    key = json_new_string("linkTopo");
    link_array = json_new_array();
    json_insert_child(key, link_array);
    json_insert_child(Obj, key);

    for (i = 0; i < g_server.max_switch; i++)
    {
        if (g_server.switches[i].state == 1)
        {
            sw = &g_server.switches[i];
            if (sw)
            {
                link_obj = json_new_object();
                json_insert_child(link_array, link_obj);

                ulli64_to_uc8(sw->dpid, dpid);
                sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                        dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                        dpid[7]);

                key = json_new_string("srcDPID");
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(link_obj, key);

                key = json_new_string("neighbors");
                neighbor_array = json_new_array();
                json_insert_child(key, neighbor_array);
                json_insert_child(link_obj, key);

                for (port = 0; port < sw->n_ports; port++)
                {
                    if (sw->neighbor[port])
                    {
                        neighbor_obj = json_new_object();
                        json_insert_child(neighbor_array, neighbor_obj);

                        key = json_new_string("srcPort");
                        sprintf(json_temp, "%d", sw->ports[port].port_no);
                        value = json_new_number(json_temp);
                        json_insert_child(key, value);
                        json_insert_child(neighbor_obj, key);

                        key = json_new_string("dstDPID");
                        if (sw->neighbor[port]->sw)
                        {
                            ulli64_to_uc8(sw->neighbor[port]->sw->dpid, dpid);
                            sprintf(json_temp,
                                    "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                                    dpid[1], dpid[2], dpid[3], dpid[4], dpid[5],
                                    dpid[6], dpid[7]);

                            value = json_new_string(json_temp);
                            json_insert_child(key, value);
                            json_insert_child(neighbor_obj, key);

                        }

                        key = json_new_string("dstPort");
                        if (sw->neighbor[port]->port)
                        {
                            sprintf(json_temp, "%d",
                                    sw->neighbor[port]->port->port_no);
                            value = json_new_number(json_temp);
                            json_insert_child(key, value);
                            json_insert_child(neighbor_obj, key);

                        }
                    }
                }
            }
        }
    }

    return json_to_reply(Obj, GN_OK);
}

static INT1 *get_topo_hosts(const const INT1 *url, json_t *root)
{
    UINT4 i, port;
    UINT4 hsize = 0;
    UINT1 dpid[8];
    INT1 json_temp[1024];
    json_t *Obj, *key, *value, *sw_array, *sw_obj, *port_array, *port_obj,
            *host_array, *host_obj;
    gn_switch_t *sw = NULL;
    mac_user_t *p_macuser = NULL;
    UINT1 port_hosts_cnt[MAX_PORTS] = { 0 };

    Obj = json_new_object();
    key = json_new_string("hostTopo");
    sw_array = json_new_array();
    json_insert_child(key, sw_array);
    json_insert_child(Obj, key);

    for (i = 0; i < g_server.max_switch; i++)
    {
        if (g_server.switches[i].state == 1)
        {
            sw_obj = json_new_object();
            json_insert_child(sw_array, sw_obj);

            sw = &g_server.switches[i];

            ulli64_to_uc8(sw->dpid, dpid);
            sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                    dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                    dpid[7]);

            key = json_new_string("DPID");
            value = json_new_string(json_temp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            key = json_new_string("ports");
            port_array = json_new_array();
            json_insert_child(key, port_array);
            json_insert_child(sw_obj, key);

            for (port = 0; port < sw->n_ports; port++)
            {
                if (NULL == (sw->neighbor[port]))
                {
                    port_obj = json_new_object();
                    json_insert_child(port_array, port_obj);

                    key = json_new_string("portNo");
                    sprintf(json_temp, "%d", sw->ports[port].port_no);
                    value = json_new_number(json_temp);
                    json_insert_child(key, value);
                    json_insert_child(port_obj, key);

                    key = json_new_string("hosts");
                    host_array = json_new_array();
                    json_insert_child(key, host_array);
                    json_insert_child(port_obj, key);

                    for (hsize = 0; hsize < g_macuser_table.macuser_hsize; hsize++)
                    {
                        p_macuser = sw->users[hsize];
                        if (p_macuser)
                        {
                            if (sw->ports[port].port_no == p_macuser->port)
                            {
                                port_hosts_cnt[port]++;
                                if (port_hosts_cnt[port] <= 10)
                                {
                                    host_obj = json_new_object();    //hosts:[{}]
                                    json_insert_child(host_array, host_obj);

                                    key = json_new_string("hwAddr");
                                    sprintf(json_temp,
                                            "%02x:%02x:%02x:%02x:%02x:%02x",
                                            p_macuser->mac[0],
                                            p_macuser->mac[1],
                                            p_macuser->mac[2],
                                            p_macuser->mac[3],
                                            p_macuser->mac[4],
                                            p_macuser->mac[5]);
                                    value = json_new_string(json_temp);
                                    json_insert_child(key, value);
                                    json_insert_child(host_obj, key);

                                    key = json_new_string("ipv4Addr");
                                    value = json_new_string(
                                            inet_htoa(p_macuser->ipv4));
                                    json_insert_child(key, value);
                                    json_insert_child(host_obj, key);

//                                    inet_ntop(AF_INET6, (char *)(p_macuser->ipv6), json_temp, 40);
//                                    value = json_new_string(json_temp);
//                                    json_insert_child(key, value);
//                                    json_insert_child(host_obj, key);
                                }
                            }
                        }
                    }

                    key = json_new_string("total");
                    sprintf(json_temp, "%d", port_hosts_cnt[port]);
                    value = json_new_number(json_temp);
                    json_insert_child(key, value);
                    json_insert_child(port_obj, key);
                }
            }
        }
    }

    return json_to_reply(Obj, GN_OK);
}

/****************************************************
 * L3
 ****************************************************/
static INT1 *get_l3_subnet(const const INT1 *url, json_t *root)
{
    json_t *array, *Obj, *key, *value, *port_obj;
    int i;

    Obj = json_new_object();
    array = json_new_array();

    key = json_new_string("subnets");
    json_insert_child(key, array);
    json_insert_child(Obj, key);

    for (i = 0; i < MAX_L3_SUBNET; i++)
    {
        if (g_subnet_info[i].is_using == TRUE)
        {
            port_obj = json_new_object();

            key = json_new_string("name");
            value = json_new_string(g_subnet_info[i].name);
            json_insert_child(key, value);
            json_insert_child(port_obj, key);

            key = json_new_string("subnet");
            value = json_new_string(g_subnet_info[i].netmask);
            json_insert_child(key, value);
            json_insert_child(port_obj, key);

            json_insert_child(array, port_obj);
        }
    }

    return json_to_reply(Obj, GN_OK);
}

static INT1 *post_l3_subnet(const INT1 *url, json_t *root)
{
    INT4 ret = 0;
    json_t *item_name = NULL, *item_masked_ip = NULL;

    item_name = json_find_first_label(root, "name");
    if (NULL == item_name)
    {
        return json_to_reply(NULL, GN_ERR);
    }

    item_masked_ip = json_find_first_label(root, "subnet");
    if (NULL == item_masked_ip)
    {
        json_free_value(&item_name);
        return json_to_reply(NULL, GN_ERR);
    }

    ret = create_l3_subnet(item_name->child->text, item_masked_ip->child->text);
    json_free_value(&item_name);
    json_free_value(&item_masked_ip);

    return json_to_reply(NULL, ret);
}

static INT1 *del_l3_subnet(const INT1 *url, json_t *root)
{
    INT4 ret = 0;
    json_t *item_name = NULL, *item_masked_ip = NULL, *Obj;

    Obj = json_new_object();

    item_masked_ip = json_find_first_label(root, "subnet");
    if (NULL == item_masked_ip)
    {
        json_free_value(&item_name);
        return json_to_reply(NULL, GN_ERR);
    }

    ret = destory_l3_subnet(item_masked_ip->child->text);
    json_free_value(&item_masked_ip);

    return json_to_reply(Obj, ret);
}

/****************************************************
 * flow mod
 ****************************************************/
static void json_add_oxm_fields(json_t *obj, gn_oxm_t *oxm_fields)
{
    INT1 json_temp[1024];
    json_t *key = NULL, *value = NULL;

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PORT))
    {
        sprintf(json_temp, "%d", oxm_fields->in_port);
        key = json_new_string("inport");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

//    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PHY_PORT))
//    {
//        sprintf(json_temp, "%d", oxm_fields->in_phy_port);
//        key = json_new_string("inPhyPort");
//        value = json_new_string(json_temp);
//        json_insert_child(key, value);
//        json_insert_child(obj, key);
//    }

//    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_METADATA))
//    {
//        sprintf(json_temp, "%d", oxm_fields->metadata);
//        key = json_new_string("metadata");
//        value = json_new_string(json_temp);
//        json_insert_child(key, value);
//        json_insert_child(obj, key);
//    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_DST))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->eth_dst[0],
                oxm_fields->eth_dst[1], oxm_fields->eth_dst[2],
                oxm_fields->eth_dst[3], oxm_fields->eth_dst[4],
                oxm_fields->eth_dst[5]);
        key = json_new_string("ethDst");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_SRC))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->eth_src[0],
                oxm_fields->eth_src[1], oxm_fields->eth_src[2],
                oxm_fields->eth_src[3], oxm_fields->eth_src[4],
                oxm_fields->eth_src[5]);
        key = json_new_string("ethSrc");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_TYPE))
    {
        key = json_new_string("ethType");
        if (oxm_fields->eth_type == ETHER_ARP)
        {
            value = json_new_string("ARP");
        }
        else if (oxm_fields->eth_type == ETHER_IP)
        {
            value = json_new_string("IPV4");
        }
        else if (oxm_fields->eth_type == ETHER_IPV6)
        {
            value = json_new_string("IPV6");
        }
        else if (oxm_fields->eth_type == ETHER_MPLS)
        {
            value = json_new_string("MPLS");
        }
        else if (oxm_fields->eth_type == ETHER_VLAN)
        {
            value = json_new_string("VLAN");
        }
        else
        {
            value = json_new_string("UNKNOW");
        }

        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_VID))
    {
        sprintf(json_temp, "%d", oxm_fields->vlan_vid);
        key = json_new_string("vlanId");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_PCP))
    {
        sprintf(json_temp, "%d", oxm_fields->vlan_pcp);
        key = json_new_string("vlanPcp");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_DSCP))
    {
        sprintf(json_temp, "%d", oxm_fields->ip_dscp);
        key = json_new_string("ipDscp");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_ECN))
    {
        sprintf(json_temp, "%d", oxm_fields->ip_ecn);
        key = json_new_string("ipEcn");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_PROTO))
    {
        key = json_new_string("ipProto");
        if (oxm_fields->ip_proto == IPPROTO_ICMP)
        {
            value = json_new_string("ICMP");
        }
        else if (oxm_fields->ip_proto == IPPROTO_TCP)
        {
            value = json_new_string("TCP");
        }
        else if (oxm_fields->ip_proto == IPPROTO_UDP)
        {
            value = json_new_string("UDP");
        }
        else
        {
            //
        }

        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC))
    {
        key = json_new_string("ipv4Src");
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX))
        {
            sprintf(json_temp, "%s/%d", inet_htoa(oxm_fields->ipv4_src),
                    oxm_fields->ipv4_src_prefix);
        }
        else
        {
            sprintf(json_temp, "%s", inet_htoa(oxm_fields->ipv4_src));
        }
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST))
    {
        key = json_new_string("ipv4Drc");
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX))
        {
            sprintf(json_temp, "%s/%d", inet_htoa(oxm_fields->ipv4_dst),
                    oxm_fields->ipv4_dst_prefix);
        }
        else
        {
            sprintf(json_temp, "%s", inet_htoa(oxm_fields->ipv4_dst));
        }
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_SRC))
    {
        sprintf(json_temp, "%d", oxm_fields->tcp_src);
        key = json_new_string("tcpSrc");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_DST))
    {
        sprintf(json_temp, "%d", oxm_fields->tcp_dst);
        key = json_new_string("tcpDst");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_SRC))
    {
        sprintf(json_temp, "%d", oxm_fields->udp_src);
        key = json_new_string("udpSrc");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_DST))
    {
        sprintf(json_temp, "%d", oxm_fields->udp_dst);
        key = json_new_string("udpDst");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_SRC))
//    {
//
//    }
//
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_DST))
//    {
//
//    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_TYPE))
    {
        sprintf(json_temp, "%d", oxm_fields->icmpv4_type);
        key = json_new_string("icmpv4Type");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_CODE))
    {
        sprintf(json_temp, "%d", oxm_fields->icmpv4_code);
        key = json_new_string("icmpv4Code");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_OP))
    {
        sprintf(json_temp, "%d", oxm_fields->arp_op);
        key = json_new_string("icmpv4Code");
        if (oxm_fields->arp_op == 1)
        {
            value = json_new_string("Request");
        }
        else
        {
            value = json_new_string("Reply");
        }

        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SPA))
    {
        key = json_new_string("arpSpa");
        value = json_new_string(inet_htoa(oxm_fields->arp_spa));
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_TPA))
    {
        key = json_new_string("arpTpa");
        value = json_new_string(inet_htoa(oxm_fields->arp_tpa));
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SHA))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->arp_sha[0],
                oxm_fields->arp_sha[1], oxm_fields->arp_sha[2],
                oxm_fields->arp_sha[3], oxm_fields->arp_sha[4],
                oxm_fields->arp_sha[5]);
        key = json_new_string("arpSha");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_THA))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->arp_tha[0],
                oxm_fields->arp_tha[1], oxm_fields->arp_tha[2],
                oxm_fields->arp_tha[3], oxm_fields->arp_tha[4],
                oxm_fields->arp_tha[5]);
        key = json_new_string("arpTha");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC))
    {
        char sipv6[40] = { 0 };
        inet_ntop(AF_INET6, (char *) (oxm_fields->ipv6_src), sipv6, 40);
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC_PREFIX))
        {
            sprintf(json_temp, "%s/%d", sipv6, oxm_fields->ipv6_src_prefix);
            value = json_new_string(json_temp);
        }
        else
        {
            value = json_new_string(sipv6);
        }

        key = json_new_string("ipv6Src");
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST))
    {
        char sipv6[40] = { 0 };
        inet_ntop(AF_INET6, (char *) (oxm_fields->ipv6_dst), sipv6, 40);
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST_PREFIX))
        {
            sprintf(json_temp, "%s/%d", sipv6, oxm_fields->ipv6_dst_prefix);
            value = json_new_string(json_temp);
        }
        else
        {
            value = json_new_string(sipv6);
        }

        key = json_new_string("ipv6Dst");
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_FLABEL))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_TYPE))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_CODE))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TARGET))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_SLL))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TLL))
//    {
//    }
    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_MPLS_LABEL))
    {
        key = json_new_string("mplsLabel");
        sprintf(json_temp, "%d", oxm_fields->mpls_label);
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_MPLS_TC))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFP_MPLS_BOS))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_PBB_ISID))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_TUNNEL_ID))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_EXTHDR))
//    {
//    }
}

static void json_add_actions(json_t *obj, gn_action_t *actions)
{
    INT1 json_temp[1024];
    gn_action_t *p_action = actions;
    json_t *key, *value, *tmp_obj = NULL;

    while (p_action)
    {
        switch (p_action->type)
        {
        case OFPAT13_OUTPUT:
        {
            gn_action_output_t *p_action_output =
                    (gn_action_output_t *) p_action;

            sprintf(json_temp, "%d", p_action_output->port);
            key = json_new_string("output");
            value = json_new_string(json_temp);
            json_insert_child(key, value);
            json_insert_child(obj, key);
            break;
        }
        case OFPAT13_COPY_TTL_OUT:
        {
            break;
        }
        case OFPAT13_COPY_TTL_IN:
        {
            break;
        }
        case OFPAT13_MPLS_TTL:
        {
            break;
        }
        case OFPAT13_DEC_MPLS_TTL:
        {
            break;
        }
        case OFPAT13_PUSH_VLAN:
        {
            key = json_new_string("pushVlan");
            value = json_new_string("PUSH_VLAN");
            json_insert_child(key, value);
            json_insert_child(obj, key);
            break;
        }
        case OFPAT13_POP_VLAN:
        {
            key = json_new_string("popVlan");
            value = json_new_string("POP_VLAN");
            json_insert_child(key, value);
            json_insert_child(obj, key);
            break;
        }
        case OFPAT13_PUSH_MPLS:
        {
            key = json_new_string("pushMpls");
            value = json_new_string("PUSH_MPLS");
            json_insert_child(key, value);
            json_insert_child(obj, key);
            break;
        }
        case OFPAT13_POP_MPLS:
        {
            key = json_new_string("pushMpls");
            value = json_new_string("POP_MPLS");
            json_insert_child(key, value);
            json_insert_child(obj, key);
            break;
        }
        case OFPAT13_SET_QUEUE:
        {
            break;
        }
        case OFPAT13_GROUP:
        {
            gn_action_group_t *p_action_group = (gn_action_group_t *) p_action;

            sprintf(json_temp, "%d", p_action_group->group_id);
            key = json_new_string("group");
            value = json_new_string(json_temp);
            json_insert_child(key, value);
            json_insert_child(obj, key);
            break;
        }
        case OFPAT13_SET_NW_TTL:
        {
            break;
        }
        case OFPAT13_DEC_NW_TTL:
        {
            break;
        }
        case OFPAT13_SET_FIELD:
        {
            gn_action_set_field_t *p_action_set_field =
                    (gn_action_set_field_t *) p_action;

            key = json_new_string("setField");
            tmp_obj = json_new_object();
            json_insert_child(key, tmp_obj);
            json_insert_child(obj, key);

            json_add_oxm_fields(tmp_obj, &(p_action_set_field->oxm_fields));
            break;
        }
        case OFPAT13_PUSH_PBB:
        {
            break;
        }
        case OFPAT13_POP_PBB:
        {
            break;
        }
        case OFPAT13_EXPERIMENTER:
        {
            break;
        }
        default:
            break;
        }
        p_action = p_action->next;
    }
}

static void json_add_instructions(json_t *obj, gn_instruction_t *instructions)
{
    INT1 json_temp[1024];
    gn_instruction_t *p_ins = instructions;
    json_t *key, *value, *tmp_obj = NULL;

    while (p_ins)
    {
        if ((p_ins->type == OFPIT_APPLY_ACTIONS)
                || (p_ins->type == OFPIT_WRITE_ACTIONS))
        {
            gn_instruction_actions_t *p_ins_actions =
                    (gn_instruction_actions_t *) p_ins;
            if (p_ins->type == OFPIT_APPLY_ACTIONS)
            {
                key = json_new_string("applyAction");
            }
            else
            {
                key = json_new_string("writeAction");
            }

            tmp_obj = json_new_object();
            json_insert_child(key, tmp_obj);
            json_insert_child(obj, key);

            json_add_actions(tmp_obj, p_ins_actions->actions);
        }
        else if (p_ins->type == OFPIT_CLEAR_ACTIONS)
        {
            key = json_new_string("clearAction");
            value = json_new_string("");
            json_insert_child(key, value);
            json_insert_child(obj, key);
        }
        else if (p_ins->type == OFPIT_GOTO_TABLE)
        {
            gn_instruction_goto_table_t *p_ins_goto_table =
                    (gn_instruction_goto_table_t *) p_ins;
            sprintf(json_temp, "%d", p_ins_goto_table->table_id);
            key = json_new_string("gotoTable");
            value = json_new_number(json_temp);
            json_insert_child(key, value);
            json_insert_child(obj, key);
        }
        else if (p_ins->type == OFPIT_METER)
        {
            gn_instruction_meter_t *p_ins_meter =
                    (gn_instruction_meter_t *) p_ins;
            sprintf(json_temp, "%d", p_ins_meter->meter_id);
            key = json_new_string("meter");
            value = json_new_number(json_temp);
            json_insert_child(key, value);
            json_insert_child(obj, key);
        }

        p_ins = p_ins->next;
    }
}

static INT1 *get_flow_entries_all(const INT1 *url, json_t *root)
{
    UINT4 idx = 0;
    UINT1 dpid[8];
    INT1 json_temp[1024];
    gn_switch_t *sw = NULL;
    gn_flow_t *flow = NULL;
    json_t *Obj, *key, *value, *sw_array, *sw_obj, *flow_array, *flow_obj,
            *tmp_obj = NULL;

    Obj = json_new_object();
    key = json_new_string("switchFlowEntries");
    sw_array = json_new_array();
    json_insert_child(key, sw_array);
    json_insert_child(Obj, key);

    for (; idx < g_server.max_switch; idx++)
    {
        if (g_server.switches[idx].state == 1)
        {
            sw_obj = json_new_object();
            json_insert_child(sw_array, sw_obj);

            sw = &g_server.switches[idx];
            ulli64_to_uc8(sw->dpid, dpid);
            sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                    dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                    dpid[7]);

            key = json_new_string("DPID");
            value = json_new_string(json_temp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            key = json_new_string("flowEntries");
            flow_array = json_new_array();
            json_insert_child(key, flow_array);
            json_insert_child(sw_obj, key);

            flow = sw->flow_entries;
            while (flow)
            {
                flow_obj = json_new_object();
                json_insert_child(flow_array, flow_obj);

                key = json_new_string("uuid");
                value = json_new_string(flow->uuid);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("creater");
                value = json_new_string(flow->creater);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("createTime");
                sprintf(json_temp, "%llu", flow->create_time);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("tableId");
                sprintf(json_temp, "%d", flow->table_id);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("idleTimeout");
                sprintf(json_temp, "%d", flow->idle_timeout);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("hardTimeout");
                sprintf(json_temp, "%d", flow->hard_timeout);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("priority");
                sprintf(json_temp, "%d", flow->priority);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("match");
                tmp_obj = json_new_object();
                json_insert_child(key, tmp_obj);
                json_insert_child(flow_obj, key);
                json_add_oxm_fields(tmp_obj, &(flow->match.oxm_fields));

                key = json_new_string("instruction");
                tmp_obj = json_new_object();
                json_insert_child(key, tmp_obj);
                json_insert_child(flow_obj, key);

                json_add_instructions(tmp_obj, flow->instructions);

                flow = flow->next;
            }
        }
    }

    return json_to_reply(Obj, GN_OK);
}

static INT1 *del_flow_entries_all(const INT1 *url, json_t *root)
{
    INT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    gn_switch_t *sw = NULL;
    json_t *item = NULL;

    item = json_find_first_label(root, "DPID");
    mac_str_to_bin(item->child->text, dpid);

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, GN_ERR);
    }

    ret = clear_flow_entries(sw);
    return json_to_reply(NULL, ret);
}

static INT4 json_parse_oxm_fields(json_t *obj, gn_oxm_t *oxm_fields)
{
    json_t *item = NULL;

    oxm_fields->mask = 0;
    item = json_find_first_label(obj, "inport");
    if (item)
    {
        oxm_fields->in_port = atoll(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IN_PORT);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "inPhyPort");
    if (item)
    {
        oxm_fields->in_phy_port = atoll(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IN_PHY_PORT);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "metadata");
    if (item)
    {
        oxm_fields->metadata = strtoull(item->child->text, NULL, 10);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_METADATA);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "ethDst");
    if (item)
    {
        macstr2hex(item->child->text, oxm_fields->eth_dst);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ETH_DST);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "ethSrc");
    if (item)
    {
        macstr2hex(item->child->text, oxm_fields->eth_src);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ETH_SRC);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "ethType");
    if (item)
    {
        if (0 == strncmp(item->child->text, "ARP", 3))
        {
            oxm_fields->eth_type = ETHER_ARP;
            json_free_value(&item);

            item = json_find_first_label(obj, "arpOp");
            if (item)
            {
                if (0 == strncmp(item->child->text, "Request", 3))
                {
                    oxm_fields->arp_op = 1;
                }
                else
                {
                    oxm_fields->arp_op = 2;
                }

                oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ARP_OP);
                json_free_value(&item);
            }

            item = json_find_first_label(obj, "arpSpa");
            if (item)
            {
                oxm_fields->arp_spa = ntohl(inet_addr(item->child->text));
                oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ARP_SPA);
                json_free_value(&item);
            }

            item = json_find_first_label(obj, "arpTpa");
            if (item)
            {
                oxm_fields->arp_tpa = ntohl(inet_addr(item->child->text));
                oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ARP_TPA);
                json_free_value(&item);
            }

            item = json_find_first_label(obj, "arpSha");
            if (item)
            {
                macstr2hex(item->child->text, oxm_fields->arp_sha);
                oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ARP_SHA);
                json_free_value(&item);
            }

            item = json_find_first_label(obj, "arpTha");
            if (item)
            {
                macstr2hex(item->child->text, oxm_fields->arp_tha);
                oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ARP_THA);
                json_free_value(&item);
            }
        }
        else if (0 == strncmp(item->child->text, "IPV4", 4))
        {
            oxm_fields->eth_type = ETHER_IP;
            json_free_value(&item);
        }
        else if (0 == strncmp(item->child->text, "IPV6", 4))
        {
            oxm_fields->eth_type = ETHER_IPV6;
            json_free_value(&item);
        }
        else if (0 == strncmp(item->child->text, "MPLS", 4))
        {
            oxm_fields->eth_type = ETHER_MPLS;
            json_free_value(&item);

            item = json_find_first_label(obj, "mplsLabel");
            {
                oxm_fields->mpls_label = atoll(item->child->text);
                oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_MPLS_LABEL);
                json_free_value(&item);
            }
        }
        else if (0 == strncmp(item->child->text, "VLAN", 3))
        {
            oxm_fields->eth_type = ETHER_VLAN;
            json_free_value(&item);
        }
        else
        {
            json_free_value(&item);
            return GN_ERR;
        }

        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ETH_TYPE);
    }

    item = json_find_first_label(obj, "vlanId");
    if (item)
    {
        oxm_fields->vlan_vid = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_VLAN_VID);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "vlanPcp");
    if (item)
    {
        oxm_fields->vlan_pcp = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_VLAN_PCP);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "ipDscp");
    if (item)
    {
        oxm_fields->ip_dscp = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IP_DSCP);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "ipEcn");
    if (item)
    {
        oxm_fields->ip_ecn = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IP_ECN);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "ipProto");
    if (item)
    {
        if (0 == strncmp(item->child->text, "TCP", 3))
        {
            oxm_fields->ip_proto = IPPROTO_TCP;
            json_free_value(&item);
        }
        else if (0 == strncmp(item->child->text, "UDP", 3))
        {
            oxm_fields->ip_proto = IPPROTO_UDP;
            json_free_value(&item);
        }
        else if (0 == strncmp(item->child->text, "ICMP", 3))
        {
            oxm_fields->ip_proto = IPPROTO_ICMP;
            json_free_value(&item);
        }
        else
        {
            json_free_value(&item);
            return GN_ERR;
        }

        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IP_PROTO);
    }

    item = json_find_first_label(obj, "ipv4Src");
    if (item)
    {
        net_mask_t net_mask;

        masked_ip_parser(item->child->text, &net_mask);
        if (0 != net_mask.prefix)
        {
            oxm_fields->ipv4_src_prefix = net_mask.prefix;
            oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX);
        }

        oxm_fields->ipv4_src = ntohl(net_mask.ip);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV4_SRC);

        json_free_value(&item);
    }

    item = json_find_first_label(obj, "ipv4Dst");
    if (item)
    {
        net_mask_t net_mask;

        masked_ip_parser(item->child->text, &net_mask);
        if (0 != net_mask.prefix)
        {
            oxm_fields->ipv4_dst_prefix = net_mask.prefix;
            oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX);
        }

        oxm_fields->ipv4_dst = ntohl(net_mask.ip);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV4_DST);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "tcpSrc");
    if (item)
    {
        oxm_fields->tcp_src = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_TCP_SRC);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "tcpDst");
    if (item)
    {
        oxm_fields->tcp_dst = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_TCP_DST);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "udpSrc");
    if (item)
    {
        oxm_fields->udp_src = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_UDP_SRC);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "udpDst");
    if (item)
    {
        oxm_fields->udp_dst = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_UDP_DST);
        json_free_value(&item);
    }

//    item = json_find_first_label(obj, "sctpSrc");
//    {
//        oxm_fields->sctp_src = atoi(item->child->text);
//        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_SCTP_SRC);
//        json_free_value(&item);
//    }
//
//    item = json_find_first_label(obj, "sctpDst");
//    {
//        oxm_fields->sctp_dst = atoi(item->child->text);
//        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_SCTP_DST);
//        json_free_value(&item);
//    }

    item = json_find_first_label(obj, "icmpv4Type");
    if (item)
    {
        oxm_fields->icmpv4_type = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ICMPV4_TYPE);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "icmpv4Code");
    if (item)
    {
        oxm_fields->icmpv4_code = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_ICMPV4_CODE);
        json_free_value(&item);
    }

//    item = json_find_first_label(obj, "ipv6Src");
//    {
//        oxm_fields->ipv6_src = atoi(item->child->text);
//        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV6_SRC);
//        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV6_SRC_PREFIX);
//        json_free_value(&item);
//    }
//
//    item = json_find_first_label(obj, "ipv6Dst");
//    {
//        oxm_fields->ipv6_dst = atoi(item->child->text);
//        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV6_DST);
//        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IPV6_DST_PREFIX);
//        json_free_value(&item);
//    }

    item = json_find_first_label(obj, "mplsLabel");
    if (item)
    {
        oxm_fields->mpls_label = atoll(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_MPLS_LABEL);
        json_free_value(&item);
    }

    return GN_OK;
}

static INT4 json_parse_actions(json_t *obj, gn_action_t **actions)
{
    INT4 ret = 0;
    json_t *item = NULL;

    item = json_find_first_label(obj, "output");
    if (item)
    {
        gn_action_output_t *p_action = (gn_action_output_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = NULL;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_OUTPUT;
        p_action->port = atoll(item->child->text);

        if(p_action->port == OFPP13_CONTROLLER)
        {
            p_action->max_len = 0xffff;
        }
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "group");
    if (item)
    {
        gn_action_group_t *p_action = (gn_action_group_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = *actions;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_GROUP;
        p_action->group_id = atoll(item->child->text);

        json_free_value(&item);
    }

    item = json_find_first_label(obj, "setField");
    if (item)
    {
        gn_action_set_field_t *p_action = (gn_action_set_field_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = *actions;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_SET_FIELD;
        ret = json_parse_oxm_fields(item->child, &(p_action->oxm_fields));

        json_free_value(&item);

        if (GN_OK != ret)
        {
            return ret;
        }
    }

    item = json_find_first_label(obj, "pushVlan");
    if (item)
    {
        gn_action_t *p_action = (gn_action_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = *actions;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_PUSH_VLAN;

        json_free_value(&item);
    }

    item = json_find_first_label(obj, "popVlan");
    if (item)
    {
        gn_action_t *p_action = (gn_action_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = *actions;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_POP_VLAN;

        json_free_value(&item);
    }

    item = json_find_first_label(obj, "pushMpls");
    if (item)
    {
        gn_action_t *p_action = (gn_action_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = *actions;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_PUSH_MPLS;

        json_free_value(&item);
    }

    item = json_find_first_label(obj, "popMpls");
    if (item)
    {
        gn_action_t *p_action = (gn_action_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = *actions;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_POP_MPLS;

        json_free_value(&item);
    }

    return GN_OK;
}

static INT4 json_parse_instructions(json_t *obj, gn_instruction_t **instructions)
{
    UINT4 ret = 0;
    json_t *item = NULL;
    struct gn_instruction *list_end = *instructions;

    item = json_find_first_label(obj, "applyAction");
    if (item)
    {
        gn_instruction_actions_t *p_ins_actions = (gn_instruction_actions_t *)mem_get(g_gninstruction_mempool_id);
        if(list_end)
        {
            list_end->next = (gn_instruction_t *)p_ins_actions;
            list_end = list_end->next;
        }
        else
        {
            *instructions = (gn_instruction_t *)p_ins_actions;
            list_end = *instructions;
        }

        p_ins_actions->type = OFPIT_APPLY_ACTIONS;
        ret = json_parse_actions(item->child, &(p_ins_actions->actions));
        json_free_value(&item);

        if (GN_OK != ret)
        {
            return ret;
        }
    }

    item = json_find_first_label(obj, "writeAction");
    if (item)
    {
        gn_instruction_actions_t *p_ins_actions = (gn_instruction_actions_t *)mem_get(g_gninstruction_mempool_id);
        if(list_end)
        {
            list_end->next = (gn_instruction_t *)p_ins_actions;
            list_end = list_end->next;
        }
        else
        {
            *instructions = (gn_instruction_t *) p_ins_actions;
            list_end = *instructions;
        }
        p_ins_actions->type = OFPIT_WRITE_ACTIONS;
        ret = json_parse_actions(item->child, &(p_ins_actions->actions));
        json_free_value(&item);

        if (GN_OK != ret)
        {
            return ret;
        }
    }

    item = json_find_first_label(obj, "clearAction");
    if (item)
    {
        gn_instruction_t *p_ins_actions = (gn_instruction_t *)mem_get(g_gninstruction_mempool_id);
        if(list_end)
        {
            list_end->next = (gn_instruction_t *)p_ins_actions;
            list_end = list_end->next;
        }
        else
        {
            *instructions = (gn_instruction_t *) p_ins_actions;
            list_end = *instructions;
        }
        p_ins_actions->type = OFPIT_CLEAR_ACTIONS;
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "gotoTable");
    if (item)
    {
        gn_instruction_goto_table_t *p_ins_actions = (gn_instruction_goto_table_t *)mem_get(g_gninstruction_mempool_id);
        if(list_end)
        {
            list_end->next = (gn_instruction_t *)p_ins_actions;
            list_end = list_end->next;
        }
        else
        {
            *instructions = (gn_instruction_t *) p_ins_actions;
            list_end = *instructions;
        }
        p_ins_actions->type = OFPIT_GOTO_TABLE;
        p_ins_actions->table_id = atoi(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "meter");
    if (item)
    {
        gn_instruction_meter_t *p_ins_actions = (gn_instruction_meter_t *)mem_get(g_gninstruction_mempool_id);
        if(list_end)
        {
            list_end->next = (gn_instruction_t *)p_ins_actions;
            list_end = list_end->next;
        }
        else
        {
            *instructions = (gn_instruction_t *) p_ins_actions;
            list_end = *instructions;
        }
        p_ins_actions->type = OFPIT_METER;
        p_ins_actions->meter_id = atoll(item->child->text);
        json_free_value(&item);
    }

    return GN_OK;
}

static INT1 *post_flow_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_flow_t *flow = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    flow = (gn_flow_t *)mem_get(g_gnflow_mempool_id);
    if (NULL == flow)
    {
        return json_to_reply(NULL, GN_ERR);
    }

    item = json_find_first_label(root, "tableId");
    if (item)
    {
        flow->table_id = atoi(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "idleTimeout");
    if (item)
    {
        flow->idle_timeout = atoi(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "hardTimeout");
    if (item)
    {
        flow->hard_timeout = atoi(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "priority");
    if (item)
    {
        flow->priority = atoi(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "match");
    if (item)
    {
        json_parse_oxm_fields(item->child, &(flow->match.oxm_fields));
        json_free_value(&item);
    }
    else
    {
        memset(&flow->match, 0, sizeof(gn_match_t));
    }
    flow->match.type = OFPMT_OXM;

    item = json_find_first_label(root, "instruction");
    if (item)
    {
        json_parse_instructions(item->child, &(flow->instructions));
        json_free_value(&item);
    }

    strncpy(flow->creater, "Restful", strlen("Restful"));
    flow->create_time = g_cur_sys_time.tv_sec;
    ret = add_flow_entry(sw, flow);
    if(ret != GN_OK)
    {
        gn_flow_free(flow);
        return json_to_reply(NULL, ret);
    }

    ret = enable_flow_entry(sw, flow);

    return json_to_reply(NULL, ret);
}

static INT1 *put_flow_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_flow_t *flow = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, GN_ERR);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    flow = (gn_flow_t *)mem_get(g_gnflow_mempool_id);
    if (NULL == flow)
    {
        return json_to_reply(NULL, GN_ERR);
    }

    item = json_find_first_label(root, "uuid");
    if (item)
    {
        strncpy(flow->uuid, item->child->text, UUID_LEN - 1);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "instruction");
    if (item)
    {
        json_parse_instructions(item->child, &(flow->instructions));
        json_free_value(&item);
    }

    strncpy(flow->creater, "Restful", strlen("Restful"));
    flow->create_time = g_cur_sys_time.tv_sec;
    ret = modify_flow_entry(sw, flow);

    return json_to_reply(NULL, ret);
}

static INT1 *del_flow_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_flow_t *flow = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, GN_ERR);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    flow = (gn_flow_t *) gn_malloc(sizeof(gn_flow_t));
    if (NULL == flow)
    {
        return json_to_reply(NULL, GN_ERR);
    }

    item = json_find_first_label(root, "uuid");
    if (item)
    {
        strncpy(flow->uuid, item->child->text, UUID_LEN - 1);
        json_free_value(&item);
    }

//    item = json_find_first_label(root, "instruction");
//    if (item)
//    {
//        json_parse_instructions(item->child, &(flow->instructions));
//        json_free_value(&item);
//    }
//
//    strncpy(flow->creater, "Restful", strlen("Restful"));
//    flow->create_time = g_cur_sys_time.tv_sec;
    ret = delete_flow_entry(sw, flow);

    return json_to_reply(NULL, ret);
}

/****************************************************
 * meter mod
 ****************************************************/
static INT1 *get_meter_entries(const INT1 *url, json_t *root)
{
    INT1 json_temp[1024];
    UINT1 dpid[8];
    UINT4 idx = 0;
    gn_meter_t *p_meter = NULL;
    json_t *obj, *sw_array, *sw_obj, *meter_array, *meter_obj, *key, *value = NULL;

    obj = json_new_object();
    key = json_new_string("switchMeters");
    sw_array = json_new_array();
    json_insert_child(key, sw_array);
    json_insert_child(obj, key);

    for (; idx < g_server.max_switch; idx++)
    {
        if (g_server.switches[idx].state == 1)
        {
            sw_obj = json_new_object();
            json_insert_child(sw_array, sw_obj);

            ulli64_to_uc8(g_server.switches[idx].dpid, dpid);
            sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                    dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                    dpid[7]);

            key = json_new_string("DPID");
            value = json_new_string(json_temp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            key = json_new_string("meters");
            meter_array = json_new_array();
            json_insert_child(key, meter_array);
            json_insert_child(sw_obj, key);

            p_meter = g_server.switches[idx].meter_entries;
            while(p_meter)
            {
                meter_obj = json_new_object();
                json_insert_child(meter_array, meter_obj);

                key = json_new_string("meterId");
                sprintf(json_temp, "%d", p_meter->meter_id);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(meter_obj, key);

                key = json_new_string("flags");
                if(p_meter->flags == OFPMF_KBPS)
                {
                    value = json_new_string("Kbps");
                }
                else if(p_meter->flags == OFPMF_PKTPS)
                {
                    value = json_new_string("Pktps");
                }
                else if(p_meter->flags == OFPMF_BURST)
                {
                    value = json_new_string("Burst");
                }
                else  //(p_meter->flags == OFPMF_STATS)
                {
                    value = json_new_string("Stats");
                }
                json_insert_child(key, value);
                json_insert_child(meter_obj, key);

                key = json_new_string("type");
                if(p_meter->type == OFPMBT_DROP)
                {
                    value = json_new_string("DROP");
                }
                else
                {
                    value = json_new_string("DSCP");
                }

                json_insert_child(key, value);
                json_insert_child(meter_obj, key);

                key = json_new_string("precLevel");
                sprintf(json_temp, "%d", p_meter->prec_level);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(meter_obj, key);

                key = json_new_string("rate");
                snprintf(json_temp, 16, "%d", p_meter->rate);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(meter_obj, key);

                key = json_new_string("burstSize");
                snprintf(json_temp, 16, "%d", p_meter->burst_size);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(meter_obj, key);

                p_meter = p_meter->next;
            }
        }
    }

    return json_to_reply(obj, GN_OK);
}

static INT1 *del_meter_entries(const INT1 *url, json_t *root)
{
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    clear_meter_entries(sw);
    return json_to_reply(NULL, GN_OK);
}

static INT1 *post_meter_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_meter_t *meter = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }
    else
    {
    	return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));
    item = json_find_first_label(root, "meterId");
    if (item)
    {
        meter->meter_id = atoll(item->child->text);
        json_free_value(&item);
    }
    else
    {
    	return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
    }

    item = json_find_first_label(root, "flags");
    if (item)
    {
        if(0 == strcmp(item->child->text, "Kbps"))
        {
            meter->flags = OFPMF_KBPS;
        }
        else if(0 == strcmp(item->child->text, "Pktps"))
        {
            meter->flags = OFPMF_PKTPS;
        }
        else if(0 == strcmp(item->child->text, "Burst"))
        {
            meter->flags = OFPMF_BURST;
        }
        else if(0 == strcmp(item->child->text, "Stats"))
        {
            meter->flags = OFPMF_STATS;
        }
        else
        {
            json_free_value(&item);
            ret = EC_METER_INVALID_FLAG;
            free(meter);
            goto EXIT;
        }

        json_free_value(&item);
    }

    item = json_find_first_label(root, "type");
    if (item)
    {
        if(0 == strcmp(item->child->text, "Drop"))
        {
            meter->type = OFPMBT_DROP;
        }
        else if(0 == strcmp(item->child->text, "Dscp"))
        {
            meter->type = OFPMBT_DSCP_REMARK;
        }
        else
        {
            json_free_value(&item);
            ret = EC_METER_INVALID_TYPE;
            free(meter);
            goto EXIT;
        }

        json_free_value(&item);
    }

    item = json_find_first_label(root, "rate");
    if (item)
    {
        meter->rate = atoll(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "precLevel");
    if (item)
    {
        meter->prec_level = atoi(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "burstSize");
    if (item)
    {
        meter->burst_size = atoll(item->child->text);
        json_free_value(&item);
    }

    ret = add_meter_entry(sw, meter);

EXIT:
    return json_to_reply(NULL, ret);
}

static INT1 *put_meter_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_meter_t *meter = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));
    item = json_find_first_label(root, "meterId");
    if (item)
    {
        meter->meter_id = atoll(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "flags");
    if (item)
    {
        if(0 == strcmp(item->child->text, "Kbps"))
        {
            meter->flags = OFPMF_KBPS;
        }
        else if(0 == strcmp(item->child->text, "Pktps"))
        {
            meter->flags = OFPMF_PKTPS;
        }
        else if(0 == strcmp(item->child->text, "Burst"))
        {
            meter->flags = OFPMF_BURST;
        }
        else if(0 == strcmp(item->child->text, "Stats"))
        {
            meter->flags = OFPMF_STATS;
        }
        else
        {
            json_free_value(&item);
            ret = EC_METER_INVALID_FLAG;
            free(meter);
            goto EXIT;
        }

        json_free_value(&item);
    }

    item = json_find_first_label(root, "type");
    if (item)
    {
        if(0 == strcmp(item->child->text, "Drop"))
        {
            meter->type = OFPMBT_DROP;
        }
        else if(0 == strcmp(item->child->text, "Dscp"))
        {
            meter->type = OFPMBT_DSCP_REMARK;
        }
        else
        {
            json_free_value(&item);
            ret = EC_METER_INVALID_TYPE;
            free(meter);
            goto EXIT;
        }

        json_free_value(&item);
    }

    item = json_find_first_label(root, "rate");
    if (item)
    {
        meter->rate = atoll(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "precLevel");
    if (item)
    {
        meter->prec_level = atoi(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "burstSize");
    if (item)
    {
        meter->burst_size = atoll(item->child->text);
        json_free_value(&item);
    }

    ret = modify_meter_entry(sw, meter);

EXIT:
    return json_to_reply(NULL, ret);
}

static INT1 *del_meter_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_meter_t *meter = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));
    item = json_find_first_label(root, "meterId");
    if (item)
    {
        meter->meter_id = atoll(item->child->text);
        json_free_value(&item);
    }

    ret = delete_meter_entry(sw, meter);
    free(meter);
    return json_to_reply(NULL, ret);
}

/****************************************************
 * group mod
 ****************************************************/
static INT1 *get_group_entries(const INT1 *url, json_t *root)
{
    INT1 json_temp[1024];
    UINT1 dpid[8];
    UINT4 idx = 0;
    gn_group_t *p_group = NULL;
    group_bucket_t *p_bucket = NULL;
    json_t *obj, *sw_array, *sw_obj, *group_array, *group_obj, *bucket_array,
         *bucket_obj, *key, *value = NULL;

    obj = json_new_object();
    key = json_new_string("switchGroups");
    sw_array = json_new_array();
    json_insert_child(key, sw_array);
    json_insert_child(obj, key);

    for (; idx < g_server.max_switch; idx++)
    {
        if (g_server.switches[idx].state == 1)
        {
            sw_obj = json_new_object();
            json_insert_child(sw_array, sw_obj);

            ulli64_to_uc8(g_server.switches[idx].dpid, dpid);
            sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                    dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                    dpid[7]);

            key = json_new_string("DPID");
            value = json_new_string(json_temp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            key = json_new_string("groups");
            group_array = json_new_array();
            json_insert_child(key, group_array);
            json_insert_child(sw_obj, key);

            p_group = g_server.switches[idx].group_entries;
            while(p_group)
            {
                group_obj = json_new_object();
                json_insert_child(group_array, group_obj);

                key = json_new_string("type");
                if(p_group->type == OFPGT_ALL)
                {
                    sprintf(json_temp, "%s", "All");
                }
                else if(p_group->type == OFPGT_SELECT)
                {
                    sprintf(json_temp, "%s", "Select");
                }
                else if(p_group->type == OFPGT_INDIRECT)
                {
                    sprintf(json_temp, "%s", "Indirect");
                }
                else  //if(p_group->type == OFPGT_FF)
                {
                    sprintf(json_temp, "%s", "FF");
                }
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(group_obj, key);

                key = json_new_string("groupId");
                sprintf(json_temp, "%d", p_group->group_id);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(group_obj, key);

                key = json_new_string("buckets");
                bucket_array = json_new_array();
                json_insert_child(key, bucket_array);
                json_insert_child(group_obj, key);

                p_bucket = p_group->buckets;
                while(p_bucket)
                {
                    bucket_obj = json_new_object();
                    json_insert_child(bucket_array, bucket_obj);

                    key = json_new_string("weight");
                    sprintf(json_temp, "%d", p_bucket->weight);
                    value = json_new_string(json_temp);
                    json_insert_child(key, value);
                    json_insert_child(bucket_obj, key);

                    key = json_new_string("watchPort");
                    sprintf(json_temp, "%d", p_bucket->watch_port);
                    value = json_new_string(json_temp);
                    json_insert_child(key, value);
                    json_insert_child(bucket_obj, key);

                    key = json_new_string("watchGroup");
                    sprintf(json_temp, "%d", p_bucket->watch_group);
                    value = json_new_string(json_temp);
                    json_insert_child(key, value);
                    json_insert_child(bucket_obj, key);

                    key = json_new_string("actions");
                    value = json_new_object();
                    json_insert_child(key, value);
                    json_insert_child(bucket_obj, key);
                    json_add_actions(value, p_bucket->actions);

                    p_bucket = p_bucket->next;
                }

                p_group = p_group->next;
            }
        }
    }

    return json_to_reply(obj, GN_OK);
}

static INT1 *del_group_entries(const INT1 *url, json_t *root)
{
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    clear_group_entries(sw);
    return json_to_reply(NULL, GN_OK);
}

static INT1 *post_group_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_group_t *group = NULL;
    group_bucket_t *bucket = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }
    else
    {
    	return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    group = (gn_group_t *)gn_malloc(sizeof(gn_group_t));
    item = json_find_first_label(root, "groupId");
    if (item)
    {
        group->group_id = atoll(item->child->text);
        json_free_value(&item);
    }
    else
    {
    	return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
    }

    item = json_find_first_label(root, "type");
    if (item)
    {
        if(0 == strcmp(item->child->text, "All"))
        {
            group->type = OFPGT_ALL;
        }
        else if(0 == strcmp(item->child->text, "Select"))
        {
            group->type = OFPGT_SELECT;
        }
        else if(0 == strcmp(item->child->text, "Indirect"))
        {
            group->type = OFPGT_INDIRECT;
        }
        else if(0 == strcmp(item->child->text, "FF"))
        {
            group->type = OFPGT_FF;
        }
        else
        {
            json_free_value(&item);
            ret = EC_GROUP_INVALID_TYPE;
            free(group);
            goto EXIT;
        }

        json_free_value(&item);
    }

    item = json_find_first_label (root, "buckets");
    if(item)
    {
        json_t *buckets = item->child->child;
        json_t *bucket_item = NULL;

        while(buckets)
        {
            bucket = (group_bucket_t *)gn_malloc(sizeof(group_bucket_t));
            bucket->next = group->buckets;
            group->buckets = bucket;

            bucket_item = json_find_first_label(buckets, "weight");
            if(bucket_item)
            {
                bucket->weight = atoi(bucket_item->child->text);
                json_free_value(&bucket_item);
            }

            if(group->type == OFPGT_FF)
            {
                bucket_item = json_find_first_label(buckets, "watchPort");
            if(bucket_item)
            {
                bucket->watch_port = atoll(bucket_item->child->text);
                json_free_value(&bucket_item);
            }

                bucket_item = json_find_first_label(buckets, "watchGroup");
            if(bucket_item)
            {
                bucket->watch_group = atoll(bucket_item->child->text);
                json_free_value(&bucket_item);
            }
            }
            else
            {
                bucket->watch_port = OFPP13_ANY;
                bucket->watch_group = OFPG_ANY;
            }

            bucket_item = json_find_first_label(buckets, "actions");
            if(bucket_item)
            {
                ret = json_parse_actions(bucket_item->child, &(bucket->actions));
                if(GN_OK != ret)
                {
                    json_free_value(&bucket_item);
                    gn_group_free(group);
                    goto EXIT;
                }

                json_free_value(&bucket_item);
            }

            buckets = buckets->next;
        }
        json_free_value(&item);
    }

    ret = add_group_entry(sw, group);
EXIT:
    return json_to_reply(NULL, ret);
}

static INT1 *put_group_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_group_t *group = NULL;
    group_bucket_t *bucket = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    group = (gn_group_t *)gn_malloc(sizeof(gn_group_t));
    item = json_find_first_label(root, "groupId");
    if (item)
    {
        group->group_id = atoll(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "type");
    if (item)
    {
        if(0 == strcmp(item->child->text, "All"))
        {
            group->type = OFPGT_ALL;
        }
        else if(0 == strcmp(item->child->text, "Select"))
        {
            group->type = OFPGT_SELECT;
        }
        else if(0 == strcmp(item->child->text, "Indirect"))
        {
            group->type = OFPGT_INDIRECT;
        }
        else if(0 == strcmp(item->child->text, "FF"))
        {
            group->type = OFPGT_FF;
        }
        else
        {
            json_free_value(&item);
            ret = EC_GROUP_INVALID_TYPE;
            free(group);
            goto EXIT;
        }

        json_free_value(&item);
    }

    item = json_find_first_label (root, "buckets");
    if(item)
    {
        json_t *buckets = item->child->child;
        json_t *bucket_item = NULL;

        while(buckets)
        {
            bucket = (group_bucket_t *)gn_malloc(sizeof(group_bucket_t));
            bucket->next = group->buckets;
            group->buckets = bucket;

            bucket_item = json_find_first_label(buckets, "weight");
            if(bucket_item)
            {
                bucket->weight = atoi(bucket_item->child->text);
                json_free_value(&bucket_item);
            }

            if(group->type == OFPGT_FF)
            {
                bucket_item = json_find_first_label(buckets, "watchPort");
            if(bucket_item)
            {
                bucket->watch_port = atoll(bucket_item->child->text);
                json_free_value(&bucket_item);
            }

                bucket_item = json_find_first_label(buckets, "watchGroup");
            if(bucket_item)
            {
                bucket->watch_group = atoll(bucket_item->child->text);
                json_free_value(&bucket_item);
            }
            }
            else
            {
                bucket->watch_port = OFPP13_ANY;
                bucket->watch_group = OFPG_ANY;
            }

            bucket_item = json_find_first_label(buckets, "actions");
            if(bucket_item)
            {
                ret = json_parse_actions(bucket_item->child, &(bucket->actions));
                if(GN_OK != ret)
                {
                    json_free_value(&bucket_item);
                    gn_group_free(group);
                    goto EXIT;
                }

                json_free_value(&bucket_item);
            }

            buckets = buckets->next;
        }
    }

    ret = modify_group_entry(sw, group);
EXIT:
    return json_to_reply(NULL, ret);
}

static INT1 *del_group_entry(const INT1 *url, json_t *root)
{
    UINT4 ret = 0;
    UINT1 dpid[8] = { 0 };
    UINT8 _dpid = 0;
    json_t *item = NULL;
    gn_switch_t *sw = NULL;
    gn_group_t *group = NULL;

    if (OFPCR_ROLE_SLAVE == g_controller_role)
    {
        return json_to_reply(NULL, EC_ROLE_IS_SLAVE);
    }

    item = json_find_first_label(root, "DPID");
    if (item)
    {
        mac_str_to_bin(item->child->text, dpid);
        json_free_value(&item);
    }

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    group = (gn_group_t *)gn_malloc(sizeof(gn_group_t));
    item = json_find_first_label(root, "groupId");
    if (item)
    {
        group->group_id = atoll(item->child->text);
        json_free_value(&item);
    }

    ret = delete_group_entry(sw, group);
    return json_to_reply(NULL, ret);
}

/****************************************************
 * stats
 ****************************************************/
static INT1 *get_path(const const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    json_t *entry = NULL;
    json_t *array = NULL;
    json_t *key = NULL;
    json_t *value = NULL;
    INT1 tmp[32] = {0};
    UINT1 dpid[8];

    INT4 src_index_id = 0;
    INT4 dst_index_id = 0;
    INT4 index_tmp = 0;

    gn_switch_t *src_sw = NULL;
    gn_switch_t *dst_sw = NULL;
    gn_switch_t *sw_pre = NULL;
    gn_switch_t *sw_next = NULL;
    UINT4 port_pre = 0;
    UINT4 port_next = 0;

    UINT8 src_dpid = 0, dst_dpid = 0;
    UINT4 src_port = 0, dst_port = 0;
    key_value_t kv_src_dpid, kv_src_port, kv_dst_dpid, kv_dst_port;
    kv_src_dpid.key = strdup("srcDPID");
    kv_src_dpid.value = NULL;

    kv_src_port.key = strdup("srcPort");
    kv_src_port.value = NULL;

    kv_dst_dpid.key = strdup("dstDPID");
    kv_dst_dpid.value = NULL;

    kv_dst_port.key = strdup("dstPort");
    kv_dst_port.value = NULL;

    get_url_argument(url + strlen("/gn/path/json") + 1, &kv_src_dpid);
    if (NULL == kv_src_dpid.value)
    {
        return json_to_reply(NULL, EC_RESTFUL_REQUIRE_SRCDPID);
    }
    get_url_argument(url + strlen("/gn/path/json") + 1, &kv_src_port);

    get_url_argument(url + strlen("/gn/path/json") + 1, &kv_dst_dpid);
    if (NULL == kv_dst_dpid.value)
    {
        return json_to_reply(NULL, EC_RESTFUL_REQUIRE_DSTDPID);
    }
    get_url_argument(url + strlen("/gn/path/json") + 1, &kv_src_port);

    mac_str_to_bin(kv_src_dpid.value, dpid);
    uc8_to_ulli64(dpid, &src_dpid);
    mac_str_to_bin(kv_dst_dpid.value, dpid);
    uc8_to_ulli64(dpid, &dst_dpid);

    if(kv_src_port.value)
    {
        src_dpid = strtoull(kv_src_port.value, NULL, 10);
    }

    if(kv_dst_port.value)
    {
        dst_dpid = strtoull(kv_dst_port.value, NULL, 10);
    }

    free(kv_src_dpid.key);
    free(kv_src_dpid.value);
    free(kv_dst_dpid.key);
    free(kv_dst_dpid.value);
    free(kv_src_port.key);
    free(kv_src_port.value);
    free(kv_dst_port.key);
    free(kv_dst_port.value);

    src_sw = find_sw_by_dpid(src_dpid);
    dst_sw = find_sw_by_dpid(dst_dpid);
    if((NULL == src_sw) || (NULL == dst_sw))
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    src_index_id = src_sw->index;
    dst_index_id = dst_sw->index;
    if((src_index_id < 0) || (dst_index_id < 0))
    {
        return json_to_reply(NULL, EC_SW_NO_PATH);
    }

    index_tmp = g_short_path[src_index_id][dst_index_id];
    if(NO_PATH == index_tmp)
    {
        return json_to_reply(NULL, EC_SW_NO_PATH);
    }
    if((src_port > 0) && (src_port != g_adac_matrix.src_port[src_index_id][index_tmp]))
    {
        return json_to_reply(NULL, EC_SW_NO_PATH);
    }

    index_tmp = g_short_path[dst_index_id][src_index_id];
    if(NO_PATH == index_tmp)
    {
        return json_to_reply(NULL, EC_SW_NO_PATH);
    }
    if((dst_port > 0) && (dst_port != g_adac_matrix.src_port[dst_index_id][index_tmp]))
    {
        return json_to_reply(NULL, EC_SW_NO_PATH);
    }

    index_tmp = src_index_id;
    array = json_new_array();
    do
    {
        src_index_id = index_tmp;
        index_tmp = g_short_path[src_index_id][dst_index_id];
        if (NO_PATH == index_tmp)
        {
            return json_to_reply(NULL, EC_SW_NO_PATH);
        }

        //锟斤拷前锟斤拷锟斤拷锟斤拷
        sw_pre = g_adac_matrix.sw[src_index_id][index_tmp];
        if (NULL == sw_pre)
        {
            return json_to_reply(NULL, EC_SW_NO_PATH);
        }
        //锟斤拷前锟斤拷锟斤拷锟斤拷锟斤拷锟?
        port_pre = g_adac_matrix.src_port[src_index_id][index_tmp];

        //锟斤拷一锟斤拷锟斤拷锟斤拷锟斤拷
        sw_next = g_adac_matrix.sw[index_tmp][src_index_id];
        if (NULL == sw_next)
        {
            return json_to_reply(NULL, EC_SW_NO_PATH);
        }
        port_next = g_adac_matrix.src_port[index_tmp][src_index_id];

        entry = json_new_object();
        //source switch info
        key = json_new_string("srcDPID");
        ulli64_to_uc8(sw_pre->dpid, dpid);
        sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0], dpid[1],
                dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
        value = json_new_string(tmp);
        json_insert_child(key, value);
        json_insert_child(entry, key);

        key = json_new_string("srcPort");
        sprintf(tmp, "%d", port_pre);
        value = json_new_number(tmp);
        json_insert_child(key, value);
        json_insert_child(entry, key);

        //destination switch info
        key = json_new_string("dstDPID");
        ulli64_to_uc8(sw_next->dpid, dpid);
        sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0], dpid[1],
                dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
        value = json_new_string(tmp);
        json_insert_child(key, value);
        json_insert_child(entry, key);

        key = json_new_string("dstPort");
        sprintf(tmp, "%d", port_next);
        value = json_new_number(tmp);
        json_insert_child(key, value);
        json_insert_child(entry, key);

        json_insert_child(array, entry);
    }
    while (dst_index_id != g_short_path[src_index_id][dst_index_id]);

    key = json_new_string("path");
    json_insert_child(key, array);
    json_insert_child(obj, key);

    return json_to_reply(obj, GN_OK);
}

/****************************************************
 * example: http://192.168.1.1:8081/gn/path/port/json&ip=100.0.0.100
 ****************************************************/
static INT1 *get_port_stats(const INT1 *url, json_t *root)
{
	INT1 temp[48] = {0};
    UINT4 ip;

    key_value_t host_ip_key;
    host_ip_key.key = strdup("ip");
    host_ip_key.value = NULL;

    get_url_argument(url + strlen("/gn/path/port/json") + 1, &host_ip_key);
    if (NULL == host_ip_key.value)
    {
        free(host_ip_key.key);
        free(host_ip_key.value);
        return json_to_reply(NULL, GN_ERR);
    }

    ip = ip2number(host_ip_key.value);

    if (ip) {
        json_t *obj, *key, *value;
        obj = json_new_object();
    
        key = json_new_string("host ip");
        value = json_new_string( host_ip_key.value);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    
        p_fabric_host_node host = get_fabric_host_from_list_by_ip(ip);
    
        if ((host) && (host->sw) && (host->port)) {    
            gn_port_t *port = NULL;
            gn_switch_t* src_sw = host->sw;
            UINT4 port_no = host->port;

	        INT4 idx = 0;
	        for (; idx < src_sw->n_ports; idx++)
            {
                port = &(src_sw->ports[idx]);
                if (port->port_no == port_no)
                {
                    key = json_new_string("rxUsedRate");
                    sprintf(temp, "%0.2lf",
                            port->stats.rx_kbps / port->stats.max_speed);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("txUsedRate");
                    sprintf(temp, "%0.2lf",
                            port->stats.tx_kbps / port->stats.max_speed);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("rxUsedStatus");
					sprintf(temp, "%d", get_load_stats(port->stats.rx_kbps / port->stats.max_speed));
					value = json_new_number(temp);
					json_insert_child(key, value);
					json_insert_child(obj, key);
					
					key = json_new_string("txUsedStatus");
					sprintf(temp, "%d", get_load_stats(port->stats.tx_kbps / port->stats.max_speed));
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("rxKbps");
                    sprintf(temp, "%0.2lf", port->stats.rx_kbps);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("txKbps");
                    sprintf(temp, "%0.2lf", port->stats.tx_kbps);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("rxKpps");
                    sprintf(temp, "%0.2lf", port->stats.rx_kpps);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("txKpps");
                    sprintf(temp, "%0.2lf", port->stats.tx_kpps);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("rxPackets");
                    sprintf(temp, "%llu", port->stats.rx_packets);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("txPackets");
                    sprintf(temp, "%llu", port->stats.tx_packets);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("rxBytes");
                    sprintf(temp, "%llu", port->stats.rx_bytes);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("txBytes");
                    sprintf(temp, "%llu", port->stats.tx_bytes);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("maxSpeed");
                    sprintf(temp, "%d", port->stats.max_speed);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);

                    key = json_new_string("durationSec");
                    sprintf(temp, "%d", port->stats.duration_sec);
                    value = json_new_number(temp);
                    json_insert_child(key, value);
                    json_insert_child(obj, key);
                }
            }
        	return json_to_reply(obj, GN_OK);
    	}
	}
    return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
}


static INT1 *get_flow_stats(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    gn_switch_t *sw_flow = NULL;
    json_t *key, *value;
    UINT1 tmp[8];
    INT1 temp[32] = { 0 };
    UINT8 sw_dpid = 0;
    UINT4 ip_change = 0;

    key_value_t swDPID, ip;
    swDPID.key = strdup("swDPID");
    swDPID.value = NULL;

    ip.key = strdup("ip");
    ip.value = NULL;

    get_url_argument(url + strlen("/gn/flow/stats/json") + 1, &swDPID);
    if (NULL == swDPID.value)
    {
        return json_to_reply(NULL, EC_RESTFUL_REQUIRE_SRCDPID);
    }

    get_url_argument(url + strlen("/gn/flow/stats/json") + 1, &ip);
    if (NULL != ip.value)
    {
        ip_change = inet_addr(ip.value);
    }
    mac_str_to_bin(swDPID.value, tmp);
    uc8_to_ulli64(tmp, &sw_dpid);

    gn_flow_t *flow = NULL;
    sw_flow = find_sw_by_dpid(sw_dpid);

    free(ip.key);
    free(swDPID.key);
    free(ip.value);
    free(swDPID.value);

    if (sw_flow)
    {
        flow = sw_flow->flow_entries;
        //UINT4 port_no = g_adac_matrix.src_port[src_sw->index][dst_sw->in->x];

        json_t *array = json_new_array();
        while(flow)
        {
            json_t *obj2 = json_new_object();
            UINT2 priority = flow->priority;
            UINT4 ip_dst = NULL;
            ip_dst = ntohl(flow->match.oxm_fields.ipv4_dst);
            if( (ip_change!=0 && ip_dst==ip_change) || (ip_change==0 && priority ==16) ){
                //nat_show_ip(ip_dst);

                key = json_new_string("ip");
                strcpy(temp, inet_ntoa(*(struct in_addr*)&ip_dst));
                value = json_new_string(temp);
                json_insert_child(key, value);
                json_insert_child(obj2, key);

                key = json_new_string("priority");
                sprintf(temp, "%u",priority);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj2, key);

                key = json_new_string("kbps");
                sprintf(temp, "%d",flow->stats.kbps);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj2, key);

                key = json_new_string("kpps");
                sprintf(temp, "%d",flow->stats.kpps);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj2, key);

                key = json_new_string("count");
                sprintf(temp, "%llu",flow->stats.byte_count);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj2, key);

                key = json_new_string("packets");
                sprintf(temp, "%llu",flow->stats.packet_count);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj2, key);

                key = json_new_string("duration");
                sprintf(temp, "%d",flow->stats.duration_sec);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj2, key);
                json_insert_child(array,obj2);

            }
            flow = flow->next;
        }
        key = json_new_string("flowstats");
        json_insert_child(key, array);
        json_insert_child(obj, key);
    }else{
        LOG_PROC("ERROR","sw_flow NULL");
       // LOG_PROC("ERROR",sw.key);
    }
    return json_to_reply(obj, GN_OK);
}

static INT1 *get_all_flow_stats(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    gn_switch_t *sw_flow = NULL;
    json_t *key, *value;
    INT1 temp[32] = { 0 };
    INT4 num;
    json_t *array = json_new_array();
    gn_flow_t *flow = NULL;
    for(num=0; num < g_server.max_switch; num++)
    {
        if (g_server.switches[num].state)
        {
            sw_flow = &g_server.switches[num];
            if(sw_flow)
            {
                flow = sw_flow->flow_entries;
        //UINT4 port_no = g_adac_matrix.src_port[src_sw->index][dst_sw->in->x];


                while(flow)
                {
                    json_t *obj2 = json_new_object();
                    UINT2 priority = flow->priority;
                    UINT4 ip_dst = NULL;
                    ip_dst = ntohl(flow->match.oxm_fields.ipv4_dst);
                    if( priority ==16){
                        //nat_show_ip(ip_dst);

                        key = json_new_string("ip");
                        strcpy(temp, inet_ntoa(*(struct in_addr*)&ip_dst));
                        value = json_new_string(temp);
                        json_insert_child(key, value);
                        json_insert_child(obj2, key);

                        key = json_new_string("priority");
                        sprintf(temp, "%u",priority);
                        value = json_new_number(temp);
                        json_insert_child(key, value);
                        json_insert_child(obj2, key);

                        key = json_new_string("kbps");
                        sprintf(temp, "%d",flow->stats.kbps);
                        value = json_new_number(temp);
                        json_insert_child(key, value);
                        json_insert_child(obj2, key);

                        key = json_new_string("kpps");
                        sprintf(temp, "%d",flow->stats.kpps);
                        value = json_new_number(temp);
                        json_insert_child(key, value);
                        json_insert_child(obj2, key);

                        key = json_new_string("count");
                        sprintf(temp, "%llu",flow->stats.byte_count);
                        value = json_new_number(temp);
                        json_insert_child(key, value);
                        json_insert_child(obj2, key);

                        key = json_new_string("packets");
                        sprintf(temp, "%llu",flow->stats.packet_count);
                        value = json_new_number(temp);
                        json_insert_child(key, value);
                        json_insert_child(obj2, key);

                        key = json_new_string("duration");
                        sprintf(temp, "%d",flow->stats.duration_sec);
                        value = json_new_number(temp);
                        json_insert_child(key, value);
                        json_insert_child(obj2, key);
                        json_insert_child(array,obj2);

                    }
                    flow = flow->next;
                }
            }
        }
    }
    key = json_new_string("flowstats");
    json_insert_child(key, array);
    json_insert_child(obj, key);

    return json_to_reply(obj, GN_OK);
}

static INT1 *clear_floatip_stats(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    gn_switch_t *sw_flow = NULL;
    gn_flow_t *flow = NULL;
    INT4 num;
    for(num=0; num < g_server.max_switch; num++)
    {
        if (g_server.switches[num].state)
        {
            sw_flow = &g_server.switches[num];
            if(sw_flow)
            {
                flow = sw_flow->flow_entries;
                //UINT4 port_no = g_adac_matrix.src_port[src_sw->index][dst_sw->in->x];
                while(flow)
                {
                    UINT2 priority = flow->priority;
                    UINT4 ip_dst = NULL;
                    ip_dst = ntohl(flow->match.oxm_fields.ipv4_dst);
                    if( priority ==16){
                        external_floating_ip_p float_port = get_external_floating_ip_by_floating_ip(ip_dst);
                        p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(float_port->fixed_ip);
                        external_port_p ext_port = find_openstack_external_by_floating_ip(ip_dst);
                        UINT1 *mod_dst_mac = fixed_port->mac;
                        UINT4 outport = 0;
                        UINT4 fixed_vlan_id = of131_fabric_impl_get_tag_sw(fixed_port->sw);
                        outport = get_out_port_between_switch(ext_port->external_dpid, fixed_port->sw->dpid);
                        fabric_openstack_floating_ip_clear_stat(sw_flow, ip_dst, float_port->fixed_ip,  mod_dst_mac, fixed_vlan_id, outport);
                    }
                    flow = flow->next;
                }
            }
        }
    }
    return json_to_reply(obj, GN_OK);
}

static INT1 *get_path_stats(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    json_t *key, *value;

    gn_switch_t *src_sw = NULL;
    gn_switch_t *dst_sw = NULL;

    INT1 temp[32] = { 0 };
    UINT1 tmp[8];
    UINT8 src_dpid = 0;
    UINT8 dst_dpid = 0;
    key_value_t src, dst;
    src.key = strdup("srcDPID");
    src.value = NULL;

    dst.key = strdup("dstDPID");
    dst.value = NULL;

    get_url_argument(url + strlen("/gn/path/stats/json") + 1, &src);
    if (NULL == src.value)
    {
        return json_to_reply(NULL, EC_RESTFUL_REQUIRE_SRCDPID);
    }

    get_url_argument(url + strlen("/gn/path/stats/json") + 1, &dst);
    if (NULL == dst.value)
    {
        return json_to_reply(NULL, EC_RESTFUL_REQUIRE_DSTDPID);
    }

    mac_str_to_bin(src.value, tmp);
    uc8_to_ulli64(tmp, &src_dpid);

    mac_str_to_bin(dst.value, tmp);
    uc8_to_ulli64(tmp, &dst_dpid);
    free(src.key);
    free(src.value);
    free(dst.key);
    free(dst.value);

    src_sw = find_sw_by_dpid(src_dpid);
    dst_sw = find_sw_by_dpid(dst_dpid);

    if (src_sw && dst_sw
            && g_adac_matrix.src_port[src_sw->index][dst_sw->index])
    {
        UINT4 idx = 0;
        gn_port_t *port = NULL;
        UINT4 port_no = g_adac_matrix.src_port[src_sw->index][dst_sw->index];
        for (; idx < src_sw->n_ports; idx++)
        {
            port = &(src_sw->ports[idx]);
            if (port->port_no == port_no)
            {
                key = json_new_string("rxUsedRate");
                sprintf(temp, "%0.2lf",
                        port->stats.rx_kbps / port->stats.max_speed);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("txUsedRate");
                sprintf(temp, "%0.2lf",
                        port->stats.tx_kbps / port->stats.max_speed);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("rxUsedStatus");
                sprintf(temp, "%d", get_load_stats(port->stats.rx_kbps / port->stats.max_speed));
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("txUsedStatus");
                sprintf(temp, "%d", get_load_stats(port->stats.tx_kbps / port->stats.max_speed));
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("rxKbps");
                sprintf(temp, "%0.2lf", port->stats.rx_kbps);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("txKbps");
                sprintf(temp, "%0.2lf", port->stats.tx_kbps);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("rxKpps");
                sprintf(temp, "%0.2lf", port->stats.rx_kpps);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("txKpps");
                sprintf(temp, "%0.2lf", port->stats.tx_kpps);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("rxPackets");
                sprintf(temp, "%llu", port->stats.rx_packets);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("txPackets");
                sprintf(temp, "%llu", port->stats.tx_packets);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("rxBytes");
                sprintf(temp, "%llu", port->stats.rx_bytes);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("txBytes");
                sprintf(temp, "%llu", port->stats.tx_bytes);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("maxSpeed");
                sprintf(temp, "%d", port->stats.max_speed);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("durationSec");
                sprintf(temp, "%d", port->stats.duration_sec);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);
            }
        }
    }

    return json_to_reply(obj, GN_OK);
}

static INT1 *get_path_status(const INT1 *url, json_t *root)
{
    gn_port_t *port = NULL;
    gn_switch_t *sw = NULL;

    UINT1 dpid_bin[8];
    char tmp_str[32];
    int i, j;
    json_t *array, *Obj, *key, *value, *entry;

    Obj = json_new_object();
    array = json_new_array();
    for (i = 0; i < g_server.max_switch; i++)
    {
        if (g_server.switches[i].state == 1)
        {
            sw = &g_server.switches[i];
            if (sw)
            {
                for (j = 0; j < sw->n_ports; j++)
                {
                    if (sw->neighbor[j] && sw->neighbor[j]->sw)
                    {
                        entry = json_new_object();
                        port = &(sw->ports[j]);

                        ulli64_to_uc8(sw->dpid, dpid_bin);
                        sprintf(tmp_str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                                dpid_bin[0], dpid_bin[1], dpid_bin[2],
                                dpid_bin[3], dpid_bin[4], dpid_bin[5],
                                dpid_bin[6], dpid_bin[7]);
                        key = json_new_string("srcDPID");
                        value = json_new_string(tmp_str);
                        json_insert_child(key, value);
                        json_insert_child(entry, key);

                        ulli64_to_uc8(sw->neighbor[j]->sw->dpid, dpid_bin);
                        sprintf(tmp_str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                                dpid_bin[0], dpid_bin[1], dpid_bin[2],
                                dpid_bin[3], dpid_bin[4], dpid_bin[5],
                                dpid_bin[6], dpid_bin[7]);
                        key = json_new_string("dstDPID");
                        value = json_new_string(tmp_str);
                        json_insert_child(key, value);
                        json_insert_child(entry, key);

                        key = json_new_string("status");
                        sprintf(tmp_str, "%d", get_load_stats(port->stats.tx_kbps / port->stats.max_speed));
                        value = json_new_number(tmp_str);
                        json_insert_child(key, value);
                        json_insert_child(entry, key);

                        json_insert_child(array, entry);
                    }
                }
            }
        }
    }

    key = json_new_string("pathStatus");
    json_insert_child(key, array);
    json_insert_child(Obj, key);

    return json_to_reply(Obj, GN_OK);
}

/****************************************************
 * tenant
 ****************************************************/
static INT1 *get_tenant(const INT1 *url, json_t *root)
{
    json_t *obj = NULL;
    json_t *array = NULL;
    json_t *entry = NULL;
    json_t *key = NULL;
    json_t *value = NULL;
    char tmp[16] = { 0 };
    tenant_container_t* tenant_con = query_tenants();

    obj = json_new_object();
    if (tenant_con && tenant_con->tenants)
    {
        int idx = 0;

        array = json_new_array();
        for (; idx < tenant_con->max_tenants; idx++)
        {
            if (tenant_con->tenants[idx])
            {
                entry = json_new_object();

                key = json_new_string("tenantId");
                snprintf(tmp, 16, "%d", tenant_con->tenants[idx]->tenant_id);
                value = json_new_string(tmp);
                json_insert_child(key, value);
                json_insert_child(entry, key);

                key = json_new_string("tenantName");
                value = json_new_string(tenant_con->tenants[idx]->tenant_name);
                json_insert_child(key, value);
                json_insert_child(entry, key);

                json_insert_child(array, entry);
            }
        }

        key = json_new_string("tenants");
        json_insert_child(key, array);
        json_insert_child(obj, key);
    }

    return json_to_reply(obj, GN_OK);
}

static INT1 *post_tenant(const INT1 *url, json_t *root)
{
    INT4 ret = -1;
    json_t *item = NULL;

    item = json_find_first_label(root, "tenanName");
    if (item)
    {
        if (item->child->text[0] == '\0')
        {
            return json_to_reply(NULL, EC_RESTFUL_REQUIRE_TENANT_NM);
        }

        ret = create_tenant(item->child->text);
        json_free_value(&item);
    }

    return json_to_reply(NULL, ret);
}

static INT1 *del_tenant(const INT1 *url, json_t *root)
{
    INT4 ret = -1;
    json_t *item = NULL;
    UINT4 tenant_id = 0;

    item = json_find_first_label(root, "tenantId");
    if (item)
    {
        tenant_id = atoll(item->child->text);

        ret = delete_tenant(tenant_id);
        json_free_value(&item);
    }

    return json_to_reply(NULL, ret);
}

static INT1 *get_tenant_member(const INT1 *url, json_t *root)
{
    json_t *obj = NULL;
    json_t *entry = NULL;
    json_t *array = NULL;
    json_t *key = NULL;
    json_t *value = NULL;
    INT1 tmp[32] = { 0 };
    tenant_t* tenant = NULL;
    tenant_container_t* tenant_con = query_tenants();

    key_value_t tenant_id;
    tenant_id.key = strdup("tenantId");
    tenant_id.value = NULL;

    get_url_argument(url + strlen("/gn/tenant/member/json") + 1, &tenant_id);
    if (NULL == tenant_id.value)
    {
        free(tenant_id.key);
        free(tenant_id.value);
        return json_to_reply(NULL, GN_ERR);
    }

    tenant = tenant_con->tenants[atoll(tenant_id.value)];
    obj = json_new_object();
    if (tenant && tenant->member_mac)
    {
        int idx = 0;

        key = json_new_string("tenantId");
        snprintf(tmp, 16, "%d", tenant->tenant_id);
        value = json_new_string(tmp);
        json_insert_child(key, value);
        json_insert_child(obj, key);

        key = json_new_string("tenantName");
        value = json_new_string(tenant->tenant_name);
        json_insert_child(key, value);
        json_insert_child(obj, key);

        array = json_new_array();
        for (; idx < tenant_con->max_memebers; idx++)
        {
            if (tenant->member_mac[idx])
            {
                entry = json_new_object();
                key = json_new_string("mac");
                mac2str(tenant->member_mac[idx], tmp);
                value = json_new_string(tmp);
                json_insert_child(key, value);
                json_insert_child(entry, key);

                json_insert_child(array, entry);
            }
        }

        key = json_new_string("members");
        json_insert_child(key, array);
        json_insert_child(obj, key);
    }

    free(tenant_id.key);
    free(tenant_id.value);
    return json_to_reply(obj, GN_OK);
}

static INT1 *post_tenant_member(const INT1 *url, json_t *root)
{
    INT4 ret = -1;
    UINT4 tenant_id = 0;
    UINT1 mac[6] = { 0 };
    json_t *item = NULL;

    item = json_find_first_label(root, "tenantId");
    if (item)
    {
        tenant_id = atoll(item->child->text);

        json_free_value(&item);
    }
    else
    {
        return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
    }

    item = json_find_first_label(root, "mac");
    if (item)
    {
        if (item->child->text[0] == '\0')
        {
            return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
        }

        json_free_value(&item);
    }
    else
    {
        return json_to_reply(NULL, EC_RESTFUL_INVALID_ARGUMENTS);
    }

    ret = add_tenant_member(tenant_id, mac);
    return json_to_reply(NULL, ret);
}

static INT1 *del_tenant_member(const INT1 *url, json_t *root)
{
    INT1 ret = -1;
    UINT1 mac[6] = { 0 };
    json_t *item = NULL;

    item = json_find_first_label(root, "mac");
    if (item)
    {
        macstr2hex(item->child->text, mac);

        ret = delete_tenant_member(mac);
        json_free_value(&item);
    }

    return json_to_reply(NULL, ret);
}
/*****************************************************
 * fabrics
 *****************************************************/
static INT1 *get_switch_name(const INT1 *url, json_t *root)
{
    UINT4 i_svr, i_br;
    UINT1 dpid_bin[8];
    INT1 tmp_str[32] = { 0 };
    json_t *obj, *array, *key, *value, *entry;

    obj = json_new_object();
    array = json_new_array();
    for(i_svr = 0; i_svr < OVSDB_MAX_CONNECTION; i_svr++)
    {
        for(i_br = 0; i_br < NEUTRON_BRIDGE_MAX_NUM; i_br++)
        {
            if(g_ovsdb_nodes[i_svr].bridge[i_br].is_using && g_ovsdb_nodes[i_svr].bridge[i_br].dpid)
            {
                entry = json_new_object();

                key = json_new_string("DPID");
                ulli64_to_uc8(g_ovsdb_nodes[i_svr].bridge[i_br].dpid, dpid_bin);
                sprintf(tmp_str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x",
                        dpid_bin[0], dpid_bin[1], dpid_bin[2],
                        dpid_bin[3], dpid_bin[4], dpid_bin[5],
                        dpid_bin[6], dpid_bin[7]);
                value = json_new_string(tmp_str);
                json_insert_child(key, value);
                json_insert_child(entry, key);

                key = json_new_string("name");
                value = json_new_string(g_ovsdb_nodes[i_svr].bridge[i_br].name);
                json_insert_child(key, value);
                json_insert_child(entry, key);

                json_insert_child(array, entry);
            }
        }
    }

    key = json_new_string("switchName");
    json_insert_child(key, array);
    json_insert_child(obj, key);
    return json_to_reply(obj, GN_OK);
}

static INT1 *setup_fabric_entries(const INT1 *url, json_t *root){

	of131_fabric_impl_setup();
    return json_to_reply(NULL, GN_OK);
}

static INT1 *setup_fabric_nat_switch(const INT1 *url, json_t *root)
{
	UINT1 nat_physical_switch_flag = 0;
	json_t *item = NULL;
	item = json_find_first_label(root, "physical");
	if (item) {
		if (item->child->text) {
			nat_physical_switch_flag =  atoi(item->child->text);
			json_free_value(&item);
		}
	}

	// modify the flag value
	update_nat_physical_switch_flag(nat_physical_switch_flag);

	return json_to_reply(NULL, GN_OK);
}
static INT1 *get_all_config_info(const INT1 *url, json_t *root){
	json_t *obj = json_new_object();
	json_t *key = NULL;
	json_t *value = NULL;


	key = json_new_string("ip_match_flows");
	value = json_new_number(get_value(g_controller_configure, "[openvstack_conf]", "ip_match_flows"));
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("auto_fabric");
	value = json_new_number(get_value(g_controller_configure, "[openvstack_conf]", "auto_fabric"));
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("openvstack_on");
	value = json_new_number(get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on"));
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("use_phy");
	value = json_new_number(get_value(g_controller_configure, "[openvstack_conf]", "use_physical_switch_modify_nat"));
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("max_switch");
	value = json_new_number(get_value(g_controller_configure, "[controller]", "max_switch"));
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("buff_num");
	value = json_new_number(get_value(g_controller_configure, "[controller]", "buff_num"));
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("buff_len");
	value = json_new_number(get_value(g_controller_configure, "[controller]", "buff_len"));
	json_insert_child(key, value);
	json_insert_child(obj, key);
	return json_to_reply(obj, GN_OK);
}
static INT1 *set_all_config_info(const INT1 *url, json_t *root){
	json_t *item = NULL;
	UINT1 ipflow = 0;
	UINT1 fabricon = 0;
	UINT1 openstackon = 0;
	UINT1 physupport = 0;
	UINT4 maxswitch = 0;
	UINT4 maxbuff = 0;
	UINT4 maxlength =0;
	item = json_find_first_label(root, "ipflow");
	if(item){
		if(item->child->text){
			ipflow =  atoi(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "fabricon");
	if(item){
		if(item->child->text){
			fabricon =  atoi(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "openstackon");
	if(item){
		if(item->child->text){
			openstackon =  atoi(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "physupport");
	if(item){
		if(item->child->text){
			physupport =  atoi(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "maxlength");
	if(item){
		if(item->child->text){
			maxlength =  atoll(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "maxswitch");
	if(item){
		if(item->child->text){
			maxswitch =  atoll(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "maxbuff");
	if(item){
		if(item->child->text){
			maxbuff =  atoll(item->child->text);
			json_free_value(&item);
		}
	}
	set_value_int(g_controller_configure, "[openvstack_conf]", "ip_match_flows",ipflow);
	set_value_int(g_controller_configure, "[openvstack_conf]", "auto_fabric",fabricon);
	set_value_int(g_controller_configure, "[openvstack_conf]", "openvstack_on",openstackon);
	set_value_int(g_controller_configure, "[openvstack_conf]", "use_physical_switch_modify_nat",physupport);
	set_value_int(g_controller_configure, "[controller]", "max_switch",maxswitch);
	set_value_int(g_controller_configure, "[controller]", "buff_num",maxbuff);
	set_value_int(g_controller_configure, "[controller]", "buff_len",maxlength);
	g_controller_configure = save_ini(g_controller_configure,CONFIGURE_FILE);
	return json_to_reply(NULL, GN_OK);
}
static INT1 *setup_fabric_external(const INT1 *url, json_t *root){
	UINT4 gatwayip=0;
	UINT1 gateway_mac[6]={0};
	UINT4 outip=0;
	UINT1 outer_mac[6]={0};
	UINT8 dpid = 0;
	UINT4 port=0;
	char network_id[48] = {0};
	json_t *item = NULL;
	item = json_find_first_label(root, "bandDpid");
	if(item){
	    if(item->child->text){
	    	UINT1 dpid_tmp[8] = { 0 };
			mac_str_to_bin(item->child->text, dpid_tmp);
			uc8_to_ulli64 (dpid_tmp,&dpid);
			json_free_value(&item);
	    }
	}
	item = json_find_first_label(root, "bandPort");
	if(item){
		if(item->child->text){
			port =  atoll(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "gatwayip");
	if(item){
		if(item->child->text){
			gatwayip = inet_addr(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "outer_interface_ip");
	if(item){
		if(item->child->text){
			outip = inet_addr(item->child->text);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "gatewaymac");
	if(item){
		if(item->child->text){
			macstr2hex(item->child->text,gateway_mac);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "mac");
	if(item){
		if(item->child->text){
			macstr2hex(item->child->text,outer_mac);
			json_free_value(&item);
		}
	}
	item = json_find_first_label(root, "networkid");
		if(item){
			if(item->child->text){
				strcpy(network_id,item->child->text);
				json_free_value(&item);
			}
		}
	create_external_port_by_rest(gatwayip,gateway_mac,outip,outer_mac,dpid,port,network_id);
    return json_to_reply(NULL, GN_OK);
}
static INT1  *update_fabric_external(const INT1 *url, json_t *root)
{
	update_floating_ip_mem_info();
	return json_to_reply(NULL, GN_OK);
}

static INT1 *del_fabric_entries(const INT1 *url, json_t *root)
{
	of131_fabric_impl_delete();
    return json_to_reply(NULL, GN_OK);
}
/*
static INT1 *ip_match_flows_fabric(const INT1 *url, json_t *root){
	return json_to_reply(NULL, GN_OK);
}
*/
static INT1 *setup_fabric_entries_parts(const INT1 *url, json_t *root){
	UINT8 dpids[100] = {0};
	UINT4 len = 0;
	json_t *item = NULL;
	char* token = NULL;

	item = json_find_first_label(root, "dpidList");
	if (item){
		token = strtok( item->child->text, ",");
	    while( token != NULL )
	    {
	        dpids[len] = strtoull(token, NULL, 10);
	        len++;
	        token = strtok( NULL, ",");
	    }
		json_free_value(&item);
	}
	of131_fabric_impl_setup_by_dpids(dpids,len);
	return json_to_reply(NULL, GN_OK);
}


static INT1* get_fabric_path(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    json_t *entry = NULL;
    json_t *array = NULL;
    json_t *key = NULL;
    json_t *value = NULL;

    json_t *item = NULL;
    INT1 tmp[32] = {0};
    UINT1 dpid[8];
    UINT8 src_dpid = 0, dst_dpid = 0;

    p_fabric_path path = NULL;
    p_fabric_path_node  node = NULL;
/*	way 1:
    key_value_t kv_src_dpid, kv_dst_dpid;

    kv_src_dpid.key = strdup("srcDPID");
    kv_src_dpid.value = NULL;

    kv_dst_dpid.key = strdup("dstDPID");
    kv_dst_dpid.value = NULL;

	obj = json_new_object();
    get_url_argument(url + strlen("/gn/fabric/getpath/json") + 1, &kv_src_dpid);
    if (NULL == kv_src_dpid.value)
    {
        return json_to_reply(NULL, EC_RESTFUL_REQUIRE_SRCDPID);
    }

    get_url_argument(url + strlen("/gn/fabric/getpath/json") + 1, &kv_dst_dpid);
    if (NULL == kv_dst_dpid.value)
    {
        return json_to_reply(NULL, EC_RESTFUL_REQUIRE_DSTDPID);
    }
    mac_str_to_bin(kv_src_dpid.value, dpid);
    uc8_to_ulli64(dpid, &src_dpid);
    mac_str_to_bin(kv_dst_dpid.value, dpid);
    uc8_to_ulli64(dpid, &dst_dpid);
*/
    item = json_find_first_label(root, "srcDPID");
    if(item){
    	src_dpid = strtoull(item->child->text, NULL, 10);
    }else{
    	return json_to_reply(NULL, EC_RESTFUL_REQUIRE_SRCDPID);
    }

    item = json_find_first_label(root, "dstDPID");
	if(item){
		dst_dpid = strtoull(item->child->text, NULL, 10);
	}else{
		return json_to_reply(NULL, EC_RESTFUL_REQUIRE_DSTDPID);
	}
    //end
    //printf("src_dpid : %d; dst_dpid: %d\n",src_dpid,dst_dpid);
    path = of131_fabric_get_path(src_dpid, dst_dpid);
    if(path != NULL){
    	array = json_new_array();
    	node = path->node_list;
    	while( node != NULL){
    		entry = json_new_object();

	        key = json_new_string("DPID");
	        ulli64_to_uc8(node->sw->dpid, dpid);
	        sprintf(tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0], dpid[1],
	                dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
	        value = json_new_string(tmp);
	        json_insert_child(key, value);
	        json_insert_child(entry, key);

	        key = json_new_string("Port");
	        if(node->port != NULL){
	        	sprintf(tmp, "%d", node->port->port_no );
	        }else{
	        	sprintf(tmp, "%d", 0);
	        }
	        value = json_new_number(tmp);
	        json_insert_child(key, value);
	        json_insert_child(entry, key);

	        json_insert_child(array, entry);
	        node = node->next;
    	}

        key = json_new_string("path");
        json_insert_child(key, array);
        json_insert_child(obj, key);
    }else{
    	return json_to_reply(NULL, EC_SW_NO_PATH);
    }


	return json_to_reply(obj, GN_OK);
}








/****************************************************
 * DCFabric 2016-01-21
 ****************************************************/
INT1 *json_to_reply_desc(json_t *obj, INT4 code, const INT1 *desc)
{
    INT1 *reply = NULL;
    INT1 json_tmp[32];
    json_t *key, *value;

    if (NULL == obj)
    {
        obj = json_new_object();
    }

    key = json_new_string("retCode");
    sprintf(json_tmp, "%d", code);
    value = json_new_number(json_tmp);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("retMsg");
    value = json_new_string(get_error_msg(code));
    json_insert_child(key, value);
    json_insert_child(obj, key);

    if (NULL != desc)
    {
        key = json_new_string("desc");
        value = json_new_string(desc);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    json_tree_to_string(obj, &reply);
    json_free_value(&obj);

    //LOG_PROC("INFO", "Reply: %s", reply);
    return reply;
}



static BOOL clear_fabric_flow_entries(const INT1 *flow_name, const gn_switch_t *sw)
{
    flow_entry_json_t *entry_list = g_flow_entry_json_list->next;
    flow_entry_json_t *cur_entry = NULL;
    while (NULL != entry_list)
    {
        if (0 == strncmp(flow_name, entry_list->flow_name, REST_MAX_PARAM_LEN) && sw == entry_list->sw)
        {
            if (NULL != entry_list->next)
            {
                entry_list->next->pre = entry_list->pre;
                
            }
            entry_list->pre->next = entry_list->next;
            cur_entry= entry_list;
            entry_list = entry_list->next;
            g_flow_entry_json_length--;
            
            install_fabric_flows(cur_entry->sw, cur_entry->idle_timeout, cur_entry->hard_timeout, cur_entry->priority, cur_entry->table_id, OFPFC_DELETE, cur_entry->flow_param);
            gn_free((void **)(&cur_entry->flow_name));
            clear_flow_param(cur_entry->flow_param);
            gn_free((void **)(&cur_entry->data));
            gn_free((void **)(&cur_entry));

            return TRUE;
        }
        
        entry_list = entry_list->next;
    }

    return FALSE;
}



static gn_oxm_t *json_fabric_parse_oxm(const json_t *match)
{
    gn_oxm_t *oxm_match = (gn_oxm_t *)gn_malloc(sizeof(gn_oxm_t));

    //in_port
    json_t *in_port_json = json_find_first_label(match->child, "inPort");
    if (NULL != in_port_json)
    {
        oxm_match->in_port = (UINT4)strtoul(in_port_json->child->text, NULL, 10);
    }

    //in_phy_port
    json_t *in_phy_port_json = json_find_first_label(match->child, "inPhyPort");
    if (NULL != in_phy_port_json)
    {
        oxm_match->in_phy_port = (UINT4)strtoul(in_phy_port_json->child->text, NULL, 10);
    }
    
    //metadata
    json_t *metadata_json = json_find_first_label(match->child, "metadata");
    if (NULL != metadata_json)
    {
        oxm_match->metadata = strtoull(metadata_json->child->text, NULL, 10);
    }
    
    //eth_dst
    json_t *eth_dst_json = json_find_first_label(match->child, "ethDst");
    if (NULL != eth_dst_json)
    {
        macstr2hex(eth_dst_json->child->text, oxm_match->eth_dst);
    }
    
    //eth_src
    json_t *eth_src_json = json_find_first_label(match->child, "ethSrc");
    if (NULL != eth_src_json)
    {
        macstr2hex(eth_src_json->child->text, oxm_match->eth_src);
    }

    //eth_type
    json_t *eth_type_json = json_find_first_label(match->child, "ethType");
    if (NULL != eth_type_json)
    {
        oxm_match->eth_type = (UINT2)strtoul(eth_type_json->child->text, NULL, 10);
    }
    
    //vlan_vid
    json_t *vlan_vid_json = json_find_first_label(match->child, "vlanVid");
    if (NULL != vlan_vid_json)
    {
        oxm_match->vlan_vid = (UINT2)strtoul(vlan_vid_json->child->text, NULL, 10);
    }
    
    //vlan_pcp
    json_t *vlan_pcp_json = json_find_first_label(match->child, "vlanPcp");
    if (NULL != vlan_pcp_json)
    {
        oxm_match->vlan_pcp = (UINT1)strtoul(vlan_pcp_json->child->text, NULL, 10);
    }
    
    //ip_dscp
    json_t *ip_dscp_json = json_find_first_label(match->child, "ipDscp");
    if (NULL != ip_dscp_json)
    {
        oxm_match->ip_dscp = (UINT1)strtoul(ip_dscp_json->child->text, NULL, 10);
    }
    
    //ip_ecn
    json_t *ip_ecn_json = json_find_first_label(match->child, "ipEcn");
    if (NULL != ip_ecn_json)
    {
        oxm_match->ip_ecn = (UINT1)strtoul(ip_ecn_json->child->text, NULL, 10);
    }
    
    //ip_proto
    json_t *ip_proto_json = json_find_first_label(match->child, "ipProto");
    if (NULL != ip_proto_json)
    {
        oxm_match->ip_proto = (UINT1)strtoul(ip_proto_json->child->text, NULL, 10);
    }
    
    //ipv4_src
    json_t *ipv4_src_json = json_find_first_label(match->child, "ipv4Src");
    if (NULL != ipv4_src_json)
    {
        oxm_match->ipv4_src = ntohl(ip2number(ipv4_src_json->child->text));
    }
    
    //ipv4_dst
    json_t *ipv4_dst_json = json_find_first_label(match->child, "ipv4Dst");
    if (NULL != ipv4_dst_json)
    {
        oxm_match->ipv4_dst = ntohl(ip2number(ipv4_dst_json->child->text));
    }
    
    //tcp_src
    json_t *tcp_src_json = json_find_first_label(match->child, "tcpSrc");
    if (NULL != tcp_src_json)
    {
        oxm_match->tcp_src = (UINT2)strtoul(tcp_src_json->child->text, NULL, 10);
    }
    
    //tcp_dst
    json_t *tcp_dst_json = json_find_first_label(match->child, "tcpDst");
    if (NULL != tcp_dst_json)
    {
        oxm_match->tcp_dst = (UINT2)strtoul(tcp_dst_json->child->text, NULL, 10);
    }
    
    //udp_src
    json_t *udp_src_json = json_find_first_label(match->child, "udpSrc");
    if (NULL != udp_src_json)
    {
        oxm_match->udp_src = (UINT2)strtoul(udp_src_json->child->text, NULL, 10);
    }
    
    //udp_dst
    json_t *udp_dst_json = json_find_first_label(match->child, "udpDst");
    if (NULL != udp_dst_json)
    {
        oxm_match->udp_dst = (UINT2)strtoul(udp_dst_json->child->text, NULL, 10);
    }
    
    //sctp_src
    //json_t *sctp_src_json = json_find_first_label(match->child, "sctpSrc");
    //if (NULL != sctp_src_json)
    //{
    //    oxm_match->sctp_src = (UINT2)strtoul(sctp_src_json->child->text, NULL, 10);
    //}
    
    //sctp_dst
    //json_t *sctp_dst_json = json_find_first_label(match->child, "sctpDst");
    //if (NULL != sctp_dst_json)
    //{
    //    oxm_match->sctp_dst = (UINT2)strtoul(sctp_dst_json->child->text, NULL, 10);
    //}
    
    //icmpv4_type
    json_t *icmpv4_type_json = json_find_first_label(match->child, "icmpv4Type");
    if (NULL != icmpv4_type_json)
    {
        oxm_match->icmpv4_type = (UINT1)strtoul(icmpv4_type_json->child->text, NULL, 10);
    }
    
    //icmpv4_code
    json_t *icmpv4_code_json = json_find_first_label(match->child, "icmpv4Code");
    if (NULL != icmpv4_code_json)
    {
        oxm_match->icmpv4_code = (UINT1)strtoul(icmpv4_code_json->child->text, NULL, 10);
    }
    
    //arp_op
    json_t *arp_op_json = json_find_first_label(match->child, "arpOp");
    if (NULL != arp_op_json)
    {
        oxm_match->arp_op = (UINT1)strtoul(arp_op_json->child->text, NULL, 10);
    }
    
    //arp_spa
    json_t *arp_spa_json = json_find_first_label(match->child, "arpSpa");
    if (NULL != arp_spa_json)
    {
        oxm_match->arp_spa = (UINT4)strtoul(arp_spa_json->child->text, NULL, 10);
    }
    
    //arp_tpa
    json_t *arp_tpa_json = json_find_first_label(match->child, "arpTpa");
    if (NULL != arp_tpa_json)
    {
        oxm_match->arp_tpa = (UINT4)strtoul(arp_tpa_json->child->text, NULL, 10);
    }
    
    //arp_sha
    json_t *arp_sha_json = json_find_first_label(match->child, "arpSha");
    if (NULL != arp_sha_json)
    {
        memcpy(oxm_match->arp_sha, arp_sha_json->child->text, 6);
    }
    
    //arp_tha
    json_t *arp_tha_json = json_find_first_label(match->child, "arpTha");
    if (NULL != arp_tha_json)
    {
        memcpy(oxm_match->arp_tha, arp_tha_json->child->text, 6);
    }
    
    //ipv6_src
    json_t *ipv6_src_json = json_find_first_label(match->child, "ipv6Src");
    if (NULL != ipv6_src_json)
    {
        ipv6_str_to_number(ipv6_src_json->child->text, oxm_match->ipv6_src);
    }

    //ipv6_dst
    json_t *ipv6_dst_json = json_find_first_label(match->child, "ipv6Dst");
    if (NULL != ipv6_dst_json)
    {
        ipv6_str_to_number(ipv6_dst_json->child->text, oxm_match->ipv6_dst);
    }

    //ipv6_flabel
    //json_t *ipv6_flabel_json = json_find_first_label(match->child, "ipv6Flabel");
    //if (NULL != ipv6_flabel_json)
    //{
    //    oxm_match->ipv6_flabel = (UINT4)strtoul(ipv6_flabel_json->child->text, NULL, 10);
    //}
    
    //icmpv6_type
    //json_t *icmpv6_type_json = json_find_first_label(match->child, "icmpv6Type");
    //if (NULL != icmpv6_type_json)
    //{
    //    oxm_match->icmpv6_type = (UINT1)strtoul(icmpv6_type_json->child->text, NULL, 10);
    //}
    
    //icmpv6_code
    //json_t *icmpv6_code_json = json_find_first_label(match->child, "icmpv6Code");
    //if (NULL != icmpv6_code_json)
    //{
    //    oxm_match->icmpv6_code = (UINT1)strtoul(icmpv6_code_json->child->text, NULL, 10);
    //}
    
    //ipv6_nd_target
    //json_t *ipv6_nd_target_json = json_find_first_label(match->child, "ipv6NdTarget");
    //if (NULL != ipv6_nd_target_json)
    //{
    //    oxm_match->ipv6_nd_target = (UINT1)strtoul(ipv6_nd_target_json->child->text, NULL, 10);
    //}
    
    //ipv6_nd_sll
    //json_t *ipv6_nd_sll_json = json_find_first_label(match->child, "ipv6NdSll");
    //if (NULL != ipv6_nd_sll_json)
    //{
    //    oxm_match->ipv6_nd_sll = (UINT1)strtoul(ipv6_nd_sll_json->child->text, NULL, 10);
    //}
    
    //ipv6_nd_tll
    //json_t *ipv6_nd_tll_json = json_find_first_label(match->child, "ipv6NdTll");
    //if (NULL != ipv6_nd_tll_json)
    //{
    //    oxm_match->ipv6_nd_tll = (UINT1)strtoul(ipv6_nd_tll_json->child->text, NULL, 10);
    //}
    
    //mpls_label
    json_t *mpls_label_json = json_find_first_label(match->child, "mplsLabel");
    if (NULL != mpls_label_json)
    {
        oxm_match->mpls_label = (UINT4)strtoul(mpls_label_json->child->text, NULL, 10);
    }
    
    //mpls_tc
    //json_t *mpls_tc_json = json_find_first_label(match->child, "mplsTc");
    //if (NULL != mpls_tc_json)
    //{
    //    oxm_match->mpls_tc = (UINT1)strtoul(mpls_tc_json->child->text, NULL, 10);
    //}
    
    //mpls_bos
    //json_t *mpls_bos_json = json_find_first_label(match->child, "mplsBos");
    //if (NULL != mpls_bos_json)
    //{
    //    oxm_match->mpls_bos = (UINT1)strtoul(mpls_bos_json->child->text, NULL, 10);
    //}
    
    //pbb_isid
    //json_t *pbb_isid_json = json_find_first_label(match->child, "pbbIsid");
    //if (NULL != pbb_isid_json)
    //{
    //    oxm_match->pbb_isid = (UINT1)strtoul(pbb_isid_json->child->text, NULL, 10);
    //}
    
    //tunnel_id
    json_t *tunnel_id_json = json_find_first_label(match->child, "tunnelId");
    if (NULL != tunnel_id_json)
    {
        oxm_match->tunnel_id = (UINT4)strtoul(tunnel_id_json->child->text, NULL, 10);
    }
    
    //ipv6_exthdr
    //json_t *ipv6_exthdr_json = json_find_first_label(match->child, "ipv6Exthdr");
    //if (NULL != ipv6_exthdr_json)
    //{
    //    oxm_match->ipv6_exthdr = (UINT1)strtoul(ipv6_exthdr_json->child->text, NULL, 10);
    //}
    
    return oxm_match;
}




static void json_fabric_add_oxm(json_t *obj, gn_oxm_t *oxm_fields)
{
    INT1 json_temp[1024];
    json_t *key = NULL, *value = NULL;

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PORT))
    {
        sprintf(json_temp, "%u", oxm_fields->in_port);
        key = json_new_string("inPort");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IN_PHY_PORT))
    {
        sprintf(json_temp, "%u", oxm_fields->in_phy_port);
        key = json_new_string("inPhyPort");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if(oxm_fields->mask & (MASK_SET << OFPXMT_OFB_METADATA))
    {
        sprintf(json_temp, "%llu", oxm_fields->metadata);
        key = json_new_string("metadata");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_DST))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->eth_dst[0],
                oxm_fields->eth_dst[1], oxm_fields->eth_dst[2],
                oxm_fields->eth_dst[3], oxm_fields->eth_dst[4],
                oxm_fields->eth_dst[5]);
        key = json_new_string("ethDst");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_SRC))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->eth_src[0],
                oxm_fields->eth_src[1], oxm_fields->eth_src[2],
                oxm_fields->eth_src[3], oxm_fields->eth_src[4],
                oxm_fields->eth_src[5]);
        key = json_new_string("ethSrc");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ETH_TYPE))
    {
        key = json_new_string("ethType");
        if (oxm_fields->eth_type == ETHER_ARP)
        {
            value = json_new_string("ARP");
        }
        else if (oxm_fields->eth_type == ETHER_IP)
        {
            value = json_new_string("IPV4");
        }
        else if (oxm_fields->eth_type == ETHER_IPV6)
        {
            value = json_new_string("IPV6");
        }
        else if (oxm_fields->eth_type == ETHER_MPLS)
        {
            value = json_new_string("MPLS");
        }
        else if (oxm_fields->eth_type == ETHER_VLAN)
        {
            value = json_new_string("VLAN");
        }
        else
        {
            value = json_new_string("UNKNOW");
        }

        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_VID))
    {
        sprintf(json_temp, "%d", oxm_fields->vlan_vid);
        key = json_new_string("vlanVid");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_VLAN_PCP))
    {
        sprintf(json_temp, "%d", oxm_fields->vlan_pcp);
        key = json_new_string("vlanPcp");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_DSCP))
    {
        sprintf(json_temp, "%d", oxm_fields->ip_dscp);
        key = json_new_string("ipDscp");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_ECN))
    {
        sprintf(json_temp, "%d", oxm_fields->ip_ecn);
        key = json_new_string("ipEcn");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IP_PROTO))
    {
        key = json_new_string("ipProto");
        if (oxm_fields->ip_proto == IPPROTO_ICMP)
        {
            value = json_new_string("ICMP");
        }
        else if (oxm_fields->ip_proto == IPPROTO_TCP)
        {
            value = json_new_string("TCP");
        }
        else if (oxm_fields->ip_proto == IPPROTO_UDP)
        {
            value = json_new_string("UDP");
        }
        else
        {
            //
        }

        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC))
    {
        key = json_new_string("ipv4Src");
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_SRC_PREFIX))
        {
            sprintf(json_temp, "%s/%u", inet_htoa(oxm_fields->ipv4_src),
                    oxm_fields->ipv4_src_prefix);
        }
        else
        {
            sprintf(json_temp, "%s", inet_htoa(oxm_fields->ipv4_src));
        }
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST))
    {
        key = json_new_string("ipv4Dst");
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV4_DST_PREFIX))
        {
            sprintf(json_temp, "%s/%u", inet_htoa(oxm_fields->ipv4_dst),
                    oxm_fields->ipv4_dst_prefix);
        }
        else
        {
            sprintf(json_temp, "%s", inet_htoa(oxm_fields->ipv4_dst));
        }
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_SRC))
    {
        sprintf(json_temp, "%d", oxm_fields->tcp_src);
        key = json_new_string("tcpSrc");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TCP_DST))
    {
        sprintf(json_temp, "%d", oxm_fields->tcp_dst);
        key = json_new_string("tcpDst");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_SRC))
    {
        sprintf(json_temp, "%d", oxm_fields->udp_src);
        key = json_new_string("udpSrc");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_UDP_DST))
    {
        sprintf(json_temp, "%d", oxm_fields->udp_dst);
        key = json_new_string("udpDst");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_SRC))
//    {
//
//    }
//
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_SCTP_DST))
//    {
//
//    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_TYPE))
    {
        sprintf(json_temp, "%d", oxm_fields->icmpv4_type);
        key = json_new_string("icmpv4Type");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ICMPV4_CODE))
    {
        sprintf(json_temp, "%d", oxm_fields->icmpv4_code);
        key = json_new_string("icmpv4Code");
        value = json_new_number(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_OP))
    {
        sprintf(json_temp, "%d", oxm_fields->arp_op);
        key = json_new_string("arpOp");
        if (oxm_fields->arp_op == 1)
        {
            value = json_new_string("Request");
        }
        else
        {
            value = json_new_string("Reply");
        }

        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SPA))
    {
        key = json_new_string("arpSpa");
        value = json_new_string(inet_htoa(oxm_fields->arp_spa));
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_TPA))
    {
        key = json_new_string("arpTpa");
        value = json_new_string(inet_htoa(oxm_fields->arp_tpa));
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_SHA))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->arp_sha[0],
                oxm_fields->arp_sha[1], oxm_fields->arp_sha[2],
                oxm_fields->arp_sha[3], oxm_fields->arp_sha[4],
                oxm_fields->arp_sha[5]);
        key = json_new_string("arpSha");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_ARP_THA))
    {
        sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x", oxm_fields->arp_tha[0],
                oxm_fields->arp_tha[1], oxm_fields->arp_tha[2],
                oxm_fields->arp_tha[3], oxm_fields->arp_tha[4],
                oxm_fields->arp_tha[5]);
        key = json_new_string("arpTha");
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC))
    {
        char sipv6[40] = { 0 };
        inet_ntop(AF_INET6, (char *) (oxm_fields->ipv6_src), sipv6, 40);
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_SRC_PREFIX))
        {
            sprintf(json_temp, "%s/%u", sipv6, oxm_fields->ipv6_src_prefix);
            value = json_new_string(json_temp);
        }
        else
        {
            value = json_new_string(sipv6);
        }

        key = json_new_string("ipv6Src");
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST))
    {
        char sipv6[40] = { 0 };
        inet_ntop(AF_INET6, (char *) (oxm_fields->ipv6_dst), sipv6, 40);
        if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_IPV6_DST_PREFIX))
        {
            sprintf(json_temp, "%s/%u", sipv6, oxm_fields->ipv6_dst_prefix);
            value = json_new_string(json_temp);
        }
        else
        {
            value = json_new_string(sipv6);
        }

        key = json_new_string("ipv6Dst");
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_FLABEL))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_TYPE))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_ICMPV6_CODE))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TARGET))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_SLL))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_ND_TLL))
//    {
//    }
    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_MPLS_LABEL))
    {
        key = json_new_string("mplsLabel");
        sprintf(json_temp, "%u", oxm_fields->mpls_label);
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_MPLS_TC))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFP_MPLS_BOS))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_PBB_ISID))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_TUNNEL_ID))
//    {
//    }
//    if(oxm_fields->wildcards & (mask_set << OFPXMT_OFB_IPV6_EXTHDR))
//    {
//    }
    if (oxm_fields->mask & (MASK_SET << OFPXMT_OFB_TUNNEL_ID))
    {
        key = json_new_string("tunnelId");
        sprintf(json_temp, "%u", oxm_fields->tunnel_id);
        value = json_new_string(json_temp);
        json_insert_child(key, value);
        json_insert_child(obj, key);
    }

}


static INT1 *json_fabric_parse_flow(json_t *Obj, stats_fabric_flow_t *list)
{
    UINT1 dpid[8];
    INT1 json_temp[1024];
    gn_switch_t *sw = NULL;
    gn_flow_t *flow = NULL;
    json_t *key, *value, *sw_array, *sw_obj, *flow_array, *flow_obj, *tmp_obj = NULL;

    key = json_new_string("switchFlowEntries");
    sw_array = json_new_array();
    json_insert_child(key, sw_array);
    json_insert_child(Obj, key);

    while (NULL != list && NULL != list->sw && NULL != list->flow)
    {
        if (1 == list->sw->state)
        {
            sw_obj = json_new_object();
            json_insert_child(sw_array, sw_obj);

            sw = list->sw;
            ulli64_to_uc8(sw->dpid, dpid);
            sprintf(json_temp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0],
                    dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6],
                    dpid[7]);

            key = json_new_string("DPID");
            value = json_new_string(json_temp);
            json_insert_child(key, value);
            json_insert_child(sw_obj, key);

            key = json_new_string("flowEntries");
            flow_array = json_new_array();
            json_insert_child(key, flow_array);
            json_insert_child(sw_obj, key);

            flow = list->flow;
            while (flow)
            {            
                flow_obj = json_new_object();
                json_insert_child(flow_array, flow_obj);
                /*
                key = json_new_string("uuid");
                value = json_new_string(flow->uuid);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("creater");
                value = json_new_string(flow->creater);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("createTime");
                sprintf(json_temp, "%llu", flow->create_time);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);
                */

                key = json_new_string("tableId");
                sprintf(json_temp, "%d", flow->table_id);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("idleTimeout");
                sprintf(json_temp, "%d", flow->idle_timeout);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("hardTimeout");
                sprintf(json_temp, "%d", flow->hard_timeout);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("priority");
                sprintf(json_temp, "%d", flow->priority);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("durationSec");
                sprintf(json_temp, "%u", flow->stats.duration_sec);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("byteCount");
                sprintf(json_temp, "%llu", flow->stats.byte_count);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("packetCount");
                sprintf(json_temp, "%llu", flow->stats.packet_count);
                value = json_new_string(json_temp);
                json_insert_child(key, value);
                json_insert_child(flow_obj, key);

                key = json_new_string("match");
                tmp_obj = json_new_object();
                json_insert_child(key, tmp_obj);
                json_insert_child(flow_obj, key);
                json_fabric_add_oxm(tmp_obj, &(flow->match.oxm_fields));

                key = json_new_string("instructions");
                tmp_obj = json_new_object();
                json_insert_child(key, tmp_obj);
                json_insert_child(flow_obj, key);

                json_add_instructions(tmp_obj, flow->instructions);

                flow = flow->next;
            }
        }

        list = list->next;
    }

    return json_to_reply_desc(Obj, GN_OK, NULL);
}




/****************************************************
 * Get all switches's properties
 * GET URL eg: http://Controllerhost:8081/dcf/get/all/switchinfo/json
 ****************************************************/
static INT1 *get_fabric_all_switch_info(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();

    json_t *nodes = json_new_string("nodeProperties");
    json_t *nodes_array = json_new_array();
    json_insert_child(nodes, nodes_array);
    json_insert_child(obj, nodes);

    json_t *node = NULL;
    json_t *node_header = NULL;
    json_t *node_body = NULL;
    json_t *body_child = NULL;
    json_t *key = NULL;
    json_t *value = NULL;
    gn_switch_t *sw = NULL;
    UINT1 buf[8] = {0};
    INT1 json_tmp[1024] = {0};

    int i = 0;
    for (; i < g_server.max_switch; i++)
    {
        sw = &g_server.switches[i];
        if (1 == sw->state)
        {
            node = json_new_object();
            json_insert_child(nodes_array,node);

            //header
            {
                key = json_new_string("node");
                node_header = json_new_object();
                json_insert_child(key, node_header);
                json_insert_child(node, key);
                //id
                ulli64_to_uc8(sw->dpid, buf);
                memset(json_tmp, 0, 1024);
                sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", buf[0],
                    buf[1], buf[2], buf[3], buf[4], buf[5], buf[6], buf[7]);
                key = json_new_string("id");
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(node_header, key);

                //type
                key = json_new_string("type");
                value = json_new_string("OF");
                json_insert_child(key, value);
                json_insert_child(node_header, key);
            }

            //body
            {
                key = json_new_string("properties");
                node_body = json_new_object();
                json_insert_child(key, node_body);
                json_insert_child(node, key);

                //tables
                key = json_new_string("tables");
                body_child = json_new_object();
                json_insert_child(key, body_child);
                json_insert_child(node_body, key);

                key = json_new_string("value");
                memset(json_tmp, 0, 1024);
                sprintf(json_tmp, "%u", sw->n_tables);
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(body_child, key);
                
                //description
                key = json_new_string("description");
                body_child = json_new_object();
                json_insert_child(key, body_child);
                json_insert_child(node_body, key);

                key = json_new_string("value");
                memset(json_tmp, 0, 1024);
                sprintf(json_tmp, "%s %s", sw->sw_desc.mfr_desc, sw->sw_desc.sw_desc);
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(body_child, key);

                //actions
                key = json_new_string("actions");
                body_child = json_new_object();
                json_insert_child(key, body_child);
                json_insert_child(node_body, key);

                key = json_new_string("value");
                value = json_new_string("-1");
                json_insert_child(key, value);
                json_insert_child(body_child, key);

                //macAddress
                key = json_new_string("macAddress");
                body_child = json_new_object();
                json_insert_child(key, body_child);
                json_insert_child(node_body, key);

                key = json_new_string("value");
                memset(json_tmp, 0, 1024);
                sprintf(json_tmp, "%02x:%02x:%02x:%02x:%02x:%02x",
                        sw->lo_port.hw_addr[0], sw->lo_port.hw_addr[1],
                        sw->lo_port.hw_addr[2], sw->lo_port.hw_addr[3],
                        sw->lo_port.hw_addr[4], sw->lo_port.hw_addr[5]);
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(body_child, key);

                //capabilities
                key = json_new_string("capabilities");
                body_child = json_new_object();
                json_insert_child(key, body_child);
                json_insert_child(node_body, key);

                key = json_new_string("value");
                memset(json_tmp, 0, 1024);
                sprintf(json_tmp, "%u", sw->capabilities);
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(body_child, key);

                //timeStamp
                key = json_new_string("timeStamp");
                body_child = json_new_object();
                json_insert_child(key, body_child);
                json_insert_child(node_body, key);

                key = json_new_string("value");
                value = json_new_string("-1");
                json_insert_child(key, value);
                json_insert_child(body_child, key);

                key = json_new_string("name");
                value = json_new_string("");
                json_insert_child(key, value);
                json_insert_child(body_child, key);

                //buffers
                key = json_new_string("buffers");
                body_child = json_new_object();
                json_insert_child(key, body_child);
                json_insert_child(node_body, key);

                key = json_new_string("value");
                memset(json_tmp, 0, 1024);
                sprintf(json_tmp, "%u", sw->n_buffers);
                value = json_new_string(json_tmp);
                json_insert_child(key, value);
                json_insert_child(body_child, key);
            }
        }
    }

    return json_to_reply(obj, GN_OK);
}



/****************************************************
 * Get switch info by dpid
 * GET URL eg: http://Controllerhost:8081/dcf/get/switchinfo/json/00:00:00:00:00:00:00:01
 ****************************************************/
static INT1 *get_fabric_switch_info(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    
    //Get node id
    INT1 *url_head = "/dcf/get/switchinfo/json/";
    UINT url_head_len = strlen(url_head);
    UINT url_len = strlen(url);
    if (url_len <= url_head_len)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid url!");
    }

    UINT8 node_id = 0;
    const INT1 *dpid = url + url_head_len;
    if (-1 == dpidStr2Uint8(dpid, &node_id))
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid dpid!");
    }

    //find the switch by nodeid
    int i = 0;
    gn_switch_t *sw = NULL;
    for (; i < g_server.max_switch; i++)
    {
        sw = &g_server.switches[i];
        if (node_id == sw->dpid && 1 == sw->state)
        {
            break;
        }
    }

    //if not exist
    if (i >= g_server.max_switch)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:can't find the switch!");
    }

    //arrange json retvalue
    json_t *node = json_new_string("nodeConnectorProperties");
    json_t *node_array = json_new_array();
    json_insert_child(node, node_array);
    json_insert_child(obj, node);

    json_t *port = NULL;
    json_t *header = NULL;
    json_t *body = NULL;
    json_t *key = NULL;
    json_t *value = NULL;
    json_t *child = NULL;
    INT1 json_tmp[1024] = {0};
    for (i = 0; i < sw->n_ports; i++)
    {
        gn_port_t *gn_port = &sw->ports[i];
        //one port info
        port = json_new_object();
        json_insert_child(node_array, port);

        //port header
        {
            key = json_new_string("nodeconnector");
            header = json_new_object();
            json_insert_child(key, header);
            json_insert_child(port, key);

            //node
            key = json_new_string("node");
            child = json_new_object();
            json_insert_child(key, child);
            json_insert_child(header, key);

            //node id
            key = json_new_string("id");
            value = json_new_string(dpid);
            json_insert_child(key, value);
            json_insert_child(child, key);

            //node type
            key = json_new_string("type");
            value = json_new_string("OF");
            json_insert_child(key, value);
            json_insert_child(child, key);

            //id
            key = json_new_string("id");
            memset(json_tmp, 0 , 1024);
            sprintf(json_tmp, "%u", gn_port->port_no);
            value = json_new_string(json_tmp);
            json_insert_child(key, value);
            json_insert_child(header, key);

            //node type
            key = json_new_string("type");
            value = json_new_string("OF");
            json_insert_child(key, value);
            json_insert_child(header, key);
        }

        //port body
        {
            key = json_new_string("properties");
            body = json_new_object();
            json_insert_child(key, body);
            json_insert_child(port, key);

            //state
            key = json_new_string("state");
            memset(json_tmp, 0 , 1024);
            sprintf(json_tmp, "%u", gn_port->state);
            value = json_new_string(json_tmp);
            json_insert_child(key, value);
            json_insert_child(body, key);

            //config
            key = json_new_string("config");
            memset(json_tmp, 0 , 1024);
            sprintf(json_tmp, "%u", gn_port->config);
            value = json_new_string(json_tmp);
            json_insert_child(key, value);
            json_insert_child(body, key);

            //name
            key = json_new_string("name");
            value = json_new_string(gn_port->name);
            json_insert_child(key, value);
            json_insert_child(body, key);
        }
    }

    return json_to_reply(obj, GN_OK);

}




/****************************************************
 * Add flow
 * PUT URL eg: http://Controllerhost:8081/dcf/put/flowentries/json
 ****************************************************/
static INT1 *put_fabric_flow_entries(const INT1 *url, json_t *root)
{   
    json_t *obj = json_new_object();
    
    json_t *flowconfig = json_find_first_label(root, "flowConfig");
	if (NULL == flowconfig)
	{
		return json_to_reply_desc(obj, GN_ERR, "[ERROR]:there is no flowConfig label!");
	}

    json_t *flow_entry = flowconfig->child;
	if (NULL == flow_entry)
	{
		return json_to_reply_desc(obj, GN_ERR, "[ERROR]:empty flowConfig!");
	}
    //installInHw
    json_t *installInHw_json = json_find_first_label(flow_entry, "installInHw");
    if (NULL != installInHw_json)
    {
        if (0 != strcmp("true", installInHw_json->child->text) 
            && 0 != strcmp("TRUE", installInHw_json->child->text))
        {
            return json_to_reply_desc(obj, GN_OK, "[WARN]:installInHw is not true or TRUE, just drop it");
        }
    }

    //name
    json_t *name_json = json_find_first_label(flow_entry, "name");
    if (NULL == name_json)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:there is no name label in flowConfig!");
    }
    INT4 len = strlen(name_json->child->text);
    if (len >= REST_MAX_PARAM_LEN)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:name is too long!");
    }
    
    //dpid
    json_t *node = json_find_first_label(flow_entry, "node");
    if (NULL == node)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:there is no node label in flowConfig!");

    }

    json_t *id = json_find_first_label(node->child, "id");
    if (NULL == id)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:there is no id label in node!");
    }

    UINT8 dpid = 0;
    if (-1 == dpidStr2Uint8(id->child->text, &dpid))
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid id in node!");

    }

    //find the switch by nodeid
    INT4 i = 0;
    gn_switch_t *sw = NULL;
    for (; i < g_server.max_switch; i++)
    {
        sw = &g_server.switches[i];
        if (dpid == sw->dpid && 1 == sw->state)
        {
            break;
        }
    }

    //if not exist
    if (i >= g_server.max_switch)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:can't find the switch!");
    }

    //check if same flow has been installed by sw and name
    INT1 *desc = NULL;
    if (clear_fabric_flow_entries(name_json->child->text, sw))
    {
        desc = "[WARN]:same flow has been installed, overwrite it.";
    }

    //priority
    json_t *priority_json = json_find_first_label(flow_entry, "priority");
    UINT2 priority = 0;
    if (NULL != priority_json)
    {
        priority = (UINT2)strtoul(priority_json->child->text, NULL, 10);
    }

    //idle_timeout
    json_t *idle_json = json_find_first_label(flow_entry, "idleTimeout");
    UINT2 idle_timeout = 0;
    if (NULL != idle_json)
    {
        idle_timeout = (UINT2)strtoul(idle_json->child->text, NULL, 10);
    }

    //hard_timeout
    json_t *hard_json = json_find_first_label(flow_entry, "hardTimeout");
    UINT2 hard_timeout = 0;
    if (NULL != hard_json)
    {
        hard_timeout = (UINT2)strtoul(hard_json->child->text, NULL, 10);
    }

    //table_id
    json_t *table_id_json = json_find_first_label(flow_entry, "tableId");
    UINT1 table_id = 0;
    if (NULL != table_id_json)
    {
        table_id = (UINT1)strtoul(table_id_json->child->text, NULL, 10);
    }

    //prepare flow_param
    flow_param_t *flow_param = init_flow_param();

    //match
    json_t *match = json_find_first_label(flow_entry, "match");
    gn_oxm_t *oxm_match = NULL;
    if (NULL != match)
    {
        oxm_match = json_fabric_parse_oxm(match);
    }

    flow_param->match_param = oxm_match;
    
    //instructions
    json_t *instructions_json = json_find_first_label(flow_entry, "instructions");
    UINT8 *value_list = gn_malloc(REST_MAX_ACTION_NUM * sizeof(UINT8));
    INT4 value_index = 0;
    if ((NULL != instructions_json) && (NULL != instructions_json->child) && (NULL != instructions_json->child->child))
    {
        json_t *instruction_json = instructions_json->child->child;
        
        while (NULL != instruction_json && value_index < REST_MAX_ACTION_NUM)
        {
            json_t *type_json = json_find_first_label(instruction_json, "type");
            if (NULL == type_json)
            {
                instruction_json = instruction_json->next;
                continue;
            }
            UINT2 type = (UINT2)strtoul(type_json->child->text, NULL, 10);
            UINT8 value = 0;
            json_t *value_json = json_find_first_label(instruction_json, "value");
            if (NULL == value_json)
            {
                if (OFPIT_WRITE_ACTIONS != type 
                        && OFPIT_APPLY_ACTIONS != type 
                        && OFPIT_CLEAR_ACTIONS != type)
                {

                    instruction_json = instruction_json->next;
                    continue;
                }
            }
            else
            {
                value = strtoull(value_json->child->text, NULL, 10);
            }
            
            switch (type)
            {
                case OFPIT_GOTO_TABLE:
                {
                    value_list[value_index] = value;
                    add_action_param(&flow_param->instruction_param, OFPIT_GOTO_TABLE, (void*)(value_list + value_index));
                    value_index++;
                    break;
                }
                case OFPIT_WRITE_METADATA:
                {
                    json_t *metadata_mask_json = json_find_first_label(instruction_json, "metadataMask");
                    if (NULL == metadata_mask_json)
                    {
                        break;
                    }
                    
                    value_list[value_index] = value;
                    add_action_param(&flow_param->instruction_param, OFPIT_WRITE_METADATA, (void*)(value_list + value_index));
                    value_index++;
                    value_list[value_index] = strtoull(metadata_mask_json->child->text, NULL, 10);
                    value_index++;
                    
                    break;
                }
                case OFPIT_METER:
                {
                    value_list[value_index] = value;
                    add_action_param(&flow_param->instruction_param, OFPIT_METER, (void*)(value_list + value_index));
                    value_index++;
                    break;
                }
                case OFPIT_EXPERIMENTER:
                {   
                    value_list[value_index] = value;
                    add_action_param(&flow_param->instruction_param, OFPIT_EXPERIMENTER, (void*)(value_list + value_index));
                    value_index++;
                    break;
                }
                case OFPIT_WRITE_ACTIONS:
                case OFPIT_APPLY_ACTIONS:
                case OFPIT_CLEAR_ACTIONS:
                {
                    //get all actions
                    json_t *actions = json_find_first_label(instruction_json, "actions");
                    if (NULL == actions || NULL == actions->child || NULL == actions->child->child)
                    {
                        break;
                    }

                    json_t *action_json = actions->child->child;
                    while (NULL != action_json)
                    {
                        json_t *action_type_json = json_find_first_label(action_json, "type");
                        if (NULL == action_type_json)
                        {
                            action_json = action_json->next;
                            continue;
                        }
                        UINT2 action_type = (UINT2)strtoul(action_type_json->child->text, NULL, 10);

                        json_t *action_value_json = json_find_first_label(action_json, "value");
                        UINT8 action_value = 0;
                        if (NULL == action_value_json)
                        {
                            if (OFPAT13_SET_FIELD != action_type)
                            {
                                action_json = action_json->next;
                                continue;
                            }
                        }
                        else
                        {
                            action_value = strtoull(action_value_json->child->text, NULL, 10);
                        }

                        switch (action_type)
                        {
                            case OFPAT13_OUTPUT:
                            case OFPAT13_MPLS_TTL:
                            case OFPAT13_POP_MPLS:
                            case OFPAT13_SET_QUEUE:
                            case OFPAT13_GROUP:
                            case OFPAT13_SET_NW_TTL:
                            case OFPAT13_PUSH_PBB:
                            case OFPAT13_PUSH_VLAN:
                            case OFPAT13_PUSH_MPLS:
                            case OFPAT13_EXPERIMENTER:
                            {
                                value_list[value_index] = action_value;
                                if (type == OFPIT_APPLY_ACTIONS)
                                {
                                    add_action_param(&flow_param->action_param, action_type, (void*)(value_list + value_index));
                                }
                                else 
                                {
                                    add_action_param(&flow_param->write_action_param, action_type, (void*)(value_list + value_index));
                                }
                                
                                value_index++;
                                break;
                            }
                            case OFPAT13_SET_FIELD:
                            {
                                //field
                                json_t *field_json = json_find_first_label(action_json, "field");
                                gn_oxm_t *field = NULL;
                                if (NULL != field_json)
                                {
                                    field = json_fabric_parse_oxm(field_json);
                                }

                                if (NULL != field)
                                {
                                    add_action_param(&flow_param->action_param, action_type, (void*)field);
                                }
                                
                                break;
                            }
                            case OFPAT13_POP_PBB:
                            case OFPAT13_DEC_NW_TTL:
                            case OFPAT13_POP_VLAN:
                            case OFPAT13_COPY_TTL_OUT:
                            case OFPAT13_COPY_TTL_IN:
                            case OFPAT13_DEC_MPLS_TTL:
                                break;
                            default:
                                break;
                        }
                        
                        action_json = action_json->next;
                    }
                    
                    add_action_param(&flow_param->instruction_param, type, NULL);
                    break;
                }
                default:
                    break;
            }

            instruction_json = instruction_json->next;
        }
    }
    
    //one flow_entry_json_t
    flow_entry_json_t *flow_entry_son = (flow_entry_json_t*)gn_malloc(sizeof(flow_entry_json_t));
    flow_entry_son->flow_name = (INT1 *)gn_malloc(sizeof(len + 1));
    memcpy(flow_entry_son->flow_name, name_json->child->text, len + 1);
    flow_entry_son->sw = sw;
    flow_entry_son->hard_timeout = hard_timeout;
    flow_entry_son->idle_timeout = idle_timeout;
    flow_entry_son->table_id = table_id;
    flow_entry_son->priority = priority;
    flow_entry_son->data = value_list;
    flow_entry_son->flow_param = flow_param;

    //install flow
	install_fabric_flows(flow_entry_son->sw, flow_entry_son->idle_timeout, flow_entry_son->hard_timeout, flow_entry_son->priority,
					 flow_entry_son->table_id, OFPFC_ADD, flow_entry_son->flow_param);
    

    //install flow success, save flow json to g_flow_entry_json_list
    flow_entry_son->next = g_flow_entry_json_list->next;
    flow_entry_son->pre = g_flow_entry_json_list;
    if (NULL != g_flow_entry_json_list->next)
    {
        g_flow_entry_json_list->next->pre = flow_entry_son;
    }
    g_flow_entry_json_list->next = flow_entry_son;
    g_flow_entry_json_length++;

    return json_to_reply_desc(root, GN_OK, desc);
}



/****************************************************
 * Del flow by node's dpid
 * GET URL eg: http://Controllerhost:8081/dcf/del/flowentries/json/00:00:00:00:00:00:00:01/flowname
 ****************************************************/
static INT1 *del_fabric_flow_entries(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();

    //Get flowname
    INT1 *url_head = "/dcf/del/flowentries/json/00:00:00:00:00:00:00:01/";
    UINT url_head_len = strlen(url_head);
    UINT url_len = strlen(url);
    if (url_len <= url_head_len)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid url!");
    }
    const INT1 *flow_name = url + url_head_len;

    //Get dpid
    url_head = "/dcf/del/flowentries/json/";
    url_head_len = strlen(url_head);
    const INT1 *dpid_str = url + url_head_len;
    UINT8 dpid = 0;
    if (-1 == dpidStr2Uint8(dpid_str, &dpid))
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid dpid in url!");
    }

    //get switch by dpid
    INT4 i = 0;
    gn_switch_t *sw = NULL;
    for (; i < g_server.max_switch; i++)
    {
        if (1 == g_server.switches[i].state && dpid == g_server.switches[i].dpid)
        {
            sw = &g_server.switches[i];
            break;
        }
    }

    //if can not find switch , just ret failure
    if (NULL == sw)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:can't find the switch");
    }

    clear_fabric_flow_entries(flow_name, sw);
    
    return json_to_reply(obj, GN_OK);
}




/****************************************************
 * Get flow entries by node's dpid and table_id
 * GET URL eg: http://Controllerhost:8081/dcf/get/flowentries/json/00:00:00:00:00:00:00:01/table_id
 ****************************************************/
static INT1 *get_fabric_flow_entries(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    
    //Get table id
    INT1 *url_head = "/dcf/get/flowentries/json/00:00:00:00:00:00:00:01/";
    UINT url_head_len = strlen(url_head);
    UINT url_len = strlen(url);
    if (url_len < url_head_len - 1)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid dpid's length in url!");
    }

    UINT1 table_id = OFPTT_ALL;
    if (url_len > url_head_len)
    {
        table_id = (UINT1)atoi(url + url_head_len);
    } 

    if (OFPTT_ALL != table_id && table_id > FABRIC_OUTPUT_TABLE)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid table_id in url!");
    }

    //Get dpid
    url_head = "/dcf/get/flowentries/json/";
    url_head_len = strlen(url_head);
    const INT1 *dpid_str = url + url_head_len;
    UINT8 dpid = 0;
    if (-1 == dpidStr2Uint8(dpid_str, &dpid))
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid dpid in url!");
    }

    //find the switch by nodeid
    int i = 0;
    gn_switch_t *sw = NULL;
    for (; i < g_server.max_switch; i++)
    {
        sw = &g_server.switches[i];
        if (dpid == sw->dpid && 1 == sw->state)
        {
            break;
        }
    }

    //if not exist
    if (i >= g_server.max_switch)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:can't find the switch!");
    }

    //query flow entries
    stats_fabric_flow_t *list = query_fabric_flow_entries_by_switch(sw, OFPG_ANY, OFPP13_ANY, table_id);

    //arrange json
    INT1 * ret = json_fabric_parse_flow(obj, list);
    //clear cache
    clear_fabric_stats();
    
    return ret;
}



/****************************************************
 * Get all nodes flow entries
 * GET URL eg: http://Controllerhost:8081/dcf/get/all/flowentries/json
 ****************************************************/
static INT1 *get_fabric_all_flow_entries(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object();
    
    //query flow entries
    stats_fabric_flow_t *list = query_fabric_all_flow_entries();
    //arrange json
    INT1 * ret = json_fabric_parse_flow(obj, list);
    //clear cache
    clear_fabric_stats();
    
    return ret;
}

/****************************************************
 * Get controller list
 * GET URL eg: http://Controllerhost:8081/gn/cluster/query/json
 ****************************************************/
static INT1* get_cluster_list(const INT1* url, json_t *root)
{
    json_t *obj = json_new_object();
    if (0 == g_is_cluster_on)
    {
        return json_to_reply(obj, GN_OK);
    }

    if (0 == g_master_id) 
    {
        return json_to_reply(obj, GN_OK);
    }
	       
    json_t *child_obj = NULL;
    json_t *array     = json_new_array();
    json_t *key, *value;
	INT1   tmp[512]   = {0};
	INT1   res[512]   = {0};
	INT1   cov[64]    = {0};
	INT1   *ptr       = NULL;
	INT4   flag       = 0;
	UINT4  ip         = 0;
	
	query_controller_all(tmp);
    
	ptr = strtok(tmp, "-");
	while(ptr)
	{
		child_obj = json_new_object(); 
		json_insert_child(array, child_obj);

		key = json_new_string("ctlIP"); 
		value = json_new_string(ptr);
		json_insert_child(key, value);             
    	json_insert_child(child_obj, key);

		query_controller_port(ptr, res);
		key = json_new_string("ctlPort");
    	value = json_new_number(res);
    	json_insert_child(key, value);             
    	json_insert_child(child_obj, key);

    	key = json_new_string("status");
    	ip = ip2number(ptr);
    	flag = get_controller_status(ntohl(ip));
		if(flag)
    	{
    		value = json_new_string("up");
    	}
    	else
    	{
    		value = json_new_string("down");
    	}
    	json_insert_child(key, value);             
    	json_insert_child(child_obj, key);

		query_controller_role(ptr, res);
    	key = json_new_string("role");
    	value = json_new_string(res);
    	json_insert_child(key, value);             
    	json_insert_child(child_obj, key);


    	key = json_new_string("ctlID");
    	ip = ip2number(ptr);
    	snprintf(cov, 64, "%u", ip);
    	value = json_new_string(cov);
    	json_insert_child(key, value);             
    	json_insert_child(child_obj, key);
    	
		ptr = strtok(NULL, "-");
	}

	key = json_new_string("clusterInfo");
	json_insert_child(key, array);  
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

/****************************************************
 * Set cluster switch
 * PUT URL eg: http://Controllerhost:8081/gn/cluster/onoff/json
 ****************************************************/
static INT1* post_cluster_onoff(const INT1 *url, json_t *root)
{
	json_t *obj = NULL;
    if (0 == g_is_cluster_on)
    {
        return json_to_reply(obj, GN_OK);
    }

    json_t *item = NULL;

	UINT4  onoff = CLUSTER_STOP;
	UINT4  ret   = GN_OK;
		
	item = json_find_first_label(root, "onoff");
    if(item)
    {
    	onoff = atoll(item->child->text); 
        json_free_value(&item);
    }

	//???????
    ret = set_cluster_onoff(onoff);

	INT1   value[32]   = {0};
	if (GN_OK == ret)
	{
		sprintf(value, "%d", onoff);
		persist_value(CLUSTER_ONOFF, value);
	}	

    return json_to_reply(obj, ret);	
}

 /****************************************************
 * Get cluster switch
 * GET URL eg: http://Controllerhost:8081/gn/cluster/onoff/json
 ****************************************************/
static INT1* get_cluster_onoff(const INT1 *url, json_t *root)
{
    json_t *obj = json_new_object(); 
    if (0 == g_is_cluster_on)
    {
        return json_to_reply(obj, GN_OK);
    }

    json_t *key, *value;
    INT1 res[TABLE_STRING_LEN] = {0};

	key = json_new_string("onoff");
	query_value(CLUSTER_ONOFF, res);
	
	if (res[0] == '\0')
	{
		value = json_new_number("0");
	}
	else
	{
		value = json_new_number(res);
	}
	
	json_insert_child(key, value);
	json_insert_child(obj, key);

    return json_to_reply(obj, GN_OK);	
}

/****************************************************
 * Set controller to master
 * PUT URL eg: http://Controllerhost:8081/gn/controller/set/json
 ****************************************************/
static INT1* post_controller_master(const INT1 *url, json_t *root)
{
    json_t *obj  = NULL;
    if (0 == g_is_cluster_on)
    {
        return json_to_reply(obj, GN_OK);
    }

    UINT8  controller_id = 0;
    UINT4  ret   = GN_OK;
    json_t *item = NULL;
    
	INT1  value[TABLE_STRING_LEN] = {0};
    item = json_find_first_label(root, "ctlID");
    if(item)
    {
    	controller_id = strtoull(item->child->text, 0, 10); 
    	snprintf(value, TABLE_STRING_LEN, "%u", ntohl(controller_id));
        json_free_value(&item);
    }

    item = json_find_first_label(root, "role");
    if(item)
    {
        json_free_value(&item);
    }

	persist_value(CUSTOM_MASTER_ID, value);
	ret = set_cluster_role(ntohl(controller_id));
	
    return json_to_reply(obj, ret);
}

/****************************************************
 * Add controller to management system
 * PUT URL eg: http://Controllerhost:8081/gn/controller/add/json
 ****************************************************/
static INT1* post_controller_add(const INT1 *url, json_t *root)
{
    json_t *obj = NULL;
    if (0 == g_is_cluster_on)
    {
        return json_to_reply(obj, GN_OK);
    }

    json_t *item	= NULL;
	UINT4   ip   	= 0;
	UINT2   port 	= 0;
	UINT4   ret 	= GN_OK;
     
    item = json_find_first_label(root, "ctlIP");
    if(item)
    {
    	ip = ip2number(item->child->text); 
        json_free_value(&item);
    }

    item = json_find_first_label(root, "ctlPort");
    if(item)
    {
    	port = atoi(item->child->text);
        json_free_value(&item);
    }

    persist_controller(ip, port, "Unknown");
	update_controllers_role(ip, 1);
	
	return json_to_reply(obj, ret);
}

/****************************************************
 * Delete controller from management system
 * PUT URL eg: http://Controllerhost:8081/gn/controller/delete/json
 ****************************************************/
static INT1 *post_controller_del(const INT1 *url, json_t *root)
{
    json_t *obj = NULL;
    if (0 == g_is_cluster_on)
    {
        return json_to_reply(obj, GN_OK);
    }

    json_t *item   = NULL;
	UINT4   ret    = GN_OK;
	UINT4   controller_id  = 0;;
	
	item = json_find_first_label(root, "ctlID");
    if(item)
    {
    	controller_id = atoll(item->child->text);
        json_free_value(&item);
    }

	update_controllers_role(ntohl(controller_id), 0);

	INT1    key[32] = {0};
	number2ip(controller_id, key);
	deletet_controller(key);

	return json_to_reply(obj, ret);
}

static INT1 *set_stats_frequency_config(const INT1 *url, json_t *root){
	json_t *item = NULL;
	UINT4 frequency = 0;

	item = json_find_first_label(root, "frequency");
	if(item){
		if(item->child->text){
			frequency =  atoi(item->child->text);
			json_free_value(&item);
		}
	}

	if(g_stats_mgr_interval != frequency)
	{
		g_stats_mgr_interval = frequency;
		set_value_int(g_controller_configure, "[stats_conf]", "sampling_interval",frequency);
		g_controller_configure = save_ini(g_controller_configure,CONFIGURE_FILE);
	}
	return json_to_reply(NULL, GN_OK);
}


/****************************************************
 * Get host info by dpid
 * GET URL eg: http://Controllerhost:8081/dcf/get/host/json/00:00:00:00:00:00:00:01
 ****************************************************/
static INT1 *get_fabric_host_by_dpid(const INT1 *url, json_t *root)
{
    json_t *obj, *array, *key, *value, *entry;
    obj = json_new_object();

    //Get dpid
    INT1 *url_head = "/dcf/get/host/json/";
    UINT url_head_len = strlen(url_head);
    UINT url_len = strlen(url);
    if (url_len <= url_head_len)
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid url!");
    }

    UINT8 t_dpid = 0;
    const INT1 *dpid = url + url_head_len;
    if (-1 == dpidStr2Uint8(dpid, &t_dpid))
    {
        return json_to_reply_desc(obj, GN_ERR, "[ERROR]:invalid dpid!");
    }

	array = json_new_array();
	
	key = json_new_string("host list");
   	json_insert_child(key, array);
   	json_insert_child(obj, key);

	p_fabric_host_node head = NULL;
	head = g_fabric_host_list.list;

    INT1 str_temp[48] = {0};
	while (head) {
        if (NULL == head->sw || head->sw->dpid != t_dpid)
        {
            head = head->next;
            continue;
        }
        
		if (g_openstack_on)  {
			entry = json_new_object();
			switch (head->type) {
				case OPENSTACK_PORT_TYPE_OTHER: 
					strcpy(str_temp, "Other");
					break;
				case OPENSTACK_PORT_TYPE_HOST:
					strcpy(str_temp, "Host");
					break;
				case OPENSTACK_PORT_TYPE_ROUTER_INTERFACE:
					strcpy(str_temp, "Router interface");
					break;
				case OPENSTACK_PORT_TYPE_GATEWAY:
					strcpy(str_temp, "gateway");
					break;
				case OPENSTACK_PORT_TYPE_FLOATINGIP:
					strcpy(str_temp, "floatingip");
					break;
				case OPENSTACK_PORT_TYPE_DHCP:
					strcpy(str_temp, "dhcp");
					break;
				case OPENSTACK_PORT_TYPE_LOADBALANCER:
					strcpy(str_temp, "load balance");
					break;
				default:
					strcpy(str_temp, "unknown");
					break;
			}

			key = json_new_string("Type");
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			if (head->sw) {
				key = json_new_string("SwIP");
				bzero(str_temp, 48);
				number2ip(head->sw->sw_ip, str_temp);
				value = json_new_string(str_temp);
				json_insert_child(key, value);
	            json_insert_child(entry, key);
			}
			
			key = json_new_string("IPv4");
			bzero(str_temp, 48);
			number2ip(head->ip_list[0], str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("Mac");
			bzero(str_temp, 48);
			mac2str(head->mac, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("port");
			bzero(str_temp, 48);
			sprintf(str_temp, "%d", head->port);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			if (head->data) {
				openstack_port_p port_p = (openstack_port_p)head->data;

				key = json_new_string("subnet id");
				bzero(str_temp, 48);
				value = json_new_string(port_p->subnet_id);
				json_insert_child(key, value);
	            json_insert_child(entry, key);
			}

			json_insert_child(array, entry);
		}
		else 
		{			
			entry = json_new_object();
            
			key = json_new_string("IPv4");
			bzero(str_temp, 48);
			number2ip(head->ip_list[0], str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("Mac");
			bzero(str_temp, 48);
			mac2str(head->mac, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("port");
			bzero(str_temp, 48);
			sprintf(str_temp, "%d", head->port);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			json_insert_child(array, entry);
		}
		head = head->next;
	}

   return json_to_reply(obj, GN_OK);
}

/****************************************************
 * neutron
 ****************************************************/
#define NEUTRON_NB_NUM      10
UINT1    NeutronNetwork_Cnt = 0;   //Neutron网络个数
UINT1    NeutronSubnet_Cnt  = 0;   //Neutron子网个数
UINT1    NeutronPort_Cnt    = 0;   //Neutron端口个数

neutron_network_t NeutronNetworkInfo[NEUTRON_NB_NUM];
neutron_subnet_t  NeutronSubnetInfo[NEUTRON_NB_NUM];
neutron_port_t    NeutronPortInfo[NEUTRON_NB_NUM];

static INT1 *get_neutron_network(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *get_neutron_subnet(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *get_neutron_port(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *post_neutron_network(const INT1 *url, json_t *root)
{
	json_t *networks=NULL,*temp = NULL;
	char tenant_id[48] ={0};
	char network_id[48] = {0};
	UINT1 shared=0;
	UINT1 external=0;
	networks = json_find_first_label(root, "network");
	if(networks){
		json_t *network  = networks->child;
		if(network){
			temp = json_find_first_label(network, "tenant_id");
			if(temp){
//				printf("tenantid:%s \n",temp->child->text);
				strcpy(tenant_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(network, "id");
			if(temp){
//				printf("id:%s \n",temp->child->text);
				strcpy(network_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(network, "shared");
			if(temp){
				if(temp->child->type==JSON_TRUE){
					shared=1;
					printf("shared:%s\n","True");
				}else if(temp->child->type==JSON_FALSE){
					shared=0;
					printf("shared:%s\n","False");
				}else{
					printf("Jon_type:error.\n");
				}
				json_free_value(&temp);
			}
			temp = json_find_first_label(network, "router:external");
			if(temp){
				if(temp->child->type==JSON_TRUE){
					external=1;
				}else if(temp->child->type==JSON_FALSE){
					external=0;
				}else{
				}
				json_free_value(&temp);
			}
			json_free_value(&network);
		}
	}
    update_openstack_app_network(tenant_id,network_id,shared,external);
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));
    return reply;
}

static INT1 *post_neutron_subnet(const INT1 *url, json_t *root)
{
	json_t *subnets=NULL,*temp = NULL;
	char tenant_id[48] ={0};
	char network_id[48] = {0};
	char subnet_id[48] = {0};
	char cidr[30] = {0};
	INT4 gateway_ip = 0, start_ip = 0, end_ip = 0;
	UINT1 gateway_ipv6[16] = {0};
	UINT1 start_ipv6[16] = {0};
	UINT1 end_ipv6[16] = {0};
	subnets = json_find_first_label(root, "subnet");
	UINT4 longtemp=0;
	if(subnets){
		json_t *subnet  = subnets->child;
		if(subnet){
			temp = json_find_first_label(subnet, "tenant_id");
			if(temp){
//				printf("tenantid:%s \n",temp->child->text);
				strcpy(tenant_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "network_id");
			if(temp){
//				printf("network_id:%s \n",temp->child->text);
				strcpy(network_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "id");
			if(temp){
//				printf("subnet_id:%s \n",temp->child->text);
				strcpy(subnet_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "cidr");
			if(temp){
//				printf("cidr:%s \n",temp->child->text);
				strcpy(cidr,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(subnet, "gateway_ip");
			if(temp){
				memset(gateway_ipv6, 0, 16);
				gateway_ip = 0;
				if (strchr(temp->child->text, ':')) {
					ipv6_str_to_number(temp->child->text, gateway_ipv6);
				}
				else {
					longtemp = inet_addr(temp->child->text) ;
					gateway_ip = longtemp;
				}
//				printf("gateway_ip:%u \n",gateway_ip);
				json_free_value(&temp);
			}
			json_t *allocations = json_find_first_label(subnet, "allocation_pools");
			if(allocations){
				json_t *allocation = allocations->child->child;
				while(allocation){
					temp = json_find_first_label(allocation, "start");
					if(temp){
						memset(start_ipv6, 0, 16);
						start_ip = 0;
						if (strchr(temp->child->text, ':')) {
							ipv6_str_to_number(temp->child->text, start_ipv6);
						}
						else {
							UINT4 longtemp = inet_addr(temp->child->text) ;
							start_ip = longtemp;
						}
//						printf("start_ip:%u \n",start_ip);
						json_free_value(&temp);
					}
					temp = json_find_first_label(allocation, "end");
					if(temp){
						memset(end_ipv6, 0, 16);
						end_ip = 0;
						if (strchr(temp->child->text, ':')) {
							ipv6_str_to_number(temp->child->text, end_ipv6);
						}
						else {
							longtemp = inet_addr(temp->child->text) ;
							end_ip = longtemp;
						}
//						printf("end_ip:%u \n",end_ip);
						json_free_value(&temp);
					}
					allocation=allocation->next;
				}
				json_free_value(&allocations);
			}
			json_free_value(&subnet);
		}
	}
    update_openstack_app_subnet(tenant_id,network_id,subnet_id,gateway_ip,start_ip,end_ip,gateway_ipv6, start_ipv6, end_ipv6, cidr);
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *post_neutron_port(const INT1 *url, json_t *root)
{
	json_t *ports=NULL,*temp = NULL;
	char tenant_id[48] ={0};
	char network_id[48] = {0};
	char subnet_id[48] = {0};
	char port_id[48] = {0};
	//char *tenant_id = NULL,*network_id = NULL,*subnet_id = NULL,*port_id = NULL;
	char port_type[40] = {0};
	char* computer = "compute:nova";
	char* dhcp="network:dhcp";
	char* floatip="network:floatingip";
	INT4 ip = 0;
	UINT1 type = 0;
	UINT1 ipv6[16] = {0};
	UINT1 mac[6]={0};
	UINT4 port_number = 0;
	ports = json_find_first_label(root, "port");
	if(ports){
		json_t *port  = ports->child;
		if(port){
			temp = json_find_first_label(port, "tenant_id");
			if(temp){
//				printf("tenantid:%s \n",temp->child->text);
				strcpy(tenant_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "network_id");
			if(temp){
//				printf("network_id:%s \n",temp->child->text);
				strcpy(network_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "id");
			if(temp){
//				printf("port_id:%s \n",temp->child->text);
				strcpy(port_id,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "device_owner");
			if(temp){
//				printf("device_owner:%s \n",temp->child->text);
//				port_type = temp->child->text;
				strcpy(port_type,temp->child->text);
				json_free_value(&temp);
			}
			temp = json_find_first_label(port, "mac_address");
			if(temp){
//				printf("mac_address:%s \n",temp->child->text);
				macstr2hex(temp->child->text,mac);
				json_free_value(&temp);
			}
			json_t *fix_ips = json_find_first_label(port, "fixed_ips");
			if(fix_ips){//ip = temp->child->text;
				json_t *fix_ip = fix_ips->child->child;
				while(fix_ip){
					temp = json_find_first_label(fix_ip, "subnet_id");
					if(temp){
//						printf("subnet_id:%s \n",temp->child->text);
						strcpy(subnet_id,temp->child->text);
						json_free_value(&temp);
					}
					temp = json_find_first_label(fix_ip, "ip_address");
					if(temp){
						memset(ipv6, 0, 16);
						ip = 0;
						if (strchr(temp->child->text, ':')) {
							// printf("ipv6: %s\n", temp->child->text);
							ipv6_str_to_number(temp->child->text, ipv6);
							// nat_show_ipv6(ipv6);
						}
						else {
							UINT4 longtemp = inet_addr(temp->child->text) ;
							ip = longtemp;
						}
//						printf("ip_address:%s \n",temp->child->text);
//						printf("ip_address:%u \n",ip);
						json_free_value(&temp);
					}
					fix_ip=fix_ip->next;
				}
				json_free_value(&fix_ips);
			}
			json_free_value(&port);
		}

		type = get_openstack_port_type(port_type);
		if(strcmp(port_type,computer)==0){
			update_openstack_app_host_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id,0,NULL);
		}else if(strcmp(port_type,dhcp)==0){
			update_openstack_app_dhcp_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id);
		}else if(strcmp(port_type,floatip)==0){
			update_openstack_app_dhcp_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id);
			create_floatting_ip_by_rest(0,ip,NULL,NULL);
		}else{
			update_openstack_app_gateway_by_rest(NULL,type,port_number,ip,ipv6,mac,tenant_id,network_id,subnet_id,port_id);
		}
	}

    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *put_neutron_network(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *put_neutron_subnet(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *put_neutron_port(const INT1 *url, json_t *root)
{
	printf("put_neutron_port 3847\n");
    INT1 *reply = (char *)gn_malloc(25);
	reload_security_group_info();
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *del_neutron_network(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *del_neutron_subnet(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *del_neutron_port(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

/****************************************************
 * restful module main function
 ****************************************************/
INT1 *proc_restful_request(UINT1 type, const INT1 *url, json_t *root)
{
    UINT4 i = 0;
    INT1 *reply = NULL;
    restful_handles_t *p_handles = NULL;

    switch (type)
    {
    case HTTP_GET:
        p_handles = g_restful_get_handles;
        break;

    case HTTP_POST:
        p_handles = g_restful_post_handles;
        break;

    case HTTP_PUT:
        p_handles = g_restful_put_handles;
        break;

    case HTTP_DELETE:
        p_handles = g_restful_delete_handles;
        break;

    default:
        return json_to_reply(NULL, EC_RESTFUL_INVALID_REQ);
    }

    for (; i < REST_CAPACITY; i++)
    {
        if (p_handles[i].used == 1)
        {
        	//printf(" test-openstack:%s   %s  /n",url,p_handles[i].url);
            if (strncmp(url, p_handles[i].url, p_handles[i].url_len) == 0)
            {
                reply = p_handles[i].handler(url, root);
                goto EXIT;
            }
        }
    }

    return json_to_reply(NULL, EC_RESTFUL_INVALID_REQ);

EXIT:
    return reply;
}

INT4 register_restful_handler(UINT1 type, const INT1 *url, restful_handler_t restful_handler)
{
    UINT4 i = 0;
    restful_handles_t *p_handles = NULL;

    switch (type)
    {
    case HTTP_GET:
        p_handles = g_restful_get_handles;
        break;

    case HTTP_POST:
        p_handles = g_restful_post_handles;
        break;

    case HTTP_PUT:
        p_handles = g_restful_put_handles;
        break;

    case HTTP_DELETE:
        p_handles = g_restful_delete_handles;
        break;

    default:
        return GN_ERR;
    }

    for (; i < REST_CAPACITY; i++)
    {
        if (p_handles[i].used == 1)
        {
            if ((strncmp(url, p_handles[i].url, p_handles[i].url_len) == 0)
                    && (p_handles[i].url_len = strlen(url)))
            {
                memcpy(p_handles[i].url, url, strlen(url));
                p_handles[i].url_len = strlen(url);
                p_handles[i].handler = restful_handler;
                p_handles[i].used = 1;

                return GN_OK;
            }
        }
    }

    for (i = 0; i < REST_CAPACITY; i++)
    {
        if (p_handles[i].used == 0)
        {
            memcpy(p_handles[i].url, url, strlen(url));
            p_handles[i].url_len = strlen(url);
            p_handles[i].handler = restful_handler;
            p_handles[i].used = 1;

            return GN_OK;
        }
    }

    return GN_ERR;
}

INT4 init_json_server()
{
    INT4 ret = 0;
    ret += register_restful_handler(HTTP_GET, "/gn/switchinfo/json", get_switch_info);
    ret += register_restful_handler(HTTP_GET, "/gn/topo/links/json", get_topo_link);
    ret += register_restful_handler(HTTP_GET, "/gn/topo/hosts/json", get_topo_hosts);

    ret += register_restful_handler(HTTP_GET, "/gn/subnet/json", get_l3_subnet);
    ret += register_restful_handler(HTTP_POST, "/gn/subnet/json", post_l3_subnet);
    ret += register_restful_handler(HTTP_DELETE, "/gn/subnet/json", del_l3_subnet);

    ret += register_restful_handler(HTTP_GET, "/gn/flows/all/json", get_flow_entries_all);
    ret += register_restful_handler(HTTP_DELETE, "/gn/flows/all/json", del_flow_entries_all);
    ret += register_restful_handler(HTTP_POST, "/gn/flow/json", post_flow_entry);
    ret += register_restful_handler(HTTP_PUT, "/gn/flow/json", put_flow_entry);
    ret += register_restful_handler(HTTP_DELETE, "/gn/flow/json", del_flow_entry);

    ret += register_restful_handler(HTTP_GET, "/gn/meters/json", get_meter_entries);
    ret += register_restful_handler(HTTP_DELETE, "/gn/meters/json", del_meter_entries);
    ret += register_restful_handler(HTTP_POST, "/gn/meter/json", post_meter_entry);
    ret += register_restful_handler(HTTP_PUT, "/gn/meter/json", put_meter_entry);
    ret += register_restful_handler(HTTP_DELETE, "/gn/meter/json", del_meter_entry);

    ret += register_restful_handler(HTTP_GET, "/gn/groups/json", get_group_entries);
    ret += register_restful_handler(HTTP_DELETE, "/gn/groups/json", del_group_entries);
    ret += register_restful_handler(HTTP_POST, "/gn/group/json", post_group_entry);
    ret += register_restful_handler(HTTP_PUT, "/gn/group/json", put_group_entry);
    ret += register_restful_handler(HTTP_DELETE, "/gn/group/json", del_group_entry);

    ret += register_restful_handler(HTTP_GET, "/gn/path/json", get_path);
    ret += register_restful_handler(HTTP_GET, "/gn/path/stats/json", get_path_stats);
    ret += register_restful_handler(HTTP_GET, "/gn/path/port/json", get_port_stats);
    ret += register_restful_handler(HTTP_GET, "/gn/path/status/json", get_path_status);
    ret += register_restful_handler(HTTP_GET, "/gn/flow/stats/json", get_flow_stats);
    ret += register_restful_handler(HTTP_GET, "/gn/flow/allstats/json", get_all_flow_stats);
    ret += register_restful_handler(HTTP_GET, "/gn/flow/clearstat/json", clear_floatip_stats);

    // fabric
    ret += register_restful_handler(HTTP_GET, "/gn/fabric/switchname/json",get_switch_name);
    ret += register_restful_handler(HTTP_DELETE, "/gn/fabric/delete/json", del_fabric_entries);
    ret += register_restful_handler(HTTP_POST, "/gn/fabric/setup/json", setup_fabric_entries);
    ret += register_restful_handler(HTTP_POST, "/gn/fabric/getpath/json", get_fabric_path);
    ret += register_restful_handler(HTTP_POST, "/gn/fabric/setupparts/json", setup_fabric_entries_parts);

    // nat
    ret += register_restful_handler(HTTP_POST, "/gn/fabric/nat/switch", setup_fabric_nat_switch);

    //config
    ret += register_restful_handler(HTTP_GET, "/gn/config/getall/json", get_all_config_info);
    ret += register_restful_handler(HTTP_POST, "/gn/config/setall/json", set_all_config_info);
 	ret += register_restful_handler(HTTP_POST, "/gn/updateflow/rate/json", set_stats_frequency_config);

    //dcfabric
    ret += register_restful_handler(HTTP_GET, "/dcf/get/all/switchinfo/json", get_fabric_all_switch_info);
    ret += register_restful_handler(HTTP_GET, "/dcf/get/switchinfo/json", get_fabric_switch_info);
    ret += register_restful_handler(HTTP_PUT, "/dcf/put/flowentries/json", put_fabric_flow_entries);
    ret += register_restful_handler(HTTP_GET, "/dcf/del/flowentries/json", del_fabric_flow_entries);
    ret += register_restful_handler(HTTP_GET, "/dcf/get/flowentries/json", get_fabric_flow_entries);
    ret += register_restful_handler(HTTP_GET, "/dcf/get/all/flowentries/json", get_fabric_all_flow_entries);
    ret += register_restful_handler(HTTP_GET, "/dcf/get/host/json", get_fabric_host_by_dpid);

	//cluster
	ret += register_restful_handler(HTTP_GET, "/gn/cluster/query/json",  get_cluster_list);
    ret += register_restful_handler(HTTP_POST, "/gn/cluster/onoff/json", post_cluster_onoff);
    ret += register_restful_handler(HTTP_GET, "/gn/cluster/onoff/json", get_cluster_onoff);
   
    ret += register_restful_handler(HTTP_POST, "/gn/controller/set/json", post_controller_master);
    ret += register_restful_handler(HTTP_POST, "/gn/controller/add/json", post_controller_add);
    ret += register_restful_handler(HTTP_POST, "/gn/controller/delete/json", post_controller_del);

	// debug
	ret += register_restful_handler(HTTP_GET, "/dcf/debug/host", fabric_debug_get_all_host);
	ret += register_restful_handler(HTTP_GET, "/dcf/debug/sw", fabric_debug_get_all_sw);
	ret += register_restful_handler(HTTP_GET, "/dcf/debug/path", fabric_debug_get_all_path);
	ret += register_restful_handler(HTTP_GET, "/dcf/debug/arprequest", fabric_debug_get_all_arp_request);
	ret += register_restful_handler(HTTP_GET, "/dcf/debug/arpflood", fabric_debug_get_all_arp_flood);

	// ret += register_restful_handler(HTTP_POST, "/dcf/debug/reload/security", fabric_debug_reload_security);
	
	INT1 *value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	UINT4 flag_openstack_on = (NULL == value)?0:atoll(value);

	if (flag_openstack_on) {
		ret += register_restful_handler(HTTP_GET, "/gn/tenant/json", get_tenant);
		ret += register_restful_handler(HTTP_POST, "/gn/tenant/json", post_tenant);
		ret += register_restful_handler(HTTP_DELETE, "/gn/tenant/json", del_tenant);
		ret += register_restful_handler(HTTP_GET, "/gn/tenant/member/json", get_tenant_member);
		ret += register_restful_handler(HTTP_POST, "/gn/tenant/member/json", post_tenant_member);
		ret += register_restful_handler(HTTP_DELETE, "/gn/tenant/member/json", del_tenant_member);
	
		ret += register_restful_handler(HTTP_GET, "/gn/neutron/networks", get_neutron_network);
		ret += register_restful_handler(HTTP_POST, "/gn/neutron/networks", post_neutron_network);
		ret += register_restful_handler(HTTP_PUT, "/gn/neutron/neutron/networks", put_neutron_network);
		ret += register_restful_handler(HTTP_DELETE, "/gn/neutron/networks", del_neutron_network);
	
		ret += register_restful_handler(HTTP_GET, "/gn/neutron/subnets", get_neutron_subnet);
		ret += register_restful_handler(HTTP_POST, "/gn/neutron/subnets", post_neutron_subnet);
		ret += register_restful_handler(HTTP_PUT, "/gn/neutron/subnets", put_neutron_subnet);
		ret += register_restful_handler(HTTP_DELETE, "/gn/neutron/subnets", del_neutron_subnet);
	
		ret += register_restful_handler(HTTP_GET, "/gn/neutron/ports", get_neutron_port);
		ret += register_restful_handler(HTTP_POST, "/gn/neutron/ports", post_neutron_port);
		ret += register_restful_handler(HTTP_PUT, "/gn/neutron/ports", put_neutron_port);
		ret += register_restful_handler(HTTP_DELETE, "/gn/neutron/ports", del_neutron_port);

		ret += register_restful_handler(HTTP_POST, "/gn/fabric/external/json", setup_fabric_external);
    	ret += register_restful_handler(HTTP_POST, "/gn/fabric/external/update/json", update_fabric_external);

		// debug
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/floatingip", fabric_debug_get_all_floatingip);
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/loadbalance/pool", fabric_debug_get_all_loadbalance_pool);
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/loadbalance/member", fabric_debug_get_all_loadbalance_member);
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/loadbalance/listener", fabric_debug_get_all_loadbalance_listener);
		ret += register_restful_handler(HTTP_POST, "/dcf/debug/loadbalance/clear", fabric_debug_clear_all_loadbalance);
		ret += register_restful_handler(HTTP_POST, "/dcf/debug/loadbalance/reload", fabric_debug_reload_all_loadbalance);
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/network", fabric_debug_get_all_network);
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/subnet", fabric_debug_get_all_subnet);
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/external", fabric_debug_get_all_external_config);
        ret += register_restful_handler(HTTP_GET, "/dcf/debug/nat/icmp", fabric_debug_get_all_nat_icmp_iden);
        ret += register_restful_handler(HTTP_GET, "/dcf/debug/nat/host", fabric_debug_get_all_nat_host);
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/qos/policy", fabric_debug_get_all_qos_policy);
		ret += register_restful_handler(HTTP_POST, "/dcf/debug/qos/policy", fabric_debug_post_qos_policy);
		ret += register_restful_handler(HTTP_DELETE, "/dcf/debug/qos/policy", fabric_debug_delete_qos_policy);

		
		ret += register_restful_handler(HTTP_GET, "/dcf/debug/security/group", fabric_debug_get_all_securitygroup);

		ret += register_restful_handler(HTTP_GET, "/dcf/debug/security/host", fabric_debug_get_all_hostsecurity);
		ret += register_restful_handler(HTTP_POST, "/dcf/debug/security/clear", fabric_debug_clear_all_security);
		ret += register_restful_handler(HTTP_POST, "/dcf/debug/security/reload", fabric_debug_reload_all_security);

		ret += register_restful_handler(HTTP_GET, "/dcf/debug/check_external", fabric_debug_get_exteral_check);
		ret += register_restful_handler(HTTP_POST, "/dcf/debug/check_external/start", fabric_debug_start_exteral_check);
		ret += register_restful_handler(HTTP_POST, "/dcf/debug/check_external/stop", fabric_debug_stop_exteral_check);
	}
    
    if (NULL == g_flow_entry_json_list)
    {
        g_flow_entry_json_list = (flow_entry_json_t *)gn_malloc(sizeof(flow_entry_json_t));
    }
    g_flow_entry_json_length = 0;

    if(GN_OK != ret)
    {
        ret = GN_ERR;
    }

    return ret;
}
