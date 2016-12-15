/*
 * group.c
 *
 *  Created on: Jun 1, 2015
 *      Author: joe
 */

#include "group.h"
#include <string.h>

p_dc_group group_head = NULL;
UINT4 g_group_list_num = 0;
UINT4 g_current_group_id = 0;

// mem id
void *g_dc_group_list_mem_id = NULL;

//////////////////////////////////////////////////////////////////////////
// module functions
//////////////////////////////////////////////////////////////////////////

/*
 * init group module
 */
void init_dc_group_list(){
	if(g_dc_group_list_mem_id != NULL){
		mem_destroy(g_dc_group_list_mem_id);
	}
	g_dc_group_list_mem_id = mem_create(sizeof(t_dc_group), DC_GROUP_MAX_NUM);
	g_current_group_id = DC_GROUP_START_ID;
	// read from file
	return;
};

/*
 * destroy group module
 */
void destroy_dc_group_list(){
	if(g_dc_group_list_mem_id != NULL){
		mem_destroy(g_dc_group_list_mem_id);
		g_dc_group_list_mem_id = NULL;
	}
	return;
};

//////////////////////////////////////////////////////////////////////////
// group functions
//////////////////////////////////////////////////////////////////////////
/*
 * create a group
 */
p_dc_group create_dc_group(const char* group_name){
	// initialize ret
	p_dc_group ret = NULL;
	// create group object
	ret = (p_dc_group)mem_get(g_dc_group_list_mem_id);
	ret->group_id = g_current_group_id;
	strcpy(ret->group_name,group_name);
	ret->next = NULL;
	ret->host_list_num = 0;
	//memset(ret->sw_dpid_list, 0, sizeof(UINT8)*DC_GROUP_SWITCH_MAX_NUM);
	// add to list
	ret->next = group_head;
	group_head = ret;
	// global varb change
	g_current_group_id++;
	g_group_list_num++;
	return ret;
};
void delete_dc_group(p_dc_group group){
	t_dc_group sentinel;
	p_dc_group p_sentinel = &sentinel;

	p_sentinel->next = group_head;
	while(p_sentinel->next != NULL){
		if(p_sentinel->next == group){
			p_sentinel->next = group->next;
			mem_free(g_dc_group_list_mem_id,group);
			group_head = sentinel.next;
			g_group_list_num--;
			return;
		}
		p_sentinel = p_sentinel->next;
	}

	group_head = sentinel.next;
	return;
};
void delete_dc_group_by_group_id(UINT4 group_id){
	t_dc_group sentinel;
	p_dc_group p_sentinel = &sentinel,group = NULL;

	p_sentinel->next = group_head;
	while(p_sentinel->next != NULL){
		if(p_sentinel->next->group_id == group_id){
			group = p_sentinel->next;
			p_sentinel->next = group->next;
			mem_free(g_dc_group_list_mem_id,group);
			group_head = sentinel.next;
			g_group_list_num--;
			return;
		}
		p_sentinel = p_sentinel->next;
	}

	group_head = sentinel.next;
	return;
};

p_dc_group get_dc_group_by_group_id(UINT4 group_id){
	p_dc_group ret = NULL;
	ret = group_head;
	while(ret != NULL){
		if(ret->group_id == group_id){
			return ret;
		}
		ret = ret->next;
	}
	return ret;
};
//////////////////////////////////////////////////////////////////////////
// group host functions
//////////////////////////////////////////////////////////////////////////
UINT1 add_host_into_group(p_dc_group group,UINT1* mac){
	UINT1* t_mac = NULL;
	if(group == NULL){
		return 0;
	}
	t_mac = group->host_mac_list[group->host_list_num];
	memcpy(t_mac,mac,6);
	group->host_list_num++;
	return 1;
};
UINT1 remove_host_from_group(p_dc_group group,UINT1* mac){
	UINT4 i = 0;
	UINT1* t_mac = NULL,l_mac = NULL;
	if(group == NULL){
		return 0;
	}
	for(i = 0 ; i < group->host_list_num; i++){
		t_mac = group->host_mac_list[i];
		if(memcmp(t_mac,mac,6) == 0){
			memset(t_mac,0,6);
			group->host_list_num--;
			l_mac = group->host_mac_list[group->host_list_num];
			memcpy(t_mac,l_mac,6);
			break;
		}
	}
	return 1;
};
UINT1 clear_host_from_group(p_dc_group group){
	if(group == NULL){
		return 0;
	}
	group->host_list_num = 0;
	return 1;
};
UINT1 find_host_from_group(p_dc_group group,UINT1* mac){
	UINT4 i = 0;
	UINT1* t_mac = NULL;
	if(group == NULL){
		return 0;
	}
	for(i = 0 ; i < group->host_list_num; i++){
		t_mac = group->host_mac_list[i];
		if(memcmp(t_mac,mac,6) == 0){
			return 1;
		}
	}
	return 0;
};

UINT1 add_host_into_group_id(UINT4 group_id,UINT1* mac){
	p_dc_group group = NULL;
	UINT1* t_mac = NULL;
	group = get_dc_group_by_group_id(group_id);
	if(group == NULL){
		return 0;
	}
	t_mac = group->host_mac_list[group->host_list_num];
	memcpy(t_mac,mac,6);
	group->host_list_num++;
	return 1;
};
UINT1 remove_host_from_group_id(UINT4 group_id,UINT1* mac){
	p_dc_group group = NULL;
	UINT4 i = 0;
	UINT1* t_mac = NULL,l_mac = NULL;
	group = get_dc_group_by_group_id(group_id);
	if(group == NULL){
		return 0;
	}
	for(i = 0 ; i < group->host_list_num; i++){
		t_mac = group->host_mac_list[i];
		if(memcmp(t_mac,mac,6) == 0){
			memset(t_mac,0,6);
			group->host_list_num--;
			l_mac = group->host_mac_list[group->host_list_num];
			memcpy(t_mac,l_mac,6);
			break;
		}
	}
	return 1;
};
UINT1 clear_host_from_group_id(UINT4 group_id){
	p_dc_group group = NULL;
	group = get_dc_group_by_group_id(group_id);
	if(group == NULL){
		return 0;
	}
	group->host_list_num = 0;
	return 1;
};
UINT1 find_host_from_group_id(UINT4 group_id,UINT1* mac){
	p_dc_group group = NULL;
	UINT4 i = 0;
	UINT1* t_mac = NULL;
	group = get_dc_group_by_group_id(group_id);
	if(group == NULL){
		return 0;
	}
	for(i = 0 ; i < group->host_list_num; i++){
		t_mac = group->host_mac_list[i];
		if(memcmp(t_mac,mac,6) == 0){
			return 1;
		}
	}
	return 0;
};

UINT1 get_group_ids_by_mac(UINT1* mac,UINT4* group_id_list){
	p_dc_group group = NULL;
	UINT1 is_in_group = 0,ret = 0;
	group = group_head;
	while(group != NULL){
		is_in_group = find_host_from_group(group,mac);
		if( 1 == is_in_group ){
			group_id_list[ret] = group->group_id;
			ret++;
		}
		group = group->next;
	}
	return ret;
};

UINT1 check_hosts_same_group(UINT1* mac1,UINT1* mac2){
	p_dc_group group = NULL;
	UINT1 mac1_is_in_group = 0,mac2_is_in_group = 0,mac1_label = 0,mac2_label = 0;
	group = group_head;
	while(group != NULL){
		mac1_is_in_group = find_host_from_group(group,mac1);
		mac2_is_in_group = find_host_from_group(group,mac2);
		if( 1 == mac1_is_in_group && 1 == mac2_is_in_group){
			return 1;
		}
		if( 1 == mac1_is_in_group ){
			mac1_label = 1;
		}
		if( 1 == mac1_is_in_group ){
			mac2_label = 1;
		}
		group = group->next;
	}
	if(mac1_label == 0 && mac2_label == 0){
		return 1;
	}
	return 0;
};
