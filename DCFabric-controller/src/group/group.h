/*
 * group.h
 *
 *  Created on: Jun 1, 2015
 *      Author: joe
 */

#ifndef SRC_GROUP_GROUP_H_
#define SRC_GROUP_GROUP_H_

#define DC_GROUP_MAX_NUM 48
#define DC_GROUP_SWITCH_MAX_NUM 1024
#define DC_GROUP_NAME_LEN 48
#define DC_GROUP_START_ID 1

typedef struct dc_group{
	UINT4 group_id;
	char group_name[DC_GROUP_NAME_LEN];
	UINT4 host_list_num;
	UINT1 host_mac_list[DC_GROUP_SWITCH_MAX_NUM][6];
	struct dc_group* next;
}t_dc_group,*p_dc_group;

void init_dc_group_list();
void destroy_dc_group_list();

p_dc_group create_dc_group(const char* group_name);
void delete_dc_group(p_dc_group group);
void delete_dc_group_by_group_id(UINT4 group_id);
p_dc_group get_dc_group_by_group_id(UINT4 group_id);

UINT1 add_host_into_group(p_dc_group group,UINT1* mac);
UINT1 remove_host_from_group(p_dc_group group,UINT1* mac);
UINT1 clear_host_from_group(p_dc_group group);
UINT1 find_host_from_group(p_dc_group group,UINT1* mac);

UINT1 add_host_into_group_id(UINT4 group_id,UINT1* mac);
UINT1 remove_host_from_group_id(UINT4 group_id,UINT1* mac);
UINT1 clear_host_from_group_id(UINT4 group_id);
UINT1 find_host_from_group_id(UINT4 group_id,UINT1* mac);

UINT1 get_group_by_mac(UINT1* mac,UINT4* group_id_list);
UINT1 check_hosts_same_group(UINT1* mac1,UINT1* mac2);
#endif /* SRC_GROUP_GROUP_H_ */
