#include "mem_pool.h"

#include "openstack_security_app.h"
#include "../restful-svr/openstack-server.h"
#include "openstack_app.h"
#include "openstack_firewall_app.h"
#include "common.h"
#include "fabric_openstack_gateway.h"

extern UINT4 g_openstack_on ;
extern char g_openstack_ip[16];
extern UINT4 g_openstack_port ;

//by:yhy group链表
openstack_security_p g_openstack_security_list = NULL;
openstack_security_p g_openstack_security_list_temp = NULL;
//by:yhy 每个虚机的安全组信息内存池
void *g_openstack_host_security_id 	= NULL;
//by:yhy group内存池
void *g_openstack_security_group_id = NULL;
void *g_openstack_security_group_id_temp = NULL;
//by:yhy rule内存池
void *g_openstack_security_rule_id = NULL;
//by:Hongyu Yang 首次更新安全组信息标志
UINT1 FirstTime_GetSecurityInfo =1;

enum DIRECTION {
	DIRECTION_IN = 1,
	DIRECTION_OUT = 2
};


INT1 Clear_g_openstack_security_list(void);

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
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_protocol_check -- Finall return GN_ERR");
		return GN_ERR;
	}
}
//by:yhy 检查  port_num是否在port_range_min和port_range_max内
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

	if (NULL == remote_ip_prefix) 
	{
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_remote_cidr_check -- NULL == remote_ip_prefix return GN_ERR");
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
	
	//by:yhy add 201701051305
	LOG_PROC("ERROR", "openstack_security_remote_cidr_check -- Finall return GN_ERR");
		
	return GN_ERR;
}
//by:yhy 如果ether_type==IPv4则返回OK,其他返回ERR
INT4 openstack_security_ether_check(char* ether_type)
{
	// printf("%s\n", FN);
	// printf("ether type: %s\n", ether_type);
	if (0 == strcmp("IPv4", ether_type)) 
	{
		return GN_OK;
	}
	else if (0 == strcmp("IPv6", ether_type)) 
	{
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_ether_check -- 0 == strcmp(IPv6, ether_type) return GN_ERR");
		// TBD
		return GN_ERR;
	}
	else 
	{
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_ether_check -- Finall return GN_ERR");
		return GN_ERR;
	}
}
//by:yhy 如果direction_num与direction表达的内容一致则返回OK其他返回ERR
INT4 openstack_security_check_direction(UINT1 direction_num, char* direction)
{
	// printf("%s\n", FN);
	UINT4 temp_direction = 0;
	// printf("%s->%d\n", direction, direction_num);

	if (0 == strcmp("ingress", direction)) 
	{
		temp_direction = 1;
	}
	else if (0 == strcmp("egress", direction)) 
	{
		temp_direction = 2;
	}
	else 
	{
		// do nothing
	}

	if (temp_direction == direction_num) 
	{
		return GN_OK;
	}
	else 
	{
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_check_direction -- Finall return GN_ERR");
		return GN_ERR;
	}
}
//by:yhy icmp检查
INT4 openstack_security_icmp_check(ip_t *ip, openstack_security_rule_p rule_p)
{
	// printf("%s\n", FN);
	if ((NULL == ip) || (NULL == ip->data))
	{
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_icmp_check -- (NULL == ip) || (NULL == ip->data) return GN_ERR");
		return GN_ERR;
	}

	icmp_t* icmp = (icmp_t*)ip->data;
	// printf("%d,%d,%d,%d\n", icmp->type, icmp->code, rule_p->port_range_min, rule_p->port_range_max);

	if ((0 == rule_p->port_range_min) && (0 == rule_p->port_range_max))
	{
		return GN_OK;
	}
	if ((icmp->type == rule_p->port_range_min) && (icmp->code == rule_p->port_range_max))
	{
		return GN_OK;
	}

	//by:yhy add 201701051305
	LOG_PROC("ERROR", "openstack_security_icmp_check -- Finall return GN_ERR");
	return GN_ERR;
}
//by:yhy tcp检查
INT4 openstack_security_tcp_check(ip_t *ip, UINT4 direction, openstack_security_rule_p rule_p)
{
	// printf("%s\n", FN);
	if ((NULL == ip) || (NULL == ip->data)) 
	{
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_tcp_check -- (NULL == ip) || (NULL == ip->data) return GN_ERR");
		return GN_ERR;
	}

	tcp_t* tcp = (tcp_t*)ip->data;
	UINT2 port_num = 0;

	if (1 == direction) 
	{
		port_num = ntohs(tcp->sport);
	}
	else 
	{
		port_num = ntohs(tcp->dport);
	}

	return openstack_security_port_num_check(port_num, rule_p->port_range_min, rule_p->port_range_max);
}
//by:yhy udp检查
INT4 openstack_security_udp_check(ip_t *ip, UINT4 direction, openstack_security_rule_p rule_p)
{
	// printf("%s\n", FN);
	if ((NULL == ip) || (NULL == ip->data)) 
	{
		//by:yhy add 201701051305
		LOG_PROC("ERROR", "openstack_security_udp_check -- (NULL == ip) || (NULL == ip->data) return GN_ERR");
		
		return GN_ERR;
	}

	udp_t* udp = (udp_t*)ip->data;
	UINT2 port_num = 0;

	if (1 == direction) 
	{
		port_num = ntohs(udp->sport);
	}
	else 
	{
		port_num = ntohs(udp->dport);
	}

	return openstack_security_port_num_check(port_num, rule_p->port_range_min, rule_p->port_range_max);
}
//by:yhy 判断proto与protocol表述内容是否一致
INT4 openstack_security_proto_check(INT4 proto, char* protocol)
{
	INT4 check_result = GN_ERR;
	// printf("%s->%d\n", protocol, proto);
	if ((IPPROTO_ICMP == proto) && (0 == strcmp("icmp", protocol))) 
	{
			check_result = GN_OK;
	}
	else if ((IPPROTO_TCP == proto) && (0 == strcmp("tcp", protocol))) 
	{
		check_result = GN_OK;
	}
	else if ((IPPROTO_UDP == proto) && (0 == strcmp("udp", protocol))) 
	{
		check_result = GN_OK;
	}
	else if ((0 == strcmp("", protocol)) || (NULL == protocol)) 
	{
		check_result = GN_OK;
	}
	else 
	{
	}

	return check_result;
}

INT4 openstack_security_check_all_remote_null(char* remote_group_id, char* remote_ip_prefix)
{
	if ((0 == strcmp("", remote_group_id)) && (0 == strcmp("", remote_ip_prefix)))
	{
		return GN_OK;
	}
	else 
	{
		//by:yhy add 201701051407
		LOG_PROC("ERROR", "openstack_security_check_all_remote_null -- Finall return GN_ERR");
		return GN_ERR;
	}
}
//by:yhy 安全组检查
INT4 openstack_security_group_rule_check(ip_t* ip, openstack_security_rule_p rule_p, UINT4 direction_num, UINT4 remote_ip, openstack_node_p remote_security_list, UINT1 proto)
{
	// printf("%s\n", FN);
	INT4 check_result = GN_ERR;

	if (GN_OK != openstack_security_ether_check(rule_p->ethertype)) 
	{
		//by:yhy add 201701051407
		LOG_PROC("ERROR", "openstack_security_group_rule_check -- openstack_security_ether_check(rule_p->ethertype) return GN_ERR");
		return GN_ERR;
	}

	if (GN_OK != openstack_security_check_direction(direction_num, rule_p->direction)) 
	{
		//by:yhy add 201701051407
		LOG_PROC("ERROR", "openstack_security_group_rule_check -- openstack_security_check_direction(direction_num, rule_p->direction) return GN_ERR");
		
		return GN_ERR;
	}

	if (GN_OK != openstack_security_proto_check(proto, rule_p->protocol)) 
	{
		//by:yhy add 201701051407
		LOG_PROC("ERROR", "openstack_security_group_rule_check -- openstack_security_proto_check(proto, rule_p->protocol) return GN_ERR");
		
		return GN_ERR;
	}

	if (IPPROTO_ICMP == proto) 
	{
		check_result = openstack_security_icmp_check(ip, rule_p);
	}
	else if (IPPROTO_TCP == proto) 
	{
		check_result = openstack_security_tcp_check(ip, direction_num, rule_p);
	}
	else if (IPPROTO_UDP == proto) 
	{
		check_result = openstack_security_udp_check(ip, direction_num, rule_p);
	}
	else 
	{
		// do nothing
		
		//by:yhy add 201701051407
		LOG_PROC("ERROR", "openstack_security_group_rule_check -- proto return GN_ERR");
		
		return GN_ERR;
	}

	if (GN_OK == check_result)
	{
		if (GN_OK == openstack_security_remote_group_check(remote_security_list, rule_p->remote_group_id))
		{
			return GN_OK;
		}

		if (GN_OK == openstack_security_remote_cidr_check(remote_ip, rule_p->remote_ip_prefix)) 
		{
			return GN_OK;
		}

		if (GN_OK == openstack_security_check_all_remote_null(rule_p->remote_group_id, rule_p->remote_ip_prefix)) 
		{
			return GN_OK;
		}
	}
	//by:yhy add 201701051407
	LOG_PROC("ERROR", "openstack_security_group_rule_check -- Finall return GN_ERR");
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
//by:yhy 返回配置文件中的security_group_on
UINT4 get_security_group_on_config()
{
	INT1 *value = NULL;
	UINT4 return_value = 0;
	value = get_value(g_controller_configure, "[openvstack_conf]", "security_group_on");
	return_value = ((NULL == value) ? 0 : atoll(value));
	return return_value;
}

//by:yhy why? 安全组检查
INT4 openstack_security_group_main_check(p_fabric_host_node src_port, p_fabric_host_node dst_port,packet_in_info_t *packet_in, security_param_t* src_security, security_param_t* dst_security)
{
	// printf("%s\n", FN);
	// openstack_show_all_port_security();

	// nat_show_ip(src_port->ip_list[0]);
	if (0 == get_security_group_on_config()) 
	{
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
	
	INT4 SECURITY_UNUSED = 0;
	INT4 SECURITY_USED   = 1;
	
	
	INT4 srcPort_Security_USING = SECURITY_UNUSED;
	INT4 dstPort_Security_USING = SECURITY_UNUSED;
	INT4 src_SecurityCheck_result = GN_ERR;
	INT4 dst_SecurityCheck_result = GN_ERR;

	// check external network
	if ((NULL == src_port) || (OPENSTACK_PORT_TYPE_HOST != src_port->type) || (NULL == dst_port) || (OPENSTACK_PORT_TYPE_HOST != dst_port->type)) 
	{
		return GN_OK;
	}

	if (NULL != src_port) 
	{
		src_port_p = (openstack_port_p)src_port->data;
	}
	if (NULL != dst_port) 
	{
		dst_port_p = (openstack_port_p)dst_port->data;
	}
	if (NULL != src_port_p) 
	{
		src_security_node_p = (openstack_node_p)src_port_p->security_data;
	}
	if (NULL != dst_port_p) 
	{
		dst_security_node_p = (openstack_node_p)dst_port_p->security_data;
	}

	while (NULL != src_security_node_p)
	{
		srcPort_Security_USING = SECURITY_USED; 
		openstack_security_p security_p = (openstack_security_p)src_security_node_p->data;
		if (NULL != security_p)
		{
			openstack_security_rule_p rule_p = (openstack_security_rule_p)security_p->security_rule_p;
			direction_num = DIRECTION_OUT;

			while (NULL != rule_p) 
			{
				INT4 result = openstack_security_group_rule_check(ip, rule_p, direction_num, ip->dest, dst_security_node_p, proto);

				if (GN_OK == result)
				{
					save_security_param(src_security, dst_security, proto, direction_num, ip);
					src_SecurityCheck_result = GN_OK;
				}
				rule_p = rule_p->next;
			}
		}
		src_security_node_p = src_security_node_p->next;
	}

	while (NULL != dst_security_node_p)
	{
		dstPort_Security_USING = SECURITY_USED; 
		openstack_security_p security_p = (openstack_security_p)dst_security_node_p->data;
		if (NULL != security_p)
		{
			openstack_security_rule_p rule_p = (openstack_security_rule_p)security_p->security_rule_p;
			direction_num = DIRECTION_IN;

			while (NULL != rule_p) 
			{
				INT4 result = openstack_security_group_rule_check(ip, rule_p, direction_num, ip->src, src_security_node_p, proto);

				if (GN_OK == result) 
				{
					save_security_param(src_security, dst_security, proto, direction_num,ip);
					dst_SecurityCheck_result = GN_OK;
				}
				rule_p = rule_p->next;
			}
		}
		dst_security_node_p = dst_security_node_p->next;
	}

	if((SECURITY_USED == srcPort_Security_USING)&&(SECURITY_USED == dstPort_Security_USING))
	{
		if((GN_OK ==src_SecurityCheck_result)&&(GN_OK ==dst_SecurityCheck_result))
		{
			return GN_OK;
		}
		else
		{
			return GN_ERR;
		}
	}
	else if((SECURITY_USED == srcPort_Security_USING)&&(SECURITY_USED != dstPort_Security_USING))
	{
		return GN_ERR;
	}
	else if((SECURITY_USED != srcPort_Security_USING)&&(SECURITY_USED == dstPort_Security_USING))
	{
		return GN_ERR;
	}
	else if((SECURITY_USED != srcPort_Security_USING)&&(SECURITY_USED != dstPort_Security_USING))
	{
		return GN_ERR;
	}
	else
	{
		return GN_ERR;
	}
}


/*********************************************************************************************************************************/





//根据DeletedSecurityRule对sw下发阻断流表
void GenerateAndInstall_DenyFlow_by_DeletedSecurityRule (p_fabric_host_node hostNode_p,gn_switch_t* sw, openstack_security_rule_p DeletedSecurityRule ,UINT1* SrcMAC_Used)
{
	/*2017/08/01by:yhy
	char  Rule_RemoteIPPrefix_Copy[OPENSTACK_SECURITY_GROUP_LEN] ={0};
	char * IPPrefix_HeadPointer =NULL;
	char  IPAddr_STR[OPENSTACK_SECURITY_GROUP_LEN] ={0};
	char  IPPrefix_STR[OPENSTACK_SECURITY_GROUP_LEN] ={0};
	UINT4 IP_Val 	=0;
	UINT4 IP_Prefix =0;
	UINT4 Temp_Port =0;
	UINT2 security_drop_idle_timeout =SECURITY_DROP_IDLE_TIMEOUT;
	UINT2 security_drop_hard_timeout =SECURITY_DROP_HARD_TIMEOUT;
	//LOG_PROC("INFO", "111111:%s---------------------------------------",FN);
	flow_param_t* flow_param = init_flow_param();
	
	if((sw != NULL)&&(DeletedSecurityRule != NULL)) 
	{
		
		if(0 == strlen(DeletedSecurityRule->protocol))
		{//协议为空,全网deny
			//TBD//////////////
			GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(hostNode_p,sw,DIRECTION_IN,SrcMAC_Used);
			GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(hostNode_p,sw,DIRECTION_OUT,SrcMAC_Used);
		}
		else
		{//存在协议区分
			memcpy(Rule_RemoteIPPrefix_Copy,DeletedSecurityRule->remote_ip_prefix,OPENSTACK_SECURITY_GROUP_LEN);
			//LOG_PROC("INFO", "TEST------%s-%s-------------------",FN,Rule_RemoteIPPrefix_Copy);
			
			if(strlen(Rule_RemoteIPPrefix_Copy)>=9)
			{//IP地址有效
				
				IPPrefix_HeadPointer =strchr(Rule_RemoteIPPrefix_Copy,'/');
				strncpy(IPAddr_STR,Rule_RemoteIPPrefix_Copy,(strlen(Rule_RemoteIPPrefix_Copy)-strlen(IPPrefix_HeadPointer)));
				strncpy(IPPrefix_STR,(IPPrefix_HeadPointer+1),(strlen(IPPrefix_HeadPointer)-1));
				
				IP_Val=inet_addr(IPAddr_STR);
				IP_Prefix =atoi(IPPrefix_STR);
				
				flow_param->match_param->eth_type = ETHER_IP;
				memcpy(flow_param->match_param->eth_src, hostNode_p->mac, 6);
				
				if(0 == strcmp("icmp", DeletedSecurityRule->protocol))
				{//ICMP
					//LOG_PROC("INFO", "TEST------%s--------------------",FN);
					flow_param->match_param->ip_proto = IPPROTO_ICMP;

					if(0 == strcmp("ingress", DeletedSecurityRule->direction))
					{//入口
						flow_param->match_param->ipv4_src = IP_Val;
						//flow_param->match_param->ipv4_src_prefix =IP_Prefix;
					}
					else if(0 == strcmp("egress", DeletedSecurityRule->direction))
					{//出口
						flow_param->match_param->ipv4_dst = IP_Val;
						//flow_param->match_param->ipv4_dst_prefix =IP_Prefix;
					}
					else
					{//错误
						return;
					}
					//LOG_PROC("INFO", "TEST------%s--------------------",FN);
					if((DeletedSecurityRule->port_range_min !=0)&&(DeletedSecurityRule->port_range_max !=0))
					{
						flow_param->match_param->icmpv4_type = DeletedSecurityRule->port_range_min;
						flow_param->match_param->icmpv4_code = DeletedSecurityRule->port_range_max;
					}
					add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
					/////////改参数
					install_fabric_flows(sw, 
										 security_drop_idle_timeout, 
										 security_drop_hard_timeout, 
										 FABRIC_PRIORITY_DENY_FLOW,
										 FABRIC_INPUT_TABLE, 
										 OFPFC_ADD, 
										 flow_param);

					clear_flow_param(flow_param);
					return;
				}
				else if(0 == strcmp("tcp", DeletedSecurityRule->protocol))
				{//TCP
					
					flow_param->match_param->ip_proto = IPPROTO_TCP;
					if(0 == strcmp("ingress", DeletedSecurityRule->direction))
					{//入口
						flow_param->match_param->ipv4_src = ntohl(IP_Val);
						//flow_param->match_param->ipv4_src_prefix =IP_Prefix;
						
						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->tcp_src =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							//LOG_PROC("INFO", "TEST------%d-%d-------------------",DeletedSecurityRule->port_range_max,DeletedSecurityRule->port_range_min);
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								//LOG_PROC("INFO", "TEST------%d-------------------",Temp_Port);
								flow_param->match_param->tcp_src = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else if(0 == strcmp("egress", DeletedSecurityRule->direction))
					{//出口
						flow_param->match_param->ipv4_dst = ntohl(IP_Val);
						//flow_param->match_param->ipv4_dst_prefix =IP_Prefix;

						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->tcp_dst =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								flow_param->match_param->tcp_dst = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else
					{//错误//不处理
						if(0 == strcmp("ingress", DeletedSecurityRule->direction))
						{//入口
							flow_param->match_param->ipv4_src = IP_Val;
						}
						else if(0 == strcmp("egress", DeletedSecurityRule->direction))
						{//出口
							flow_param->match_param->ipv4_dst = IP_Val;
						}
						else
						{//错误
							return;
						}
						
						add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
						/////////改参数
						install_fabric_flows(sw, 
											 security_drop_idle_timeout, 
											 security_drop_hard_timeout, 
											 FABRIC_PRIORITY_DENY_FLOW,
											 FABRIC_INPUT_TABLE, 
											 OFPFC_ADD, 
											 flow_param);

						clear_flow_param(flow_param);
						return;
					}	
				}
				else if(0 == strcmp("udp", DeletedSecurityRule->protocol))
				{//UDP
					flow_param->match_param->ip_proto = IPPROTO_UDP;
					if(0 == strcmp("ingress", DeletedSecurityRule->direction))
					{//入口
						flow_param->match_param->ipv4_src = ntohl(IP_Val);
						//flow_param->match_param->ipv4_src_prefix =IP_Prefix;

						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->udp_src =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								flow_param->match_param->udp_src = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else if(0 == strcmp("egress", DeletedSecurityRule->direction))
					{//出口
						flow_param->match_param->ipv4_dst = ntohl(IP_Val);
						//flow_param->match_param->ipv4_dst_prefix =IP_Prefix;

						if((DeletedSecurityRule->port_range_max)==(DeletedSecurityRule->port_range_min))
						{//最大端口号与最小端口号一致
							if(0==(DeletedSecurityRule->port_range_max))
							{//端口为0//不对端口做限制
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
							else
							{//只有一个端口//下发一个deny
								flow_param->match_param->udp_dst =DeletedSecurityRule->port_range_min;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);

								clear_flow_param(flow_param);
								return ;
							}
						}
						else
						{//最大端口号与最小端口号不一致
							for(Temp_Port = DeletedSecurityRule->port_range_min ; Temp_Port <=DeletedSecurityRule->port_range_max; Temp_Port++)
							{
								flow_param->match_param->udp_dst = Temp_Port;
								add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
								install_fabric_flows(sw, 
													 security_drop_idle_timeout, 
													 security_drop_hard_timeout, 
													 FABRIC_PRIORITY_DENY_FLOW,
													 FABRIC_INPUT_TABLE, 
													 OFPFC_ADD, 
													 flow_param);
							}
							clear_flow_param(flow_param);
							return ;
						}
					}
					else
					{//错误//不处理
						return;
					}
				}
				else
				{//其他协议不支持//不处理
					return;
				}
			}
			else
			{//IP为null
				////////////不处理
			}			
		}
	}
	*/
}



//by:yhy 在g_openstack_security_list查找名为security_group的项,若找不到则新建;返回同名项的指针
openstack_security_p update_openstack_security_group(char* security_group)
{
	openstack_security_p security_p = g_openstack_security_list;
	while (security_p) 
	{
		if (0 == strcmp(security_p->security_group, security_group)) 
		{
			return security_p;
		}
		security_p = security_p->next;
	}
	// create
	security_p = (openstack_security_p)mem_get(g_openstack_security_group_id);
	if (NULL != security_p) 
	{
		//LOG_PROC("INFO", "%s_%d:g_openstack_security_group_id  %p",FN,LN,security_p);
		memset(security_p->security_group, 0, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(security_p->security_group, security_group, OPENSTACK_SECURITY_GROUP_LEN);
		security_p->next = g_openstack_security_list;
		g_openstack_security_list = security_p;
	}
	else 
	{
		LOG_PROC("ERROR", "%s:Security: Get memeory fail!",FN);
	}
	return security_p;
}




//对本host(port)进行全阻断
void GenerateAndInstall_DenyAllFlow_bec_NoSecurityGroup(p_fabric_host_node hostNode_p,gn_switch_t* sw,INT1 Direction,UINT1* SrcMAC_Used)
{
	/*
	UINT2 security_drop_idle_timeout =SECURITY_DROP_IDLE_TIMEOUT;
	UINT2 security_drop_hard_timeout =SECURITY_DROP_HARD_TIMEOUT;
	//LOG_PROC("INFO", "222222:%s---------------------------------------",FN);
	if(sw != NULL) 
	{
		flow_param_t* flow_param = init_flow_param();
		
		flow_param->match_param->eth_type = ETHER_IP;

		if(Direction == DIRECTION_IN)
		{//阻断进
			if (hostNode_p->mac)
			{
				memcpy(flow_param->match_param->eth_src, (hostNode_p->mac), 6);
				(* SrcMAC_Used) =SRC_MAC_USED;
				add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
				install_fabric_flows(sw, 
									 security_drop_idle_timeout, 
									 security_drop_hard_timeout, 
									 FABRIC_PRIORITY_DENY_FLOW,
									 FABRIC_INPUT_TABLE, 
									 OFPFC_ADD, 
									 flow_param);

				clear_flow_param(flow_param);
			}		
		}
		else if(Direction == DIRECTION_OUT)
		{//阻断出
			if (hostNode_p->mac)
			{
				memcpy(flow_param->match_param->eth_dst, (hostNode_p->mac), 6);
				add_action_param(&flow_param->instruction_param, OFPIT_APPLY_ACTIONS, NULL);
				install_fabric_flows(sw, 
									 security_drop_idle_timeout, 
									 security_drop_hard_timeout, 
									 FABRIC_PRIORITY_DENY_FLOW,
									 FABRIC_INPUT_TABLE, 
									 OFPFC_ADD, 
									 flow_param);

				clear_flow_param(flow_param);
			}
		}
		else
		{//其他//不处理
		}
	}
	*/
}





openstack_security_p update_openstack_security_group_temp(char* security_group)
{
	openstack_security_p security_p = g_openstack_security_list_temp;
	while (security_p) 
	{
		if (0 == strcmp(security_p->security_group, security_group)) 
		{
			return security_p;
		}
		security_p = security_p->next;
	}
	// create
	security_p = (openstack_security_p)mem_get(g_openstack_security_group_id_temp);
	if (NULL != security_p) 
	{
		memset(security_p->security_group, 0, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(security_p->security_group, security_group, OPENSTACK_SECURITY_GROUP_LEN);
		security_p->next = g_openstack_security_list_temp;
		g_openstack_security_list_temp = security_p;
	}
	else 
	{
		LOG_PROC("ERROR", "%s:Security: Get memory fail!",FN);
	}
	return security_p;
}





void clear_all_security_group_info()
{
}








/********************************************************************************************************************************************************************************************************/
/* by:HongyuYang
 * 刷新security_group 信息
 */
INT4 reload_security_group_info()
{
	INT4 iRet = GN_ERR;
	INT1 *value = NULL;
	value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_on");
	g_openstack_on = (NULL == value)?0:atoll(value);
	if( 1 == g_openstack_on)
	{
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_ip");
		(NULL == value)?strncpy(g_openstack_ip, "192.168.52.200", (16 - 1)) : strncpy(g_openstack_ip, value, (16 - 1));
		value = get_value(g_controller_configure, "[openvstack_conf]", "openvstack_port");
		g_openstack_port = ((NULL == value) ? 9696 : atoll(value));
		
		reset_openstack_router_outerinterface_checkflag();
		iRet = getOpenstackInfo(g_openstack_ip, "/v2.0/ports", g_openstack_port, "port", NULL, EOPENSTACK_GET_NORMAL);
		if(GN_OK == iRet)
		{
			remove_openstack_router_outerinterface_uncheck();
		}
		if(get_security_group_on_config())	
		{
			Clear_g_openstack_security_list();
			iRet = getOpenstackInfo(g_openstack_ip, "/v2.0/security-group-rules", g_openstack_port, "security-group-rules" , NULL, EOPENSTACK_GET_NORMAL);
			//iRet += getOpenstackInfo(g_openstack_ip, "/v2.0/ports", g_openstack_port, "port", NULL, EOPENSTACK_GET_NORMAL);
			if(GN_OK == iRet)
			{
				if(!FirstTime_GetSecurityInfo)
				{
					openstack_firewall_GenerateFirewallPolicy();
					Clear_g_openstack_security_list();
					//fabric_firewall_ShowAllTempPolicy();
					//fabric_firewall_ShowAllPersistPolicy();
					fabric_firewall_Compare_policyList(&G_firewall_temp_policy_list,&G_firewall_persist_policy_list);	
					//fabric_firewall_ShowAllPersistPolicy();
					fabric_firewall_RefreshFirewallFlow();
					fabric_firewall_D_policy_list(&G_firewall_temp_policy_list);
				}
				else
				{
					FirstTime_GetSecurityInfo =0;
				}
			}
		}
		else
		{
			UINT2 i =0;
			gn_switch_t *sw = NULL;
			for(i = 0; i < g_server.max_switch; i++)
			{
				sw =&(g_server.switches[i]);
				if (sw&&(CONNECTED == sw->conn_state)) 
				{
					install_add_FirewallIn_functionOff_flow(sw);
					install_add_FirewallOut_functionOff_flow(sw);
				}
			}
		}
	} 
	return iRet;
}

/* by:Hongyu Yang
 * 更新安全组
 */
openstack_security_rule_p update_security_rule(char* group_id, char* rule_id, char* direction, char* ethertype, char* port_range_max,char* port_range_min, char* protocol, char* remote_group_id, char* remote_ip_prefix, char* tenant_id,UINT2 priority,UINT1 enabled)
{
	openstack_security_rule_p rule_p = create_security_rule(group_id, rule_id, direction, ethertype, port_range_max, port_range_min, protocol,remote_group_id, remote_ip_prefix, tenant_id,priority,enabled);
	if (rule_p) 
	{
		rule_p->check_status = (UINT2)CHECK_CREATE;
		add_security_rule_into_group(group_id, rule_p);
	}
	return rule_p;
}

/* by:Hongyu Yang
 * 获取规则条目空间,创建一条规则
 */
openstack_security_rule_p create_security_rule(char* group_id, char* rule_id, char* direction, char* ethertype, char* port_range_max,char* port_range_min, char* protocol, char* remote_group_id, char* remote_ip_prefix, char* tenant_id,UINT2 priority,UINT1 enabled)
{
	openstack_security_rule_p rule_p = mem_get(g_openstack_security_rule_id);
	//LOG_PROC("INFO", "%s_%d:g_openstack_security_rule_id  %p",FN,LN,g_openstack_security_rule_id);
	if (NULL != rule_p) 
	{
		memcpy(rule_p->direction, direction, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->ethertype, ethertype, OPENSTACK_SECURITY_GROUP_LEN);
		rule_p->port_range_max = (0 == strcmp(port_range_max, "")) ? 0:atoi(port_range_max);
		rule_p->port_range_min = (0 == strcmp(port_range_min, "")) ? 0:atoi(port_range_min);
		memcpy(rule_p->protocol, protocol, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->remote_group_id, remote_group_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->remote_ip_prefix, remote_ip_prefix, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->tenant_id, tenant_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->rule_id, rule_id, OPENSTACK_SECURITY_GROUP_LEN);
		memcpy(rule_p->group_id, group_id, OPENSTACK_SECURITY_GROUP_LEN);
		rule_p->priority = priority;
		rule_p->enabled = enabled;
		rule_p->next =NULL;
	}
	return rule_p;
}

/* by:Hongyu Yang
 * 将规则条目加入安全组
 */
openstack_security_p add_security_rule_into_group(char* group_id, openstack_security_rule_p rule_p)
{
	openstack_security_p security_p = g_openstack_security_list;
	//遍历现有安全组,找到对应项,添加规则条目
	while (NULL !=	security_p) 
	{
		if (0 == strcmp(group_id, security_p->security_group))
		{
			rule_p->next = security_p->security_rule_p;
			security_p->security_rule_p = rule_p;
			return security_p;
		}
		security_p = security_p->next;
	}
	//新建安全组
	if (NULL == security_p) 
	{
		security_p = update_openstack_security_group(group_id);
		if (security_p) 
		{
			rule_p->next = security_p->security_rule_p;
			security_p->security_rule_p = rule_p;
			return security_p;
		}
	}
	
	return NULL;
}

/* by:Hongyu Yang
 * 根据安全组名称找到对应安全组
 */
openstack_security_p find_security_group_into_SecurityGroupList (char* SecurityGroup_id)
{
	openstack_security_p security_p = g_openstack_security_list;
	while (NULL !=	security_p) 
	{
		if (0 == strcmp(SecurityGroup_id, security_p->security_group))
		{
			return security_p;
		}
		security_p = security_p->next;
	}
	return NULL;
}

/* by:Hongyu Yang
 * 把data加到head_p链上,并将head_p链返回(在安全组里,这里把安全组的组名指针存成一条链)
 */
openstack_node_p add_openstack_host_security_node(UINT1* data, openstack_node_p head_p)
{
	openstack_node_p node_p = head_p;
	if (NULL == data) 
	{
		return head_p;
	}
	while (node_p) 
	{//查找是否已经存在
		if (0==strcmp( data,(node_p->data) ) )
		{
			return head_p;
		}
		node_p = node_p->next;
	}
	openstack_node_p ret = NULL;
	ret = (openstack_node_p)mem_get(g_openstack_host_security_id);
	if (NULL == ret) 
	{
		LOG_PROC("ERROR", "%s_%d:Security node: Get memeory fail!",FN,LN);
		return head_p;
	}
	memset(ret, 0, sizeof(openstack_node));
	ret->data = data;
	ret->next = head_p;
	return ret;
}

/* by:Hongyu Yang
 * 删除head_p链上的所有安全组节点,回收内存
 */
void clear_openstack_host_security_node(UINT1* head_p)
{	
	openstack_node_p node_p = (openstack_node_p)head_p;
	if (NULL == node_p)
	{
		return;
	}
	openstack_node_p temp_p = node_p->next;

	while (temp_p) 
	{
		mem_free(g_openstack_host_security_id, node_p);
		node_p = temp_p;
		temp_p = temp_p->next;
	}
	mem_free(g_openstack_host_security_id, node_p);
}




/* by:Hongyu Yang
 * 释放持久化的安全组,以及组内的规则
 */
INT1 Clear_g_openstack_security_list(void)
{
	openstack_security_p temp_node =NULL;
	openstack_security_rule_p Temp_NeedDeletedSecurityRule =NULL;
	//LOG_PROC("INFO","%s_%d",FN,LN);
	//LOG_PROC("INFO", "%s_%d:g_openstack_security_group_id  %p",FN,LN,g_openstack_security_group_id);
	//LOG_PROC("INFO", "%s_%d:g_openstack_security_rule_id  %p",FN,LN,g_openstack_security_rule_id);
	while(g_openstack_security_list)
	{
		temp_node = g_openstack_security_list;
		g_openstack_security_list =g_openstack_security_list->next;
		Temp_NeedDeletedSecurityRule = temp_node->security_rule_p;
		
		while(Temp_NeedDeletedSecurityRule)
		{
			openstack_security_rule_p ReadyToDeleted =Temp_NeedDeletedSecurityRule;
			Temp_NeedDeletedSecurityRule =Temp_NeedDeletedSecurityRule ->next;
			//LOG_PROC("INFO", "%s_%d:g_openstack_security_rule_id  %p",FN,LN,ReadyToDeleted);
			mem_free(g_openstack_security_rule_id,ReadyToDeleted);
			
		}
		//LOG_PROC("INFO", "%s_%d:g_openstack_security_group_id  %p",FN,LN,temp_node);
		mem_free(g_openstack_security_group_id,temp_node);
	}
	g_openstack_security_list=NULL;
	return 1;
}




/* by:Hongyu Yang
 * 
 */
void openstack_show_security_rule(openstack_security_rule_p rule_p)
{
	printf("direction: %s\n", rule_p->direction);
	printf("ethertype: %s\n", rule_p->ethertype);
	printf("rule_id: %s\n", rule_p->rule_id);
	printf("port_range_max: %u\n", rule_p->port_range_max);
	printf("port_range_min: %u\n", rule_p->port_range_min);
	printf("protocol: %s\n", rule_p->protocol);
	printf("remote_group_id: %s\n", rule_p->remote_group_id);
	printf("remote_ip_prefix: %s\n", rule_p->remote_ip_prefix);
	printf("tenant_id: %s\n", rule_p->tenant_id);
	printf("priority: %d\n", rule_p->priority);
	printf("enabled: %d\n", rule_p->enabled);
}

/* by:Hongyu Yang
 * 
 */
void openstack_show_all_security_group()
{
	openstack_security_p temp_p = g_openstack_security_list;
	while (NULL != temp_p) 
	{
		printf("security group id:%s\n", temp_p->security_group);
		openstack_security_rule_p rule_p = temp_p->security_rule_p;
		while (NULL != rule_p) 
		{
			openstack_show_security_rule(rule_p);
			rule_p = rule_p->next;
		}
		temp_p = temp_p->next;
	}
}

