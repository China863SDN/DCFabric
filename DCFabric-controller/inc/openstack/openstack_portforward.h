#ifndef INC_OPENSTACK_PORTFORWARD_H_
#define INC_OPENSTACK_PORTFORWARD_H_
////////////////////////////////////////////////////////////////////////

openstack_port_forward_p find_internal_dnatip_by_external_ip(UINT4 extIp, UINT4 port, UINT4 proto);
openstack_port_forward_p get_internal_dnatip_by_external_ip(UINT4 extIp, UINT4 port, UINT4 proto);
INT4 fabric_openstack_dnatip_packet_out_handle(p_fabric_host_node dst_port, packet_in_info_t *packet_in, openstack_port_forward_p pfip, param_set_p param_set);

INT4 external_dnat_packet_out_compute_forward(p_fabric_host_node src_port, UINT4 sendip, UINT4 targetip, packet_in_info_t *packet_in, UINT1 proto, param_set_p param_set);

INT4 external_floatingip_dnat_packet_in_compute_forward(p_fabric_host_node src_port, UINT4 src_ip, UINT4 targetip, UINT4 dest_port, packet_in_info_t* packet_in, UINT1 proto, param_set_p param_set);
INT4 destroy_portforward_old_flows(openstack_port_forward_p p_forward_proc_list);

#endif

