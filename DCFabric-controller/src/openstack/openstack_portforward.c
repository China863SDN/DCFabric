
#include "common.h"

#include "gn_inet.h"
#include "openflow-10.h"
#include "openflow-13.h"


#include "fabric_host.h"
#include "forward-mgr.h"
#include "fabric_impl.h"
#include "fabric_floating_ip.h"
#include "openstack_lbaas_app.h"
#include "fabric_openstack_arp.h"
#include "fabric_openstack_nat.h"
#include "openstack_routers.h"
#include "openstack_portforward.h"
#include "../conn-svr/conn-svr.h"
#include "openstack_clbaas_app.h"
#include "fabric_openstack_gateway.h"


extern UINT4 g_proactive_flow_flag ;
openstack_port_forward_p find_internal_dnatip_by_external_ip(UINT4 extIp, UINT4 port, UINT4 proto)
{
	struct _openstack_port_forward* node_p = g_openstack_forward_list;

	if(!port || !proto)
	{
		return NULL;
	}
    while(node_p != NULL)
	{
        if((node_p->n_src_ip== extIp) &&
           (node_p->src_port_start <= port) &&
           (node_p->src_port_end >= port) &&
           (node_p->proto == proto))
		{
            return node_p;
        }
        node_p = node_p->next;
    }

    return NULL;
}
openstack_port_forward_p find_external_dnatip_by_internal_ip(UINT4 intIp, UINT4 port, UINT4 proto)
{

	struct _openstack_port_forward *  node_p = g_openstack_forward_list;
    while(node_p != NULL)
	{    	
        if((node_p->n_dst_ip== intIp) &&
           (node_p->dst_port_start <= port) &&
           (node_p->dst_port_end >= port) &&
           (node_p->proto == proto))
		{
            return node_p;
        }
        node_p = node_p->next;
    }

    return NULL;
}


void find_fabric_network_by_dnat_ip(UINT4 portforward_ip,UINT2 port, UINT4 proto, char* network_id)
{
	struct _openstack_port_forward *  node_p = g_openstack_forward_list;
	while(node_p != NULL)
	{
        if((node_p->n_dst_ip== portforward_ip) &&
           (node_p->dst_port_start <= port) &&
           (node_p->dst_port_end >= port) &&
           (node_p->proto == proto))
		{
            strcpy(network_id, node_p->network_id);
			return;
        }
        node_p = node_p->next;
    }
	return ;
}



external_port_p find_openstack_external_by_dnat_ip(openstack_port_forward_p node_portforward )
{
	external_port_p epp = NULL;
	openstack_external_node_p node_p = g_openstack_external_list;
    char network_id[48];

	if(NULL == node_portforward)
	{
		return NULL;
	}

    find_fabric_network_by_dnat_ip(node_portforward->n_dst_ip, node_portforward->dst_port_start, node_portforward->proto, network_id);
    
	while(node_p != NULL )
	{
		epp = (external_port_p)node_p->data;
		if(epp->external_dpid  && epp->external_port)
		{
			if((0 != strlen(network_id)) && (0 == strcmp(epp->network_id,network_id)))
			{
				return epp;
			}
		}
		node_p=node_p->next;
	}

	return NULL;
}



openstack_port_forward_p get_internal_dnatip_by_external_ip(UINT4 extIp, UINT4 port,  UINT4 proto)
{
	return find_internal_dnatip_by_external_ip(extIp, port, proto);
}



external_port_p get_external_port_by_hostip(UINT4 hostip) 
{
	INT1 ret = GN_OK;
	char network_id[48] = {0};
	char subnet_id[48] = {0};	
  	external_port_p epp = NULL;
	p_fabric_host_node hostNode=NULL;
	UINT4 external_outer_interface_ip = 0;
	openstack_router_outerinterface_p node_router_outerinterface = NULL;
    openstack_external_node_p node_p = g_openstack_external_list;
	hostNode = get_fabric_host_from_list_by_ip(hostip);
	if(NULL == hostNode)
	{
		return NULL;
	}
	openstack_port_p list_port_p = (openstack_port_p)hostNode->data;
	if(NULL == list_port_p)
	{
		return NULL;
	}

	
	find_fabric_network_by_floating_ip(hostNode->ip_list[0],network_id, subnet_id);
	
	node_router_outerinterface = find_openstack_router_outerinterface_by_networkAndsubnetid(network_id, subnet_id);
	if(NULL != node_router_outerinterface)
	{
		ret = find_openstack_router_outerinterfaceip_by_router(node_router_outerinterface ,&external_outer_interface_ip);
		if(GN_OK == ret)
		{
			epp = find_openstack_external_by_outer_ip(external_outer_interface_ip);
		}
	}
				
	return epp;
}

INT4 fabric_openstack_dnatip_packet_out_handle(p_fabric_host_node src_port, packet_in_info_t *packet_in, openstack_port_forward_p pfip, param_set_p param_set)
{
	UINT4 vlan_id = 0 ;
	UINT1 proto = 0;
	UINT2 proto_src_port = 0;
	//p_fabric_host_node int_host = NULL;
	// printf("%s\n", FN);
	ip_t *ip = (ip_t *)(packet_in->data);
	if (IPPROTO_ICMP == proto)
	{
		icmp_t* icmp = (icmp_t*)ip->data;
		
	}
	else if (IPPROTO_TCP == proto) 
	{
		tcp_t* tcp = (tcp_t*)ip->data;
		proto_src_port = tcp->sport;
	}
	else if (IPPROTO_UDP == proto) 
	{
		udp_t* udp = (udp_t*)ip->data;
		proto_src_port = udp->sport;
		
	}
	external_port_p ext_port = NULL;
	ext_port = find_openstack_external_by_dnat_ip(pfip );

	if ((src_port == NULL || ext_port == NULL) || (src_port->sw == NULL))
	{
		LOG_PROC("INFO", "Port Forward: switch is NULL!");
		return IP_DROP;
	}

	
	gn_switch_t * switch_gw = find_sw_by_dpid(ext_port->external_dpid);
	if (NULL == switch_gw) {
		LOG_PROC("INFO", "Port Forward: External switch is NULL!");
		return IP_DROP;
	}
	
	vlan_id = of131_fabric_impl_get_tag_sw(switch_gw);
	param_set->src_sw = src_port->sw; //----
	param_set->dst_ip = ip->dest; // 
	memcpy(param_set->src_mac, src_port->mac, 6);
	param_set->mod_src_ip = pfip->n_src_ip; // 
	//param_set->src_port_no = pfip->src_port;
	memcpy(param_set->dst_gateway_mac,  ext_port->external_gateway_mac, 6);
	param_set->src_vlanid = vlan_id;

	//response rule
	vlan_id = of131_fabric_impl_get_tag_sw(src_port->sw);

	UINT4 out_port = get_out_port_between_switch(ext_port->external_dpid, src_port->sw->dpid);
	if (0 != out_port) {
		//fabric_openstack_floating_ip_install_set_vlan_in_flow(switch_gw, fip->floating_ip, ip->src, ip->eth_head.src, vlan_id, out_port);
		param_set->dst_sw = switch_gw;
		param_set->src_ip = ip->src;
		memcpy(param_set->packet_src_mac, ip->eth_head.src, 6);
		param_set->dst_vlanid = vlan_id;
		param_set->dst_inport = out_port;
		param_set->src_inport = packet_in->inport;

		param_set->proto = ip->proto;
		param_set->mod_src_port_no = pfip->src_port_start + (ntohs(proto_src_port) - pfip->dst_port_start);
		param_set->mod_dst_port_no = ntohs(proto_src_port);
		return Portforward_ip_flow; //DNAT
	}

	return IP_DROP;
}
INT4 fabric_openstack_clbip_packet_out_handle(UINT4 sendip, UINT4 targetip, param_set_p param_set, BOOL *bClbFlag)
{
	INT4 foward_type = IP_DROP;
	gn_switch_t * external_sw = NULL;
	external_port_p  epp  = NULL;
	openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
	p_fabric_host_node hostNode = get_fabric_host_from_list_by_ip(sendip);
	
	
	if(hostNode&&(OPENSTACK_PORT_TYPE_CLBLOADBALANCER ==hostNode->type))
	{

		lb_vipfloating = find_openstack_clbaas_vipfloatingpool_by_extip(sendip);
		if(NULL == lb_vipfloating)
		{
			LOG_PROC("INFO", "%s %d can't get clb host\n",FN,LN);
			return IP_DROP;
		}
		
		epp  = get_external_port_by_hostip(lb_vipfloating->inside_ip);
		if(NULL == epp)
		{
			LOG_PROC("INFO", "%s %d can't get external port\n",FN,LN);
			return IP_DROP;
		}
		LOG_PROC("INFO", "%s %d  targetip=0x%x sendip=0x%x \n",FN,LN,targetip,sendip);
		*bClbFlag = TRUE;
		external_sw = get_ext_sw_by_dpid(epp->external_dpid);
		if (NULL == external_sw) {
			LOG_PROC("INFO", "CLB IP: Can't get external switch");
			foward_type = IP_DROP;
			return foward_type;
		}
		UINT4 out_port = get_out_port_between_switch(epp->external_dpid, hostNode->sw->dpid);
		if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) {
			if (NULL == hostNode->sw) {
					LOG_PROC("INFO", "%s %d sw isn't exist\n",FN,LN);
					foward_type =  IP_DROP;
					return foward_type;
					//return create_arp_flood_parameter(fip_dnat_dst->n_dst_ip, hostNode, param_set);
				}
				UINT4 out_port = get_out_port_between_switch(epp->external_dpid, hostNode->sw->dpid);
				if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) {			

					param_set->dst_sw = external_sw;
					param_set->src_ip = hostNode->ip_list[0];

					memcpy(param_set->packet_src_mac, hostNode->mac, 6);
					param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(hostNode->sw);
					param_set->dst_inport = out_port;

					param_set->src_sw = hostNode->sw;
					param_set->dst_ip = targetip;
					memcpy(param_set->src_mac, hostNode->mac, 6);
					memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
					param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
					param_set->src_inport = hostNode->port;
				
					//fabric_openstack_clbforward_ip_install_set_vlan_out_flow(param_set->src_sw, param_set->dst_ip, param_set->src_mac, param_set->dst_gateway_mac, param_set->src_vlanid, param_set->src_security);

					//fabric_openstack_clbforward_ip_install_set_vlan_in_flow(param_set->dst_sw, param_set->src_ip, param_set->packet_src_mac,  param_set->src_vlanid, param_set->src_security);
																		

					//LOG_PROC("INFO", "%s %d clb forward \n ",FN,LN);

					foward_type = Clb_forward_ip_flow;
				}
		}
		foward_type =  IP_DROP;
	
	}
	*bClbFlag = FALSE;
	return foward_type;
}


INT4 external_dnat_packet_out_compute_forward(p_fabric_host_node src_port, UINT4 sendip, UINT4 targetip, packet_in_info_t *packet_in, UINT1 proto, param_set_p param_set)
{
	INT4 foward_type = IP_DROP;
	UINT2 proto_src_port = 0;
	external_floating_ip_p fip_src = NULL;
	external_floating_ip_p fip_dst = NULL;
	p_fabric_host_node fixed_dst_port = NULL;

	
	openstack_port_forward_p fip_dnat_src = NULL;

	ip_t *p_ip = (ip_t *)(packet_in->data);

	if (IPPROTO_ICMP == proto)
	{
		icmp_t* icmp = (icmp_t*)p_ip->data;
		
	}
	else if (IPPROTO_TCP == proto) 
	{
		tcp_t* tcp = (tcp_t*)p_ip->data;
		proto_src_port = tcp->sport;
	}
	else if (IPPROTO_UDP == proto) 
	{
		udp_t* udp = (udp_t*)p_ip->data;
		proto_src_port = udp->sport;
		
	}
	
	LOG_PROC("PACKET_IN", "%s %d  targetip=0x%x sendip=0x%x proto=%d proto_src_port=%d\n",FN,LN,targetip,sendip,proto,proto_src_port);
	// get floating ip
	fip_src = get_external_floating_ip_by_fixed_ip(sendip);
	fip_dst = get_external_floating_ip_by_floating_ip(targetip);
	fip_dnat_src = find_external_dnatip_by_internal_ip(sendip, ntohs(proto_src_port),proto);
	// if source port is floating ip
	
	if(NULL != fip_dnat_src)
	{
		foward_type = fabric_openstack_dnatip_packet_out_handle(src_port, packet_in, fip_dnat_src , param_set);
	}
	else if(NULL != fip_src)
	{
		foward_type = fabric_openstack_floating_ip_packet_out_handle(src_port, packet_in, fip_src, param_set);
	}
	else if ((fip_dst) && (find_openstack_lbaas_pool_by_ip(fip_dst->fixed_ip))) 
	{
		ip_t* ip = (ip_t*)packet_in->data;
		fixed_dst_port = get_fabric_host_from_list_by_ip(fip_dst->fixed_ip);
		if (fixed_dst_port)
		{
			foward_type = internal_packet_compute_floating_vip_forward(src_port, fixed_dst_port, fip_dst->fixed_ip,param_set, ip, NULL, fip_dst);
		}
	}
	else
	{
		external_port_p  epp = NULL;
		gn_switch_t * external_sw = NULL;
		openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
		p_fabric_host_node hostNode = get_fabric_host_from_list_by_ip(sendip);
		
		
		if(hostNode&&(OPENSTACK_PORT_TYPE_CLBLOADBALANCER ==hostNode->type))
		{
			lb_vipfloating = find_openstack_clbaas_vipfloatingpool_by_extip(sendip);
			if(NULL == lb_vipfloating)
			{
				LOG_PROC("INFO", "%s %d can't get clb host\n",FN,LN);
				return IP_DROP;
			}
			
			epp  = get_external_port_by_hostip(lb_vipfloating->inside_ip);
			if(NULL == epp)
			{
				LOG_PROC("INFO", "%s %d can't get external port\n",FN,LN);
				return IP_DROP;
			}
			external_sw = get_ext_sw_by_dpid(epp->external_dpid);
			if (NULL == external_sw) 
			{
				LOG_PROC("INFO", "CLB IP: Can't get external switch");
				return IP_DROP;
			}
			UINT4 out_port = get_out_port_between_switch(epp->external_dpid, hostNode->sw->dpid);
			if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) 
			{
				if (NULL == hostNode->sw) 
				{
					LOG_PROC("INFO", "%s %d sw isn't exist\n",FN,LN);
					return IP_DROP;
				}
				UINT4 out_port = get_out_port_between_switch(epp->external_dpid, hostNode->sw->dpid);
				if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) 
				{			

					param_set->dst_sw = external_sw;
					param_set->src_ip = hostNode->ip_list[0];

					memcpy(param_set->packet_src_mac, hostNode->mac, 6);
					param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(hostNode->sw);
					param_set->dst_inport = out_port;

					param_set->src_sw = hostNode->sw;
					param_set->dst_ip = targetip;
					memcpy(param_set->src_mac, hostNode->mac, 6);
					memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
					param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
					param_set->src_inport = hostNode->port;
				
					return Clb_forward_ip_flow;
				}
			}
			return IP_DROP;
		}
		if (IPPROTO_ICMP == proto) 
		{
			foward_type = fabric_openstack_nat_icmp_comute_foward(src_port->sw, packet_in,TRUE,param_set);
		}
		else 
		{
			foward_type = fabric_openstack_ip_nat_comute_foward(src_port->sw, packet_in, TRUE, param_set);
		}
	}
	return foward_type;
}


INT4 external_floatingip_dnat_packet_in_compute_forward(p_fabric_host_node src_port, UINT4 src_ip, UINT4 targetip, UINT4 dest_port, packet_in_info_t* packet_in, UINT1 proto, param_set_p param_set)
{
	//outer -> inner
	external_port_p epp = NULL;
	external_floating_ip_p fip = NULL;
	p_fabric_host_node dst_port = NULL;
	openstack_port_forward_p fip_dnat_dst = NULL;
	gn_switch_t * external_sw = NULL;
	INT4 foward_type = IP_DROP;
	
	ip_t *ip = (ip_t *)(packet_in->data);
	if(NULL == ip)
	{
		LOG_PROC("ERROR", "Ip data is NULL!");
	    return IP_DROP;
	}
	tcp_t* tcp_pkt = (tcp_t*)ip->data;
	
	LOG_PROC("PACKET_IN", "%s %d  targetip=0x%x src_ip=0x%x dest_port=0x%x proto=%d\n",FN,LN,targetip,src_ip,dest_port,proto);
	fip_dnat_dst = find_internal_dnatip_by_external_ip(targetip , ntohs(dest_port), proto );
	
	if (NULL != fip_dnat_dst)
	{
		LOG_PROC("INFO", "%s Port Forward IP start ",FN);

		epp = find_openstack_external_by_dnat_ip(fip_dnat_dst);
		if(NULL == epp)
		{
			LOG_PROC("ERROR", "Port Forward IP: Can't get external switch");
			return IP_DROP;
		}
		dst_port = get_fabric_host_from_list_by_ip(fip_dnat_dst->n_dst_ip);
		if (NULL == dst_port) {
			LOG_PROC("ERROR", "Port Forward IP: Port Forward ip is not exist");
			return IP_DROP;
		} 
		external_sw = get_ext_sw_by_dpid(epp->external_dpid);
		if (NULL == external_sw) {
		  LOG_PROC("ERROR", "Port Forward IP: Can't get external switch");
		  return IP_DROP;
		}
	
		if (NULL == dst_port->sw) {
				LOG_PROC("INFO", "Port Forward IP: Port Forward ip sw is NULL");
				return create_arp_flood_parameter(fip_dnat_dst->n_dst_ip, dst_port, param_set);
			}
			UINT4 out_port = get_out_port_between_switch(epp->external_dpid, dst_port->sw->dpid);
			if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) {
			param_set->dst_sw = external_sw;
			param_set->src_ip = dst_port->ip_list[0];
			memcpy(param_set->packet_src_mac, dst_port->mac, 6);
			param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(dst_port->sw);
			param_set->dst_inport = out_port;

			param_set->proto = ip->proto;
			param_set->mod_src_port_no = ntohs(dest_port); //fip_dnat_dst->src_port_start;
			param_set->mod_dst_port_no =  fip_dnat_dst->dst_port_start + (ntohs(dest_port) - fip_dnat_dst->src_port_start);//fip_dnat_dst->dst_port_start;

			param_set->src_sw = dst_port->sw;
			param_set->dst_ip = src_ip;
			memcpy(param_set->src_mac, dst_port->mac, 6);
			param_set->mod_src_ip = fip_dnat_dst->n_src_ip; //fip_dnat_dst->src_ip
			if((PROTO_TCP ==ip->proto)&&tcp_pkt)
			{
				param_set->src_port_no = ntohs(tcp_pkt->sport);//fip_dnat_dst->src_port; //fip_dnat_dst->src_ip
			}
			memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
			param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
			param_set->src_inport = dst_port->port;
			LOG_PROC("INFO", "%s Port Forward IP end ",FN);
			return Portforward_ip_flow;
		}
		return IP_DROP;
	}
	else 
	{
		fip = get_external_floating_ip_by_floating_ip(targetip);
		epp = get_external_port_by_floatip(targetip);
		
		if ((NULL != fip) && (NULL != epp))
		{   
			LOG_PROC("INFO", "%s Floating IP start ",FN);
	
			if (g_proactive_flow_flag)
			{
				LOG_PROC("ERROR", "Floating IP: g_proactive_flow_flag is 1");
		        return IP_DROP;
			}
	
	        external_sw = get_ext_sw_by_dpid(epp->external_dpid);
			if (NULL == external_sw) {
				LOG_PROC("INFO", "Floating IP: Can't get external switch");
				return IP_DROP;
			}
			
			dst_port = get_fabric_host_from_list_by_ip(fip->fixed_ip);
			if (NULL == dst_port) {
				LOG_PROC("INFO", "Floating IP: Fixed ip 0x%x floatingip 0x%x is not exist",fip->fixed_ip,fip->floating_ip);
				return IP_DROP;
			}

			if (NULL != find_openstack_lbaas_pool_by_ip(fip->fixed_ip)) {
				return external_packet_in_compute_vip_forward(src_port, dst_port, fip->fixed_ip, param_set, ip, epp, fip);
			}

			if (NULL == dst_port->sw) {
				LOG_PROC("INFO", "Floating IP: Fixed ip 0x%x floatingip 0x%x sw is NULL", fip->fixed_ip,fip->floating_ip);
				return create_arp_flood_parameter(fip->fixed_ip, dst_port, param_set);
			}

			UINT4 out_port = get_out_port_between_switch(epp->external_dpid, dst_port->sw->dpid);
			if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) {
				param_set->dst_sw = external_sw;
				param_set->src_ip = dst_port->ip_list[0];
				memcpy(param_set->packet_src_mac, dst_port->mac, 6);
				param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(dst_port->sw);
				param_set->dst_inport = out_port;

				param_set->src_sw = dst_port->sw;
				param_set->dst_ip = src_ip;
				memcpy(param_set->src_mac, dst_port->mac, 6);
				param_set->mod_src_ip = fip->floating_ip;
				memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
				param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
				param_set->src_inport = dst_port->port;
				LOG_PROC("INFO", "%s Floating IP end ",FN);
				return Floating_ip_flow;
			}
		}
		else
		{
			openstack_clbaas_vipfloating_p lb_vipfloating = NULL;
			p_fabric_host_node hostNode = get_fabric_host_from_list_by_ip(targetip);
			
			if(hostNode&&(OPENSTACK_PORT_TYPE_CLBLOADBALANCER ==hostNode->type))
			{
				
				LOG_PROC("INFO", "%s %d clb forward  start\n ",FN,LN);
				lb_vipfloating = find_openstack_clbaas_vipfloatingpool_by_extip(targetip);
				if(NULL == lb_vipfloating)
				{
					LOG_PROC("INFO", "%s %d can't get clb host\n",FN,LN);
					return IP_DROP;
				}
				
				epp  = get_external_port_by_hostip(lb_vipfloating->inside_ip);
				if(NULL == epp)
				{
					LOG_PROC("INFO", "%s %d can't get external port\n",FN,LN);
					return IP_DROP;
				}
				if (NULL == hostNode->sw) {
					LOG_PROC("INFO", "%s %d sw isn't exist\n",FN,LN);
					return IP_DROP;
					//return create_arp_flood_parameter(fip_dnat_dst->n_dst_ip, hostNode, param_set);
				}
				external_sw = get_ext_sw_by_dpid(epp->external_dpid);
				if (NULL == external_sw) {
					LOG_PROC("INFO", "CLB IP: Can't get external switch");
					return IP_DROP;
				}
				UINT4 out_port = get_out_port_between_switch(epp->external_dpid, hostNode->sw->dpid);
				if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) {

					param_set->dst_sw = external_sw;
					param_set->src_ip = hostNode->ip_list[0];

					memcpy(param_set->packet_src_mac, hostNode->mac, 6);
					param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(hostNode->sw);
					param_set->dst_inport = out_port;

					param_set->src_sw = hostNode->sw;
					param_set->dst_ip = src_ip;
					memcpy(param_set->src_mac, hostNode->mac, 6);
					memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
					param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
					param_set->src_inport = hostNode->port;
																		

					LOG_PROC("INFO", "%s %d clb forward  end\n ",FN,LN);

					return Clb_forward_ip_flow;
				}
				return IP_DROP;
			}
			else
			{
				if (IPPROTO_ICMP == proto) {
					foward_type = fabric_openstack_nat_icmp_comute_foward(NULL, packet_in, FALSE, param_set);
				}
				else {
			//	LOG_PROC("INFO", "%s targetip = 0x%x dest_port=0x%x", FN, targetip,dest_port);
					foward_type = fabric_openstack_ip_nat_comute_foward(NULL, packet_in, FALSE, param_set);
				}
			}
		}
	}

	return foward_type;
}

INT4 destroy_portforward_old_flows(openstack_port_forward_p p_forward_proc_list)
{
	external_port_p epp = NULL;
	gn_switch_t * external_sw = NULL;
	p_fabric_host_node dst_port = NULL;
	openstack_port_forward_p  port_forward_templist = NULL;
	if(NULL == p_forward_proc_list)
	{
		return GN_ERR;
	}

	port_forward_templist = p_forward_proc_list;
	while(port_forward_templist)
	{
		//LOG_PROC("INFO","destroy_portforward_old_flows proto=%d state=%d src_port=%d dst_port=%d",port_forward_templist->proto, port_forward_templist->state, port_forward_templist->src_port, port_forward_templist->dst_port);
		
		//LOG_PROC("INFO","destroy_portforward_old_flows n_src_ip=0x%x n_dst_ip=0x%x src_ip=%s dst_ip=%s",port_forward_templist->n_src_ip, port_forward_templist->n_dst_ip, port_forward_templist->src_ip, port_forward_templist->dst_ip);
		
		//LOG_PROC("INFO","destroy_portforward_old_flows network_id=%s",port_forward_templist->network_id);
		
		epp = get_external_port_by_out_interface_ip(port_forward_templist->n_src_ip);
		dst_port = get_fabric_host_from_list_by_ip(port_forward_templist->n_dst_ip);
		if ((NULL != epp) &&(NULL != dst_port))
		{
			external_sw = get_ext_sw_by_dpid(epp->external_dpid);
			if (NULL != external_sw)
			{
				LOG_PROC("INFO","destroy_portforward_old_flows external_sw->sw_ip=0x%x port_forward_templist->n_src_ip=0x%x port_forward_templist->src_port=%d-%d",
                    external_sw->sw_ip,port_forward_templist->n_src_ip,port_forward_templist->src_port_start,port_forward_templist->src_port_end);
                UINT2 port = port_forward_templist->src_port_start;
                for (; port <= port_forward_templist->src_port_end; port++)
    				delete_fabric_input_portforwardflow_by_ip_portno(external_sw, port_forward_templist->n_src_ip, port, port_forward_templist->proto);
			}
			if ((NULL != dst_port)&&(NULL != dst_port->sw))
			{
				
				LOG_PROC("INFO","destroy_portforward_old_flows dst_port->sw->sw_ip=0x%x dst_port->mac=%s port_forward_templist->dst_port=%d-%d",
                    dst_port->sw->sw_ip,dst_port->mac,port_forward_templist->dst_port_start,port_forward_templist->dst_port_end);
                UINT2 port = port_forward_templist->dst_port_start;
                for (; port <= port_forward_templist->dst_port_end; port++)
    				delete_fabric_input_portforwardflow_by_mac_portno(dst_port->sw, dst_port->mac, port, port_forward_templist->proto);
			}
		}

		port_forward_templist = port_forward_templist->next;
	}
	return GN_OK;
}

INT4 multicast_packet_out_compute_forward(p_fabric_host_node src_port, UINT4 src_ip, UINT4 targetip,  packet_in_info_t* packet_in, UINT1 proto, param_set_p param_set)
{
	char lb_pool_id[64] = {0};
	UINT4 HAInterface_peerIp = 0;
	p_fabric_host_node peerhostNode=NULL;
	openstack_clbass_loadbalancer_p lb_loadbalancerMaster = NULL;
	openstack_clbass_loadbalancer_p lb_loadbalancerBackup = NULL;
	lb_loadbalancerMaster = find_openstack_clbaas_loadbalancer_by_HAInterfaceIp(src_ip);
	if(lb_loadbalancerMaster&&lb_loadbalancerMaster->loadbalancer_id)
	{
		lb_loadbalancerBackup = find_openstack_clbaas_loadbalancer_by_PoolidAndHAInterfaceIp(lb_loadbalancerMaster->pool_id,src_ip);
		if(NULL == lb_loadbalancerBackup)
		{
			
			return IP_DROP; //drop table flow
		}
		HAInterface_peerIp = lb_loadbalancerBackup->HA_interfaceIp;
		if(HAInterface_peerIp)
		{
			peerhostNode = get_fabric_host_from_list_by_ip(HAInterface_peerIp);
			if(peerhostNode)
			{
				if((NULL == peerhostNode->sw)||(NULL == src_port->sw))
				{
					//LOG_PROC("INFO", "%s: Can't get switch!!! src_ip=0x%x HAInterface_peerIp=0x%x",FN, src_ip, HAInterface_peerIp);
					return BROADCAST_DHCP;
				}
				UINT4 out_port = get_out_port_between_switch(src_port->sw->dpid, peerhostNode->sw->dpid);
				if ((0 != out_port) || (0 == get_nat_physical_switch_flag())) 
				{
					param_set->proto = proto;
					param_set->dst_sw = peerhostNode->sw;
					param_set->src_ip = src_ip;
					memcpy(param_set->src_mac, src_port->mac, 6);
					
					param_set->dst_vlanid = of131_fabric_impl_get_tag_sw(peerhostNode->sw);
					param_set->dst_inport = peerhostNode->port;

					param_set->src_sw = src_port->sw;
					param_set->dst_ip = targetip;
					param_set->mod_dst_ip = peerhostNode->ip_list[0];
					memcpy(param_set->dst_mac, peerhostNode->mac, 6);
					param_set->src_vlanid = of131_fabric_impl_get_tag_sw(src_port->sw);
					param_set->src_inport = src_port->port;
					return Clb_HA_MULTICAST;
				}
			}
		}
	}
	return IP_DROP;
}
