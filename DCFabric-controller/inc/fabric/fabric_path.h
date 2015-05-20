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
 * fabric_path.h
 *
 *  Created on: Mar 20, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#ifndef GNFLUSH_INC_FABRIC_PATH_H_
#define GNFLUSH_INC_FABRIC_PATH_H_

#include "gnflush-types.h"

#define FABRIC_IMPL_FLOW_NODE_MAX_NUM 40960 //
#define FABRIC_IMPL_FLOW_LIST_MAX_NUM 1024  // according to max switch 1024
#define FABRIC_IMPL_FLOW_SET_MAX_NUM 4		// min 4
#define FABRIC_PATH_NODE_MAX_NUM 1024*1024
#define FABRIC_PATH_MAX_NUM 1024*1024
#define FABRIC_PATH_LIST_MAX_NUM 1024
#define FABRIC_SW_NODE_MAX_NUM 2048			// according to max switch 1024
#define FABRIC_SW_NODE_LIST_MAX_NUM 4		// min 4
void init_fabric_path_mem();
void destory_fabric_path_men();
////////////////////////////////////////////////////////////////////////
typedef struct fabric_impl_flow_node{
	gn_switch_t* sw;
	UINT4 port_no;
	UINT4 tag;
	UINT1 table_id;
	struct fabric_impl_flow_node* next;
}t_fabric_impl_flow_node,*p_fabric_impl_flow_node;
typedef struct fabric_impl_flow_list{
	gn_switch_t* sw;
	p_fabric_impl_flow_node list;
	struct fabric_impl_flow_list* next;
}t_fabric_impl_flow_list,*p_fabric_impl_flow_list;

typedef struct fabric_impl_flow_list_set{
	UINT4 num;
	p_fabric_impl_flow_list list;
}t_fabric_impl_flow_list_set,*p_fabric_impl_flow_list_set;
////////////////////////////////////////////////////////////////////////
p_fabric_impl_flow_node create_fabric_impl_flow_node(gn_switch_t* sw,UINT4 port_no,UINT4 tag,UINT1 tabel_id);
p_fabric_impl_flow_node delete_fabric_impl_flow_node(p_fabric_impl_flow_node node);

////////////////////////////////////////////////////////////////////////
p_fabric_impl_flow_list create_fabric_impl_flow_list(gn_switch_t* sw);
void add_fabric_impl_flow_node_to_list(p_fabric_impl_flow_list list,p_fabric_impl_flow_node node);
void insert_fabric_impl_flow_node_to_list(p_fabric_impl_flow_list list,p_fabric_impl_flow_node node);
p_fabric_impl_flow_node remove_fabric_impl_flow_node_from_list(p_fabric_impl_flow_list list,p_fabric_impl_flow_node node);
p_fabric_impl_flow_node remove_fabric_impl_flow_node_from_list_by_tag_and_table(p_fabric_impl_flow_list list,gn_switch_t* sw,UINT4 tag,UINT1 table_id);
p_fabric_impl_flow_node get_fabric_impl_flow_node_from_list_by_tag_and_table(p_fabric_impl_flow_list list,gn_switch_t* sw,UINT4 tag,UINT1 table_id);
p_fabric_impl_flow_list delete_fabric_impl_flow_list(p_fabric_impl_flow_list list);
////////////////////////////////////////////////////////////////////////
p_fabric_impl_flow_list_set create_fabric_impl_flow_list_set();
void add_fabric_impl_flow_list_to_set(p_fabric_impl_flow_list_set set, p_fabric_impl_flow_list list);
void insert_fabric_impl_flow_list_to_set(p_fabric_impl_flow_list_set set, p_fabric_impl_flow_list list);
p_fabric_impl_flow_list remove_fabric_impl_flow_list_from_set(p_fabric_impl_flow_list_set set, p_fabric_impl_flow_list list);
p_fabric_impl_flow_list remove_fabric_impl_flow_list_from_set_by_sw(p_fabric_impl_flow_list_set set, gn_switch_t* sw);
p_fabric_impl_flow_list get_fabric_impl_flow_list_from_set_by_sw(p_fabric_impl_flow_list_set set, gn_switch_t* sw);
p_fabric_impl_flow_list clear_fabric_impl_flow_list_set(p_fabric_impl_flow_list_set set);
void delete_fabric_impl_flow_list_set(p_fabric_impl_flow_list_set set);

////////////////////////////////////////////////////////////////////////
typedef struct fabric_path_node{
	gn_switch_t* sw;
	gn_port_t* port;
	struct fabric_path_node* next;
}t_fabric_path_node,* p_fabric_path_node;

typedef struct fabric_path{
	gn_switch_t* src;
	gn_switch_t* dst;
	p_fabric_path_node node_list;
	struct fabric_path* next;
	UINT4 len;
}t_fabric_path,* p_fabric_path;

typedef struct fabric_path_list{
	gn_switch_t* sw;
	UINT4 num;
	p_fabric_path path_list;
	struct fabric_path_list* next;
}t_fabric_path_list,* p_fabric_path_list;

////////////////////////////////////////////////////////////////////////

p_fabric_path_node create_fabric_path_node(gn_switch_t* sw,gn_port_t* port);
p_fabric_path_node delete_fabric_path_node(p_fabric_path_node node);
p_fabric_path_node copy_fabric_path_node(p_fabric_path_node node);

////////////////////////////////////////////////////////////////////////

p_fabric_path create_fabric_path(gn_switch_t* sw,gn_port_t* port);
void add_fabric_node_to_path(p_fabric_path path,p_fabric_path_node node);
void insert_fabric_node_to_path(p_fabric_path path,p_fabric_path_node node);
p_fabric_path_node remove_fabric_node_from_path(p_fabric_path path,gn_switch_t* sw);
p_fabric_path delete_fabric_path(p_fabric_path path);
p_fabric_path copy_fabric_path(p_fabric_path path);

////////////////////////////////////////////////////////////////////////
p_fabric_path_list create_fabric_path_list(gn_switch_t* sw);
p_fabric_path_list create_fabric_path_list_NULL(gn_switch_t* sw);
void add_fabric_path_to_list(p_fabric_path_list list, p_fabric_path path);
void insert_fabric_list_path_to_list(p_fabric_path_list list, p_fabric_path path);
p_fabric_path remove_fabric_path_from_list_by_sw(p_fabric_path_list list,gn_switch_t* src,gn_switch_t* dst);
p_fabric_path get_fabric_path_from_list_by_sw(p_fabric_path_list list,gn_switch_t* src,gn_switch_t* dst);
p_fabric_path get_fabric_path_from_list_by_sw_dpid(p_fabric_path_list list,UINT8 src_dpid,UINT8 dst_dpid);
p_fabric_path remove_fabric_path_from_list(p_fabric_path_list list, p_fabric_path path);
p_fabric_path get_fabric_path_from_list(p_fabric_path_list list, p_fabric_path path);
p_fabric_path clear_fabric_path_list(p_fabric_path_list list);
p_fabric_path remove_first_fabric_path_from_list(p_fabric_path_list list);
void delete_fabric_path_list(p_fabric_path_list list);
//p_fabric_path_list copy_fabric_path_list(p_fabric_path_list list);

////////////////////////////////////////////////////////////////////////
typedef struct fabric_sw_node{
	gn_switch_t* sw;
	UINT4 tag;
	struct fabric_sw_node* next;
}t_fabric_sw_node,* p_fabric_sw_node;

typedef struct fabric_sw_list{
	UINT4 num;
	p_fabric_sw_node node_list;
}t_fabric_sw_list,*p_fabric_sw_list;
////////////////////////////////////////////////////////////////////////
p_fabric_sw_node create_fabric_sw_node(gn_switch_t* sw,UINT4 tag);
p_fabric_sw_node delete_fabric_sw_node(p_fabric_sw_node node);
p_fabric_sw_node copy_fabric_sw_node(p_fabric_sw_node node);
////////////////////////////////////////////////////////////////////////
p_fabric_sw_list create_fabric_sw_list();
void insert_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw,UINT4 tag);
void add_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw,UINT4 tag);
p_fabric_sw_node remove_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw);
p_fabric_sw_node get_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw);
p_fabric_sw_node get_fabric_sw_list_by_tag(p_fabric_sw_list list,UINT4 tag);
void insert_fabric_sw_list(p_fabric_sw_list list,p_fabric_sw_node node);
void add_fabric_sw_list(p_fabric_sw_list list,p_fabric_sw_node node);
p_fabric_sw_node remove_fabric_sw_list(p_fabric_sw_list list,p_fabric_sw_node node);
p_fabric_sw_node clear_fabric_sw_list(p_fabric_sw_list list);
void delete_fabric_sw_list(p_fabric_sw_list list);
p_fabric_sw_list copy_fabric_sw_list(p_fabric_sw_list list);
p_fabric_sw_node pop_fabric_sw_list(p_fabric_sw_list list);
#endif /* GNFLUSH_INC_FABRIC_PATH_H_ */
