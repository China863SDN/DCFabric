/*
 * fabric_path.h
 *
 *  Created on: Mar 6, 2015
 *      Author: joe
 */

#ifndef GNFLUSH_INC_FABRIC_PATH_H_
#define GNFLUSH_INC_FABRIC_PATH_H_

#include "gnflush-types.h"

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

#endif /* GNFLUSH_INC_FABRIC_PATH_H_ */
