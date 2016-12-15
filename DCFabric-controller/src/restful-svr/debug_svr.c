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
 *  debug_svr.c
 *
 *  Created on: Feb 29, 2016
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */
#include "debug_svr.h"
#include "../ovsdb/ovsdb.h"
#include "openstack_host.h"
#include "fabric_impl.h"
#include "openstack_lbaas_app.h"
#include "openstack_host.h"
#include "openstack-server.h"
#include "fabric_openstack_external.h"
#include "fabric_host.h"
#include "fabric_openstack_nat.h"
#include "../qos-mgr/qos-mgr.h"
#include "../qos-mgr/qos-meter.h"
#include "../qos-mgr/qos-policy.h"



extern t_fabric_sw_list g_fabric_sw_list_total;
extern t_fabric_sw_list g_fabric_sw_list;
extern t_fabric_host_list g_fabric_host_list;
extern p_fabric_path_list g_fabric_path_list;
extern openstack_lbaas_node_p g_openstack_lbaas_pools_list;
extern openstack_lbaas_node_p g_openstack_lbaas_members_list;
extern openstack_security_p g_openstack_security_list;
extern openstack_lbaas_node_p g_openstack_lbaas_listener_list;
extern openstack_external_node_p g_openstack_floating_list;
extern t_fabric_arp_request_list g_arp_request_list;
extern t_fabric_arp_flood_queue g_arp_flood_queue;
extern openstack_node_p g_openstack_host_network_list;
extern openstack_node_p g_openstack_host_subnet_list;
extern qos_policy_p g_qos_policy_list;

extern UINT4 g_openstack_on;
extern UINT4 g_external_check_on;
extern UINT4 g_external_check_interval;

INT1 *fabric_debug_get_all_host()
{
	/*
	* example:
	* {"host list":[{"SwIP":"192.168.1.1","IPv4":"200.0.0.1","Mac":"aa:bb:cc:dd:ee:ff"},
	* 			  {"SwIP":"192.168.1.2","IPv4":"200.0.0.2","Mac":"ff:ee:dd:cc:bb:aa"}]}
	*/
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();
	
	key = json_new_string("host list");
   	json_insert_child(key, array);
   	json_insert_child(obj, key);

	p_fabric_host_node head = NULL;
	head = g_fabric_host_list.list;

    INT1 str_temp[48] = {0};
	while (head) {
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
			if (head->sw) {
				key = json_new_string("DPID");
				bzero(str_temp, 48);
				dpidUint8ToStr(head->sw->dpid, str_temp);
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

			json_insert_child(array, entry);
		}
		head = head->next;
	}

   return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_get_all_sw()
{
	/*
	* example:
	* {"sw list":[{"IP":"192.168.1.1","VlanId":"4"},
	* 			{"IP":"192.168.1.2","VlanId":"5"}]}
	*/
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("sw list");
	json_insert_child(key, array);
    json_insert_child(obj, key);

	p_fabric_sw_list list = NULL;

	list = &g_fabric_sw_list;
	// printf("****\n sw list\n");
	if (list) {
		p_fabric_sw_node sw_node = list->node_list;
		while (sw_node) {
			entry = json_new_object();
			INT1 str_temp[48] = {0};
			
			gn_switch_t* cur_sw = sw_node->sw;
			if (cur_sw) {
				// nat_show_ip(cur_sw->sw_ip);
				key = json_new_string("IP");
				number2ip(cur_sw->sw_ip, str_temp);
				value = json_new_string(str_temp);
				json_insert_child(key, value);
	            json_insert_child(entry, key);
			}
			
			// printf("tag is : %d\t", sw_node->tag);
			key = json_new_string("VlanId");
			sprintf(str_temp, "%d", sw_node->tag);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			json_insert_child(array, entry);
			sw_node = sw_node->next;
		}
	}

   /*
   list = &g_fabric_sw_list_total;
   // printf("****\n total list\n");
   if (list) {
	   p_fabric_sw_node sw_node = list->node_list;
	   while (sw_node) {
		   entry = json_new_object();
		   INT1 str_ip[20] = {0};
		   INT1 str_vlan_id[20] = {0};
		
		   gn_switch_t* cur_sw = sw_node->sw;
		   if (cur_sw) {
			   // nat_show_ip(cur_sw->sw_ip);
			    key = json_new_string("Total Sw IP");
				number2ip(cur_sw->sw_ip, str_ip);
				value = json_new_string(str_ip);
				json_insert_child(key, value);
	            json_insert_child(entry, key);
		   }
		   // printf("tag is : %d\t", sw_node->tag);
		   key = json_new_string("Total Sw VlanId");
		   sprintf(str_vlan_id, "%d", sw_node->tag);
		   value = json_new_string(str_vlan_id);
		   json_insert_child(key, value);
		   json_insert_child(entry, key);

		   json_insert_child(array, entry);		   
		   sw_node = sw_node->next;
	   }
   }

   key = json_new_string("Total sw list");
   json_insert_child(key, array);
   json_insert_child(obj, key); 
   */
   
   return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_get_all_path()
{
	/*
	* example:
	* {{"path_list":[{"dst_sw":"ip:192.168.1.1, dpid: 12345678, num: 1","path":
	* [{"src_sw":"src ip:192.168.1.2, dpid: 87654321","dst_sw":"dst ip:192.168.1.1, dpid: 12345678",
	* "node_list":[{"node":"ip:192.168.1.2, dpid:87654321, port:1"},{"node":"ip:192.168.1.3, dpid:99998888, port:1"},
	* {"node":"ip:192.168.1.1, dpid:12345678, port:0"}]}}
	*/
	json_t *obj, *src_array, *src_value, *dst_array, *dst_value, *node_value, *node_array, *key, *value;
	obj = json_new_object();
	dst_array = json_new_array();
	key = json_new_string("path_list");
	json_insert_child(key, dst_array);
	json_insert_child(obj, key);
	INT1 json_temp[1024] = {0};
	INT1 str_ip[48] = {0};
	
	p_fabric_path_list list = g_fabric_path_list;
	while (list) {
		// printf("show all path--dpid: %llu, num: %d\n", list->sw->dpid, list->num);
		dst_value = json_new_object();
		json_insert_child(dst_array, dst_value);

		key = json_new_string("dst_sw");
		bzero(json_temp, 1024);
		bzero(str_ip, 48);
		number2ip(list->sw->sw_ip, str_ip);
		sprintf(json_temp, "ip:%s, dpid: %llu, num: %d", str_ip, list->sw->dpid, list->num);
		value = json_new_string(json_temp);
		json_insert_child(key, value);
		json_insert_child(dst_value, key);

		src_array = json_new_array();
		key = json_new_string("path");
		json_insert_child(key, src_array);
		json_insert_child(dst_value, key);
		
		p_fabric_path path = list->path_list;
		while (path) {
			// printf("\t--path: src: %llu, dst: %llu\n", path->src->dpid, path->dst->dpid);
			// key = json_new_string("src dpid");
			src_value = json_new_object();
			json_insert_child(src_array, src_value);

			key = json_new_string("src_sw");
			bzero(json_temp, 1024);
			bzero(str_ip, 48);
			number2ip(path->src->sw_ip, str_ip);
			sprintf(json_temp, "src ip:%s, dpid: %llu", str_ip, path->src->dpid);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
			json_insert_child(src_value, key);

			key = json_new_string("dst_sw");
			bzero(json_temp, 1024);
			bzero(str_ip, 48);
			number2ip(path->dst->sw_ip, str_ip);
			sprintf(json_temp, "dst ip:%s, dpid: %llu", str_ip, path->dst->dpid);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
			json_insert_child(src_value, key);

			node_array = json_new_array();
			key = json_new_string("node_list");
			json_insert_child(key, node_array);
			json_insert_child(src_value, key);
			
			p_fabric_path_node node = path->node_list;
			while (node) {
				INT1 json_temp[1024] = {0};
				UINT4 port_no = (NULL != node->port) ? node->port->port_no: 0;
				// printf("\t--node: dpid:%llu, port:%d", node->sw->dpid, port_no);//, node->port->port_no);
				node_value = json_new_object();
				json_insert_child(node_array, node_value);

				key = json_new_string("node");
				bzero(str_ip, 48);
				number2ip(node->sw->sw_ip, str_ip);
				sprintf(json_temp, "ip:%s, dpid:%llu, port:%d", str_ip, node->sw->dpid, port_no);//, node->port->port_no);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(node_value, key);
				
				node = node->next;
			}
			
			// printf("\n");
			path = path->next;
		}
		// printf("\n");
		list = list->next;
	}
   return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_get_all_securitygroup()
{
	json_t *obj, *array, *key, *value, *entry, *member_array, *member_value;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("security groups");
    json_insert_child(key, array);
    json_insert_child(obj, key);
	
	openstack_security_p temp_p = g_openstack_security_list;
	while (NULL != temp_p) {
		entry = json_new_object();

		key = json_new_string("group id");
		value = json_new_string(temp_p->security_group);
		json_insert_child(key, value);
        json_insert_child(entry, key);
		
		member_array = json_new_array();
		key = json_new_string("rule");
		json_insert_child(key, member_array);
		json_insert_child(entry, key);
		
		// printf("security group id:%s\n", temp_p->security_group);
		openstack_security_rule_p rule_p = temp_p->security_rule_p;
		while (NULL != rule_p) {
			// openstack_show_security_rule(rule_p);
			member_value = json_new_object();
							
			key = json_new_string("rule id");
			value = json_new_string(rule_p->rule_id);
			json_insert_child(key, value);
			json_insert_child(member_value, key);

			json_insert_child(member_array, member_value);
			rule_p = rule_p->next;
		}
		json_insert_child(array, entry);
		
		temp_p = temp_p->next;
	}

	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_get_all_hostsecurity()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("host security");
	json_insert_child(key, array);
	json_insert_child(obj, key);
	INT1 json_temp[96] = {0};

	p_fabric_host_node list = g_fabric_host_list.list;
	while (NULL != list) {
		openstack_port_p port_p = (openstack_port_p)list->data;
		if (NULL != port_p) {
			openstack_node_p node_p = (openstack_node_p)port_p->security_data;
			while (NULL != node_p) {
				openstack_security_p security_p = (openstack_security_p)node_p->data;
				if (NULL != security_p) {
					openstack_security_rule_p rule_p = (openstack_security_rule_p)security_p->security_rule_p;
					while (NULL != rule_p) {
						// openstack_show_security_rule(rule_p);
						entry = json_new_object();

						key = json_new_string("ip");
						bzero(json_temp, 96);
						number2ip(list->ip_list[0], json_temp);
						value = json_new_string(json_temp);
						json_insert_child(key, value);
						json_insert_child(entry, key);

						key = json_new_string("rule id");
						value = json_new_string(rule_p->rule_id);
						json_insert_child(key, value);
						json_insert_child(entry, key);

						json_insert_child(array, entry);
						
						rule_p = rule_p->next;
					}
					// security_p = security_p->next;
				}
				node_p = node_p->next;
			}
		}
		list = list->next;
	}

	return json_to_reply(obj, GN_OK);	
}

INT1 *fabric_debug_clear_all_security(const INT1 *url, json_t *root)
{
	clear_all_security_group_info();

	json_t *obj, *key, *value;
	obj = json_new_object();

	key = json_new_string("clear");
	value = json_new_string("OK");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_reload_all_security(const INT1 *url, json_t *root)
{
	clear_all_security_group_info();
	reload_security_group_info();

	json_t *obj, *key, *value;
	obj = json_new_object();

	key = json_new_string("reload");
	// value = (GN_OK == reload_vlalue) ? json_new_string("OK") : json_new_string("Error");
	value = json_new_string("OK");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_clear_all_loadbalance(const INT1 *url, json_t *root)
{
	clear_openstack_lbaas_info();

	json_t *obj, *key, *value;
	obj = json_new_object();

	key = json_new_string("clear");
	// value = (GN_OK == reload_vlalue) ? json_new_string("OK") : json_new_string("Error");
	value = json_new_string("OK");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_reload_all_loadbalance(const INT1 *url, json_t *root)
{
	reload_openstack_lbaas_info();

	json_t *obj, *key, *value;
	obj = json_new_object();

	key = json_new_string("clear");
	// value = (GN_OK == reload_vlalue) ? json_new_string("OK") : json_new_string("Error");
	value = json_new_string("OK");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

INT1* fabric_debug_get_all_loadbalance_pool()
{
	/*
	* example:
	* {"pool_list":[{"pool_id":"abcde-12345-addxc-123456",
	* "pool_ip":"100.0.0.1","member":[{"member_id":"abcde-ssssss-11111","member_ip":"100.0.0.2","connect":"10"}]}}
	*/
	json_t *obj, *array, *key, *value, *entry, *member_array, *member_value;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("pool_list");
    json_insert_child(key, array);
    json_insert_child(obj, key);
		
	openstack_lbaas_node_p list_p = g_openstack_lbaas_pools_list;
	while (list_p) {		
		openstack_lbaas_pools_p pool_p = (openstack_lbaas_pools_p)list_p->data;
		if (pool_p) {
			// printf("******** member in pool *******\n");
			INT1 json_temp[48] = {0};
			entry = json_new_object();

			key = json_new_string("pool_id");
			value = json_new_string(pool_p->pools_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("pool_ip");
			bzero(json_temp, 48);
			number2ip(pool_p->ipaddress, json_temp);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

            key = json_new_string("flow installed");
            bzero(json_temp, 48);
			sprintf(json_temp, "%d", pool_p->group_flow_installed);
            value = json_new_string(json_temp);
			json_insert_child(key, value);
	       	json_insert_child(entry, key);

            key = json_new_string("status");
            bzero(json_temp, 48);
			sprintf(json_temp, "%d", pool_p->status);
            value = json_new_string(json_temp);
			json_insert_child(key, value);
	       	json_insert_child(entry, key);

			/*
			key = json_new_string("pool_flag");
			value = (GN_OK == pool_p->update_flag) ? json_new_string("Checked") : json_new_string("Unchecked");
			json_insert_child(key, value);
	        	son_insert_child(entry, key);
			*/
			
			member_array = json_new_array();
			key = json_new_string("member");
			json_insert_child(key, member_array);
			json_insert_child(entry, key);
			
			openstack_lbaas_node_p member_list_p = pool_p->pool_member_list;
			while (member_list_p) {
				openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)member_list_p->data;
				// printf("member id:%s weight:%d\n", member_p->member_id, member_p->weight);				
				member_value = json_new_object();
				
				key = json_new_string("member_id");
				value = json_new_string(member_p->member_id);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

				key = json_new_string("member_ip");
				bzero(json_temp, 48);
				number2ip(member_p->fixed_ip, json_temp);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

				/*
				key = json_new_string("member_flag");
				value = (GN_OK == member_p->update_flag) ? json_new_string("Checked") : json_new_string("Unchecked");
				json_insert_child(key, value);
				json_insert_child(member_value, key);
				*/

				key = json_new_string("connect");
				bzero(json_temp, 48);
				sprintf(json_temp, "%d", member_p->connect_numbers);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

                key = json_new_string("flow installed");
                bzero(json_temp, 48);
    			sprintf(json_temp, "%d", member_p->group_flow_installed);
                value = json_new_string(json_temp);
    			json_insert_child(key, value);
    	       	json_insert_child(member_value, key);

                key = json_new_string("status");
                value = (1 == member_p->status) ? json_new_string("Active") : json_new_string("Inactive");
    			json_insert_child(key, value);
    	       	json_insert_child(member_value, key);
				
				json_insert_child(member_array, member_value);

				member_list_p = member_list_p->next;
			}
			
			
			json_insert_child(array, entry);
		}
		list_p = list_p->next;
	}
   return json_to_reply(obj, GN_OK);

}


INT1* fabric_debug_get_all_loadbalance_member()
{
	/*
	* example:
	* {"pool_list":[{"pool_id":"abcde-12345-addxc-123456",
	* "pool_ip":"100.0.0.1","member":[{"member_id":"abcde-ssssss-11111","member_ip":"100.0.0.2","connect":"10"}]}}
	*/
	json_t *obj, *array, *key, *value, *entry, *member_array, *member_value;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("member_list");
    json_insert_child(key, array);
    json_insert_child(obj, key);
		
	openstack_lbaas_node_p list_p = g_openstack_lbaas_members_list;
	while (list_p) {		
		openstack_lbaas_members_p member_p = (openstack_lbaas_members_p)list_p->data;
		if (member_p) {
			// printf("******** member in pool *******\n");
			INT1 json_temp[48] = {0};
			entry = json_new_object();

			key = json_new_string("member_id");
			value = json_new_string(member_p->member_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("member_ip");
			bzero(json_temp, 48);
			number2ip(member_p->fixed_ip, json_temp);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

            key = json_new_string("status");
			value = (1 == member_p->status) ? json_new_string("Active") : json_new_string("Inactive");
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			member_array = json_new_array();
			key = json_new_string("ips");
			json_insert_child(key, member_array);
			json_insert_child(entry, key);
			
			openstack_lbaas_node_p connect_ip_p = member_p->connect_ips;
			while (connect_ip_p) {
				openstack_lbaas_connect_p connect_p = (openstack_lbaas_connect_p)connect_ip_p->data;
				// printf("member id:%s weight:%d\n", member_p->member_id, member_p->weight);				
				member_value = json_new_object();

				key = json_new_string("ext_ip");
				bzero(json_temp, 48);
				number2ip(connect_p->ext_ip, json_temp);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

				key = json_new_string("inside_ip");
				bzero(json_temp, 48);
				number2ip(connect_p->inside_ip, json_temp);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);
				
				json_insert_child(member_array, member_value);

				connect_ip_p = connect_ip_p->next;
			}

            key = json_new_string("flow installed");
            bzero(json_temp, 48);
		    sprintf(json_temp, "%d", member_p->group_flow_installed);
            value = json_new_string(json_temp);
			json_insert_child(key, value);
	       	json_insert_child(entry, key);
			
			json_insert_child(array, entry);
		}
		list_p = list_p->next;
	}
   return json_to_reply(obj, GN_OK);

}

INT1* fabric_debug_get_all_loadbalance_listener()
{
	/*
	* example:
	* {"pool_list":[{"pool_id":"abcde-12345-addxc-123456",
	* "pool_ip":"100.0.0.1","member":[{"member_id":"abcde-ssssss-11111","member_ip":"100.0.0.2","connect":"10"}]}}
	*/
	json_t *obj, *array, *key, *value, *entry, *member_array, *member_value;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("listener_list");
    json_insert_child(key, array);
    json_insert_child(obj, key);
		
	openstack_lbaas_node_p list_p = g_openstack_lbaas_listener_list;
	while (list_p) {		
		openstack_lbaas_listener_p listener_p = (openstack_lbaas_listener_p)list_p->data;
		if (listener_p) {
			// printf("******** member in pool *******\n");
			INT1 json_temp[48] = {0};
			entry = json_new_object();

			key = json_new_string("listener_id");
			value = json_new_string(listener_p->listener_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("listener_type");
			if (LBAAS_LISTENER_PING == listener_p->type) 
                value = json_new_string("PING");
            else if (LBAAS_LISTENER_TCP== listener_p->type) 
                value = json_new_string("TCP");
            else if (LBAAS_LISTENER_HTTP == listener_p->type) 
                value = json_new_string("HTTP");
            else if (LBAAS_LISTENER_HTTPS == listener_p->type) 
                value =json_new_string("HTTPS");
            else 
                value = json_new_string("UNKNOWN");
			json_insert_child(key, value);
	        json_insert_child(entry, key);

            key = json_new_string("check frequency");
			bzero(json_temp, 48);
			sprintf(json_temp, "%d second", listener_p->check_frequency);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
			json_insert_child(entry, key);

            key = json_new_string("overtime");
			bzero(json_temp, 48);
			sprintf(json_temp, "%d second", listener_p->overtime);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
			json_insert_child(entry, key);


            key = json_new_string("retry");
			bzero(json_temp, 48);
			sprintf(json_temp, "%d times", listener_p->retries);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
			json_insert_child(entry, key);

            key = json_new_string("check status");
			bzero(json_temp, 48);
			sprintf(json_temp, "%d times", listener_p->check_status);
			value = json_new_string(json_temp);
			json_insert_child(key, value);
			json_insert_child(entry, key);

			member_array = json_new_array();
			key = json_new_string("member");
			json_insert_child(key, member_array);
			json_insert_child(entry, key);
			
			openstack_lbaas_listener_member_p listener_member_p = listener_p->listener_member_list;
			while (listener_member_p) {			
				member_value = json_new_object();

                // printf("memberid:%s\n", listener_member_p->member_id);
                key = json_new_string("member id");
				value = json_new_string(listener_member_p->member_id);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

				key = json_new_string("member_ip");
				bzero(json_temp, 48);
				number2ip(listener_member_p->dst_ip, json_temp);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

                key = json_new_string("init time");
				bzero(json_temp, 48);
				sprintf(json_temp, "%d", listener_member_p->init_time);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

                key = json_new_string("request");
				bzero(json_temp, 48);
				sprintf(json_temp, "%d times", listener_member_p->request_time);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

				key = json_new_string("seq id");
				bzero(json_temp, 48);
				sprintf(json_temp, "%d", listener_member_p->seq_id);
                value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

				key = json_new_string("wait status");
				value = (0 == listener_member_p->wait_status) ? json_new_string("Receive responce"):json_new_string("Wait for responce");
				json_insert_child(key, value);
				json_insert_child(member_value, key);

                if (LBAAS_LISTENER_PING == listener_p->type) {
                   key = json_new_string("ping status");
                   value = (1 == listener_member_p->member_p->ping_status) ? json_new_string("Active"):json_new_string("Inactive");
                   json_insert_child(key, value);
                   json_insert_child(member_value, key);

                }
                else {
                    key = json_new_string("status");
    				value = (1 == listener_member_p->member_p->status) ? json_new_string("Active"):json_new_string("Inactive");
    				json_insert_child(key, value);
    				json_insert_child(member_value, key);
                }
				               
				json_insert_child(member_array, member_value);

				listener_member_p = listener_member_p->next;
			}
			
			
			json_insert_child(array, entry);
		}
		list_p = list_p->next;
	}
   return json_to_reply(obj, GN_OK);

}

INT1 *fabric_debug_get_all_floatingip()
{
	/*
	* example:
	* {"sw list":[{"IP":"192.168.1.1","VlanId":"4"},
	* 			{"IP":"192.168.1.2","VlanId":"5"}]}
	*/
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("floating ip list");
	json_insert_child(key, array);
    json_insert_child(obj, key);

	openstack_external_node_p node_p = g_openstack_floating_list;

	while (node_p) {
		external_floating_ip_p epp = (external_floating_ip_p)node_p->data;
		entry = json_new_object();
		INT1 str_temp[48] = {0};

		if (epp) {
			key = json_new_string("floating ip");
			bzero(str_temp, 48);
			number2ip(epp->floating_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("fixed ip");
			bzero(str_temp, 48);
			number2ip(epp->fixed_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("port id");
			value = json_new_string(epp->port_id);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("router id");
			value = json_new_string(epp->router_id);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("check flag");
			bzero(str_temp, 48);
			sprintf(str_temp, "%d", epp->check_status);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

            key = json_new_string("flow installed");
			bzero(str_temp, 48);
			sprintf(str_temp, "%d", epp->flow_installed);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);
		}

		json_insert_child(array, entry);
		node_p = node_p->next;
	}
   return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_get_all_arp_request()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();
	INT1 str_temp[48] = {0};

	p_fabric_arp_request_node node_p = g_arp_request_list.list;
	sprintf(str_temp, "arp request list: %d", g_arp_request_list.list_num);
	key = json_new_string(str_temp);
	json_insert_child(key, array);
    json_insert_child(obj, key);

	while (node_p) {
		bzero(str_temp, 48);		
		entry = json_new_object();
		
		key = json_new_string("src ip");
		bzero(str_temp, 48);
		number2ip(node_p->src_IP, str_temp);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
		json_insert_child(entry, key);

		key = json_new_string("dst ip");
		bzero(str_temp, 48);
		number2ip(node_p->dst_IP, str_temp);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
		json_insert_child(entry, key);

		json_insert_child(array, entry);
		node_p = node_p->next;
	}
   return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_get_all_arp_flood()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();
	INT1 str_temp[48] = {0};

	p_fabric_arp_flood_node node_p =  g_arp_flood_queue.head;
	sprintf(str_temp, "arp flood list: %d", g_arp_flood_queue.queue_num);
	key = json_new_string(str_temp);
	json_insert_child(key, array);
	json_insert_child(obj, key);

	while (node_p) {
		bzero(str_temp, 48);		
		entry = json_new_object();
		
		key = json_new_string("dst ip");
		bzero(str_temp, 48);
		number2ip(node_p->ip, str_temp);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
		json_insert_child(entry, key);

		json_insert_child(array, entry);
		node_p = node_p->next;
	}
   return json_to_reply(obj, GN_OK);
}


INT1 *fabric_debug_get_all_external_config()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("external config list");
	json_insert_child(key, array);
    json_insert_child(obj, key);

	openstack_external_node_p node_p = g_openstack_external_list;

	while (node_p) {
		external_port_p epp = (external_port_p)node_p->data;
		entry = json_new_object();
		INT1 str_temp[48] = {0};

		if (epp) {

            key = json_new_string("external_dpid");
            bzero(str_temp, 48);
            dpidUint8ToStr(epp->external_dpid, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

            key = json_new_string("external_port");
            bzero(str_temp, 48);
            sprintf(str_temp, "%u", epp->external_port);
            value = json_new_string(str_temp);
            json_insert_child(key, value);
            json_insert_child(entry, key);
            
			key = json_new_string("network_id");
			value = json_new_string(epp->network_id);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("external_gateway_ip");
			bzero(str_temp, 48);
			number2ip(epp->external_gateway_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("external_gateway_mac");
            bzero(str_temp, 48);
            mac2str(epp->external_gateway_mac, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("external_out_interface_ip");
			bzero(str_temp, 48);
			number2ip(epp->external_outer_interface_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("external_out_interface_mac");
            bzero(str_temp, 48);
            mac2str(epp->external_outer_interface_mac, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

            key = json_new_string("id");
            bzero(str_temp, 48);
            sprintf(str_temp, "%u", epp->ID);
            value = json_new_string(str_temp);
            json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("status");
            value = (GN_OK == epp->status) ? json_new_string("Active") : json_new_string("Inactive");
            json_insert_child(key, value);
            json_insert_child(entry, key);

		}

		json_insert_child(array, entry);
		node_p = node_p->next;
	}
   return json_to_reply(obj, GN_OK);
}




INT1 *fabric_debug_get_all_nat_icmp_iden()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("nat icmp iden list");
	json_insert_child(key, array);
    json_insert_child(obj, key);

	openstack_external_node_p node_p = g_nat_icmp_iden_list;

	while (node_p) {
		nat_icmp_iden_p epp = (nat_icmp_iden_p)node_p->data;
		entry = json_new_object();
		INT1 str_temp[48] = {0};

		if (epp) {

            key = json_new_string("identifier");
            bzero(str_temp, 48);
            sprintf(str_temp, "%d", epp->identifier);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("host_ip");
			bzero(str_temp, 48);
			number2ip(epp->host_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("host_mac");
            bzero(str_temp, 48);
            mac2str(epp->host_mac, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

            key = json_new_string("sw_dpid");
            bzero(str_temp, 48);
            dpidUint8ToStr(epp->sw_dpid, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

            key = json_new_string("inport");
            bzero(str_temp, 48);
            sprintf(str_temp, "%u", epp->inport);
            value = json_new_string(str_temp);
            json_insert_child(key, value);
            json_insert_child(entry, key);

		}

		json_insert_child(array, entry);
		node_p = node_p->next;
	}
   return json_to_reply(obj, GN_OK);
}


INT1 *fabric_debug_get_all_nat_host()
{
	json_t *obj, *array, *key, *value, *entry, *tcp, *tcparray;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("nat icmp iden list");
	json_insert_child(key, array);
    json_insert_child(obj, key);

	nat_host_p node_p = g_nat_host_list_head;

	while (node_p) {
        entry = json_new_object();
		INT1 str_temp[48] = {0};
        
    	key = json_new_string("host_ip");
		bzero(str_temp, 48);
		number2ip(node_p->host_ip, str_temp);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
        json_insert_child(entry, key);

        nat_port_p tcp_node = node_p->tcp_port_list;
        if (tcp_node)
        {
            tcparray = json_new_array();
        	key = json_new_string("tcp port list");
        	json_insert_child(key, tcparray);
            json_insert_child(entry, key);
            
            while (tcp_node)
            {
                tcp = json_new_object();
                
                key = json_new_string("external_port_no");
                bzero(str_temp, 48);
                sprintf(str_temp, "%d", tcp_node->external_port_no);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);
            
                key = json_new_string("internal_ip");
                bzero(str_temp, 48);
                number2ip(tcp_node->internal_ip, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);
            
                key = json_new_string("internal_mac");
                bzero(str_temp, 48);
                mac2str(tcp_node->internal_mac, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);

                key = json_new_string("internal_port_no");
                bzero(str_temp, 48);
                sprintf(str_temp, "%d", tcp_node->internal_port_no);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);
            
                key = json_new_string("gateway_dpid");
                bzero(str_temp, 48);
                dpidUint8ToStr(tcp_node->gateway_dpid, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);

                key = json_new_string("src_dpid");
                bzero(str_temp, 48);
                dpidUint8ToStr(tcp_node->src_dpid, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);

                json_insert_child(tcparray, tcp);
                tcp_node = tcp_node->next;
            }

        }
        
        nat_port_p udp_node = node_p->udp_port_list;
        if (udp_node)
        {
            tcparray = json_new_array();
        	key = json_new_string("udp port list");
        	json_insert_child(key, tcparray);
            json_insert_child(entry, key);
            
            while (udp_node)
            {
                tcp = json_new_object();
                
                key = json_new_string("external_port_no");
                bzero(str_temp, 48);
                sprintf(str_temp, "%d", udp_node->external_port_no);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);
            
                key = json_new_string("internal_ip");
                bzero(str_temp, 48);
                number2ip(udp_node->internal_ip, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);
            
                key = json_new_string("internal_mac");
                bzero(str_temp, 48);
                mac2str(udp_node->internal_mac, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);

                key = json_new_string("internal_port_no");
                bzero(str_temp, 48);
                sprintf(str_temp, "%d", udp_node->internal_port_no);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);
            
                key = json_new_string("gateway_dpid");
                bzero(str_temp, 48);
                dpidUint8ToStr(udp_node->gateway_dpid, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);

                key = json_new_string("src_dpid");
                bzero(str_temp, 48);
                dpidUint8ToStr(udp_node->src_dpid, str_temp);
                value = json_new_string(str_temp);
                json_insert_child(key, value);
                json_insert_child(tcp, key);

                json_insert_child(tcparray, tcp);
                udp_node = udp_node->next;
            }

        }

		json_insert_child(array, entry);
		node_p = node_p->next;
	}
    
    return json_to_reply(obj, GN_OK);
}

INT1* fabric_debug_get_all_network()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("network list");
    json_insert_child(key, array);
    json_insert_child(obj, key);
	INT1 str_temp[24] = {0};
	
	openstack_node_p node_p = g_openstack_host_network_list;
	while (NULL != node_p) {
		openstack_network_p network_p = (openstack_network_p)(node_p->data);
		if (network_p) {
			entry = json_new_object();

			key = json_new_string("network id");
			value = json_new_string(network_p->network_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("tenant id");
			value = json_new_string(network_p->tenant_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("shared");
			bzero(str_temp, 24);
			sprintf(str_temp, "%d", network_p->shared);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("subnet num");
			bzero(str_temp, 24);
			sprintf(str_temp, "%d", network_p->subnet_num);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("check flag");
			bzero(str_temp, 24);
			sprintf(str_temp, "%d", network_p->check_status);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);
			
			json_insert_child(array, entry);
		}		
		node_p = node_p->next;
	}

	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_get_exteral_check()
{
	json_t *obj, *key, *value;
	obj = json_new_object();
	INT1 str_tmp[24] = {0};

	key = json_new_string("external check status");
	value = (0 == g_external_check_on) ? json_new_string("OFF") : json_new_string("ON");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	sprintf(str_tmp, "%d", g_external_check_interval);
	key = json_new_string("external check internal");
	value = json_new_string(str_tmp);
	json_insert_child(key, value);
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_start_exteral_check(const INT1 *url, json_t *root)
{
    json_t *item = NULL;
	UINT4 interval = 0;
	UINT4 old_check_status = g_external_check_on;
	UINT4 old_check_interval = g_external_check_interval;

    item = json_find_first_label(root, "interval");
    if (item)
    {
        interval = atoi(item->child->text);
        json_free_value(&item);
    }

	json_t *obj, *key, *value;
	obj = json_new_object();

	stop_external_mac_check();
	start_external_mac_check(1, interval);
	
	INT1 str_tmp[24] = {0};

	key = json_new_string("old external check status");
	value = (0 == old_check_status) ? json_new_string("OFF") : json_new_string("ON");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	sprintf(str_tmp, "%d", old_check_interval);
	key = json_new_string("old external check internal");
	value = json_new_string(str_tmp);
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("new external check status");
	value = (0 == g_external_check_on) ? json_new_string("OFF") : json_new_string("ON");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	bzero(str_tmp, 24);
	sprintf(str_tmp, "%d", g_external_check_interval);
	key = json_new_string("new external check internal");
	value = json_new_string(str_tmp);
	json_insert_child(key, value);
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_stop_exteral_check()
{
	json_t *obj, *key, *value;
	obj = json_new_object();
	UINT4 old_check_status = g_external_check_on;
	UINT4 old_check_interval = g_external_check_interval;
	
	stop_external_mac_check();
	
	INT1 str_tmp[24] = {0};

	key = json_new_string("old external check status");
	value = (0 == old_check_status) ? json_new_string("OFF") : json_new_string("ON");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	sprintf(str_tmp, "%d", old_check_interval);
	key = json_new_string("old external check internal");
	value = json_new_string(str_tmp);
	json_insert_child(key, value);
	json_insert_child(obj, key);

	key = json_new_string("new external check status");
	value = (0 == g_external_check_on) ? json_new_string("OFF") : json_new_string("ON");
	json_insert_child(key, value);
	json_insert_child(obj, key);

	bzero(str_tmp, 24);
	sprintf(str_tmp, "%d", g_external_check_interval);
	key = json_new_string("new external check internal");
	value = json_new_string(str_tmp);
	json_insert_child(key, value);
	json_insert_child(obj, key);

	return json_to_reply(obj, GN_OK);
}

INT1* fabric_debug_get_all_subnet()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("subnet list");
    json_insert_child(key, array);
    json_insert_child(obj, key);
	INT1 str_temp[48] = {0};
	
	openstack_node_p node_p = g_openstack_host_subnet_list;
	while (NULL != node_p) {
		openstack_subnet_p subnet_p = (openstack_subnet_p)(node_p->data);
		if (subnet_p) {
			entry = json_new_object();

			key = json_new_string("network id");
			value = json_new_string(subnet_p->network_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);
			
			key = json_new_string("subnet id");
			value = json_new_string(subnet_p->subnet_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("tenant id");
			value = json_new_string(subnet_p->tenant_id);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("gateway ip");
			bzero(str_temp, 48);
			number2ip(subnet_p->gateway_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
			json_insert_child(entry, key);

			key = json_new_string("start ip");
			bzero(str_temp, 48);
			number2ip(subnet_p->start_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
			json_insert_child(entry, key);

			key = json_new_string("end ip");
			bzero(str_temp, 48);
			number2ip(subnet_p->end_ip, str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
			json_insert_child(entry, key);

			key = json_new_string("cidr");
			value = json_new_string(subnet_p->cidr);
			json_insert_child(key, value);
			json_insert_child(entry, key);


			key = json_new_string("port num");
			bzero(str_temp, 48);
			sprintf(str_temp, "%d", subnet_p->port_num);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("external");
			bzero(str_temp, 48);
			sprintf(str_temp, "%d", subnet_p->external);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);

			key = json_new_string("check flag");
			bzero(str_temp, 48);
			sprintf(str_temp, "%d", subnet_p->check_status);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
	        json_insert_child(entry, key);
			
			json_insert_child(array, entry);
		}		
		node_p = node_p->next;
	}

	return json_to_reply(obj, GN_OK);
}


INT1* fabric_debug_get_all_qos_policy()
{
	json_t *obj, *array, *key, *value, *entry;
	obj = json_new_object();
	array = json_new_array();

	key = json_new_string("qos policy list");
    json_insert_child(key, array);
    json_insert_child(obj, key);
	INT1 str_temp[48] = {0};

	qos_policy_p policy_p = g_qos_policy_list;

	while (policy_p) {
		entry = json_new_object();

		key = json_new_string("name");
		value = json_new_string(policy_p->qos_policy_name);
		json_insert_child(key, value);
	    json_insert_child(entry, key);

		key = json_new_string("id");
		bzero(str_temp, 48);
		sprintf(str_temp, "%d", policy_p->qos_policy_id);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
	    json_insert_child(entry, key);

		key = json_new_string("dpid");
		bzero(str_temp, 48);
		dpidUint8ToStr(policy_p->sw->dpid,str_temp);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
	    json_insert_child(entry, key);

		key = json_new_string("dst ip");
		bzero(str_temp, 48);
		number2ip(policy_p->dst_ip, str_temp);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
	    json_insert_child(entry, key);

		key = json_new_string("min speed");
		bzero(str_temp, 48);
		sprintf(str_temp, "%llu", policy_p->min_speed);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
	    json_insert_child(entry, key);

		key = json_new_string("max speed");
		bzero(str_temp, 48);
		sprintf(str_temp, "%llu", policy_p->max_speed);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
	    json_insert_child(entry, key);

		key = json_new_string("burst");
		bzero(str_temp, 48);
		sprintf(str_temp, "%llu", policy_p->burst_size);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
	    json_insert_child(entry, key);


		key = json_new_string("priority");
		bzero(str_temp, 48);
		sprintf(str_temp, "%d", policy_p->priority);
		value = json_new_string(str_temp);
		json_insert_child(key, value);
	    json_insert_child(entry, key);		
 
		if (QOS_TYPE_METER == policy_p->sw->qos_type) {
			qos_meter_p meter_p = (qos_meter_p)policy_p->qos_service;

			if (meter_p) {
				key = json_new_string("meter id");
				bzero(str_temp, 48);
				sprintf(str_temp, "%d", meter_p->meter_id);
				value = json_new_string(str_temp);
				json_insert_child(key, value);
				json_insert_child(entry, key);	
			}
		}
		else if (QOS_TYPE_QUEUE == policy_p->sw->qos_type) {
			gn_queue_t* queue_p = (gn_queue_t*)policy_p->qos_service;

			if (queue_p) {
				key = json_new_string("port no");
				bzero(str_temp, 48);
				sprintf(str_temp, "%d", queue_p->port_no);
				value = json_new_string(str_temp);
				json_insert_child(key, value);
				json_insert_child(entry, key);	

				key = json_new_string("queue id");
				bzero(str_temp, 48);
				sprintf(str_temp, "%d", queue_p->queue_id);
				value = json_new_string(str_temp);
				json_insert_child(key, value);
				json_insert_child(entry, key);

				key = json_new_string("queue uuid");
				bzero(str_temp, 48);
				sprintf(str_temp, "%s", queue_p->queue_uuid);
				value = json_new_string(str_temp);
				json_insert_child(key, value);
				json_insert_child(entry, key);
			}
		}
		else {
			// do nothing
		}

		json_insert_child(array, entry);
		
		policy_p = policy_p->next;
	}
	
	return json_to_reply(obj, GN_OK);
}


INT1 *fabric_debug_post_qos_policy(const INT1 *url, json_t *root)
{
    json_t *item = NULL;
	INT1 name_str[48] = {0};
	INT1 dpid_str[48] = {0};
	INT4 type = 0;
	UINT4 dst_ip = 0;
	INT4  id = 0;
	UINT8 min_speed = 0;
	UINT8 max_speed = 0;
	UINT8 burst = 0;
	INT4 priority = 0;

	item = json_find_first_label(root, "name");
    if (item)
    {
        strcpy(name_str, item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "dpid");
    if (item)
    {
        strcpy(dpid_str, item->child->text);
        json_free_value(&item);
    }

	item = json_find_first_label(root, "dst_ip");
    if (item)
    {
        dst_ip = ip2number(item->child->text);
        json_free_value(&item);
    }

	item = json_find_first_label(root, "id");
    if (item)
    {
        min_speed = atoi(item->child->text);
        json_free_value(&item);
    }

	item = json_find_first_label(root, "min_speed");
    if (item)
    {
        min_speed = strtoull(item->child->text, 0, 10);
        json_free_value(&item);
    }

	item = json_find_first_label(root, "max_speed");
    if (item)
    {
        max_speed = strtoull(item->child->text, 0, 10);
        json_free_value(&item);
    }

	item = json_find_first_label(root, "burst");
    if (item)
    {
        burst = strtoull(item->child->text, 0, 10);
        json_free_value(&item);
    }

	item = json_find_first_label(root, "priority");
    if (item)
    {
        priority = atoi(item->child->text);
        json_free_value(&item);
    }

	if (strlen(dpid_str)) {
		if (0 == strcmp(dpid_str, "external")) {
			type = SW_EXTERNAL;
		}
		else {
			type = SW_DPID;
		}
	}
	else {
		type = SW_INVALID;
	}

	add_qos_rule_by_rest_api(name_str, id, type, dpid_str, dst_ip , min_speed, max_speed, burst, priority);

	json_t *obj, *key, *value;
	obj = json_new_object();
	
	key = json_new_string("qos poicy");
	value = json_new_string("Success");
	json_insert_child(key, value);
	json_insert_child(obj, key);


	return json_to_reply(obj, GN_OK);
}

INT1 *fabric_debug_delete_qos_policy(const INT1 *url, json_t *root)
{
    json_t *item = NULL;
	INT1 dpid_str[48] = {0};
	UINT4 dst_ip = 0;
	INT4 type = 0;
	INT4 return_value = 0;

	item = json_find_first_label(root, "dst_ip");
    if (item)
    {
        dst_ip = ip2number(item->child->text);
        json_free_value(&item);
    }

    item = json_find_first_label(root, "dpid");
    if (item)
    {
        strcpy(dpid_str, item->child->text);
        json_free_value(&item);
    }


	if (strlen(dpid_str)) {
		if (0 == strcmp(dpid_str, "external")) {
			type = SW_EXTERNAL;
		}
		else {
			type = SW_DPID;
		}
	}
	else {
		type = SW_INVALID;
	}


	return_value = delete_qos_rule_by_rest_api(type, dpid_str, dst_ip);

	json_t *obj, *key, *value;
	obj = json_new_object();
	
	key = json_new_string("qos poicy");
	value = (0 == return_value) ? json_new_string("Fail") : json_new_string("Success");
	json_insert_child(key, value);
	json_insert_child(obj, key);


	return json_to_reply(obj, GN_OK);
}


