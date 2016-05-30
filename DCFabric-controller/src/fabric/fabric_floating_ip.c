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
 * fabric_floating_ip.c
 *
 *  Created on: Sep 8, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: Sep 8, 2015
 */

#include "fabric_floating_ip.h"
#include "common.h"
#include "mod-types.h"
#include "fabric_flows.h"
#include "timer.h"
#include "gn_inet.h"
#include "openstack_host.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "../conn-svr/conn-svr.h"
#include "fabric_impl.h"
#include "fabric_openstack_nat.h"
#include "openstack_app.h"
#include "fabric_openstack_arp.h"
#include "openstack_lbaas_app.h"
#include "../group-mgr/group-mgr.h"
#include "../cluster-mgr/cluster-mgr.h"

void *g_floating_check_timerid = NULL;
UINT4 g_floating_check_interval = 5;
void *g_floating_check_timer = NULL;
UINT4 g_proactive_flow_flag = 0;

extern openstack_external_node_p g_openstack_floating_list;
extern openstack_node_p g_openstack_host_subnet_list;
extern UINT4 g_openstack_on;

/* 
 * internal functions
 */
INT4 create_proactive_floating_with_lbaas_group_flows(gn_switch_t* ext_sw, p_fabric_host_node fixed_port, 
                                        external_port_p ext_port, UINT4 floatingip);
INT4 remove_proactive_floating_with_lbaas_group_flows(gn_switch_t* ext_sw, p_fabric_host_node fixed_port, 
                                        external_port_p ext_port, UINT4 floatingip);



INT4 fabric_openstack_floating_ip_packet_out_handle(p_fabric_host_node src_port, packet_in_info_t *packet_in, external_floating_ip_p fip, param_set_p param_set)
{
	// printf("%s\n", FN);
	ip_t *ip = (ip_t *)(packet_in->data);

	external_port_p ext_port = NULL;
	ext_port = get_external_port_by_floatip(fip->floating_ip);

	if ((src_port == NULL || ext_port == NULL) || (src_port->sw == NULL))
	{
		return IP_DROP;
	}

	//write flow table
	//packet out rule
	gn_switch_t * switch_gw = find_sw_by_dpid(ext_port->external_dpid);
	if (NULL == switch_gw) {
		LOG_PROC("INFO", "Floating: External switch is NULL!");
		return IP_DROP;
	}

	UINT4 vlan_id = of131_fabric_impl_get_tag_sw(switch_gw);
	param_set->src_sw = src_port->sw;
	param_set->dst_ip = ip->dest;
	memcpy(param_set->src_mac, src_port->mac, 6);
	param_set->mod_src_ip = fip->floating_ip;
	memcpy(param_set->dst_gateway_mac, ext_port->external_gateway_mac, 6);
	param_set->src_vlanid = vlan_id;

	//response rule
	vlan_id = of131_fabric_impl_get_tag_sw(src_port->sw);

	UINT4 out_port = get_out_port_between_switch(ext_port->external_dpid, src_port->sw->dpid);
	if (0 != out_port) {
		//fabric_openstack_floating_ip_install_set_vlan_in_flow(switch_gw, fip->floating_ip, ip->src, ip->eth_head.src, vlan_id, out_port);
		param_set->dst_sw = switch_gw;
		param_set->src_ip = ip->src;
		memcpy(param_set->packet_src_mac, ip->eth_head.src, 6);
		param_set->dst_vlanid = vlan_id;
		param_set->dst_inport = out_port;
		param_set->src_inport = packet_in->inport;

		return Floating_ip_flow;
	}

	return IP_DROP;
}

INT4 compare_sw_between_host(p_fabric_host_node host_p, p_fabric_host_node fixed_port)
{
    if ((NULL == host_p) || (NULL == host_p->sw) || (NULL == fixed_port) || (host_p == fixed_port)) {
        return 0;
    }

    if (OPENSTACK_PORT_TYPE_HOST == fixed_port->type) {
        if (fixed_port->sw == host_p->sw) {
            return 1;
        }
    }
    else if (OPENSTACK_PORT_TYPE_LOADBALANCER == fixed_port->type) {
       openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_ip(fixed_port->ip_list[0]);
       if (pool_p) {
           openstack_lbaas_node_p node_p = pool_p->pool_member_list;
            while (node_p) {
                openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)node_p->data;
                if (member_p) {
                    p_fabric_host_node fixed_host_p = get_fabric_host_from_list_by_ip(member_p->fixed_ip);

                    if ((fixed_host_p) && (fixed_host_p->sw) && (fixed_host_p->sw == host_p->sw)) {
                        return 1;
                    }
                }
                node_p = node_p->next;
           }
       }
    }
    else {
        }

    return 0;
}


INT4 judge_sw_in_use(p_fabric_host_node host_p)
{
    if ((NULL == host_p) || (NULL == host_p->sw) || (OPENSTACK_PORT_TYPE_HOST != host_p->type)) {
        return 0;
    }

    INT4 return_value = 0;
    
    openstack_external_node_p node_p = g_openstack_floating_list;
    p_fabric_host_node fixed_port = NULL;
        
    while (NULL != node_p) {
        external_floating_ip_p efp = (external_floating_ip_p)node_p->data;
        
        if ((efp->fixed_ip) && (efp->floating_ip)) {
            fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);

            return_value = compare_sw_between_host(host_p, fixed_port);

            if (return_value) {
                return 1;
            }
        }
        node_p = node_p->next;
    }

	
	return return_value;
}


INT4 update_floating_internal_subnet_flow(p_fabric_host_node host_p, UINT4 type, char* cidr, UINT1 external)
{
    if ((NULL == host_p) || (0 == strlen(cidr)) || (NULL == host_p->sw)) {
        return 0;
    }

    UINT4 ip = 0;
    UINT4 mask = 0;
    cidr_str_to_ip_prefix(cidr, &ip, &mask);

    if ((ip) && (mask) && (mask <=32)) {
        if (0 == external) {
            if ((2 == type) && (judge_sw_in_use(host_p))) {
                return 0;
            }
            install_fabric_openstack_floating_internal_subnet_flow(host_p->sw, type, ip, mask, NULL);
        }
        else {
            install_fabric_openstack_floating_internal_subnet_flow(host_p->sw, 2, ip, mask, NULL);
        }
    }
    return 0;
}


INT4 process_floating_internal_subnet_flow(p_fabric_host_node fixed_port, openstack_subnet_p subnet, INT4 type)
{
    if ((NULL == fixed_port) || (NULL == subnet) || (0 == strlen(subnet->cidr))) {
        return 0;
    }

	if (OPENSTACK_PORT_TYPE_HOST == fixed_port->type) {
		update_floating_internal_subnet_flow(fixed_port, type, subnet->cidr, subnet->external);
	}
    else if (OPENSTACK_PORT_TYPE_LOADBALANCER == fixed_port->type) {
       openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_ip(fixed_port->ip_list[0]);
       if (pool_p) {
           openstack_lbaas_node_p node_p = pool_p->pool_member_list;
            while (node_p) {
                openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)node_p->data;
                if (member_p) {
                    p_fabric_host_node host_p = get_fabric_host_from_list_by_ip(member_p->fixed_ip);
                    update_floating_internal_subnet_flow(host_p, type, subnet->cidr, subnet->external);
                }
                node_p = node_p->next;
           }
       }
    }
    else {
        }
	return 0;
}

INT4 process_floating_internal_subnet_flow_by_subnet(openstack_subnet_p subnet, INT4 type)
{
    if (0 == g_proactive_flow_flag) {
        return 0;
    }

    if (OFPCR_ROLE_SLAVE == g_controller_role) {
        return 0;
    }
        
    if (NULL == subnet) {
		return 0;
	}
	
	external_floating_ip_p efp = NULL;
	openstack_external_node_p node_p = g_openstack_floating_list;
	while (NULL != node_p) {
		efp = (external_floating_ip_p)node_p->data;

		if (efp) {
			p_fabric_host_node fixed_port = get_fabric_host_from_list_by_ip(efp->fixed_ip);
			if (fixed_port) {
				process_floating_internal_subnet_flow(fixed_port, subnet, type);
			}	
		}
			
		node_p = node_p->next;
	}

	return 1;
}

INT4 create_proactive_floating_internal_subnet_flow_by_subnet(openstack_subnet_p subnet)
{
	return process_floating_internal_subnet_flow_by_subnet(subnet, 1);
}

INT4 remove_proactive_floating_internal_subnet_flow_by_subnet(openstack_subnet_p subnet)
{
	return process_floating_internal_subnet_flow_by_subnet(subnet, 2);
}


INT4 process_proactive_internal_subnet_flow_by_fixed_port(p_fabric_host_node fixed_port, INT4 type)
{
	if (NULL == fixed_port) {
		return 0;
	}
	
	openstack_subnet_p subnet = NULL;
	openstack_node_p node_p = g_openstack_host_subnet_list;
	while (node_p != NULL) {
		subnet = (openstack_subnet_p)node_p->data;
		if (subnet) {
			process_floating_internal_subnet_flow(fixed_port, subnet, type);
		}
		node_p = node_p->next;
	}	

	return 1;
}

INT4 create_proactive_internal_subnet_flow_by_fixed_port(p_fabric_host_node fixed_port)
{
	return process_proactive_internal_subnet_flow_by_fixed_port(fixed_port, 1);
}

INT4 remove_proactive_internal_subnet_flow_by_fixed_port(p_fabric_host_node fixed_port)
{
	return process_proactive_internal_subnet_flow_by_fixed_port(fixed_port, 2);
}

INT4 process_floating_lbaas_group_flow_by_pool_ip(UINT4 ip, UINT4 type)
{
    if (0 == g_proactive_flow_flag) {
        return 0;
    }

    if (OFPCR_ROLE_SLAVE == g_controller_role) {
         return 0;
    }
        
    external_floating_ip_p fip = get_external_floating_ip_by_fixed_ip(ip);
    if (NULL == fip) {
        // printf("fip is NULL!\n");
        return 0;
    }
    
	external_port_p ext_port = NULL;
	gn_switch_t* ext_sw = NULL;
	p_fabric_host_node fixed_port = NULL;
    
	ext_port = find_openstack_external_by_floating_ip(fip->floating_ip);
	if (NULL == ext_port) {
		return 0;
	}

	ext_sw = find_sw_by_dpid(ext_port->external_dpid);
    fixed_port = get_fabric_host_from_list_by_ip(fip->fixed_ip);
    if ((ext_sw) && (fixed_port)) {
        if (1 == type) {
            create_proactive_floating_with_lbaas_group_flows(ext_sw, fixed_port, ext_port ,fip->floating_ip);
        }
        else if (2 == type) {
            remove_proactive_floating_with_lbaas_group_flows(ext_sw, fixed_port, ext_port, fip->floating_ip);
        }
        else {
            // do nothing
        }        
    }

    return 0;
}

INT4 create_proactive_floating_lbaas_flow_by_pool_ip(UINT4 pool_ip)
{
    return process_floating_lbaas_group_flow_by_pool_ip(pool_ip, 1);
}

INT4 remove_proactive_floating_lbaas_flow_by_pool_ip(UINT4 pool_ip)
{
    return process_floating_lbaas_group_flow_by_pool_ip(pool_ip, 2);
}


INT4 process_floating_lbaas_group_flow_by_member_ip(UINT4 ip, UINT4 member_ip, UINT4 member_portno, UINT4 type)
{   
    if (0 == g_proactive_flow_flag) {
        return 0;
    }

    if (OFPCR_ROLE_SLAVE == g_controller_role) {
        return 0;
    }
        
    external_floating_ip_p fip = get_external_floating_ip_by_fixed_ip(ip);
    if (NULL == fip) {
        // printf("fip is NULL!\n");
        return 0;
    }
        
	external_port_p ext_port = NULL;
	gn_switch_t* ext_sw = NULL;
	p_fabric_host_node fixed_port = NULL;
    p_fabric_host_node member_port = NULL;
    
	ext_port = find_openstack_external_by_floating_ip(fip->floating_ip);
	if (NULL == ext_port) {
		return 0;
	}

	ext_sw = find_sw_by_dpid(ext_port->external_dpid);
    fixed_port = get_fabric_host_from_list_by_ip(fip->fixed_ip);
    member_port = get_fabric_host_from_list_by_ip(member_ip);
    
    if ((ext_sw) && (fixed_port) && (member_port) && (member_port->sw)) {
        if (1 == type) {
            create_proactive_floating_with_lbaas_group_flows(ext_sw, fixed_port, ext_port, fip->floating_ip);
        }
        else if (2 == type) {
            UINT1 zero_mac[6] = {0};
            install_proactive_floating_lbaas_to_external_flow(member_port->sw, 2, member_port->ip_list[0] ,member_portno,
                    zero_mac, 0, 0, 0, zero_mac);
            create_proactive_floating_with_lbaas_group_flows(ext_sw, fixed_port, ext_port, fip->floating_ip);
        }
        else {
            // do nothing
        }        
    }

    return 0;
   
}

INT4 create_proactive_floating_lbaas_flow_by_member_ip(UINT4 pool_ip, UINT4 member_ip, UINT4 member_portno)
{
    return process_floating_lbaas_group_flow_by_member_ip(pool_ip, member_ip, member_portno, 1);
}

INT4 remove_proactive_floating_lbaas_flow_by_member_ip(UINT4 pool_ip, UINT4 member_ip, UINT4 member_portno)
{
    return process_floating_lbaas_group_flow_by_member_ip(pool_ip, member_ip, member_portno, 2);
}

INT4 create_proactive_floating_with_lbaas_group_flows(gn_switch_t* ext_sw, p_fabric_host_node fixed_port, 
                                    external_port_p ext_port, UINT4 floatingip)
{
    if ((NULL == fixed_port) || (NULL == ext_sw) || (NULL == ext_port)) {
        return 0;
    }
    
    group_bucket_t* all_bucket = NULL;
    UINT4 ext_vlan_id = of131_fabric_impl_get_tag_sw(ext_sw); 
     
    openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_ip(fixed_port->ip_list[0]);
    if (NULL == pool_p) {
        return 0;
    }
    
    if  (0 != pool_p->group_flow_installed) {
        return 0;
    }

    openstack_lbaas_node_p node_p = pool_p->pool_member_list;
    
    while (node_p) {
        openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)node_p->data;
        
        if ((member_p)&& (1 == member_p->status)) {
            p_fabric_host_node host_p = get_fabric_host_from_list_by_ip(member_p->fixed_ip);

            if ((host_p) && (host_p->sw) && (host_p->port)) {

                UINT4 host_vlan_id = of131_fabric_impl_get_tag_sw(host_p->sw);
                UINT4 host_tcp_dst = member_p->protocol_port;
                UINT4 between_portno = get_out_port_between_switch(ext_sw->dpid, host_p->sw->dpid);

                // nat_show_ip(member_p->fixed_ip);
                if ((host_vlan_id) && (host_tcp_dst) && (between_portno)) {
                    group_bucket_t* bucket = create_lbaas_group_bucket(between_portno, host_p->mac, host_p->ip_list[0], 
                        host_tcp_dst, host_vlan_id, member_p->weight);
                    
                    if (NULL == bucket) {
                        LOG_PROC("INFO", "Faill to create lbaas group bucket!");
                        return 0;
                    }
                    
                    bucket->next = all_bucket;
                    all_bucket = bucket;
    
                    if (0 == member_p->group_flow_installed) {
                        install_fabric_output_flow(host_p->sw, host_p->mac, host_p->port);
                        create_proactive_internal_subnet_flow_by_fixed_port(host_p);
                        install_proactive_floating_lbaas_to_external_flow(host_p->sw, 1, host_p->ip_list[0], host_tcp_dst, 
                                           ext_port->external_gateway_mac, ext_vlan_id, floatingip, pool_p->protocol_port, fixed_port->mac);  
                        install_add_fabric_controller_flow(host_p->sw);
                    }
    
                    member_p->group_flow_installed = 1;
                }
            }
        }
        node_p = node_p->next;
    }

    gn_group_t* group = (gn_group_t *)gn_malloc(sizeof(gn_group_t));
    group->buckets = all_bucket;

    group->type = OFPGT_SELECT;
    group->group_id = fixed_port->ip_list[0];
    group->next = NULL;

    if (all_bucket) {
        if (find_group_by_id(ext_sw, group->group_id)) {
            modify_group_entry(ext_sw, group);
        }
        else {
            // delete_group_entry(fixed_port->sw, &group);
            add_group_entry(ext_sw, group);
        }

        install_proactive_floating_external_to_lbaas_group_flow(ext_sw, 1, floatingip, pool_p->protocol_port, fixed_port->ip_list[0]);
        pool_p->group_flow_installed = 1;
        return 0;
    }
    else {
        if (find_group_by_id(ext_sw, group->group_id)) {
            delete_group_entry(ext_sw, group);
        }

        // process_proactive_internal_subnet_flow_by_fixed_port(fixed_port, 2);
        install_proactive_floating_external_to_lbaas_group_flow(ext_sw, 2, floatingip, pool_p->protocol_port, 0);
        pool_p->group_flow_installed = 0;
    }

    
    return 0;
}

INT4 remove_proactive_floating_with_lbaas_group_flows(gn_switch_t* ext_sw, p_fabric_host_node fixed_port, 
                                         external_port_p ext_port, UINT4 floatingip)
{   
    if (NULL == ext_sw) {
        return 0;
    }

    openstack_lbaas_pools_p pool_p = find_openstack_lbaas_pool_by_ip(fixed_port->ip_list[0]);
    if (NULL == pool_p) {
        return 0;
    }
    
    openstack_lbaas_node_p node_p = pool_p->pool_member_list;
    while (node_p) {
        openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)node_p->data;
        if (member_p) {
            p_fabric_host_node host_p = get_fabric_host_from_list_by_ip(member_p->fixed_ip);

            if ((host_p) && (host_p->sw) && (host_p->port)) {
                // nat_show_ip(member_p->fixed_ip);
                   
                UINT1 zero_mac[6] = {0};
                install_proactive_floating_lbaas_to_external_flow(host_p->sw, 2, host_p->ip_list[0], member_p->protocol_port,
                    zero_mac, 0, 0, 0, zero_mac);     

                member_p->group_flow_installed = 0;
            }
        }
        node_p = node_p->next;
    }
    
    gn_group_t* group = (gn_group_t *)gn_malloc(sizeof(gn_group_t));
    group->buckets = NULL;

    // group->buckets = bucket;
    group->type = OFPGT_SELECT;
    group->group_id = fixed_port->ip_list[0];
    group->next = NULL;

    if (find_group_by_id(ext_sw, group->group_id)) {
        delete_group_entry(ext_sw, group);
    }

    remove_proactive_internal_subnet_flow_by_fixed_port(fixed_port);
    install_proactive_floating_external_to_lbaas_group_flow(ext_sw, 2, floatingip, pool_p->protocol_port, 0);
    pool_p->group_flow_installed = 0;
    
    return 0;
}

INT4 create_proactive_floating_with_host_flows(p_fabric_host_node fixed_port, gn_switch_t* ext_sw, 
                                    external_port_p ext_port, UINT4 floatingip)
{
    if ((NULL == fixed_port) || (NULL == ext_sw) || (NULL == ext_port)) {
        return 0;
    }

    if ((NULL == fixed_port->sw) || (0 == fixed_port->port)) {
        p_fabric_host_node gateway_p = find_openstack_app_gateway_by_host(fixed_port);
				
		if (NULL != gateway_p) {
			fabric_opnestack_create_arp_flood(gateway_p->ip_list[0], fixed_port->ip_list[0], gateway_p->mac);
		}
		else {
			fabric_opnestack_create_arp_flood(g_reserve_ip, fixed_port->ip_list[0], g_reserve_mac);
		}
        
        return 0;
    }

    UINT4 fixed_vlan_id = of131_fabric_impl_get_tag_sw(fixed_port->sw);
    UINT4 ext_vlan_id = of131_fabric_impl_get_tag_sw(ext_sw);

    if ((0 == fixed_vlan_id) || (0 == ext_vlan_id)) {
       return 0;
    }

    UINT4 outport = get_out_port_between_switch(ext_port->external_dpid, fixed_port->sw->dpid);

    if (0 == outport) {
        return 0;
    }

    create_proactive_internal_subnet_flow_by_fixed_port(fixed_port);

    install_proactive_floating_host_to_external_flow(fixed_port->sw, 1, fixed_port->ip_list[0], fixed_port->mac, 
       floatingip, ext_port->external_gateway_mac, ext_vlan_id, NULL);

    install_add_fabric_controller_flow(fixed_port->sw);

    fabric_openstack_floating_ip_install_set_vlan_in_flow(ext_sw, floatingip, 
       fixed_port->ip_list[0], fixed_port->mac, fixed_vlan_id, outport);

    install_fabric_output_flow(fixed_port->sw, fixed_port->mac, fixed_port->port);

    return 1;
}

INT4 create_proactive_floating_flows_by_floating(external_floating_ip_p fip)
{   
    if (OFPCR_ROLE_SLAVE == g_controller_role) {
        return 0;
    }

	external_port_p ext_port = NULL;
	gn_switch_t* ext_sw = NULL;
	p_fabric_host_node fixed_port = NULL;
    UINT4 return_value = 0;
    
	ext_port = get_external_port_by_floatip(fip->floating_ip);
	if (NULL == ext_port) {
		return 0;
	}

	ext_sw = find_sw_by_dpid(ext_port->external_dpid);
    if (NULL == ext_sw) {
        return 0;
    }
    
	fixed_port = get_fabric_host_from_list_by_ip(fip->fixed_ip);
	if (NULL == fixed_port) {
        return 0;
    }

	if (OPENSTACK_PORT_TYPE_LOADBALANCER == fixed_port->type) {
        return_value = create_proactive_floating_with_lbaas_group_flows(ext_sw, fixed_port, ext_port, fip->floating_ip);
    }
    else if (OPENSTACK_PORT_TYPE_HOST == fixed_port->type) {
        return_value = create_proactive_floating_with_host_flows(fixed_port, ext_sw, ext_port, fip->floating_ip);
    }
    else {
        return 0;
    }

	return return_value;
}

INT4 remove_proactive_floating_flows_by_floating(external_floating_ip_p fip)
{   
    if (0 == g_proactive_flow_flag) {
        return 0;
    }

    if (OFPCR_ROLE_SLAVE == g_controller_role) {
        return 0;
    }
    
	external_port_p ext_port = NULL;
	gn_switch_t* ext_sw = NULL;
	p_fabric_host_node fixed_port = NULL;
    
	ext_port = find_openstack_external_by_floating_ip(fip->floating_ip);
    
	if (ext_port) {
		ext_sw = find_sw_by_dpid(ext_port->external_dpid);
        if (ext_sw) {
		    delete_fabric_input_flow_by_ip(ext_sw, fip->floating_ip);
        }
	}

	fixed_port = get_fabric_host_from_list_by_ip(fip->fixed_ip);
    
    if (NULL == fixed_port) {
        return 0;
    }
    
	if ((OPENSTACK_PORT_TYPE_HOST == fixed_port->type) && (fixed_port->sw)) {
		UINT1 zero_mac[6] = {0};
		install_proactive_floating_host_to_external_flow(fixed_port->sw, 2, fixed_port->ip_list[0], fixed_port->mac, 
				fip->floating_ip, zero_mac, 0, NULL);
	}
    else if (OPENSTACK_PORT_TYPE_LOADBALANCER == fixed_port->type) {
        remove_proactive_floating_lbaas_flow_by_pool_ip(fixed_port->ip_list[0]);
    }

	fip->flow_installed = 0;
	
	return 0;
}

void proactive_floating_check_tx_timer(void *para, void *tid)
{
	// if openstack on
	if (0 == g_openstack_on) {
		// return
		return;
	}

   if (OFPCR_ROLE_SLAVE == g_controller_role) {
        return ;
   }

	external_floating_ip_p efp = NULL;
	openstack_external_node_p node_p = g_openstack_floating_list;
    
	while (NULL != node_p) {
		efp = (external_floating_ip_p)node_p->data;
		
		if ((efp->fixed_ip) && (efp->floating_ip) && (strlen(efp->port_id)) && (strlen(efp->router_id)) && (0 == efp->flow_installed)) {
			efp->flow_installed = create_proactive_floating_flows_by_floating(efp);
		}
		node_p = node_p->next;
	}
}

void init_proactive_floating_check_mgr()
{
    INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	UINT4 openstack_flag = (NULL == value) ? 0: atoi(value);
	value = get_value(g_controller_configure, "[openvstack_conf]", "proactive_flow_on");
	g_proactive_flow_flag = (NULL == value) ? 0: atoi(value);

    if ((0 == openstack_flag) || (0 == g_proactive_flow_flag)) {
        return ;
    }

	proactive_floating_check_tx_timer(NULL, NULL);
	
	// set the timer
	g_floating_check_timerid = timer_init(1);
	timer_creat(g_floating_check_timerid, g_floating_check_interval, NULL, &g_floating_check_timer, proactive_floating_check_tx_timer);
}
