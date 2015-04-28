/*
 * fabric_impl.c
 *
 *  Created on: 2015-1-20
 *      Author: zhaoliangzhi
 */
#include "fabric_path.h"
#include "fabric_impl.h"
#include "fabric_flows.h"
#include "fabric_thread.h"
#include "fabric_host.h"
#include <stdio.h>
/*****************************
 * intern functions
 *****************************/
void clear_fabric_server();
void init_fabric_id();
void init_fabric_id_by_dpids(UINT8* dpids,UINT4 len);
UINT1 check_dpid_avaliable(UINT8 dpid,UINT8* dpids,UINT4 len);
void init_fabric_base_flows();
void init_fabric_swap_flows();
void delete_fabric_flows();

p_fabric_path_list init_fabric_swap_flows_single(gn_switch_t* sw, UINT4 tag);
UINT2 init_fabric_neighbor_flows(gn_switch_t* sw,
		p_fabric_sw_list sw_list,
		p_fabric_path current_path,
		p_fabric_path_list current_list,
		UINT4 tag);
gn_port_t* get_neighbor_port(gn_switch_t* sw,gn_switch_t* neighbor);


/*****************************
 * global variables
 *****************************/
// switch server
extern gn_server_t g_server;

// fabric id & switch list
t_fabric_sw_list g_fabric_sw_list;
t_fabric_sw_list g_fabric_sw_list_total;
UINT4 g_fabric_tag = 0;

// fabric path list
p_fabric_path_list g_fabric_path_list = NULL;

// fabric label
UINT1 g_fabric_state = 0;
//UINT4 g_fabric_impl_flow_idle_time;    //idle time
//UINT4 g_fabric_impl_flow_hard_time;    //hard time

/*****************************
 * interface functions
 *****************************/
/*
 * setup fabric by dpids
 */
void of131_fabric_impl_setup_by_dpids(UINT8* dpids,UINT4 len){
	if(len != 0){

		// clear global variables
		clear_fabric_server();

		// initialize fabric id
		init_fabric_id_by_dpids(dpids,len);

		// initialize fabric base flows
		init_fabric_base_flows();

		// initialize fabric swap flows
		init_fabric_swap_flows();

		// start fabric threads
		start_fabric_thread();

		// set the state is available
		g_fabric_state = 1;
		printf("of131_fabric_impl_setup_by_dpids!\n");
	}
	return;
};
/*
 * setup fabric
 */
void of131_fabric_impl_setup(){

	// clear global variables
	clear_fabric_server();

	// initialize fabric id
	init_fabric_id();

	// initialize fabric base flows
	init_fabric_base_flows();

	// initialize fabric swap flows
	init_fabric_swap_flows();

	// start fabric threads
	start_fabric_thread();

	// set the state is available
	g_fabric_state = 1;
	printf("Fabric: v 1940 \nof131_fabric_impl_setup!\n");
	return;
};
/*
 * delete fabric
 */
void of131_fabric_impl_delete(){
	// set the state is unavailable
	g_fabric_state = 0;

	// stop fabric threads
	stop_fabric_thread();
	// initialize flows
	init_fabric_flows();

	// clear flows
	delete_fabric_flows();

	// clear global variables
	clear_fabric_server();

	printf("of131_fabric_impl_delete!\n");
	return;
};
/*
 * Get tag by switch object
 */
UINT4 of131_fabric_impl_get_tag_sw(gn_switch_t *sw){
	// initialize the variables
	p_fabric_sw_node p_sentinel = NULL;

	if( NULL != sw){
		p_sentinel = g_fabric_sw_list.node_list;
		while(p_sentinel != NULL){
			if(p_sentinel->sw->dpid == sw->dpid){
				return p_sentinel->tag;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return 0;
};
/*
 * Get tag by switch dpid
 */
UINT4 of131_fabric_impl_get_tag_dpid(UINT8 dpid){
	// initialize the variables
	p_fabric_sw_node p_sentinel = NULL;

	if( 0 != dpid){
		p_sentinel = g_fabric_sw_list.node_list;
		while(p_sentinel != NULL){
			if(p_sentinel->sw->dpid == dpid){
				return p_sentinel->tag;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return 0;
};
/*
 * get switch object by tag
 */
gn_switch_t* of131_fabric_impl_get_sw_tag(UINT4 tag){

	// initialize the variables
	p_fabric_sw_node p_sentinel = NULL;

	if( 0 != tag){
		p_sentinel = g_fabric_sw_list.node_list;
		while(p_sentinel != NULL){
			if(p_sentinel->tag == tag){
				return p_sentinel->sw;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return NULL;
};
/*
 * get switch dpid by tag
 */
UINT8 of131_fabric_impl_get_dpid_tag(UINT4 tag){
	// initialize the variables
	p_fabric_sw_node p_sentinel = NULL;

	if( 0 != tag){
		p_sentinel = g_fabric_sw_list.node_list;
		while(p_sentinel != NULL){
			if(p_sentinel->tag == tag){
				return p_sentinel->sw->dpid;
			}else{
				p_sentinel = p_sentinel->next;
			}
		}
	}
	return 0;
};
/*
 * get the fabric state
 */
UINT1 get_fabric_state(){
	return g_fabric_state;
}
/*
 * get path by src switch dpid and dst switch's dpid
 */
p_fabric_path of131_fabric_get_path(UINT8 src_dpid,UINT8 dst_dpid){

	p_fabric_path ret = NULL;
	p_fabric_path_list p_sentinel = NULL;
	p_sentinel = g_fabric_path_list;
	while(p_sentinel != NULL){
		if(p_sentinel->sw->dpid == dst_dpid){
			ret = get_fabric_path_from_list_by_sw_dpid(p_sentinel,src_dpid, dst_dpid);
			return ret;
		}
		p_sentinel = p_sentinel->next;
	}

	return ret;
};
/*****************************
 * intern function
 *****************************/
/*
 * initialize swap flows for fabric
 */
void init_fabric_swap_flows(){

	// initialize the variables
	t_fabric_path_list sentinel;
	p_fabric_path_list p_sentinel = &sentinel;
	// get sw node list
	p_fabric_sw_node sw_node= g_fabric_sw_list.node_list;
	p_sentinel->next = g_fabric_path_list;

	while(sw_node != NULL){
		// initialize each switch's swap flows
		p_sentinel->next = init_fabric_swap_flows_single(sw_node->sw,sw_node->tag);
		sw_node = sw_node->next;
		p_sentinel = p_sentinel->next;
	}
	// set the global path list variable
	g_fabric_path_list = sentinel.next;
	return;
};
/*
 * initialize a switch for fabric swap flows
 * return the swap flows' path list
 */
p_fabric_path_list init_fabric_swap_flows_single(gn_switch_t* sw, UINT4 tag){
	// initialize the variables
	p_fabric_path_list current_list = NULL; // current path list
	p_fabric_path_list ret_list = NULL; // return path list
	p_fabric_path temp_path = NULL; // temp variable : for path
	p_fabric_path_node temp_path_node = NULL; // temp variable : for path node

	p_fabric_sw_list sw_list = NULL; // copy total switch list
	p_fabric_sw_node temp_sw_node = NULL; // temp variable : for switch node

	UINT2 next_num = 0;// next switch num

//	printf("switch id:%d\n",sw->dpid);
	// check swith object
	if(sw != NULL){
		// copy total switch list
		sw_list = copy_fabric_sw_list(&g_fabric_sw_list_total);

		// create list
		current_list = create_fabric_path_list_NULL(sw);
		ret_list = create_fabric_path_list_NULL(sw);

		// create path
		temp_path = create_fabric_path(sw,NULL);

		// remove destination switch from sw_list
		temp_sw_node = remove_fabric_sw_list_by_sw(sw_list,sw);
		delete_fabric_sw_node(temp_sw_node);

		// create destination node swap flow
		install_fabric_last_flow(sw,tag);
		// install neighbors' flows
		init_fabric_neighbor_flows(sw,sw_list,temp_path,current_list,tag);

		// insert first path to list
		insert_fabric_list_path_to_list(ret_list,temp_path);

		while(current_list->num > 0){
			temp_path = remove_first_fabric_path_from_list(current_list);
			temp_path_node = temp_path->node_list;
			// install neighbors' flows
			next_num = init_fabric_neighbor_flows(temp_path_node->sw,sw_list,temp_path,current_list,tag);
			if(next_num > 0 ){
				install_fabric_middle_flow(temp_path_node->sw,temp_path_node->port->port_no,tag);
			}
			insert_fabric_list_path_to_list(ret_list,temp_path);
		}

		// clear variables
		delete_fabric_path_list(current_list);
		delete_fabric_sw_list(sw_list);
	}
	return ret_list;
};
/*
 * initialize fabric neighbor flows
 * return number of neighbors
 */
UINT2 init_fabric_neighbor_flows(gn_switch_t* sw,
		p_fabric_sw_list sw_list,
		p_fabric_path current_path,
		p_fabric_path_list current_list,
		UINT4 tag){
	// initialize the variables
	UINT2 i = 0,ret=0;
	gn_port_t* sw_port = NULL;
	gn_switch_t* neighbor = NULL;
	p_fabric_sw_node sw_node = NULL;
	p_fabric_path temp_path = NULL;
	p_fabric_path_node temp_node = NULL;
//	printf("Fabric switch:%d get neighbors!\n",sw->dpid);
	while(i < sw->n_ports){
		// get neighbor switch
		if(sw->neighbor[i] != NULL && sw->neighbor[i]->sw != NULL){
			neighbor = sw->neighbor[i]->sw;
			// get a sw node by switch
			sw_node = remove_fabric_sw_list_by_sw(sw_list,neighbor);
			if(sw_node != NULL){
				// install first flows to neighbor switch
				//sw_port = get_neighbor_port(sw,neighbor);
				sw_port = sw->neighbor[i]->port;

//				printf("Fabric switch:%d neighbor:: port:%d;neighbor:%d\n",sw->dpid,sw_port->port_no,neighbor->dpid);
				if(sw_node->tag != 0)
					install_fabric_first_flow(neighbor,sw_port->port_no,tag);
				// add neighbor
				// create current path node
				temp_node = create_fabric_path_node(sw_node->sw,sw_port);
				// copy current path
				temp_path = copy_fabric_path(current_path);
				// insert into the copied current path
				insert_fabric_node_to_path(temp_path,temp_node);
				// add the path to last one
				add_fabric_path_to_list(current_list,temp_path);

				ret++;
			}
			delete_fabric_sw_node(sw_node);
		}
		i++;
	}
	return ret;
};
/*
 * get neighbor port to self(switch)
 */
gn_port_t* get_neighbor_port(gn_switch_t* sw,gn_switch_t* neighbor){
	// initialize the variables
	UINT2 i = 0;
	while(i < neighbor->n_ports){
		if(neighbor->neighbor[i] != NULL && neighbor->neighbor[i]->sw == sw){
			return neighbor->neighbor[i]->port;
		}
		i++;
//		if(neighbor->neighbor[i] != NULL){
//			printf("Fabric switch:%d's neighbor switch:%d;port:%d\n",neighbor->dpid,neighbor->neighbor[i]->sw->dpid,neighbor->neighbor[i]->port->port_no);
//			if(neighbor->neighbor[i]->sw == sw){
//				return neighbor->neighbor[i]->port;
//			}
//		}
//		i++;
	}
	return NULL;
};
/*
 * initialize fabric base flows
 */
void init_fabric_base_flows(){

	// initialize the variables
	gn_switch_t* sw = NULL;
	p_fabric_sw_node p_sentinel = g_fabric_sw_list.node_list;

	while(p_sentinel != NULL){
		sw = p_sentinel->sw;
		install_fabric_base_flows(sw);
		p_sentinel = p_sentinel->next;
	}

	return;
}
/*
 * initialize id for each switch
 */
void init_fabric_id(){

	// initialize the variables
	t_fabric_sw_node sentinel;
	p_fabric_sw_node p_sentinel = NULL;
	gn_switch_t* sw = NULL;
	UINT4 i=0;

	// initialize
	g_fabric_tag = FABRIC_START_TAG;
	p_sentinel = &sentinel;
	p_sentinel->next = NULL;

	for(i = 0; i < g_server.max_switch; i++){
		if (g_server.switches[i].state){
			sw = &g_server.switches[i];
			p_sentinel->next = create_fabric_sw_node(sw,g_fabric_tag);
			// increase tag value
			g_fabric_tag++;
			p_sentinel = p_sentinel->next;
			g_fabric_sw_list.num++;
		}
	}
	g_fabric_sw_list.node_list = sentinel.next;
	g_fabric_sw_list_total.node_list = copy_fabric_sw_node(sentinel.next);
	g_fabric_sw_list_total.num = g_fabric_sw_list.num;
	return;
};
void init_fabric_id_by_dpids(UINT8* dpids,UINT4 len){
	// initialize the variables
	t_fabric_sw_node sentinel,total_sentinel;
	p_fabric_sw_node p_sentinel = NULL,p_total_sentinel = NULL;
	gn_switch_t* sw = NULL;
	UINT4 i=0;

	// initialize
	g_fabric_tag = FABRIC_START_TAG;
	p_sentinel = &sentinel;
	p_sentinel->next = NULL;
	p_total_sentinel = &total_sentinel;
	p_total_sentinel->next = NULL;

	for(i = 0; i < g_server.max_switch; i++){
		if (g_server.switches[i].state){
			sw = &g_server.switches[i];
			if( 0 != check_dpid_avaliable(sw->dpid,dpids,len)){
				p_sentinel->next = create_fabric_sw_node(sw,g_fabric_tag);
				p_total_sentinel->next = create_fabric_sw_node(sw,g_fabric_tag);
				// increase tag value
				g_fabric_tag++;
				p_sentinel = p_sentinel->next;
				g_fabric_sw_list.num++;
			}else{
				p_total_sentinel->next = create_fabric_sw_node(sw,0);
			}
			p_total_sentinel = p_total_sentinel->next;
			g_fabric_sw_list_total.num++;
		}
	}
	g_fabric_sw_list.node_list = sentinel.next;
	g_fabric_sw_list_total.node_list = total_sentinel.next;
};
UINT1 check_dpid_avaliable(UINT8 dpid,UINT8* dpids,UINT4 len){
	UINT4 i = 0;
	for(i = 0 ; i < len; i++){
		if(dpids[i] == dpid){
			return 1;
		}
	}
	return 0;
};
/*
 * clear all global variables
 */
void clear_fabric_server(){
	// initialize the variables
	p_fabric_sw_node p_sentinel = NULL;
	p_fabric_path_list p_sentinel_list = NULL;
	//printf("before clear g_fabric_sw_list num: %d;total num:%d\n",g_fabric_sw_list.num,g_fabric_sw_list_total.num);
	// clear all node
	p_sentinel = clear_fabric_sw_list(&g_fabric_sw_list_total);
	while(p_sentinel != NULL){
		p_sentinel = delete_fabric_sw_node(p_sentinel);
	}

	// clear node and id list
	p_sentinel = clear_fabric_sw_list(&g_fabric_sw_list);
	while(p_sentinel != NULL){
		p_sentinel = delete_fabric_sw_node(p_sentinel);
	}
	g_fabric_tag = 0;

	// clear path
	p_sentinel_list = g_fabric_path_list;
	while(p_sentinel_list != NULL){
		delete_fabric_path_list(p_sentinel_list);
		g_fabric_path_list = g_fabric_path_list->next;
		p_sentinel_list = g_fabric_path_list;
	}
//	// clear host
//	init_fabric_host_list();
	//printf("after clear g_fabric_sw_list num: %d;total num:%d\n",g_fabric_sw_list.num,g_fabric_sw_list_total.num);
	return;
};
/*
 * delete switch flows
 */
void delete_fabric_flows(){
	UINT4 i=0;
	for(i = 0; i < g_server.max_switch; i++){
		if (g_server.switches[i].state){
			delete_fabric_flow(&g_server.switches[i]);
		}
	}
	return;
}
