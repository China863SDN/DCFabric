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


extern t_fabric_sw_list g_fabric_sw_list_total;
extern t_fabric_sw_list g_fabric_sw_list;
extern t_fabric_host_list g_fabric_host_list;
extern p_fabric_path_list g_fabric_path_list;
extern openstack_lbaas_node_p g_openstack_lbaas_pools_list;
extern openstack_lbaas_node_p g_openstack_lbaas_members_list;
extern openstack_security_p g_openstack_security_list;
extern openstack_lbaas_node_p g_openstack_lbaas_listener_list;

extern UINT4 g_openstack_on;

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
	
	while (head) {
		if (((g_openstack_on && (OPENSTACK_PORT_TYPE_HOST == head->type)) || (0 == g_openstack_on))) {
			INT1 str_temp[48] = {0};
			entry = json_new_object();

			if (head->sw) {
				key = json_new_string("SwIP");
				number2ip(head->sw->sw_ip, str_temp);
				value = json_new_string(str_temp);
				json_insert_child(key, value);
	            json_insert_child(entry, key);
			}
			
			key = json_new_string("IPv4");
			number2ip(head->ip_list[0], str_temp);
			value = json_new_string(str_temp);
			json_insert_child(key, value);
            json_insert_child(entry, key);

			key = json_new_string("Mac");
			mac2str(head->mac, str_temp);
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

			/*
			* test use
			key = json_new_string("member_flag");
			value = (GN_OK == member_p->update_flag) ? json_new_string("Checked") : json_new_string("Unchecked");
			json_insert_child(key, value);
	       	json_insert_child(entry, key);
			*/

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

			/*
			key = json_new_string("listener_flag");
			value = (GN_OK == listener_p->update_flag) ? json_new_string("Checked") : json_new_string("Unchecked");
			json_insert_child(key, value);
	        	son_insert_child(entry, key);
			*/

			member_array = json_new_array();
			key = json_new_string("member");
			json_insert_child(key, member_array);
			json_insert_child(entry, key);
			
			openstack_lbaas_listener_member_p listener_member_p = listener_p->listener_member_list;
			while (listener_member_p) {			
				member_value = json_new_object();

				key = json_new_string("member_ip");
				bzero(json_temp, 48);
				number2ip(listener_member_p->dst_ip, json_temp);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);

				key = json_new_string("wait_status");
				bzero(json_temp, 48);
				sprintf(json_temp, "%d", listener_member_p->wait_status);
				value = json_new_string(json_temp);
				json_insert_child(key, value);
				json_insert_child(member_value, key);
				
				json_insert_child(member_array, member_value);

				listener_member_p = listener_member_p->next;
			}
			
			
			json_insert_child(array, entry);
		}
		list_p = list_p->next;
	}
   return json_to_reply(obj, GN_OK);

}
