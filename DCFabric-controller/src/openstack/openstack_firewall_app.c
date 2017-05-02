/*
 * openstack_firewall_app.c
 *
 *  Created on: Dec 9, 2015
 *      Author: yang
 */

#include "openstack_firewall_app.h"


INT4 openstack_firewall_protocol_check(UINT4 proto_num, char* proto)
{
//	printf("%s\n", FN);
//	if ((IPPROTO_ICMP == proto_num) && (0 == strcmp("icmp", proto))) {
//		return GN_OK;
//	}
//	else if ((IPPROTO_TCP == proto_num) && (0 == strcmp("tcp", proto))) {
//		return GN_OK;
//	}
//	else if ((IPPROTO_UDP == proto_num) && (0 == strcmp("udp", proto))) {
//		return GN_OK;
//	}
//	else {
//		return GN_ERR;
//	}
	return GN_OK;
}

INT4 openstack_firewall_port_num_check(UINT4 port_num, UINT4 port_range_min, UINT4 port_range_max)
{
//	printf("%s\n", FN);
//	// if port is in the range
//	if ((port_num >= port_range_min) && (port_num <= port_range_max))
//	{
//		return GN_OK;
//	}
//
//	return GN_ERR;
	return GN_OK;
}

INT4 openstack_firewall_remote_group_check(openstack_firewall_p remote_security_p, char* remote_group_id)
{
//	printf("%s\n", FN);
//	// if contains remote group is
//	openstack_security_p security_p = remote_security_p;
//	while (NULL != security_p) {
//		if (0 == strcmp(security_p->security_group, remote_group_id)) {
//			return GN_ERR;
//		}
//		security_p = security_p->next;
//	}

	return GN_OK;
}

INT4 openstack_firewall_remote_cidr_check(UINT4 remote_ip, char* remote_ip_prefix)
{
//	printf("%s\n", FN);
//	UINT4 cidr_ip = 0;
//	UINT4 cidr_mask = 0;
//#if 1
//	if (remote_ip == ip2number("100.0.0.81")) {
//		return GN_ERR;
//	}
//#endif
//	remote_ip = remote_ip & cidr_mask;
//	if (remote_ip == cidr_ip) {
//		return GN_ERR;
//	}

	return GN_OK;
}

INT4 openstack_firewall_ether_check(char* ether_type)
{
//	printf("%s\n", FN);
//	printf("ether type: %s\n", ether_type);
//	if (0 == strcmp("IPv4", ether_type)) {
//		return GN_OK;
//	}
//	else if (0 == strcmp("IPv6", ether_type)) {
//		// TBD
//		return GN_ERR;
//	}
//	else {
//		return GN_ERR;
//	}
	return GN_OK;
}

INT4 openstack_firewall_check_direction(UINT1 direction_num, char* direction)
{
//	printf("%s\n", FN);
//	UINT4 temp_direction = 0;
//
//	if (0 == strcmp("ingress", direction)) {
//		temp_direction = 1;
//	}
//	else if (0 == strcmp("egress", direction)) {
//		temp_direction = 2;
//	}
//	else {
//		// do nothing
//	}
//
//	if (temp_direction == direction_num) {
//		return GN_OK;
//	}
//	else {
//		return GN_ERR;
//	}
	return GN_OK;
}

INT4 openstack_firewall_icmp_check(ip_t *ip, openstack_firewall_rule_p rule_p)
{
//	printf("%s\n", FN);
//	icmp_t* icmp = (icmp_t*)ip->data;
//	//openstack_security_protocol_check((UINT4)IPPROTO_ICMP, rule_p->protocol);
//	if (0 != strcmp("icmp", rule_p->protocol))
//		return GN_ERR;
//
//	if ((icmp->type == rule_p->port_range_min) && (icmp->code == rule_p->port_range_min))
//		return GN_OK;

//	return GN_ERR;

//	openstack_security_remote_group_check();
//	openstack_security_remote_cidr_check();
//	return 0;
	return GN_OK;
}

INT4 openstack_firewall_tcp_check(ip_t *ip, UINT4 direction, openstack_firewall_rule_p rule_p)
{
//	printf("%s\n", FN);
//	tcp_t* tcp = (tcp_t*)ip->data;
//	UINT2 port_num = 0;
//	//openstack_security_protocol_check((UINT4)IPPROTO_ICMP, rule_p->protocol);
//	if (0 != strcmp("tcp", rule_p->protocol))
//		return GN_ERR;
//
//	if (1 == direction) {
//		port_num = tcp->sport;
//	}
//	else {
//		port_num = tcp->dport;
//	}

//	return openstack_firewall_port_num_check(port_num, rule_p->port_range_min, rule_p->port_range_max);
//	openstack_security_protocol_check();
//    openstack_security_port_check();
//	openstack_security_remote_group_check();
//	openstack_security_remote_cidr_check();
//	return 0;
	return GN_OK;
}

INT4 openstack_firewall_udp_check(ip_t *ip, UINT4 direction, openstack_firewall_rule_p rule_p)
{
//	printf("%s\n", FN);
//	udp_t* udp = (udp_t*)ip->data;
//	UINT2 port_num = 0;
//	//openstack_security_protocol_check((UINT4)IPPROTO_ICMP, rule_p->protocol);
//	if (0 != strcmp("udp", rule_p->protocol))
//		return GN_ERR;
//
//	if (1 == direction) {
//		port_num = udp->sport;
//	}
//	else {
//		port_num = udp->dport;
//	}

//	return openstack_firewall_port_num_check(port_num, rule_p->port_range_min, rule_p->port_range_max);
//	openstack_security_protocol_check();
//    openstack_security_port_check();
//	openstack_security_remote_group_check();
//	openstack_security_remote_cidr_check();
//	return 0;
	return GN_OK;
}

INT4 openstack_firewall_rule_check(ip_t* ip, openstack_firewall_rule_p rule_p, UINT4 direction_num, UINT4 remote_ip, openstack_security_p remote_security_list, UINT1 proto)
{
//	INT4 check_result = GN_ERR;
//
//	if (GN_OK != openstack_firewall_ether_check(rule_p->ethertype)) {
//		return GN_ERR;
//	}
//
//	if (GN_OK != openstack_firewall_check_direction(direction_num, rule_p->direction)) {
//		return GN_ERR;
//	}
//
//	if (GN_OK != openstack_firewall_remote_cidr_check(remote_ip, rule_p->remote_ip_prefix)) {
//		return GN_OK;
//	}
//
//	if (GN_OK != openstack_firewall_remote_group_check(remote_security_list, rule_p->remote_group_id)) {
//		return GN_ERR;
//	}
//
//	if (IPPROTO_ICMP == proto) {
//		check_result = openstack_firewall_icmp_check(ip, rule_p);
//	}
//	else if (IPPROTO_TCP == proto) {
//		check_result = openstack_firewall_tcp_check(ip, direction_num, rule_p);
//	}
//	else if (IPPROTO_UDP == proto) {
//		check_result = openstack_firewall_udp_check(ip, direction_num, rule_p);
//	}
//	else {
//		// do nothing
//	}

//	return check_result;
	return GN_OK;
}

INT4 openstack_firewall_main_check(p_fabric_host_node src_port, p_fabric_host_node dst_port, packet_in_info_t *packet_in)
{
//	printf("%s\n", FN);
//	ip_t *ip = (ip_t *)(packet_in->data);
//	INT4 check_result = GN_ERR;
//	UINT1 proto = ip->proto;
//	UINT4 direction_num = 0;
//	UINT4 remote_ip = 0;
//	openstack_port_p src_port_p = NULL;
//	openstack_port_p dst_port_p = NULL;
//	openstack_node_p src_firewall_node_p = NULL;
//	openstack_node_p dst_firewall_node_p = NULL;
//	openstack_firewall_p src_firewall_list = NULL;
//	openstack_firewall_p dst_firewall_list = NULL;
//	openstack_firewall_p remote_firewall_p_list = NULL;
//
//	if (NULL != src_port) {
//		src_port_p = (openstack_port_p)src_port->data;
//	}
//
//	if (NULL != dst_port) {
//		dst_port_p = (openstack_port_p)dst_port->data;
//	}
//
//	if (NULL != src_port_p) {
//		src_firewall_node_p = (openstack_node_p)src_port_p->security_data;
//	}
//
//	if (NULL != dst_port_p) {
//		src_firewall_node_p = (openstack_node_p)dst_port_p->security_data;
//	}

//	if (NULL != src_port_p) {
//		src_security_list = (openstack_security_p)src_port_p->security_data;
//	}
//
//	if (NULL != dst_port_p) {
//		dst_security_list = (openstack_security_p)dst_port_p->security_data;
//	}


//	while ((src_firewall_node_p) && (GN_ERR == check_result))
//	{
//		src_firewall_list = (openstack_security_p)src_firewall_node_p->data;
//		while (src_firewall_list)
//		{
//			openstack_security_rule_p rule_p = src_firewall_list->firewall_rule_p;
//			// printf("%s\n", src_security_list->security_group);
//			direction_num = 2;
//			remote_ip = ip->dest;
//			remote_firewall_p_list = dst_firewall_list;
//			check_result = openstack_firewall_rule_check(ip, rule_p, direction_num, remote_ip, remote_firewall_p_list, proto);
//			src_firewall_list = src_firewall_list->next;
//		}
//		src_firewall_node_p = src_firewall_node_p->next;
//	}



//    return check_result;
	return GN_OK;
}
