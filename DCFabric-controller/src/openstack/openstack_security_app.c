/*
 * openstack_security_app.c
 *
 *  Created on: Dec 7, 2015
 *      Author: yang
 */

#include "openstack_security_app.h"

enum DIRECTION {
	DIRECTION_IN = 1,
	DIRECTION_OUT = 2
};

INT4 openstack_security_protocol_check(UINT4 proto_num, char* proto)
{
	// // printf("%s\n", FN);("%s\n", FN);
	if ((IPPROTO_ICMP == proto_num) && (0 == strcmp("icmp", proto))) {
		return GN_OK;
	}
	else if ((IPPROTO_TCP == proto_num) && (0 == strcmp("tcp", proto))) {
		return GN_OK;
	}
	else if ((IPPROTO_UDP == proto_num) && (0 == strcmp("udp", proto))) {
		return GN_OK;
	}
	else {
		return GN_ERR;
	}
}

INT4 openstack_security_port_num_check(UINT4 port_num, UINT4 port_range_min, UINT4 port_range_max)
{
	// printf("%s\n", FN);
	// if port is in the range
	// printf("%d,%d,%d\n", port_num, port_range_min, port_range_max);
	if ((0 == port_range_min) && (0 == port_range_max))
	{
		return GN_OK;
	}

	if ((port_num >= port_range_min) && (port_num <= port_range_max))
	{
		return GN_OK;
	}

	return GN_ERR;
}

INT4 openstack_security_remote_group_check(openstack_node_p remote_security_p, char* remote_group_id)
{
	// printf("%s\n", FN);
	// if contains remote group is
	openstack_node_p node_p = remote_security_p;
	while (NULL != node_p) {
		openstack_security_p security_p = (openstack_security_p)remote_security_p->data;
		// printf("%s\n", security_p->security_group);
		if (NULL != security_p) {
			if (0 == strcmp(security_p->security_group, remote_group_id)) {
				return GN_OK;
			}
		}
		node_p = node_p->next;
	}

	return GN_ERR;
}

INT4 openstack_security_remote_cidr_check(UINT4 remote_ip, char* remote_ip_prefix)
{
	// printf("%s\n", FN);
	UINT4 cidr_ip = 0;
	UINT4 cidr_mask = 0;

	if (NULL == remote_ip_prefix) {
		return GN_ERR;
	}

	char buffer[48] = {0};
    char *token = NULL;
    memcpy(buffer, remote_ip_prefix, 48);
    // strcpy(buf, remote_ip_prefix);
    // buf = remote_ip_prefix;
    char* buf = buffer;
    // printf("remote ip prefix: %s", remote_ip_prefix);
    // printf("buf: %s\n", buf);
    INT4 count = 0;

    while((token = strsep(&buf, "/")) != NULL)
    {
    	if (0 == count) {
    		// printf("cidr ip: %s\n", token);
    		cidr_ip = ip2number(token);
    		// printf("%u\n", cidr_ip);
    	}
    	else {
    		// printf("mask is: %s\n", token);
    	    cidr_mask = (0 == strcmp("0", token)) ? 0:atoll(token);
    		// printf("%u\n", cidr_mask);
    	}
    	count++;
    }

    cidr_mask = (0 == cidr_mask) ? 0 : 0xffffffff<<(32-cidr_mask);
    remote_ip = ntohl((htonl(remote_ip) & cidr_mask));
   	// printf("cidrmas:%u remoteip:%d\n", cidr_mask, remote_ip);
	remote_ip = remote_ip & cidr_mask;
	// nat_show_ip(remote_ip);
	if (remote_ip == cidr_ip) {
		return GN_OK;
	}

	return GN_ERR;
}

INT4 openstack_security_ether_check(char* ether_type)
{
	// printf("%s\n", FN);
	// printf("ether type: %s\n", ether_type);
	if (0 == strcmp("IPv4", ether_type)) {
		return GN_OK;
	}
	else if (0 == strcmp("IPv6", ether_type)) {
		// TBD
		return GN_ERR;
	}
	else {
		return GN_ERR;
	}
}

INT4 openstack_security_check_direction(UINT1 direction_num, char* direction)
{
	// printf("%s\n", FN);
	UINT4 temp_direction = 0;
	// printf("%s->%d\n", direction, direction_num);

	if (0 == strcmp("ingress", direction)) {
		temp_direction = 1;
	}
	else if (0 == strcmp("egress", direction)) {
		temp_direction = 2;
	}
	else {
		// do nothing
	}

	if (temp_direction == direction_num) {
		return GN_OK;
	}
	else {
		return GN_ERR;
	}
}

INT4 openstack_security_icmp_check(ip_t *ip, openstack_security_rule_p rule_p)
{
	// printf("%s\n", FN);
	if ((NULL == ip) || (NULL == ip->data)) {
		return GN_ERR;
	}

	icmp_t* icmp = (icmp_t*)ip->data;
	// printf("%d,%d,%d,%d\n", icmp->type, icmp->code, rule_p->port_range_min, rule_p->port_range_max);

	if ((0 == rule_p->port_range_min) && (0 == rule_p->port_range_max))
		return GN_OK;

	if ((icmp->type == rule_p->port_range_min) && (icmp->code == rule_p->port_range_max))
		return GN_OK;

	return GN_ERR;
}

INT4 openstack_security_tcp_check(ip_t *ip, UINT4 direction, openstack_security_rule_p rule_p)
{
	// printf("%s\n", FN);
	if ((NULL == ip) || (NULL == ip->data)) {
		return GN_ERR;
	}

	tcp_t* tcp = (tcp_t*)ip->data;
	UINT2 port_num = 0;

	if (1 == direction) {
		port_num = ntohs(tcp->sport);
	}
	else {
		port_num = ntohs(tcp->dport);
	}

	return openstack_security_port_num_check(port_num, rule_p->port_range_min, rule_p->port_range_max);
}

INT4 openstack_security_udp_check(ip_t *ip, UINT4 direction, openstack_security_rule_p rule_p)
{
	// printf("%s\n", FN);
	if ((NULL == ip) || (NULL == ip->data)) {
		return GN_ERR;
	}

	udp_t* udp = (udp_t*)ip->data;
	UINT2 port_num = 0;

	if (1 == direction) {
		port_num = ntohs(udp->sport);
	}
	else {
		port_num = ntohs(udp->dport);
	}

	return openstack_security_port_num_check(port_num, rule_p->port_range_min, rule_p->port_range_max);
}

INT4 openstack_security_proto_check(INT4 proto, char* protocol)
{
	INT4 check_result = GN_ERR;
	// printf("%s->%d\n", protocol, proto);
	if ((IPPROTO_ICMP == proto) && (0 == strcmp("icmp", protocol))) {
			check_result = GN_OK;
	}
	else if ((IPPROTO_TCP == proto) && (0 == strcmp("tcp", protocol))) {
		check_result = GN_OK;
	}
	else if ((IPPROTO_UDP == proto) && (0 == strcmp("udp", protocol))) {
		check_result = GN_OK;
	}
	else if ((0 == strcmp("", protocol)) || (NULL == protocol)) {
		check_result = GN_OK;
	}
	else {
	}

	return check_result;
}

INT4 openstack_security_check_all_remote_null(char* remote_group_id, char* remote_ip_prefix)
{
	if ((0 == strcmp("", remote_group_id)) && (0 == strcmp("", remote_ip_prefix)))
	{
		return GN_OK;
	}
	else {
		return GN_ERR;
	}
}

INT4 openstack_security_group_rule_check(ip_t* ip, openstack_security_rule_p rule_p, UINT4 direction_num, UINT4 remote_ip, openstack_node_p remote_security_list, UINT1 proto)
{
	// printf("%s\n", FN);
	INT4 check_result = GN_ERR;

	if (GN_OK != openstack_security_ether_check(rule_p->ethertype)) {
		return GN_ERR;
	}

	if (GN_OK != openstack_security_check_direction(direction_num, rule_p->direction)) {
		return GN_ERR;
	}

	if (GN_OK != openstack_security_proto_check(proto, rule_p->protocol)) {
		return GN_ERR;
	}

	if (IPPROTO_ICMP == proto) {
		check_result = openstack_security_icmp_check(ip, rule_p);
	}
	else if (IPPROTO_TCP == proto) {
		check_result = openstack_security_tcp_check(ip, direction_num, rule_p);
	}
	else if (IPPROTO_UDP == proto) {
		check_result = openstack_security_udp_check(ip, direction_num, rule_p);
	}
	else {
		// do nothing
		return GN_ERR;
	}

	if (GN_OK == check_result) {
		if (GN_OK == openstack_security_remote_group_check(remote_security_list, rule_p->remote_group_id)) {
			return GN_OK;
		}

		if (GN_OK == openstack_security_remote_cidr_check(remote_ip, rule_p->remote_ip_prefix)) {
			return GN_OK;
		}

		if (GN_OK == openstack_security_check_all_remote_null(rule_p->remote_group_id, rule_p->remote_ip_prefix)) {
			return GN_OK;
		}
	}
	return GN_ERR;
}

void save_security_param(security_param_t* src_security, security_param_t* dst_security, UINT1 proto, UINT2 direction_num, ip_t * ip)
{
	src_security->ip_proto = proto;
	dst_security->ip_proto = proto;

	if (IPPROTO_ICMP == proto) {
		icmp_t* icmp = (icmp_t*)ip->data;
		if (DIRECTION_OUT == direction_num) {
			src_security->imcp_type = icmp->type;
			src_security->icmp_code = icmp->code;
		}
		else {
			dst_security->imcp_type = icmp->type;
			dst_security->icmp_code = icmp->code;
		}
	}
	else if (IPPROTO_TCP == proto) {
		tcp_t* tcp = (tcp_t*)ip->data;
		if (DIRECTION_OUT == direction_num) {
			src_security->tcp_port_num = ntohs(tcp->sport);
		}
		else {
			dst_security->tcp_port_num = ntohs(tcp->dport);
		}
	}
	else if (IPPROTO_UDP == proto) {
		udp_t* udp = (udp_t*)ip->data;
		if (DIRECTION_OUT == direction_num) {
			src_security->udp_port_num = ntohs(udp->sport);
		}
		else {
			dst_security->udp_port_num = ntohs(udp->dport);
		}
	}
}

UINT4 get_security_group_on_config()
{
	INT1 *value = NULL;
	UINT4 return_value = 0;
	value = get_value(g_controller_configure, "[openvstack_conf]", "security_group_on");
	return_value = ((NULL == value) ? 0 : atoll(value));
	return return_value;
}


INT4 openstack_security_group_main_check(p_fabric_host_node src_port, p_fabric_host_node dst_port,
		packet_in_info_t *packet_in, security_param_t* src_security, security_param_t* dst_security)
{
	// printf("%s\n", FN);
	// openstack_show_all_port_security();

	// nat_show_ip(src_port->ip_list[0]);
	if (0 == get_security_group_on_config()) {
		return GN_OK;
	}

	ip_t *ip = (ip_t *)(packet_in->data);
	INT4 check_result = GN_ERR;
	UINT1 proto = ip->proto;
	UINT4 direction_num = 0;
	openstack_port_p src_port_p = NULL;
	openstack_port_p dst_port_p = NULL;
	openstack_node_p src_security_node_p = NULL;
	openstack_node_p dst_security_node_p = NULL;

	// check external network
	if ((NULL == src_port) || (OPENSTACK_PORT_TYPE_HOST != src_port->type) 
		|| (NULL == dst_port) || (OPENSTACK_PORT_TYPE_HOST != dst_port->type)) {
		return GN_OK;
	}

	if (NULL != src_port) {
		src_port_p = (openstack_port_p)src_port->data;
	}
	if (NULL != dst_port) {
		dst_port_p = (openstack_port_p)dst_port->data;
	}
	if (NULL != src_port_p) {
		src_security_node_p = (openstack_node_p)src_port_p->security_data;
	}
	if (NULL != dst_port_p) {
		dst_security_node_p = (openstack_node_p)dst_port_p->security_data;
	}

	while (NULL != src_security_node_p)
	{
		openstack_security_p security_p = (openstack_security_p)src_security_node_p->data;
		if (NULL != security_p)
		{
			openstack_security_rule_p rule_p = (openstack_security_rule_p)security_p->security_rule_p;
			direction_num = DIRECTION_OUT;

			while (NULL != rule_p) {
				INT4 result = openstack_security_group_rule_check(ip, rule_p, direction_num, ip->dest, dst_security_node_p, proto);

				if (GN_OK == result) {
					save_security_param(src_security, dst_security, proto, direction_num, ip);
					return GN_OK;
				}
				rule_p = rule_p->next;
			}
		}
		src_security_node_p = src_security_node_p->next;
	}

	while (NULL != dst_security_node_p)
	{
		openstack_security_p security_p = (openstack_security_p)dst_security_node_p->data;
		if (NULL != security_p)
		{
			openstack_security_rule_p rule_p = (openstack_security_rule_p)security_p->security_rule_p;
			direction_num = DIRECTION_IN;

			while (NULL != rule_p) {
				INT4 result = openstack_security_group_rule_check(ip, rule_p, direction_num, ip->src, src_security_node_p, proto);

				if (GN_OK == result) {
					save_security_param(src_security, dst_security, proto, direction_num,ip);
					return GN_OK;
				}
				rule_p = rule_p->next;
			}
		}
		dst_security_node_p = dst_security_node_p->next;
	}

    return check_result;
}
