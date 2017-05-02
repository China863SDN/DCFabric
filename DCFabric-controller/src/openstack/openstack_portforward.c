
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


extern UINT4 g_proactive_flow_flag ;
openstack_port_forward_p find_internal_dnatip_by_external_ip(UINT4 extIp, UINT4 port, UINT4 proto)
{

	struct _openstack_port_forward *  node_p = g_openstack_forward_list;
	
    while(node_p != NULL)
	{
    	
        if((node_p->n_src_ip== extIp)&&(node_p->src_port == port)&&(node_p->proto == proto)) //
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
    	
        if((node_p->n_dst_ip== intIp)&&(node_p->dst_port == port)&&(node_p->proto == proto)) //
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
    	
        if((node_p->n_dst_ip== portforward_ip)&&(node_p->dst_port == port)&&(node_p->proto == proto)) //
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

    find_fabric_network_by_dnat_ip(node_portforward->n_dst_ip, node_portforward->dst_port, node_portforward->proto, network_id);
    
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
INT4 fabric_openstack_dnatip_packet_out_handle(p_fabric_host_node src_port, packet_in_info_t *packet_in, openstack_port_forward_p pfip, param_set_p param_set)
{
	UINT4 vlan_id = 0 ;
	//p_fabric_host_node int_host = NULL;
	// printf("%s\n", FN);
	ip_t *ip = (ip_t *)(packet_in->data);

	external_port_p ext_port = NULL;
	ext_port = find_openstack_external_by_dnat_ip(pfip );

	if ((src_port == NULL || ext_port == NULL) || (src_port->sw == NULL))
	{
		LOG_PROC("INFO", "Port Forward: switch is NULL!");
		return IP_DROP;
	}

	//
	#if 0
	int_host = get_fabric_host_from_list_by_ip(pfip->n_dst_ip);
	if (NULL == int_host) {
		LOG_PROC("INFO", "Floating: Internal host is NULL!");
		return IP_DROP;
	}

	
	//write flow table
	//packet out rule
	if (NULL == int_host->sw) {
		LOG_PROC("INFO", "Port forward: external switch is NULL!");
		return IP_DROP;
	}
	#endif

	
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
		param_set->mod_src_port_no = pfip->src_port;
		param_set->mod_dst_port_no = pfip->dst_port;
		return Portforward_ip_flow; //DNAT
	}

	return IP_DROP;
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

	if (IPPROTO_ICMP == proto) {
		icmp_t* icmp = (icmp_t*)p_ip->data;
		
	}
	else if (IPPROTO_TCP == proto) {
		tcp_t* tcp = (tcp_t*)p_ip->data;
		proto_src_port = tcp->sport;
	}
	else if (IPPROTO_UDP == proto) {
		udp_t* udp = (udp_t*)p_ip->data;
		proto_src_port = udp->sport;
		
	}
	
	// get floating ip
	fip_src = get_external_floating_ip_by_fixed_ip(sendip);
	fip_dst = get_external_floating_ip_by_floating_ip(targetip);
	fip_dnat_src = find_external_dnatip_by_internal_ip(sendip, ntohs(proto_src_port),proto );
	// if source port is floating ip
	
	if(NULL != fip_dnat_src)
	{
		foward_type = fabric_openstack_dnatip_packet_out_handle(src_port, packet_in, fip_dnat_src , param_set);
	}
	else if(NULL != fip_src)
	{
		foward_type = fabric_openstack_floating_ip_packet_out_handle(src_port, packet_in, fip_src, param_set);
	}
	else if ((fip_dst) && (find_openstack_lbaas_pool_by_ip(fip_dst->fixed_ip))) {
		ip_t* ip = (ip_t*)packet_in->data;
		fixed_dst_port = get_fabric_host_from_list_by_ip(fip_dst->fixed_ip);
		if (fixed_dst_port)
			foward_type = internal_packet_compute_floating_vip_forward(src_port, fixed_dst_port, fip_dst->fixed_ip,
							param_set, ip, NULL, fip_dst);
	}
	else
	{
		if (IPPROTO_ICMP == proto) {
			foward_type = fabric_openstack_nat_icmp_comute_foward(src_port->sw, packet_in,TRUE,param_set);
		}
		else {
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
	
	fip_dnat_dst = find_internal_dnatip_by_external_ip(targetip , ntohs(dest_port), proto );
	
	if (NULL != fip_dnat_dst)
	{
		LOG_PROC("INFO", "%s Port Forward IP start ",FN);

		epp = find_openstack_external_by_dnat_ip(fip_dnat_dst);
		if(NULL == epp)
		{
			LOG_PROC("INFO", "Port Forward IP: Can't get external switch");
			return IP_DROP;
		}
		dst_port = get_fabric_host_from_list_by_ip(fip_dnat_dst->n_dst_ip);
		if (NULL == dst_port) {
			LOG_PROC("INFO", "Port Forward IP: Port Forward ip is not exist");
			return IP_DROP;
		} 
		external_sw = get_ext_sw_by_dpid(epp->external_dpid);
		if (NULL == external_sw) {
		  LOG_PROC("INFO", "Port Forward IP: Can't get external switch");
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
			param_set->mod_src_port_no = fip_dnat_dst->src_port;
			param_set->mod_dst_port_no = fip_dnat_dst->dst_port;

			param_set->src_sw = dst_port->sw;
			param_set->dst_ip = src_ip;
			memcpy(param_set->src_mac, dst_port->mac, 6);
			param_set->mod_src_ip = fip_dnat_dst->n_src_ip; //fip_dnat_dst->src_ip
			param_set->src_port_no = fip_dnat_dst->src_port; //fip_dnat_dst->src_ip
			memcpy(param_set->dst_gateway_mac, epp->external_gateway_mac, 6);
			param_set->src_vlanid = of131_fabric_impl_get_tag_sw(external_sw);
			param_set->src_inport = dst_port->port;
			LOG_PROC("INFO", "%s Port Forward IP end ",FN);
			return Portforward_ip_flow;
		}
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
				LOG_PROC("INFO", "Floating IP: Fixed ip is not exist");
				return IP_DROP;
			}

			if (NULL != find_openstack_lbaas_pool_by_ip(fip->fixed_ip)) {
				return external_packet_in_compute_vip_forward(src_port, dst_port, fip->fixed_ip, param_set, ip, epp, fip);
			}

			if (NULL == dst_port->sw) {
				LOG_PROC("INFO", "Floating IP: Fixed ip sw is NULL");
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
			if (IPPROTO_ICMP == proto) {
				foward_type = fabric_openstack_nat_icmp_comute_foward(NULL, packet_in, FALSE, param_set);
			}
			else {
		//	LOG_PROC("INFO", "%s targetip = 0x%x dest_port=0x%x", FN, targetip,dest_port);
				foward_type = fabric_openstack_ip_nat_comute_foward(NULL, packet_in, FALSE, param_set);
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
				LOG_PROC("INFO","destroy_portforward_old_flows external_sw->sw_ip=0x%x port_forward_templist->n_src_ip=0x%x port_forward_templist->src_port=%d",external_sw->sw_ip,port_forward_templist->n_src_ip,port_forward_templist->src_port);
				delete_fabric_input_portforwardflow_by_ip_portno(external_sw, port_forward_templist->n_src_ip, port_forward_templist->src_port, port_forward_templist->proto);
			}
			if ((NULL != dst_port)&&(NULL != dst_port->sw))
			{
				
		//		LOG_PROC("INFO","destroy_portforward_old_flows dst_port->sw->sw_ip=0x%x dst_port->mac=%s port_forward_templist->dst_port=%d",dst_port->sw->sw_ip,dst_port->mac,port_forward_templist->dst_port);
				delete_fabric_input_portforwardflow_by_mac_portno(dst_port->sw, dst_port->mac, port_forward_templist->dst_port, port_forward_templist->proto);
			}
		}

		port_forward_templist = port_forward_templist->next;
	}
	return GN_OK;
}
