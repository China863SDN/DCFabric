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
 *  Hbase_sync.c
 *
 *  Created on: March 17, 2016
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */
#include "hbase_sync.h"
#include "fabric_impl.h"
#include "fabric_host.h"
#include "hbase_client.h"
#include "../topo-mgr/topo-mgr.h"
#include "common.h"




void persist_fabric_sw_list()
{
    LOG_PROC("INFO", "Ready to sync sw list.");
    p_fabric_sw_node sw_node = g_fabric_sw_list.node_list;
    UINT4 index = 0;
    neighbor_t* neighbor_sw = NULL;
    UINT8 pre_dpid = 0;
    while (NULL != sw_node && NULL != sw_node->sw)
    {
        g_filed_num_master = 0;
        memset(g_filed_pad_master, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    
        //save dpid
		g_filed_pad_master[g_filed_num_master].type = SW_LIST_OWN_DPID;
		sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", sw_node->sw->dpid);
		g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
		g_filed_num_master++;

        //save tag
		g_filed_pad_master[g_filed_num_master].type = SW_LIST_OWN_TAG;
		sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", sw_node->tag);
		g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
		g_filed_num_master++;
        
        //save pre dpid
		g_filed_pad_master[g_filed_num_master].type = SW_LIST_OWN_PREDPID;
		sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", pre_dpid);
		g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
		g_filed_num_master++;

        for (index = 0; index < sw_node->sw->n_ports; index++)
        {
            neighbor_sw = sw_node->sw->neighbor[index];
            if (NULL != neighbor_sw && NULL != neighbor_sw->sw && NULL != neighbor_sw->port)
            {
                //save own port_no
                g_filed_pad_master[g_filed_num_master].type = SW_LIST_OWN_PORT;
        		sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", sw_node->sw->ports[index].port_no);
        		g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        		g_filed_num_master++;

                //save neighbor dpid
                g_filed_pad_master[g_filed_num_master].type = SW_LIST_NEIGHBOR_DPID;
        		sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", neighbor_sw->sw->dpid);
        		g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        		g_filed_num_master++;

                //save neighbor port_no
                g_filed_pad_master[g_filed_num_master].type = SW_LIST_NEIGHBOR_PORT;
        		sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", neighbor_sw->port->port_no);
        		g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        		g_filed_num_master++;
            }
        }

		persist_data(FABRIC_SW_NODE, OPERATE_ADD, g_filed_num_master, g_filed_pad_master);

        pre_dpid = sw_node->sw->dpid;
        sw_node = sw_node->next;       
    }
}

void recover_fabric_sw_list(INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
    UINT8 own_dpid = 0;
    UINT4 own_tag = 0;
    UINT8 own_pre_dpid = 0;
    UINT4 own_port_no = 0;
    UINT8 neighbor_dpid = 0;
    UINT4 neighbor_port_no = 0;
    p_fabric_sw_node node = NULL;
    while (index < num)
    {
        if (SW_LIST_OWN_DPID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        own_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;

        if (SW_LIST_OWN_TAG != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        own_tag = atoll(field_pad_p[index].pad);
        index++;

        if (SW_LIST_OWN_PREDPID!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        own_pre_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;

        //同步tag
        node = get_fabric_sw_node_by_dpid(own_dpid);
        if (NULL == node)
        {
            continue;
        }
        node->tag = own_tag;

        //同步位置
        if(0 == adjust_fabric_sw_node_list(own_pre_dpid, own_dpid))
        {
            continue;
        }

        //同步相邻节点信息
        while (SW_LIST_OWN_PORT == field_pad_p[index].type && index < num)
        {
            own_port_no = atoll(field_pad_p[index].pad);
            index++;

            if (SW_LIST_NEIGHBOR_DPID != field_pad_p[index].type)
            {
                index++;
                break;
            }
            neighbor_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
            index++;

            if (SW_LIST_NEIGHBOR_PORT != field_pad_p[index].type)
            {
                index++;
                break;
            }
            neighbor_port_no = atoll(field_pad_p[index].pad);
            index++;

            mapping_new_neighbor(node->sw, own_port_no, neighbor_dpid, neighbor_port_no);
        
		}
    }
}



void persist_fabric_host_list()
{
    LOG_PROC("INFO", "Ready to sync host list.");
	p_fabric_host_node node = g_fabric_host_list.list;
	while(NULL != node)
	{
	    g_filed_num_master = 0;
        memset(g_filed_pad_master, 0, sizeof(struct field_pad) * MAX_FILED_NUM);

        if (0 != node->ip_list[0] && NULL != node->sw)
        {
            //save ip
            g_filed_pad_master[g_filed_num_master].type = HOST_LIST_HOST_IP;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", node->ip_list[0]);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save dpid
            g_filed_pad_master[g_filed_num_master].type = HOST_LIST_SW_DPID;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", node->sw->dpid);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save port
            g_filed_pad_master[g_filed_num_master].type = HOST_LIST_SW_PORT;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", node->port);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            persist_data(FABRIC_HOST_NODE, OPERATE_ADD, g_filed_num_master, g_filed_pad_master);
        }
        
		node = node->next;
	}

}


void recover_fabric_host_list(INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
    UINT4 ip = 0;
    UINT8 dpid = 0;
    UINT4 port = 0;
    while (index < num)
    {
        if (HOST_LIST_HOST_IP!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        ip = atoll(field_pad_p[index].pad);
        index++;

        if (HOST_LIST_SW_DPID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;

        if (HOST_LIST_SW_PORT != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        port = atoll(field_pad_p[index].pad);
        index++;

        //find host node
        p_fabric_host_node node = get_fabric_host_from_list_by_ip(ip);
        if (NULL != node)
        {
            p_fabric_sw_node sw_node = get_fabric_sw_node_by_dpid(dpid);
            if (NULL != sw_node && NULL != sw_node->sw)
            {
                node->sw = sw_node->sw;
                node->port = port;
            }
        }
    }
}



void persist_fabric_openstack_external_list()
{
    LOG_PROC("INFO", "Ready to sync openstack external config.");
	openstack_external_node_p node = g_openstack_external_list;
    external_port_p ext = NULL;
	while(NULL != node)
	{
		g_filed_num_master = 0;
        memset(g_filed_pad_master, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
	    ext = (external_port_p)node->data;
        if (NULL != ext)
        {
            //save network id
            g_filed_pad_master[g_filed_num_master].type = EXTERNAL_NETWORK_ID;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%s", ext->network_id);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save ip
            g_filed_pad_master[g_filed_num_master].type = EXTERNAL_GATEWAY_IP;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", ext->external_gateway_ip);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save outer ip
            g_filed_pad_master[g_filed_num_master].type = EXTERNAL_OUTER_INTERFACE_IP;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", ext->external_outer_interface_ip);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save mac
            g_filed_pad_master[g_filed_num_master].type = EXTERNAL_GATEWAY_MAC;
            mac2str(ext->external_gateway_mac, g_filed_pad_master[g_filed_num_master].pad);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save outer mac
            g_filed_pad_master[g_filed_num_master].type = EXTERNAL_OUTER_INTERFACE_MAC;
            mac2str(ext->external_outer_interface_mac, g_filed_pad_master[g_filed_num_master].pad);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save dpid
            g_filed_pad_master[g_filed_num_master].type = EXTERNAL_DPID;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", ext->external_dpid);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            //save port
            g_filed_pad_master[g_filed_num_master].type = EXTERNAL_PORT;
            sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", ext->external_port);
            g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
            g_filed_num_master++;
            
            persist_data(OPENSTACK_EXTERNAL_NODE, OPERATE_ADD, g_filed_num_master, g_filed_pad_master);

        }
        
        node = node->next;
	}
}



void recover_fabric_openstack_external_list(INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
    INT1 network_id[48] = {0};
	UINT4 external_gateway_ip;
	UINT4 external_outer_interface_ip;
	UINT1 external_gateway_mac[6] = {0};
	UINT1 external_outer_interface_mac[6] = {0};
	UINT8 external_dpid;
	UINT4 external_port;
    while (index < num)
    {
        if (EXTERNAL_NETWORK_ID!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        strncpy(network_id, field_pad_p[index].pad, 48);
        index++;
        
        if (EXTERNAL_GATEWAY_IP!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_gateway_ip = atoll(field_pad_p[index].pad);
        index++;
        
         if (EXTERNAL_OUTER_INTERFACE_IP!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_outer_interface_ip = atoll(field_pad_p[index].pad);
        index++;
        
        if (EXTERNAL_GATEWAY_MAC!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        macstr2hex((INT1*)field_pad_p[index].pad, external_gateway_mac);
        index++;
        
        if (EXTERNAL_OUTER_INTERFACE_MAC!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        macstr2hex((INT1*)field_pad_p[index].pad, external_outer_interface_mac);
        index++;
        
        if (EXTERNAL_DPID!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;
        
        if (EXTERNAL_PORT!= field_pad_p[index].type)
        {
            index++;
            continue;
        }
        external_port = atoll(field_pad_p[index].pad);
        index++;

        //add to list
       create_external_port_by_rest(
            external_gateway_ip,
            external_gateway_mac,
            external_outer_interface_ip,
            external_outer_interface_mac,
            external_dpid,
            external_port,
            network_id);

    }
}


void persist_fabric_nat_icmp_iden_single(UINT4 operation_type, openstack_external_node_p node)
{
    LOG_PROC("INFO", "Ready to sync nat icmp iden, operation type=%d.", operation_type);
	g_filed_num_master = 0;
    memset(g_filed_pad_master, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    nat_icmp_iden_p ext = (nat_icmp_iden_p)node->data;
    if (NULL != ext)
    {
        //save identifier
        g_filed_pad_master[g_filed_num_master].type = NAT_ICMP_IDENTIFIER;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%d", ext->identifier);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
        
        //save host_ip
        g_filed_pad_master[g_filed_num_master].type = NAT_ICMP_HOST_IP;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", ext->host_ip);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
        
        //save host_mac
        g_filed_pad_master[g_filed_num_master].type = NAT_ICMP_MAC;
        mac2str(ext->host_mac, g_filed_pad_master[g_filed_num_master].pad);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
        
        //save sw_dpid
        g_filed_pad_master[g_filed_num_master].type = NAT_ICMP_SW_DPID;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", ext->sw_dpid);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
        
        //save inport
        g_filed_pad_master[g_filed_num_master].type = NAT_ICMP_INPORT;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", ext->inport);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
        
        persist_data(NAT_ICMP, operation_type, g_filed_num_master, g_filed_pad_master);

    }

}





void persist_fabric_nat_icmp_iden_list()
{
    LOG_PROC("INFO", "Ready to sync nat icmp iden.");
	openstack_external_node_p node = g_nat_icmp_iden_list;
	while(NULL != node)
	{
        persist_fabric_nat_icmp_iden_single(OPERATE_ADD, node);
        node = node->next;
	}
}



void recover_fabric_nat_icmp_iden_list(UINT4 operation_type, INT4 num, const field_pad_t* field_pad_p)
{
	UINT2 identifier;
	UINT4 host_ip;
	UINT1 host_mac[6] = {0};
	UINT8 sw_dpid;
	UINT4 inport;
    INT4 index = 0;
    while (index < num)
    {
        if (NAT_ICMP_IDENTIFIER != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        identifier = atoi(field_pad_p[index].pad);
        index++;
        
        if (NAT_ICMP_HOST_IP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        host_ip = atoll(field_pad_p[index].pad);
        index++;
        
         if (NAT_ICMP_MAC != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        macstr2hex((INT1*)field_pad_p[index].pad, host_mac);
        index++;
        
        if (NAT_ICMP_SW_DPID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        sw_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
        index++;
        
        if (NAT_ICMP_INPORT != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        inport = atoll(field_pad_p[index].pad);
        index++;

        if (OPERATE_DEL == operation_type)
        {
            return;
        }
        
        update_nat_icmp_iden(
            identifier,
            host_ip,
            host_mac,
            sw_dpid,
            inport);
    }
}


void persist_fabric_nat_host_list()
{
    LOG_PROC("INFO", "Ready to sync nat host.");
    if (NULL != g_nat_host_list_head)
    {
    	nat_host_p node = g_nat_host_list_head->next;
    	while(NULL != node)
    	{
            persist_fabric_nat_host_single(OPERATE_ADD, node);
            node = node->next;
    	}
    }
}



void persist_fabric_nat_host_single(UINT4 operation_type, nat_host_p host_p)
{
    LOG_PROC("INFO", "Ready to sync nat host, operation type=%d.", operation_type);
    g_filed_num_master = 0;
    memset(g_filed_pad_master, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
    
    //save host_ip
    g_filed_pad_master[g_filed_num_master].type = NAT_HOST_IP;
    sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", host_p->host_ip);
    g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
    g_filed_num_master++;
    
    //tcp list
    nat_port_p tcp_node = host_p->tcp_port_list->next;
    if (NULL != tcp_node)
    {
        //save tcp port type
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_TCP_PORT;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%s", "TCP");
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    }
    
    while (NULL != tcp_node)
    {
        //save external_port_no
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_EXTERNAL_PORTNO;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%d", tcp_node->external_port_no);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save internal_ip
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_INTERNAL_IP;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", tcp_node->internal_ip);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save internal_port_no
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_INTERNAL_PORTNO;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%d", tcp_node->internal_port_no);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save internal_mac
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_INTERNAL_MAC;
        mac2str(tcp_node->internal_mac, g_filed_pad_master[g_filed_num_master].pad);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save gateway_dpid
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_GATEWAY_DPID;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", tcp_node->gateway_dpid);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save src_dpid
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_SRC_DPID;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", tcp_node->src_dpid);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        tcp_node = tcp_node->next;
    }
    
    //udp list
    nat_port_p udp_node = host_p->udp_port_list->next;
    if (NULL != udp_node)
    {
        //save tcp port type
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_UDP_PORT;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%s", "UDP");
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    }
    
    while (NULL != udp_node)
    {
        //save external_port_no
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_EXTERNAL_PORTNO;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%d", udp_node->external_port_no);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save internal_ip
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_INTERNAL_IP;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", udp_node->internal_ip);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save internal_port_no
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_INTERNAL_PORTNO;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%d", udp_node->internal_port_no);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save internal_mac
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_INTERNAL_MAC;
        mac2str(udp_node->internal_mac, g_filed_pad_master[g_filed_num_master].pad);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save gateway_dpid
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_GATEWAY_DPID;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", udp_node->gateway_dpid);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        //save src_dpid
        g_filed_pad_master[g_filed_num_master].type = NAT_HOST_SRC_DPID;
        sprintf(g_filed_pad_master[g_filed_num_master].pad, "%llu", udp_node->src_dpid);
        g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
        g_filed_num_master++;
    
        udp_node = udp_node->next;
    }
    
    persist_data(NAT_HOST, operation_type, g_filed_num_master, g_filed_pad_master);
}




void recover_fabric_nat_host_list(UINT4 operation_type, INT4 num, const field_pad_t* field_pad_p)
{
    UINT4 host_ip;
    //UINT2 external_port_no;
	UINT4 internal_ip;
	UINT2 internal_port_no;
	UINT1 internal_mac[6] = {0};
	UINT8 gateway_dpid;
	UINT8 src_dpid;
    INT4 index = 0;
    nat_host_p nat_host = NULL;
    while (index < num)
    {
        if (NAT_HOST_IP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        host_ip = atoll(field_pad_p[index].pad);
        index++;

        nat_host = find_nat_host_by_ip(host_ip);
        if (NULL != nat_host)
        {
            if (NULL != nat_host->tcp_port_list)
            {
                remove_nat_all_port(nat_host->tcp_port_list);
            }
            
            if (NULL != nat_host->udp_port_list)
            {
                remove_nat_all_port(nat_host->udp_port_list);
            }
        }

        if (OPERATE_DEL == operation_type)
        {
            remove_nat_host_by_ip(host_ip);
            continue;
        }

        if (NAT_HOST_TCP_PORT == field_pad_p[index].type)
        {
            index++;
            while (index < num && NAT_HOST_EXTERNAL_PORTNO == field_pad_p[index].type)
            {
                //external_port_no = atoi(field_pad_p[index].pad);
                index++;
            
                if (NAT_HOST_INTERNAL_IP != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                internal_ip = atoll(field_pad_p[index].pad);
                index++;

                if (NAT_HOST_INTERNAL_PORTNO != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                internal_port_no = atoi(field_pad_p[index].pad);
                index++;

                if (NAT_HOST_INTERNAL_MAC != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                macstr2hex((INT1*)field_pad_p[index].pad, internal_mac);
                index++;

                if (NAT_HOST_GATEWAY_DPID != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                gateway_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
                index++;

                if (NAT_HOST_SRC_DPID != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                src_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
                index++;

                create_nat_connect(
                    internal_ip, 
                    host_ip, 
                    internal_port_no, 
                    IPPROTO_TCP, 
                    internal_mac,
                    gateway_dpid,
                    src_dpid);
            }
        }

        if (index < num && NAT_HOST_UDP_PORT == field_pad_p[index].type)
        {
            index++;
            while (index < num && NAT_HOST_EXTERNAL_PORTNO == field_pad_p[index].type)
            {
                //external_port_no = atoi(field_pad_p[index].pad);
                index++;
            
                if (NAT_HOST_INTERNAL_IP != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                internal_ip = atoll(field_pad_p[index].pad);
                index++;

                if (NAT_HOST_INTERNAL_PORTNO != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                internal_port_no = atoi(field_pad_p[index].pad);
                index++;

                if (NAT_HOST_INTERNAL_MAC != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                macstr2hex((INT1*)field_pad_p[index].pad, internal_mac);
                index++;

                if (NAT_HOST_GATEWAY_DPID != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                gateway_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
                index++;

                if (NAT_HOST_SRC_DPID != field_pad_p[index].type)
                {
                    index++;
                    break;
                }
                src_dpid = strtoull(field_pad_p[index].pad, NULL, 10);
                index++;

                create_nat_connect(
                    internal_ip, 
                    host_ip, 
                    internal_port_no, 
                    IPPROTO_UDP, 
                    internal_mac,
                    gateway_dpid,
                    src_dpid);
            }
        }
    }
}



void persist_fabric_openstack_lbaas_members_single(UINT4 operation_type, openstack_lbaas_connect_p connect_ips)
{
    LOG_PROC("INFO", "Ready to sync openstack lbaas members conns, operation type=%d.", operation_type);

    g_filed_num_master = 0;
    memset(g_filed_pad_master, 0, sizeof(struct field_pad) * MAX_FILED_NUM);

    //save ext_ip
    g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_EXT_ID;
    sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->ext_ip);
    g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
    g_filed_num_master++;

    //save inside_ip
    g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_INSIDE_IP;
    sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->inside_ip);
    g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
    g_filed_num_master++;

    //save vip
    g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_VIP;
    sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->vip);
    g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
    g_filed_num_master++;
    
    //save src_port_no
    g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_SRC_PORTNO;
    sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->src_port_no);
    g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
    g_filed_num_master++;

    //save ext_port_no
    g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_EXT_PORTNO;
    sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->ext_port_no);
    g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
    g_filed_num_master++;
                
    persist_data(OPENSTACK_LBAAS_MEMBERS, operation_type, g_filed_num_master, g_filed_pad_master);

}


void persist_fabric_openstack_lbaas_members_list()
{
    LOG_PROC("INFO", "Ready to sync openstack lbaas members conns.");

    openstack_lbaas_node_p node = g_openstack_lbaas_members_list;
    openstack_lbaas_members_p mem = NULL;
    openstack_lbaas_node_p connect_node = NULL;
    openstack_lbaas_connect_p connect_ips = NULL;
	while(NULL != node)
	{
	    g_filed_num_master = 0;
        memset(g_filed_pad_master, 0, sizeof(struct field_pad) * MAX_FILED_NUM);
        
	    mem  = (openstack_lbaas_members_p)node->data;
        if (NULL != mem)
        {
            connect_node = mem->connect_ips;
            while (NULL != connect_node && NULL != connect_node->data)
            {
                connect_ips = (openstack_lbaas_connect_p)connect_node->data;
                //save ext_ip
                g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_EXT_ID;
                sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->ext_ip);
                g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
                g_filed_num_master++;

                //save inside_ip
                g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_INSIDE_IP;
                sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->inside_ip);
                g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
                g_filed_num_master++;

                //save vip
                g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_VIP;
                sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->vip);
                g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
                g_filed_num_master++;
                
                //save src_port_no
                g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_SRC_PORTNO;
                sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->src_port_no);
                g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
                g_filed_num_master++;

                //save ext_port_no
                g_filed_pad_master[g_filed_num_master].type = LBAAS_MEMBERS_EXT_PORTNO;
                sprintf(g_filed_pad_master[g_filed_num_master].pad, "%u", connect_ips->ext_port_no);
                g_filed_pad_master[g_filed_num_master].len = strlen(g_filed_pad_master[g_filed_num_master].pad);
                g_filed_num_master++;
                
                connect_node = connect_node->next;
            }
            
            persist_data(OPENSTACK_LBAAS_MEMBERS, OPERATE_ADD, g_filed_num_master, g_filed_pad_master);
        }

        node = node->next;
    }
}



void recover_fabric_openstack_lbaas_members_list(UINT4 operation_type, INT4 num, const field_pad_t* field_pad_p)
{
    INT4 index = 0;
	UINT4 ext_ip;
	UINT4 inside_ip;
	UINT4 vip;
	UINT4 src_port_no;
	//UINT4 ext_port_no;
    while (index < num)
    {
        if (LBAAS_MEMBERS_EXT_ID != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        ext_ip = atoll(field_pad_p[index].pad);
        index++;
        
        if (LBAAS_MEMBERS_INSIDE_IP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        inside_ip = atoll(field_pad_p[index].pad);
        index++;

        if (LBAAS_MEMBERS_VIP != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        vip = atoll(field_pad_p[index].pad);
        index++;

        if (LBAAS_MEMBERS_SRC_PORTNO != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        src_port_no = atoll(field_pad_p[index].pad);
        index++;
        
        if (LBAAS_MEMBERS_EXT_PORTNO != field_pad_p[index].type)
        {
            index++;
            continue;
        }
        //ext_port_no = atoll(field_pad_p[index].pad);
        index++;

        if (OPERATE_DEL == operation_type)
        {
            remove_openstack_lbaas_connect(ext_ip, inside_ip, vip, src_port_no);
            continue;
        }
        else if (OPERATE_ADD == operation_type)
        {
            //add to list
            create_openstack_lbaas_connect(
                ext_ip,
                inside_ip,
                vip,
                src_port_no);
        }
    }
}



void persist_fabric_all()
{
    persist_fabric_sw_list();
    persist_fabric_host_list();
    persist_fabric_openstack_external_list();
    persist_fabric_nat_icmp_iden_list();
    persist_fabric_nat_host_list();
    persist_fabric_openstack_lbaas_members_list();
}
