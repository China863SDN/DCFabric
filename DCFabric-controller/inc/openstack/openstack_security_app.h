/*
 * openstack_security_app.h
 *
 *  Created on: Dec 7, 2015
 *      Author: yang
 */

#ifndef SRC_OPENSTACK_OPENSTACK_SECURITY_APP_H_
#define SRC_OPENSTACK_OPENSTACK_SECURITY_APP_H_

#include "gnflush-types.h"
#include "openstack_host.h"
#include "forward-mgr.h"

INT4 openstack_security_group_main_check(p_fabric_host_node src_port, p_fabric_host_node dst_port,
		packet_in_info_t *packet_in, security_param_t* src_security, security_param_t* dst_security);

//INT4 openstack_security_icmp_check(openstack_security_rule_p osrp,ip_t *ip);
//INT4 openstack_security_tcp_udp_check(openstack_security_rule_p osrp,ip_t *ip);
//INT4 openstack_security_group_protocol_check(openstack_security_rule_p osrp,ip_t *ip);
//INT4 openstack_security_specifical_ip_check(openstack_security_rule_p osrp,ip_t *ip);
//INT4 openstack_security_other_check(openstack_security_rule_p osrp,ip_t *ip);

#endif /* SRC_OPENSTACK_OPENSTACK_SECURITY_APP_H_ */
