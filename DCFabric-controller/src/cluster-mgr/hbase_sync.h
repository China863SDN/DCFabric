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
#ifndef HBASE_SYNC_H_
#define HBASE_SYNC_H_

#include "gnflush-types.h"
#include "fabric_path.h"
#include "hbase_client.h"
#include "fabric_openstack_nat.h"
#include "openstack_lbaas_app.h"
#include "fabric_openstack_external.h"



void persist_fabric_sw_list();
void recover_fabric_sw_list(INT4 num, const field_pad_t* field_pad_p);

void persist_fabric_host_list();
void recover_fabric_host_list(INT4 num, const field_pad_t* field_pad_p);

void persist_fabric_openstack_external_list();
void recover_fabric_openstack_external_list(INT4 num, const field_pad_t* field_pad_p);

void persist_fabric_nat_icmp_iden_single(UINT4 operation_type, openstack_external_node_p node);
void persist_fabric_nat_icmp_iden_list();
void recover_fabric_nat_icmp_iden_list(UINT4 operation_type, INT4 num, const field_pad_t* field_pad_p);

void persist_fabric_nat_host_single(UINT4 operation_type, nat_host_p host_p);
void persist_fabric_nat_host_list();
void recover_fabric_nat_host_list(UINT4 operation_type, INT4 num, const field_pad_t* field_pad_p);

void persist_fabric_openstack_lbaas_members_single(UINT4 operation_type, openstack_lbaas_connect_p connect_ips);
void persist_fabric_openstack_lbaas_members_list();
void recover_fabric_openstack_lbaas_members_list(UINT4 operation_type, INT4 num, const field_pad_t* field_pad_p);

void persist_fabric_all();

 
#endif /* CLUSTER_MGR_H_ */
