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
#include "gn_inet.h"
#include "timer.h"
#include "../conn-svr/conn-svr.h"
#include "../flow-mgr/flow-mgr.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "../forward-mgr/forward-mgr.h"
#include "../tenant-mgr/tenant-mgr.h"
#include "../stats-mgr/stats-mgr.h"
#include "../meter-mgr/meter-mgr.h"
#include "../group-mgr/group-mgr.h"
#include "../topo-mgr/topo-mgr.h"
#include "../user-mgr/user-mgr.h"
#include "error_info.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "fabric_impl.h"
//锟斤拷锟斤拷url锟斤拷锟斤拷锟侥诧拷锟斤拷
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

//锟斤拷json锟斤拷锟斤拷转锟斤拷为锟街凤拷锟斤拷锟酵凤拷json锟斤拷锟襟，凤拷锟斤拷锟街凤拷锟斤拷锟斤拷锟斤拷锟揭拷头锟�
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

//    LOG_PROC("INFO", "Reply: %s", reply);
    return reply;
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
        else
        {
            //
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
            value = json_new_string("REQ");
        }
        else
        {
            value = json_new_string("REP");
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
        else    //锟斤拷锟斤拷锟斤拷
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
        else    //锟斤拷锟斤拷锟斤拷
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
        oxm_fields->in_port = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IN_PORT);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "inPhyPort");
    if (item)
    {
        oxm_fields->in_phy_port = atoi(item->child->text);
        oxm_fields->mask |= (MASK_SET << OFPXMT_OFB_IN_PHY_PORT);
        json_free_value(&item);
    }

    item = json_find_first_label(obj, "metadata");
    if (item)
    {
        oxm_fields->metadata = atoi(item->child->text);
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
                if (0 == strncmp(item->child->text, "REQ", 3))
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
                oxm_fields->mpls_label = atoi(item->child->text);
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
        oxm_fields->mpls_label = atoi(item->child->text);
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
        p_action->port = atoi(item->child->text);

        json_free_value(&item);
    }

    item = json_find_first_label(obj, "group");
    if (item)
    {
        gn_action_group_t *p_action = (gn_action_group_t *)mem_get(g_gnaction_mempool_id);
        p_action->next = *actions;
        *actions = (gn_action_t *) p_action;
        p_action->type = OFPAT13_GROUP;
        p_action->group_id = atoi(item->child->text);

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
        p_ins_actions->meter_id = atoi(item->child->text);
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
        flow->match.type = OFPMT_OXM;
        json_parse_oxm_fields(item->child, &(flow->match.oxm_fields));
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
    ret = add_flow_entry(sw, flow);
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

    item = json_find_first_label(root, "instruction");
    if (item)
    {
        json_parse_instructions(item->child, &(flow->instructions));
        json_free_value(&item);
    }

    strncpy(flow->creater, "Restful", strlen("Restful"));
    flow->create_time = g_cur_sys_time.tv_sec;
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

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    meter = (gn_meter_t *)gn_malloc(sizeof(gn_meter_t));
    item = json_find_first_label(root, "meterId");
    if (item)
    {
        meter->meter_id = atoi(item->child->text);
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
            meter->flags = OFPMBT_DROP;
        }
        else if(0 == strcmp(item->child->text, "Dscp"))
        {
            meter->flags = OFPMBT_DSCP_REMARK;
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
        meter->rate = atoi(item->child->text);
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
        meter->burst_size = atoi(item->child->text);
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
        meter->meter_id = atoi(item->child->text);
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
            meter->flags = OFPMBT_DROP;
        }
        else if(0 == strcmp(item->child->text, "Dscp"))
        {
            meter->flags = OFPMBT_DSCP_REMARK;
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
        meter->rate = atoi(item->child->text);
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
        meter->burst_size = atoi(item->child->text);
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
        meter->meter_id = atoi(item->child->text);
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
                sprintf(json_temp, "%d", p_group->type);
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
                }
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

    sw = find_sw_by_dpid(uc8_to_ulli64(dpid, &_dpid));
    if (NULL == sw)
    {
        return json_to_reply(NULL, EC_SW_NOT_EXIST);
    }

    group = (gn_group_t *)gn_malloc(sizeof(gn_group_t));
    item = json_find_first_label(root, "groupId");
    if (item)
    {
        group->group_id = atoi(item->child->text);
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

            bucket_item = json_find_first_label(buckets, "watch_port");
            if(bucket_item)
            {
                bucket->watch_port = atoi(bucket_item->child->text);
                json_free_value(&bucket_item);
            }

            bucket_item = json_find_first_label(buckets, "watch_group");
            if(bucket_item)
            {
                bucket->watch_group = atoi(bucket_item->child->text);
                json_free_value(&bucket_item);
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
        group->group_id = atoi(item->child->text);
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

            bucket_item = json_find_first_label(buckets, "watch_port");
            if(bucket_item)
            {
                bucket->watch_port = atoi(bucket_item->child->text);
                json_free_value(&bucket_item);
            }

            bucket_item = json_find_first_label(buckets, "watch_group");
            if(bucket_item)
            {
                bucket->watch_group = atoi(bucket_item->child->text);
                json_free_value(&bucket_item);
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

    ret = add_group_entry(sw, group);
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
        group->group_id = atoi(item->child->text);
        json_free_value(&item);
    }

    ret = delete_group_entry(sw, group);
    free(group);
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
        src_dpid = atoi(kv_src_port.value);
    }

    if(kv_dst_port.value)
    {
        dst_dpid = atoi(kv_dst_port.value);
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
        //锟斤拷前锟斤拷锟斤拷锟斤拷锟斤拷锟�
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
                sprintf(temp, "%d", port->stats.rx_status);
                value = json_new_number(temp);
                json_insert_child(key, value);
                json_insert_child(obj, key);

                key = json_new_string("txUsedStatus");
                sprintf(temp, "%d", port->stats.tx_status);
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
                        sprintf(tmp_str, "%d", port->stats.tx_status);
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
        tenant_id = atoi(item->child->text);

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

    tenant = tenant_con->tenants[atoi(tenant_id.value)];
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
        tenant_id = atoi(item->child->text);

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
static INT1 *setup_fabric_entries(const INT1 *url, json_t *root){

	of131_fabric_impl_setup();
    return json_to_reply(NULL, GN_OK);
}
static INT1 *del_fabric_entries(const INT1 *url, json_t *root)
{
	of131_fabric_impl_delete();
    return json_to_reply(NULL, GN_OK);
}
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
	        dpids[len] = atoi(token);
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
    	src_dpid = atoi(item->child->text);
    }else{
    	return json_to_reply(NULL, EC_RESTFUL_REQUIRE_SRCDPID);
    }

    item = json_find_first_label(root, "dstDPID");
	if(item){
		dst_dpid = atoi(item->child->text);
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
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *post_neutron_subnet(const INT1 *url, json_t *root)
{
    INT1 *reply = (char *)gn_malloc(25);
    memcpy(reply,"{\"status\":\"success\"}", strlen("{\"status\":\"success\"}"));

    return reply;
}

static INT1 *post_neutron_port(const INT1 *url, json_t *root)
{
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
    INT1 *reply = (char *)gn_malloc(25);
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
    ret += register_restful_handler(HTTP_POST, "/gn/meter/json", put_meter_entry);
    ret += register_restful_handler(HTTP_DELETE, "/gn/meter/json", del_meter_entry);

    ret += register_restful_handler(HTTP_GET, "/gn/groups/json", get_group_entries);
    ret += register_restful_handler(HTTP_DELETE, "/gn/groups/json", del_group_entries);
    ret += register_restful_handler(HTTP_POST, "/gn/group/json", post_group_entry);
    ret += register_restful_handler(HTTP_POST, "/gn/group/json", put_group_entry);
    ret += register_restful_handler(HTTP_DELETE, "/gn/group/json", del_group_entry);

    ret += register_restful_handler(HTTP_GET, "/gn/path/json", get_path);
    ret += register_restful_handler(HTTP_GET, "/gn/path/stats/json", get_path_stats);
    ret += register_restful_handler(HTTP_GET, "/gn/path/status/json", get_path_status);

    ret += register_restful_handler(HTTP_GET, "/gn/tenant/json", get_tenant);
    ret += register_restful_handler(HTTP_POST, "/gn/tenant/json", post_tenant);
    ret += register_restful_handler(HTTP_DELETE, "/gn/tenant/json", del_tenant);
    ret += register_restful_handler(HTTP_GET, "/gn/tenant/member/json", get_tenant_member);
    ret += register_restful_handler(HTTP_POST, "/gn/tenant/member/json", post_tenant_member);
    ret += register_restful_handler(HTTP_DELETE, "/gn/tenant/member/json", del_tenant_member);

    ret += register_restful_handler(HTTP_GET, "/controller/nb/v2/neutron/networks", get_neutron_network);
    ret += register_restful_handler(HTTP_POST, "/controller/nb/v2/neutron/networks", post_neutron_network);
    ret += register_restful_handler(HTTP_PUT, "/controller/nb/v2/neutron/networks", put_neutron_network);
    ret += register_restful_handler(HTTP_DELETE, "/controller/nb/v2/neutron/networks", del_neutron_network);

    ret += register_restful_handler(HTTP_GET, "/controller/nb/v2/neutron/subnets", get_neutron_subnet);
    ret += register_restful_handler(HTTP_POST, "/controller/nb/v2/neutron/subnets", post_neutron_subnet);
    ret += register_restful_handler(HTTP_PUT, "/controller/nb/v2/neutron/subnets", put_neutron_subnet);
    ret += register_restful_handler(HTTP_DELETE, "/controller/nb/v2/neutron/networks", del_neutron_subnet);

    ret += register_restful_handler(HTTP_GET, "/controller/nb/v2/neutron/ports", get_neutron_port);
    ret += register_restful_handler(HTTP_POST, "/controller/nb/v2/neutron/ports", post_neutron_port);
    ret += register_restful_handler(HTTP_PUT, "/controller/nb/v2/neutron/ports", put_neutron_port);
    ret += register_restful_handler(HTTP_DELETE, "/controller/nb/v2/neutron/ports", del_neutron_port);

    // fabric
    ret += register_restful_handler(HTTP_DELETE, "/gn/fabric/delete/json", del_fabric_entries);
    ret += register_restful_handler(HTTP_POST, "/gn/fabric/setup/json", setup_fabric_entries);
    ret += register_restful_handler(HTTP_POST, "/gn/fabric/getpath/json", get_fabric_path);
    ret += register_restful_handler(HTTP_POST, "/gn/fabric/setupparts/json", setup_fabric_entries_parts);
    if(GN_OK != ret)
    {
        ret = GN_ERR;
    }

    return ret;
}
