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
 * fabric_path.c
 *
 *  Created on: Mar 20, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include "fabric_path.h"
#include "mem_pool.h"
#include <stdlib.h>
void *g_fabric_impl_flow_node_mem_id = NULL;
void *g_fabric_impl_flow_list_mem_id = NULL;
void *g_fabric_impl_flow_set_mem_id = NULL;
void *g_fabric_path_node_mem_id = NULL;
void *g_fabric_path_mem_id = NULL;
void *g_fabric_path_list_mem_id = NULL;
void *g_fabric_sw_node_mem_id = NULL;
void *g_fabric_sw_node_list_mem_id = NULL;
UINT1 g_mem_state = 0;
void init_fabric_path_mem(){
//	if(g_mem_state == 0){
//		g_fabric_impl_flow_node_mem_id = mem_create(sizeof(t_fabric_impl_flow_node), FABRIC_IMPL_FLOW_NODE_MAX_NUM);
//		g_fabric_impl_flow_list_mem_id = mem_create(sizeof(t_fabric_impl_flow_list), FABRIC_IMPL_FLOW_LIST_MAX_NUM);
//		g_fabric_impl_flow_set_mem_id = mem_create(sizeof(t_fabric_impl_flow_list_set), FABRIC_IMPL_FLOW_SET_MAX_NUM);
//		g_fabric_path_node_mem_id = mem_create(sizeof(t_fabric_path_node), FABRIC_PATH_NODE_MAX_NUM);
//		g_fabric_path_mem_id = mem_create(sizeof(t_fabric_path), FABRIC_PATH_MAX_NUM);
//		g_fabric_path_list_mem_id = mem_create(sizeof(t_fabric_path_list), FABRIC_PATH_LIST_MAX_NUM);
//		g_fabric_sw_node_mem_id = mem_create(sizeof(t_fabric_sw_node), FABRIC_SW_NODE_MAX_NUM);
//		g_fabric_sw_node_list_mem_id = mem_create(sizeof(t_fabric_sw_list), FABRIC_SW_NODE_LIST_MAX_NUM);
//		g_mem_state = 1;
//	}
	return;
};
void destory_fabric_path_men(){
//	if(g_mem_state == 1){
//		mem_destroy(g_fabric_impl_flow_node_mem_id);
//		mem_destroy(g_fabric_impl_flow_list_mem_id);
//		mem_destroy(g_fabric_impl_flow_set_mem_id);
//		mem_destroy(g_fabric_path_node_mem_id);
//		mem_destroy(g_fabric_path_mem_id);
//		mem_destroy(g_fabric_path_list_mem_id);
//		mem_destroy(g_fabric_sw_node_mem_id);
//		mem_destroy(g_fabric_sw_node_list_mem_id);
//		g_mem_state = 0;
//	}
	return;
};
////////////////////////////////////////////////////////////////////////
p_fabric_impl_flow_node create_fabric_impl_flow_node(gn_switch_t* sw,UINT4 port_no,UINT4 tag,UINT1 tabel_id){
	p_fabric_impl_flow_node ret = NULL;
	// check parameters
	if(NULL != sw){
		// alloc space for a node
		ret = (p_fabric_impl_flow_node)malloc(sizeof(t_fabric_impl_flow_node));
		//ret = (p_fabric_impl_flow_node)mem_get(g_fabric_impl_flow_node_mem_id);

		// set the data by parameters
		ret->sw = sw;
		ret->port_no = port_no;
		ret->tag = tag;
		ret->table_id = tabel_id;
		ret->next = NULL;
	}
	return ret;
};
p_fabric_impl_flow_node delete_fabric_impl_flow_node(p_fabric_impl_flow_node node){
	// initialize the variables
	p_fabric_impl_flow_node ret = NULL;

	// free the space and get next node
	if(NULL != node){
		ret = node->next;
		free(node);
		//mem_free(g_fabric_impl_flow_node_mem_id,node);
	}
	return ret;
};

////////////////////////////////////////////////////////////////////////
p_fabric_impl_flow_list create_fabric_impl_flow_list(gn_switch_t* sw){
	p_fabric_impl_flow_list ret = NULL;
	if(NULL != sw){
		ret = (p_fabric_impl_flow_list)malloc(sizeof(t_fabric_impl_flow_list));
		//ret = (p_fabric_impl_flow_list)mem_get(g_fabric_impl_flow_list_mem_id);
		ret->sw = sw;
		ret->list = NULL;
		ret->next = NULL;
	}
	return ret;
};
void add_fabric_impl_flow_node_to_list(p_fabric_impl_flow_list list,p_fabric_impl_flow_node node){
	// initialize the variables
	t_fabric_impl_flow_node t_head;
	p_fabric_impl_flow_node head = &t_head;

	// check parameters
	if(NULL == list || NULL == node){
		return;
	}
	// find last node
	head->next = list->list;
	while(head->next != NULL){
		head = head->next;
	}
	// add last node into path
	head->next = node;

	list->list = t_head.next;
	return;
};
void insert_fabric_impl_flow_node_to_list(p_fabric_impl_flow_list list,p_fabric_impl_flow_node node){
	// check parameters
	if(NULL == list || NULL == node){
		return;
	}
	node->next = list->list;
	list->list = node;

	return;
};
p_fabric_impl_flow_node remove_fabric_impl_flow_node_from_list(p_fabric_impl_flow_list list,p_fabric_impl_flow_node node){
	// initialize the variables
	t_fabric_impl_flow_node t_head;
	p_fabric_impl_flow_node head = &t_head;
	if( NULL != list && NULL != node){
		head->next = list->list;
		while(head->next != NULL){
			if(head->next == node){
				head->next = node->next;
				node->next = NULL;
				// reset list
				list->list = t_head.next;
				return node;
			}
			head = head->next;
		}
		list->list = t_head.next;
	}
	return node;
};
p_fabric_impl_flow_node remove_fabric_impl_flow_node_from_list_by_tag_and_table(p_fabric_impl_flow_list list,gn_switch_t* sw,UINT4 tag,UINT1 table_id){
	// initialize the variables
	t_fabric_impl_flow_node t_head;
	p_fabric_impl_flow_node head = &t_head,ret=NULL;
	if( NULL != list && NULL != sw){
		head->next = list->list;
		while(head->next != NULL){
			if(head->next->table_id == table_id && head->next->tag == tag && head->next->sw == sw){
				ret = head->next;
				head->next = ret->next;
				ret->next = NULL;
				// reset list
				list->list = t_head.next;
				return ret;
			}
			head = head->next;
		}
		list->list = t_head.next;;
	}
	return ret;
};
p_fabric_impl_flow_node get_fabric_impl_flow_node_from_list_by_tag_and_table(p_fabric_impl_flow_list list,gn_switch_t* sw,UINT4 tag,UINT1 table_id){
	p_fabric_impl_flow_node head = NULL;
	if( NULL != list && NULL != sw){
		head = list->list;
		while(head != NULL){
			if(head->table_id == table_id && head->tag == tag && head->sw == sw){
				return head;
			}
			head = head->next;
		}
	}
	return head;
};
p_fabric_impl_flow_list delete_fabric_impl_flow_list(p_fabric_impl_flow_list list){
	// initialize the variables
	p_fabric_impl_flow_list ret = NULL;
	p_fabric_impl_flow_node sentinel = NULL;

	// check parameters
	if( NULL != list){
		// get next path
		ret = list->next;
		// set NULL
		sentinel = list->list;
		list->list = NULL;
		while(sentinel != NULL){
			sentinel = delete_fabric_impl_flow_node(sentinel);
		}
		free(list);
		//mem_free(g_fabric_impl_flow_list_mem_id,list);
	}

	return ret;
};
////////////////////////////////////////////////////////////////////////
p_fabric_impl_flow_list_set create_fabric_impl_flow_list_set(){
	p_fabric_impl_flow_list_set ret = NULL;
	ret = (p_fabric_impl_flow_list_set)malloc(sizeof(t_fabric_impl_flow_list_set));
	//ret = (p_fabric_impl_flow_list_set)mem_get(g_fabric_impl_flow_set_mem_id);
	ret->num = 0;
	ret->list = 0;
	return ret;
};
void add_fabric_impl_flow_list_to_set(p_fabric_impl_flow_list_set set, p_fabric_impl_flow_list list){
	t_fabric_impl_flow_list t_head;
	p_fabric_impl_flow_list head = &t_head;
	if(NULL != set && NULL != list){
		head->next = set->list;
		while(head->next != NULL){
			head = head->next;
		}
		head->next = list;
		set->list = t_head.next;
		set->num++;
	}
	return;
};
void insert_fabric_impl_flow_list_to_set(p_fabric_impl_flow_list_set set, p_fabric_impl_flow_list list){
	if(NULL != set && NULL != list){
		list->next = set->list;
		set->list = list;
		set->num++;
	}
	return;
};
p_fabric_impl_flow_list remove_fabric_impl_flow_list_from_set(p_fabric_impl_flow_list_set set, p_fabric_impl_flow_list list){
	// initialize the variables
	t_fabric_impl_flow_list t_head;
	p_fabric_impl_flow_list head = &t_head;
	if( NULL != set && NULL != list){
		head->next = set->list;
		while(head->next != NULL){
			if(head->next == list){
				head->next = list->next;
				list->next = NULL;
				// reset set
				set->list = t_head.next;
				return list;
			}
			head = head->next;
		}
		set->list = t_head.next;
	}
	return list;

};
p_fabric_impl_flow_list remove_fabric_impl_flow_list_from_set_by_sw(p_fabric_impl_flow_list_set set, gn_switch_t* sw){
	// initialize the variables
	t_fabric_impl_flow_list t_head;
	p_fabric_impl_flow_list head = &t_head,ret=NULL;
	if( NULL != set && NULL != sw){
		head->next = set->list;
		while(head->next != NULL){
			if(head->next->sw == sw){
				ret = head->next;
				head->next = ret->next;
				ret->next = NULL;
				// reset set
				set->list = t_head.next;
				return ret;
			}
			head = head->next;
		}
		set->list = t_head.next;
	}
	return ret;
};
p_fabric_impl_flow_list get_fabric_impl_flow_list_from_set_by_sw(p_fabric_impl_flow_list_set set,gn_switch_t* sw){
	p_fabric_impl_flow_list head = NULL;
	if( NULL != set && NULL != sw){
		head = set->list;
		while(head != NULL){
			if(head->sw == sw ){
				return head;
			}
			head = head->next;
		}
	}
	return head;
};
p_fabric_impl_flow_list clear_fabric_impl_flow_list_set(p_fabric_impl_flow_list_set set){
	// initialize the variables
	p_fabric_impl_flow_list ret=NULL;
	if(set != NULL){
		ret = set->list;

		set->list = NULL;
		set->num = 0;
	}
	return ret;
};
void delete_fabric_impl_flow_list_set(p_fabric_impl_flow_list_set set){

	// initialize the variables
	p_fabric_impl_flow_list p_sentinel = NULL;
	if(set != NULL){
		p_sentinel = set->list;
		while(p_sentinel != NULL){
			p_sentinel = delete_fabric_impl_flow_list(p_sentinel);
		}
		free(set);
		//mem_free(g_fabric_impl_flow_set_mem_id,set);
	}
	return;
};

////////////////////////////////////////////////////////////////////
/*
 * create a path node
 */
p_fabric_path_node create_fabric_path_node(gn_switch_t* sw,gn_port_t* port){
	// initialize the variables
	p_fabric_path_node ret = NULL;

	// check parameters
	if(NULL != sw){
		// alloc space for a node
		ret = (p_fabric_path_node)malloc(sizeof(t_fabric_path_node));
		//ret = (p_fabric_path_node)mem_get(g_fabric_path_node_mem_id);

		// set the data by parameters
		ret->sw = sw;
		ret->port = port;
		ret->next = NULL;
	}
	return ret;
};

/*
 * delete a path node and return the next node
 */
p_fabric_path_node delete_fabric_path_node(p_fabric_path_node node){
	// initialize the variables
	p_fabric_path_node ret = NULL;

	// free the space and get next node
	if(NULL != node){
		ret = node->next;
		free(node);
		//mem_free(g_fabric_path_node_mem_id,node);
	}
	return ret;
};
/*
 * deep copy node and its next nodes (list)
 */
p_fabric_path_node copy_fabric_path_node(p_fabric_path_node node){
	// initialize the variables
	t_fabric_path_node sentinel;
	p_fabric_path_node p_sentinel = &sentinel;

	// old codes
	/*
	if(node == NULL){
		return temp;
	}
	t.next = init_fabric_path_node(node->sw,node->port);
	temp = t.next;
	while(NULL != node->next)
	{
		node = node->next;
		temp->next = init_fabric_path_node(node->sw,node->port);
		temp = temp->next;
	}*/
	// initialize sentinel node
	p_sentinel->next = NULL;

	// deep copy data
	while(node != NULL){
		p_sentinel->next = create_fabric_path_node(node->sw,node->port);
		p_sentinel = p_sentinel->next;
		node = node->next;
	}
	return sentinel.next;
};
////////////////////////////////////////////////////////////////////////
/*
 * create a path
 */
p_fabric_path create_fabric_path(gn_switch_t* sw,gn_port_t* port){
	// initialize the variables`
	p_fabric_path ret = NULL;
	p_fabric_path_node p_node = NULL;

	// create a path
	if(sw != NULL){
		// create a node
		p_node = create_fabric_path_node(sw,port);
		// alloc space for the path
		ret = (p_fabric_path)malloc(sizeof(t_fabric_path));
		//ret = (p_fabric_path)mem_get(g_fabric_path_mem_id);

		// set the head node
		ret->node_list = p_node;

		// source node and destination node is the same
		ret->dst = sw;
		ret->src = sw;

		// next is NULL
		ret->next = NULL;

		// path length is zero
		ret->len = 0;
	}
	return ret;
};
/*
 * add a node into a path at the end
 */
void add_fabric_node_to_path(p_fabric_path path,p_fabric_path_node node){
	// initialize the variables
	p_fabric_path_node head = NULL;

	// check parameters
	if(NULL == path || NULL == node){
		return;
	}
	// find last node
	head = path->node_list;
	while(head->next != NULL){
		head = head->next;
	}
	// add last node into path
	head->next = node;

	// change the destination node
	path->dst = node->sw;

	// length add 1
	path->len++;
	return;
};

/*
 * insert a node into a path at the head
 */
void insert_fabric_node_to_path(p_fabric_path path,p_fabric_path_node node){
	// check parameters
	if(NULL == path || NULL == node){
		return;
	}
	// insert the head node
	node->next = path->node_list;
	path->node_list = node;

	// change the source node
	path->src = node->sw;

	// length add 1
	path->len++;
	return;
};
/*
 * remove a node from a path
 */
p_fabric_path_node remove_fabric_node_from_path(p_fabric_path path,gn_switch_t* sw){
	t_fabric_path_node sentinel;
	p_fabric_path_node ret=NULL,p_sentinel = &sentinel;

	if(NULL != path && NULL != sw){
		p_sentinel->next = path->node_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->sw == sw){
				ret = p_sentinel->next;
				p_sentinel->next = ret->next;
				ret->next = NULL;
				// reset the list
				path->node_list = sentinel.next;
				path->len--;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * delete the fabric path and return next path
 */
p_fabric_path delete_fabric_path(p_fabric_path path){
	// initialize the variables
	p_fabric_path ret = NULL;
	p_fabric_path_node sentinel = NULL;

	// check parameters
	if( NULL != path){
		// get next path
		ret = path->next;
		// set NULL
//		path->dst = NULL;
//		path->src = NULL;
//		path->len = 0;
//		path->next = NULL;

		// delete path nodes (list)
		sentinel = path->node_list;
		path->node_list = NULL;
		while(sentinel != NULL){
			sentinel = delete_fabric_path_node(sentinel);
		}
		free(path);
		//mem_free(g_fabric_path_mem_id,path);
	}

	return ret;
};
/*
 * deep copy a path
 */
p_fabric_path copy_fabric_path(p_fabric_path path){
	// initialize the variables
	p_fabric_path ret = NULL;

	// check parameters
	if( NULL != path){
		ret = (p_fabric_path)malloc(sizeof(t_fabric_path));
		ret->src = path->src;
		ret->dst = path->dst;
		ret->len = path->len;
		ret->node_list = copy_fabric_path_node(path->node_list);
		ret->next = NULL;
	}
	return ret;
};
////////////////////////////////////////////////////////////////////////
/*
 * create a list
 */
p_fabric_path_list create_fabric_path_list(gn_switch_t* sw){
	// initialize the variables
	p_fabric_path_list ret = NULL;
	if(sw != NULL){
		ret = (p_fabric_path_list)malloc(sizeof(t_fabric_path_list));
		//ret = (p_fabric_path_list)mem_get(g_fabric_path_list_mem_id);

		ret->path_list = create_fabric_path(sw,NULL);
		ret->num = 1;
		ret->sw = sw;
		ret->next = NULL;
	}
	return ret;
};
/*
 * create a null list
 */
p_fabric_path_list create_fabric_path_list_NULL(gn_switch_t* sw){
	// initialize the variables
	p_fabric_path_list ret = NULL;

	ret = (p_fabric_path_list)malloc(sizeof(t_fabric_path_list));
	//ret = (p_fabric_path_list)mem_get(g_fabric_path_list_mem_id);

	ret->path_list = NULL;
	ret->num = 0;
	ret->sw = sw;
	ret->next = NULL;

	return ret;
};
/*
 * add a path to list
 */
void add_fabric_path_to_list(p_fabric_path_list list, p_fabric_path path){
	// initialize the variables
	t_fabric_path sentinel;
	p_fabric_path p_sentinel = &sentinel;

	if(NULL != list && NULL != path){
		p_sentinel->next = list->path_list;
		while(p_sentinel->next != NULL){
			p_sentinel = p_sentinel->next;
		}
		p_sentinel->next = path;
		// reset the list
		list->path_list = sentinel.next;
		list->num++;
	}
	return;
};
/*
 *  insert a path to list
 */
void insert_fabric_list_path_to_list(p_fabric_path_list list, p_fabric_path path){
	// initialize the variables
	t_fabric_path sentinel;
	p_fabric_path p_sentinel = &sentinel;

	if(NULL != list && NULL != path){
		p_sentinel->next = list->path_list;
		path->next = p_sentinel->next;
		// reset the list
		list->path_list = path;
		list->num++;
	}
	return;
}
/*
 * remove a path from list by source and destination
 */
p_fabric_path remove_fabric_path_from_list_by_sw(p_fabric_path_list list,gn_switch_t* src,gn_switch_t* dst){
	// initialize the variables
	t_fabric_path sentinel;
	p_fabric_path ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != src && NULL != dst){
		p_sentinel->next = list->path_list;
		while(p_sentinel->next != NULL){
			// check destination and source sw;
			if(p_sentinel->next->src == src && p_sentinel->next->dst == dst){
				ret = p_sentinel->next;
				p_sentinel->next = ret->next;
				ret->next = NULL;
				// reset the list
				list->path_list = sentinel.next;
				list->num--;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * get a path from the list by source and destination
 */
p_fabric_path get_fabric_path_from_list_by_sw(p_fabric_path_list list,gn_switch_t* src,gn_switch_t* dst){
	// initialize the variables
	t_fabric_path sentinel;
	p_fabric_path ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != src && NULL != dst){
		p_sentinel->next = list->path_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->src == src && p_sentinel->next->dst == dst){
				ret = p_sentinel->next;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
p_fabric_path get_fabric_path_from_list_by_sw_dpid(p_fabric_path_list list,UINT8 src_dpid,UINT8 dst_dpid){
	// initialize the variables
	t_fabric_path sentinel;
	p_fabric_path ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && 0 != src_dpid && 0 != dst_dpid){
		p_sentinel->next = list->path_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->src->dpid == src_dpid && p_sentinel->next->dst->dpid == dst_dpid){
				ret = p_sentinel->next;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * remove a path from list by path
 */
p_fabric_path remove_fabric_path_from_list(p_fabric_path_list list, p_fabric_path path){
	// initialize the variables
	t_fabric_path sentinel;
	p_fabric_path ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != path){
		p_sentinel->next = list->path_list;
		while(p_sentinel->next != NULL){
			// check destination and source sw;
			if(p_sentinel->next->src == path->src && p_sentinel->next->dst == path->dst){
				ret = p_sentinel->next;
				p_sentinel->next = ret->next;
				ret->next = NULL;
				// reset the list
				list->path_list = sentinel.next;
				list->num--;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * get a path from the list by path
 */
p_fabric_path get_fabric_path_from_list(p_fabric_path_list list, p_fabric_path path){
	// initialize the variables
	t_fabric_path sentinel;
	p_fabric_path ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != path){
		p_sentinel->next = list->path_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->src == path->src && p_sentinel->next->dst == path->dst){
				ret = p_sentinel->next;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * get the first path from list
 */
p_fabric_path remove_first_fabric_path_from_list(p_fabric_path_list list){

	p_fabric_path ret = NULL;
	ret = list->path_list;
	list->path_list = ret->next;
	ret->next = NULL;
	list->num--;
	return ret;
};
/*
 * remove all fabric path from list
 */
p_fabric_path clear_fabric_path_list(p_fabric_path_list list){
	// initialize the variables
	p_fabric_path ret=NULL;
	if(list != NULL){
		ret = list->path_list;

		list->sw = NULL;
		list->path_list = NULL;
		list->num = 0;
	}
	return ret;
};
/*
 *
 */
void delete_fabric_path_list(p_fabric_path_list list){

	// initialize the variables
	p_fabric_path p_sentinel = NULL;
	if(list != NULL){
		p_sentinel = list->path_list;
		while(p_sentinel != NULL){
			p_sentinel = delete_fabric_path(p_sentinel);
		}
		free(list);
		//mem_free(g_fabric_path_list_mem_id,list);
	}
	return;
};
////////////////////////////////////////////////////////////////////////
/*
 * create a fabric sw node
 */
p_fabric_sw_node create_fabric_sw_node(gn_switch_t* sw,UINT4 tag){
	// initialize the variables
	p_fabric_sw_node ret = NULL;

	// check parameters
	if(sw != NULL){
		// alloc space for a node
		ret = (p_fabric_sw_node)malloc(sizeof(t_fabric_sw_node));
		//ret = (p_fabric_sw_node)mem_get(g_fabric_sw_node_mem_id);

		// set the data by parameters
		ret->sw = sw;
		ret->tag = tag;
		ret->next = NULL;
	}
	return ret;
};
/*
 * delete a fabric sw node and return next node
 */
p_fabric_sw_node delete_fabric_sw_node(p_fabric_sw_node node){
	// initialize the variables
	p_fabric_sw_node ret = NULL;

	// free the space and get next node
	if(node != NULL){
		ret = node->next;
		free(node);
		//mem_free(g_fabric_sw_node_mem_id,node);
	}
	return ret;
};

/*
 * deep copy a fabric sw node
 */
p_fabric_sw_node copy_fabric_sw_node(p_fabric_sw_node node){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node p_sentinel = &sentinel;

	// initialize sentinel node
	p_sentinel->next = NULL;

	// deep copy data
	while(node != NULL){
		p_sentinel->next = create_fabric_sw_node(node->sw,node->tag);
		p_sentinel = p_sentinel->next;
		node = node->next;
	}
	return sentinel.next;
};
////////////////////////////////////////////////////////////////////////

/*
 * create a fabric sw list
 */
p_fabric_sw_list create_fabric_sw_list(){
	// initialize the variables
	p_fabric_sw_list ret = NULL;

	ret = (p_fabric_sw_list)malloc(sizeof(t_fabric_sw_list));
	//ret = (p_fabric_sw_list)mem_get(g_fabric_sw_node_list_mem_id);
	ret->num = 0;
	ret->node_list = NULL;
	return ret;
};
/*
 * insert a fabric sw node in the head
 */
void insert_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw,UINT4 tag){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node node = NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != sw){
		node = create_fabric_sw_node(sw,tag);
		p_sentinel->next = list->node_list;
		node->next = p_sentinel->next;
		// reset the list
		list->node_list = node;
		list->num++;
	}
	return;

	/*	// initialize the variables
	p_fabric_sw_node temp = NULL;

	if(NULL != list && NULL != sw){
		temp = create_fabric_sw_node(sw,tag);
		if(list->list == NULL){
			list->list = temp;
			list->num = 1;
		}else{
			temp->next = list->list;
			list->list = temp;
			list->num++;
		}
	}
	*/

	return;
};
/*
 * add a fabric sw node at the end
 */
void add_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw,UINT4 tag){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node node = NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != sw){
		node = create_fabric_sw_node(sw,tag);
		p_sentinel->next = list->node_list;
		while(p_sentinel->next != NULL){
			p_sentinel = p_sentinel->next;
		}
		p_sentinel->next = node;
		// reset the list
		list->node_list = sentinel.next;
		list->num++;
	}
	return;
	/*	// initialize the variables
	p_fabric_sw_node temp = NULL;
	p_fabric_sw_node head = NULL;
	if(NULL != list && NULL != sw){
		temp = create_fabric_sw_node(sw,tag);
		if(list->list == NULL){
			list->list = temp;
			list->num = 1;
		}else{
			head = list->list;
			while(head->next != NULL){
				head = head->next;
			}
			head->next = temp;

			list->num++;
		}
	}*/
	return;
};
/*
 * remove a fabric sw node from the list
 */
p_fabric_sw_node remove_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != sw){
		p_sentinel->next = list->node_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->sw == sw){
				ret = p_sentinel->next;
				p_sentinel->next = ret->next;
				ret->next = NULL;
				// reset the list
				list->node_list = sentinel.next;
				list->num--;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * get a fabric sw node by sw
 */
p_fabric_sw_node get_fabric_sw_list_by_sw(p_fabric_sw_list list,gn_switch_t* sw){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != sw){
		p_sentinel->next = list->node_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->sw == sw){
				ret = p_sentinel->next;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * get a fabric sw node by tag
 */
p_fabric_sw_node get_fabric_sw_list_by_tag(p_fabric_sw_list list,UINT4 tag){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && tag != 0){
		p_sentinel->next = list->node_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->tag == tag){
				ret = p_sentinel->next;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
}
/*
 * insert a new node
 */
void insert_fabric_sw_list(p_fabric_sw_list list,p_fabric_sw_node node){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node p_sentinel = &sentinel;

	if(NULL != list && NULL != node){
		p_sentinel->next = list->node_list;
		node->next = p_sentinel->next;
		// reset the list
		list->node_list = node;
		list->num++;
	}
	return;
};
/*
 * add a new node
 */
void add_fabric_sw_list(p_fabric_sw_list list,p_fabric_sw_node node){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node p_sentinel = &sentinel;

	if(NULL != list && NULL != node){
		p_sentinel->next = list->node_list;
		while(p_sentinel->next != NULL){
			p_sentinel = p_sentinel->next;
		}
		p_sentinel->next = node;
		// reset the list
		list->node_list = sentinel.next;
		list->num++;
	}
	return;
};
/*
 * remove a fabric sw node
 */
p_fabric_sw_node remove_fabric_sw_list(p_fabric_sw_list list,p_fabric_sw_node node){
	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node ret=NULL,p_sentinel = &sentinel;

	if(NULL != list && NULL != node){
		p_sentinel->next = list->node_list;
		while(p_sentinel->next != NULL){
			if(p_sentinel->next->sw == node->sw){
				ret = p_sentinel->next;
				p_sentinel->next = ret->next;
				ret->next = NULL;
				// reset the list
				list->num--;
				list->node_list = sentinel.next;
				return ret;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return ret;
};
/*
 * clear fabric sw list
 */
p_fabric_sw_node clear_fabric_sw_list(p_fabric_sw_list list){
	// initialize the variables
	p_fabric_sw_node ret=NULL;
	if(list != NULL){
		ret = list->node_list;
		list->node_list = NULL;
		list->num = 0;
	}
	return ret;
};
/*
 * delete fabric sw list
 */
void delete_fabric_sw_list(p_fabric_sw_list list){
	// initialize the variables
	p_fabric_sw_node p_sentinel = NULL;
	if(list != NULL){
		p_sentinel = list->node_list;
		while(p_sentinel != NULL){
			p_sentinel = delete_fabric_sw_node(p_sentinel);
		}
		free(list);
		//mem_free(g_fabric_sw_node_list_mem_id,list);
	}
	return;
};
/*
 * copy a list
 */
p_fabric_sw_list copy_fabric_sw_list(p_fabric_sw_list list){
	p_fabric_sw_list ret = NULL;
	if(list != NULL){
		ret = (p_fabric_sw_list)malloc(sizeof(t_fabric_sw_list));
		ret->num = list->num;
		ret->node_list = copy_fabric_sw_node(list->node_list);
	}
	return ret;
};
/*
 * pop the head p_fabric_sw_node
 */
p_fabric_sw_node pop_fabric_sw_list(p_fabric_sw_list list){
	p_fabric_sw_node ret = NULL;
	if(NULL != list && list->num > 0){
		ret = list->node_list;
		list->node_list = ret->next;
		ret->next = 0;
		list->num--;
	}
	return ret;
};
