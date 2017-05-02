/*
 * openstack_firewall_app.h
 *
 *  Created on: Dec 9, 2015
 *      Author: yang
 */

#ifndef SRC_OPENSTACK_OPENSTACK_FIREWALL_APP_H_
#define SRC_OPENSTACK_OPENSTACK_FIREWALL_APP_H_

#include "gnflush-types.h"
#include "openstack_host.h"

INT4 openstack_firewall_main_check(p_fabric_host_node src_port,p_fabric_host_node dst_port,packet_in_info_t *packet_in);


#endif /* SRC_OPENSTACK_OPENSTACK_FIREWALL_APP_H_ */
