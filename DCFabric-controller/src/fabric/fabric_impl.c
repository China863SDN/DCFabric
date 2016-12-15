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
 * fabric_impl.c
 *
 *  Created on: Jun 20, 2015
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 *
 *  Modified on: May 19, 2015
 */
#include "fabric_path.h"
#include "fabric_impl.h"
#include "fabric_flows.h"
#include "fabric_thread.h"
#include "fabric_host.h"
#include "../event/event_service.h"
#include <stdio.h>
#include "forward-mgr.h"
#include "fabric_stats.h"
#include "../cluster-mgr/cluster-mgr.h"
#include "../cluster-mgr/hbase_sync.h"
#include "openflow-common.h"
#include "../qos-mgr/qos-mgr.h"





extern UINT4 g_openstack_on;
/*****************************
 * intern functions
 *****************************/
////////////////////////////////////////////////
// initialize functions
////////////////////////////////////////////////
void clear_fabric_server();
void init_fabric_id();
void init_fabric_id_by_dpids(UINT8* dpids,UINT4 len);
void init_fabric_base_flows();
void init_fabric_swap_flows();
p_fabric_path_list init_fabric_swap_flows_single(gn_switch_t* sw, UINT4 tag);
UINT2 init_fabric_neighbor_flows(gn_switch_t* sw,
		p_fabric_sw_list sw_list,
		p_fabric_path current_path,
		p_fabric_path_list current_list,
		p_fabric_path_list ret_list,
		p_fabric_sw_list marked_list,
		UINT4 tag);
gn_port_t* get_neighbor_port(gn_switch_t* sw,gn_switch_t* neighbor);
////////////////////////////////////////////////
// update functions
////////////////////////////////////////////////
void of131_fabric_impl_add_sw_ports(p_event_sw_port sw_portList,UINT4 num);
void of131_fabric_impl_delete_sw_ports(p_event_sw_port sw_portList,UINT4 num);
void update_fabric_swap_flows();
p_fabric_path_list update_fabric_swap_flows_single(gn_switch_t* sw, UINT4 tag);
UINT2 update_fabric_neighbor_flows(gn_switch_t* sw,
		p_fabric_sw_list sw_list,
		p_fabric_path current_path,
		p_fabric_path_list current_list,
		p_fabric_path_list ret_list,
		p_fabric_sw_list marked_list,
		UINT4 tag);
////////////////////////////////////////////////
// delete functions
////////////////////////////////////////////////
void delete_fabric_flows();

////////////////////////////////////////////////
// help functions
////////////////////////////////////////////////
UINT1 check_dpid_avaliable(UINT8 dpid);
////////////////////////////////////////////////
// register functions
////////////////////////////////////////////////
void register_fabric_functions_into_event();
void unregister_fabric_functions_into_event();
////////////////////////////////////////////////
// weight functions
////////////////////////////////////////////////
BOOL is_min_weight_neighbor(gn_switch_t *sw, UINT8 weight, p_fabric_sw_list sw_list, p_fabric_sw_list marked_list, p_fabric_path_list current_list, p_fabric_path_list ret_list, UINT4 tag);
void upadte_fabric_neighbor_weight(gn_switch_t *sw, gn_switch_t *neighbor, UINT8 sw_inc, UINT8 edge_inc);
void init_fabric_neighbor_weight(p_fabric_sw_node node_list, UINT8 sw_weight, UINT8 edge_weight);

/*****************************
 * global variables
 *****************************/
// switch server
extern gn_server_t g_server;

// fabric id & switch list
t_fabric_sw_list g_fabric_sw_list;
t_fabric_sw_list g_fabric_sw_list_total;
t_fabric_sw_list g_fabric_sw_list_old_tag;
UINT4 g_fabric_tag = 0;
UINT4 vlan_start_tag = FABRIC_START_TAG;

// fabric path list
p_fabric_path_list g_fabric_path_list = NULL;

// fabric label
UINT1 g_fabric_state = 0;
UINT8* g_fabric_dpids = NULL;
UINT4 g_fabric_dpids_num = 0;

// register fucntions label
UINT1 g_register_state = 0;

//  fabric path weight
UINT8 g_sw_init_weight = 0;
UINT8 g_sw_once_inc = 0;
UINT8 g_path_init_weight = 0;
UINT8 g_path_once_inc = 0;

/*****************************
 * interface functions
 *****************************/



/*
 * setup fabric by dpids
 */
void of131_fabric_impl_setup_by_dpids(UINT8* dpids,UINT4 len){
	if(len != 0){
		// update dpids
		of131_fabric_impl_set_dpids(dpids,len);
	}
	of131_fabric_impl_setup();

	return;
}



/*
 * setup fabric
 */
void of131_fabric_impl_setup(){
    //path weight init
	INT1* value = get_value(g_controller_configure, "[topo_conf]", "sw_init_weight");
	g_sw_init_weight = (NULL == value) ? 10 : strtoull(value, NULL, 10);
	value = get_value(g_controller_configure, "[topo_conf]", "sw_once_inc");
	g_sw_once_inc = (NULL == value) ? 1 : strtoull(value, NULL, 10);

	value = get_value(g_controller_configure, "[topo_conf]", "path_init_weight");
	g_path_init_weight = (NULL == value) ? 10 : strtoull(value, NULL, 10);
	value = get_value(g_controller_configure, "[topo_conf]", "path_once_inc");
	g_path_once_inc = (NULL == value) ? 1 : strtoull(value, NULL, 10);

	value = get_value(g_controller_configure, "[controller]", "vlan_start_tag");
	vlan_start_tag = (NULL == value) ? FABRIC_START_TAG: strtoull(value, NULL, 10);

	// alloc memory from pool
	init_fabric_path_mem();

	// clear global variables
	clear_fabric_server();

	// initialize fabric id
	if(g_fabric_dpids_num == 0){
		init_fabric_id();
	}else{
		init_fabric_id_by_dpids(g_fabric_dpids,g_fabric_dpids_num);
	}

	// initialize fabric base flows
	init_fabric_base_flows();

	// initialize fabric swap flows
	init_fabric_swap_flows();

	if (0 == g_openstack_on) {
		// start fabric threads
		start_fabric_thread();
	}

	// set the state is available
	g_fabric_state = 1;

	init_handler();

	// register event
	register_fabric_functions_into_event();

    init_fabric_stats();

    //syn data to hbase
    if (g_controller_role == OFPCR_ROLE_MASTER)
	{	
	    persist_fabric_all();
	}

	start_qos_mgr();

	//printf("Fabric: v 2015/05/15 \nof131_fabric_impl_setup!\n");
	LOG_PROC("INFO", "of131_fabric_impl_setup!");

	return;
}




/*
 * delete fabric
 */
void of131_fabric_impl_delete(){
	// set the state is unavailable
	g_fabric_state = 0;
	// unregister event
	unregister_fabric_functions_into_event();

	// stop fabric threads
	stop_fabric_thread();
	// initialize flows
	init_fabric_flows();

	// clear flows
	delete_fabric_flows();

	// clear global variables
	clear_fabric_server();

	// delete memory to pool
	destory_fabric_path_men();

    fini_fabric_stats();
    
	//printf("of131_fabric_impl_delete!\n");
	LOG_PROC("INFO", "of131_fabric_impl_delete!");
	return;
}



/*
 * set fabric setup by the assigned dpids' switchs
 */
void of131_fabric_impl_set_dpids(UINT8* dpids, UINT4 len){

	if(g_fabric_dpids != NULL){
		free(g_fabric_dpids);
		g_fabric_dpids = NULL;
	}
	g_fabric_dpids = (UINT8*)malloc(sizeof(UINT8)*len);
	g_fabric_dpids_num = len;
	return;
}




/*
 * set fabric setup by all switch
 */
void of131_fabric_impl_unset_dpids(){
	if(g_fabric_dpids != NULL){
		free(g_fabric_dpids);
		g_fabric_dpids = NULL;
	}
	g_fabric_dpids_num = 0;
	return;
}



/*
 *
 */
void of131_fabric_impl_add_sws(gn_switch_t** swList, UINT4 num){
	UINT4 i = 0;
	gn_switch_t* sw = NULL;
	p_fabric_sw_node node = NULL;
	p_fabric_sw_node available_node = NULL;
	//LOG_PROC("INFO", "of131_fabric_impl_add_sws");
	// assign a new tag for new switch
	// and initialize base flows
	for(i = 0 ; i < num; i++){
		// get sw;
		sw = swList[i];
		// init sw
		if(sw->state)
        {      
			delete_fabric_flow(sw);
        }
        else
        {
            continue;
        }

		// initialize base flows if avaliable
		if( 1 == check_dpid_avaliable(sw->dpid)){
			// get old tag first
			// if there has, use it
			// if there has not, create a new one
			node = pop_fabric_sw_list(&g_fabric_sw_list_old_tag);
			if(node == NULL){
				node = create_fabric_sw_node(sw,g_fabric_tag);
				g_fabric_tag++;
			}else{
				node->sw = sw;
			}
			available_node = create_fabric_sw_node(node->sw,node->tag);
			// add to fabric node list
			insert_fabric_sw_list(&g_fabric_sw_list,available_node);
			// install base flows
			install_fabric_base_flows(sw);
		}else{
			node = create_fabric_sw_node(sw,0);
		}
        
		// add sw node to total list
		insert_fabric_sw_list(&g_fabric_sw_list_total,node);
	}

    //syn data to hbase
    if (g_controller_role == OFPCR_ROLE_MASTER)
    {   
        persist_fabric_all();
    }

	return;
}




void of131_fabric_impl_remove_sws(gn_switch_t** swList, UINT4 num){
	UINT4 i = 0;
	gn_switch_t* sw = NULL;
	p_fabric_sw_node node = NULL;
	// LOG_PROC("INFO", "of131_fabric_impl_remove_sws");
	// assign a new tag for new switch
	// and initialize base flows
	for(i = 0 ; i < num; i++){
		sw = swList[i];
		delete_fabric_host_from_list_by_sw(sw);
		node = remove_fabric_sw_list_by_sw(&g_fabric_sw_list_total,sw);
		if(node != NULL){
			// delete sw node
			delete_fabric_sw_node(node);
		}
		node = remove_fabric_sw_list_by_sw(&g_fabric_sw_list,sw);
		if(node != NULL){
			// add to old ones
			insert_fabric_sw_list(&g_fabric_sw_list_old_tag,node);
		}
	}

    //syn data to hbase
    if (g_controller_role == OFPCR_ROLE_MASTER)
    {   
        persist_fabric_all();
    }

	return;
}




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
	//printf("Err in %s  dpid: %llu \n",FN,sw->dpid);
	return 0;
}



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
}



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
}


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
}



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
}


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
    p_sentinel->next = NULL;

    // get sw node list
    p_fabric_sw_node sw_node= g_fabric_sw_list.node_list;

    //initialize sw's neighbor weight(edge weight) sw weight(node weight)
    init_fabric_neighbor_weight(
        sw_node, 
        g_sw_init_weight,
        g_path_init_weight);
    
    while(sw_node != NULL){
        // initialize each switch's swap flows
        p_sentinel->next = init_fabric_swap_flows_single(sw_node->sw,sw_node->tag);
        sw_node = sw_node->next;
        if (NULL != p_sentinel->next)
        {
            p_sentinel = p_sentinel->next;
        }
        
    }
    
    // set the global path list variable
    g_fabric_path_list = sentinel.next;
    return;
}



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
    p_fabric_sw_list marked_list = NULL; //mark the sw when find path

    UINT2 next_num = 0;// next switch num

    // check swith object
    if(sw != NULL && 1 == sw->state){
        // copy total switch list
        sw_list = copy_fabric_sw_list(&g_fabric_sw_list_total);
        marked_list = create_fabric_sw_list();
        insert_fabric_sw_list_by_sw(marked_list, sw, tag);
        // create list
        current_list = create_fabric_path_list_NULL(sw);
        ret_list = create_fabric_path_list_NULL(sw);

        // create path
        temp_path = create_fabric_path(sw,NULL);

        // remove destination switch from sw_list
        temp_sw_node = remove_fabric_sw_list_by_sw(sw_list, sw);
        delete_fabric_sw_node(temp_sw_node);

        // create destination node swap flow
        install_fabric_last_flow(sw,tag);

        // install neighbors' flows
        init_fabric_neighbor_flows(sw, sw_list, temp_path, current_list, ret_list, marked_list, tag);

        // insert first path to list
        insert_fabric_list_path_to_list(ret_list,temp_path);

        while(current_list->num > 0){
            temp_path = remove_first_fabric_path_from_list(current_list);
            temp_path_node = temp_path->node_list;
            next_num = init_fabric_neighbor_flows(temp_path_node->sw, sw_list, temp_path, current_list, ret_list, marked_list, tag);
            if(next_num > 0 ){
                install_fabric_middle_flow(temp_path_node->sw,temp_path_node->port->port_no,tag);
            }

            insert_fabric_list_path_to_list(ret_list,temp_path);
        }

        // clear variables
        delete_fabric_path_list(current_list);
        delete_fabric_sw_list(sw_list);
        delete_fabric_sw_list(marked_list);
    }

    return ret_list;
}



/*
 * initialize fabric neighbor flows
 * return number of neighbors
 */
UINT2 init_fabric_neighbor_flows(gn_switch_t* sw,
        p_fabric_sw_list sw_list,
        p_fabric_path current_path,
        p_fabric_path_list current_list,
        p_fabric_path_list ret_list,
        p_fabric_sw_list marked_list,
        UINT4 tag){

    // initialize the variables
    UINT2 i = 0,ret=0;
    gn_port_t* sw_port = NULL;
    gn_switch_t* neighbor = NULL;
    p_fabric_sw_node sw_node = NULL;
    p_fabric_path temp_path = NULL;
    p_fabric_path_node temp_node = NULL;

    UINT8 total_weight = 0;
    while(i < sw->n_ports){

        // get neighbor switch
        if(NULL != sw->neighbor[i] && NULL != sw->neighbor[i]->sw && NULL != sw->neighbor[i]->port){

            total_weight = sw->neighbor[i]->weight + sw->weight + sw->neighbor[i]->sw->weight;
            if (!is_min_weight_neighbor(sw->neighbor[i]->sw, total_weight, sw_list, marked_list, current_list, ret_list, tag))
            {
                i++;
                continue;
            }

            // get a sw node by switch
            neighbor = sw->neighbor[i]->sw;
            sw_node = remove_fabric_sw_list_by_sw(sw_list, neighbor);
            if(NULL != sw_node){
                // install first flows to neighbor switch
                sw_port = sw->neighbor[i]->port;

                if(sw_node->tag != 0){
                    install_fabric_first_flow(neighbor, sw_port->port_no, tag);
                }
                // add neighbor
                // create current path node
                temp_node = create_fabric_path_node(sw_node->sw, sw_port);
                // copy current path
                temp_path = copy_fabric_path(current_path);
                // insert into the copied current path
                insert_fabric_node_to_path(temp_path, temp_node);
                // add the path to last one
                add_fabric_path_to_list(current_list, temp_path);

                upadte_fabric_neighbor_weight(
                    sw, 
                    neighbor,
                    g_sw_once_inc,
                    g_path_once_inc);
                insert_fabric_sw_list(marked_list, sw_node);
                ret++;
            }
        }
        
        i++;
    }
    
    return ret;
}



BOOL is_min_weight_neighbor(
    gn_switch_t* sw, 
    UINT8 weight, 
    p_fabric_sw_list sw_list, 
    p_fabric_sw_list marked_list, 
    p_fabric_path_list current_list, 
    p_fabric_path_list ret_list, 
    UINT4 tag)
{
	gn_switch_t* sw_node;
	t_fabric_sw_node sentinel;
	p_fabric_sw_node p_sentinel = &sentinel;
	p_sentinel->next = marked_list->node_list;
    
	INT4 i = 0;
    BOOL flag = TRUE;
    UINT8 local_weight = 0;
    UINT8 min_weight = weight;
    gn_switch_t *min_sw = NULL;
    neighbor_t *min_neighbor = NULL;
    //find the min neighbor from marked_list
	while(NULL != p_sentinel->next && NULL != p_sentinel->next->sw)
	{
		sw_node = p_sentinel->next->sw;
		for(i = 0; i < sw_node->n_ports; i++) {
  			if(NULL != sw_node->neighbor[i] && NULL != sw_node->neighbor[i]->sw && sw_node->neighbor[i]->sw == sw)
			{
			    local_weight = sw_node->neighbor[i]->weight + sw_node->weight + sw->weight;
                if (local_weight < min_weight)
                {
                    flag = FALSE;
                    min_sw = sw_node;
                    min_neighbor = sw_node->neighbor[i];
                    min_weight = local_weight;
                }
			}
		}
        
		p_sentinel = p_sentinel->next;
	}
    
    if (!flag && NULL != current_list && NULL != min_sw && NULL != min_neighbor)
    {
        p_fabric_path min_path = NULL;
        //find the min path from current list
        p_fabric_path path_list = current_list->path_list;
        while (NULL != path_list)
        {
            if (NULL != path_list->src && path_list->src->dpid == min_sw->dpid)
            {
                min_path = path_list;
                break;
            }

            path_list = path_list->next;
        }

        //find the min path from ret list
        if (NULL == min_path)
        {
            path_list = ret_list->path_list;
            while (NULL != path_list)
            {
                if (NULL != path_list->src && path_list->src->dpid == min_sw->dpid)
                {
                    min_path = path_list;
                    break;
                }
            
                path_list = path_list->next;
            }
        }

        if (NULL == min_path)
        {
            LOG_PROC("WARN", "Can't find the min path from the existing path list");
            return TRUE;
        }

        //add min_path to current_path
        p_fabric_sw_node node = remove_fabric_sw_list_by_sw(sw_list, sw);
        if(NULL != node){
            // install first flows to neighbor switch
            gn_port_t *sw_port = min_neighbor->port;
        
            if(node->tag != 0){
                install_fabric_first_flow(sw, sw_port->port_no, tag);
            }

            // create current path node
            p_fabric_path_node temp_node = create_fabric_path_node(node->sw, sw_port);
            // copy current path
            p_fabric_path temp_path = copy_fabric_path(min_path);
            // insert into the copied current path
            insert_fabric_node_to_path(temp_path, temp_node);
            // add the path to last one
            add_fabric_path_to_list(current_list, temp_path);
        
            upadte_fabric_neighbor_weight(
                min_sw, 
                sw, 
                g_sw_once_inc,
                g_path_once_inc);
            insert_fabric_sw_list(marked_list, node);

            //add middle flow for min_sw
            p_fabric_path_node min_node = min_path->node_list;
            if (NULL != min_node && NULL != min_node->sw && NULL != min_node->port)
            {
                install_fabric_middle_flow(min_node->sw, min_node->port->port_no, tag);
            }
        }

    }

	return flag;
}


//when find path change sw neighbor weight
void upadte_fabric_neighbor_weight(gn_switch_t * sw, gn_switch_t * neighbor, UINT8 sw_inc, UINT8 edge_inc)
{
	INT4 i = 0;
	for(; i < sw->n_ports; i++)
	{
	    if(NULL != sw->neighbor[i] && sw->neighbor[i]->sw == neighbor)
	    {
	    	sw->neighbor[i]->weight += edge_inc;
	    	sw->weight += sw_inc;
	    	neighbor->weight += sw_inc;
	    }
	}
    
	return;
}


// initialize switch neighbors' weights
void init_fabric_neighbor_weight(p_fabric_sw_node node_list, UINT8 sw_weight, UINT8 edge_weight)
{
    UINT4 i = 0;
	while(NULL != node_list && NULL != node_list->sw)
	{
	   node_list->sw->weight = sw_weight;
       for (i = 0; i < node_list->sw->n_ports; i++)
       {
           if (NULL != node_list->sw->neighbor[i])
           {
               node_list->sw->neighbor[i]->weight = edge_weight;
           }
       }

	   node_list = node_list->next;
	}

	return;
}


void of131_fabric_impl_add_sw_ports(p_event_sw_port sw_portList,UINT4 num){
	update_fabric_swap_flows();
    
    //syn data to hbase
    if (g_controller_role == OFPCR_ROLE_MASTER)
    {   
        persist_fabric_all();
    }
    
	return;
}


void of131_fabric_impl_delete_sw_ports(p_event_sw_port sw_portList,UINT4 num){
	update_fabric_swap_flows();
    
    //syn data to hbase
    if (g_controller_role == OFPCR_ROLE_MASTER)
    {   
        persist_fabric_all();
    }
    
	return;
}



void update_fabric_swap_flows(){
    // initialize the variables
    t_fabric_path_list sentinel;
    p_fabric_path_list p_sentinel = &sentinel;
    p_sentinel->next = NULL;
    LOG_PROC("INFO","update_fabric_swap_flows begin!");

    p_fabric_sw_node sw_node= g_fabric_sw_list.node_list;
    
    //initialize sw's neighbor weight(edge weight) sw weight(node weight)
    init_fabric_neighbor_weight(
        sw_node, 
        g_sw_init_weight,
        g_path_init_weight);
    while(sw_node != NULL){
        // initialize each switch's swap flows
        p_sentinel->next = update_fabric_swap_flows_single(sw_node->sw,sw_node->tag);
        //print_list_flow_num();
        sw_node = sw_node->next;
        if (NULL != p_sentinel->next)
        {
            p_sentinel = p_sentinel->next;
        }
    }

    // swap path
    p_sentinel = g_fabric_path_list;
    // clear path
    //LOG_PROC("INFO","update_fabric_swap_flows: clear path!");
    while(p_sentinel != NULL){
        g_fabric_path_list = g_fabric_path_list->next;
        delete_fabric_path_list(p_sentinel);
        p_sentinel = g_fabric_path_list;
    }

    // set the global path list variable
    g_fabric_path_list = sentinel.next;
    //LOG_PROC("INFO","update_fabric_swap_flows end!");

    return;
}




/*
 * update a switch for fabric swap flows
 * return the swap flows' path list
 */
p_fabric_path_list update_fabric_swap_flows_single(gn_switch_t* sw, UINT4 tag){
    // initialize the variables
    p_fabric_path_list current_list = NULL; // current path list
    p_fabric_path_list ret_list = NULL; // return path list
    p_fabric_path temp_path = NULL; // temp variable : for path
    p_fabric_path_node temp_path_node = NULL; // temp variable : for path node

    p_fabric_sw_list sw_list = NULL; // copy total switch list
    p_fabric_sw_node temp_sw_node = NULL; // temp variable : for switch node
    p_fabric_sw_list marked_list = NULL; //mark the sw when find path

    UINT2 next_num = 0;// next switch num

    // check swith object
    if(sw != NULL && 1 == sw->state){

        // copy total switch list
        sw_list = copy_fabric_sw_list(&g_fabric_sw_list_total);
        marked_list = create_fabric_sw_list();
        insert_fabric_sw_list_by_sw(marked_list, sw, tag);
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
        update_fabric_neighbor_flows(sw,sw_list, temp_path, current_list, ret_list, marked_list, tag);

        // insert first path to list
        insert_fabric_list_path_to_list(ret_list,temp_path);

        while(current_list->num > 0){
            temp_path = remove_first_fabric_path_from_list(current_list);
            temp_path_node = temp_path->node_list;
            // install neighbors' flows
            next_num = update_fabric_neighbor_flows(temp_path_node->sw, sw_list, temp_path, current_list, ret_list, marked_list, tag);
            if(next_num > 0 ){
                install_fabric_middle_flow(temp_path_node->sw,temp_path_node->port->port_no,tag);
            }
            insert_fabric_list_path_to_list(ret_list,temp_path);
        }

        // clear variables
        delete_fabric_path_list(current_list);
        delete_fabric_sw_list(sw_list);
        delete_fabric_sw_list(marked_list);
    }
    
    return ret_list;
}




/*
 * update fabric neighbor flows
 * return number of neighbors
 */
UINT2 update_fabric_neighbor_flows(gn_switch_t* sw,
        p_fabric_sw_list sw_list,
        p_fabric_path current_path,
        p_fabric_path_list current_list,
        p_fabric_path_list ret_list,
        p_fabric_sw_list marked_list,
        UINT4 tag){
    // initialize the variables
    UINT2 i = 0,ret=0;
    gn_port_t* sw_port = NULL;
    gn_switch_t* neighbor = NULL;
    p_fabric_sw_node sw_node = NULL;
    p_fabric_path temp_path = NULL;
    p_fabric_path_node temp_node = NULL;

    UINT8 total_weight = 0;

    while(i < sw->n_ports){
    
        // get neighbor switch
        if(NULL != sw->neighbor[i] && NULL != sw->neighbor[i]->sw && NULL != sw->neighbor[i]->port){
    
            total_weight = sw->neighbor[i]->weight + sw->weight + sw->neighbor[i]->sw->weight;
            if (!is_min_weight_neighbor(sw->neighbor[i]->sw, total_weight, sw_list, marked_list, current_list, ret_list, tag))
            {
                i++;
                continue;
            }

            neighbor = sw->neighbor[i]->sw;
            // get a sw node by switch
            sw_node = remove_fabric_sw_list_by_sw(sw_list,neighbor);
            if(NULL != sw_node){
                // install first flows to neighbor switch
                sw_port = sw->neighbor[i]->port;

                // if is fabric sw
                if(sw_node->tag != 0){
                    install_fabric_first_flow(neighbor,sw_port->port_no,tag);
                }
                // add neighbor
                // create current path node
                temp_node = create_fabric_path_node(sw_node->sw,sw_port);
                // copy current path
                temp_path = copy_fabric_path(current_path);
                // insert into the copied current path
                insert_fabric_node_to_path(temp_path,temp_node);
                // add the path to last one
                add_fabric_path_to_list(current_list,temp_path);

                upadte_fabric_neighbor_weight(
                    sw, 
                    neighbor,
                    g_sw_once_inc,
                    g_path_once_inc);
                insert_fabric_sw_list(marked_list, sw_node);
                ret++;
            }
        }
        i++;
    }
    
    return ret;
}





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
}



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
	g_fabric_tag = (0 == vlan_start_tag) ? FABRIC_START_TAG : vlan_start_tag;
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
}



void init_fabric_id_by_dpids(UINT8* dpids,UINT4 len){
	// initialize the variables
	t_fabric_sw_node sentinel,total_sentinel;
	p_fabric_sw_node p_sentinel = NULL,p_total_sentinel = NULL;
	gn_switch_t* sw = NULL;
	UINT4 i=0;

	// initialize
	g_fabric_tag = (0 == vlan_start_tag) ? FABRIC_START_TAG : vlan_start_tag;
	p_sentinel = &sentinel;
	p_sentinel->next = NULL;
	p_total_sentinel = &total_sentinel;
	p_total_sentinel->next = NULL;

	for(i = 0; i < g_server.max_switch; i++){
		if (g_server.switches[i].state){
			sw = &g_server.switches[i];
			if( 0 != check_dpid_avaliable(sw->dpid)){
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
}




UINT1 check_dpid_avaliable(UINT8 dpid){
	UINT4 i = 0;
	if(g_fabric_dpids_num == 0){
		return 1;
	}
	for(i = 0 ; i < g_fabric_dpids_num; i++){
		if(g_fabric_dpids[i] == dpid){
			return 1;
		}
	}
	return 0;
}




/*
 * clear all global variables
 */
void clear_fabric_server(){
	// initialize the variables
	p_fabric_sw_node p_sentinel = NULL;
	p_fabric_path_list p_sentinel_list = NULL;
	//printf("before clear g_fabric_sw_list num: %d;total num:%d\n",g_fabric_sw_list.num,g_fabric_sw_list_total.num);

	// clear old node and id list
	p_sentinel = clear_fabric_sw_list(&g_fabric_sw_list_old_tag);
	while(p_sentinel != NULL){
		p_sentinel = delete_fabric_sw_node(p_sentinel);
	}

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
		g_fabric_path_list = g_fabric_path_list->next;
		delete_fabric_path_list(p_sentinel_list);
		p_sentinel_list = g_fabric_path_list;
	}
//	// clear host
//	init_fabric_host_list();
	//printf("after clear g_fabric_sw_list num: %d;total num:%d\n",g_fabric_sw_list.num,g_fabric_sw_list_total.num);
	return;
}



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
/////////////////////////////////////////////////////////
// get flows
/////////////////////////////////////////////////////////
//p_fabric_impl_flow_node remove_fabric_impl_flow_node_from_old_set(gn_switch_t* sw,UINT4 tag,UINT1 table_id){
//	p_fabric_impl_flow_node ret = NULL;
//	p_fabric_impl_flow_list list = NULL;
//    list = get_fabric_impl_flow_list_from_set_by_sw(&g_fabric_impl_flow_list_set_old,sw);
//    if(list != NULL){
//    	ret = remove_fabric_impl_flow_node_from_list_by_tag_and_table(list,sw,tag,table_id);
//    	//print_list_flow_ones(list);
//    }
//    return ret;
//};
//p_fabric_impl_flow_node remove_fabric_impl_flow_nodes_by_tag(UINT4 tag){
//	t_fabric_impl_flow_node sentienal,ret;
//	p_fabric_impl_flow_node p_sentienal = &sentienal,p_ret=&ret;
//	p_fabric_impl_flow_list list = NULL;
//	// get all list
//	list = g_fabric_impl_flow_list_set.list;
//	ret.next = NULL;
//	while(list != NULL){
//		sentienal.next = list->list;
//		p_sentienal = &sentienal;
//		while(p_sentienal->next != NULL){
//			if(p_sentienal->next->tag == tag){
//				p_ret->next = p_sentienal->next;
//				p_sentienal->next = p_sentienal->next->next;
//				p_ret = p_ret->next;
//			}else{
//				p_sentienal = p_sentienal->next;
//			}
//		}
//		list->list = sentienal.next;
//		list = list->next;
//	}
//	return ret.next;
//};
//void insert_fabric_impl_flow_node(p_fabric_impl_flow_node node){
//	p_fabric_impl_flow_list list = NULL;
//    list = get_fabric_impl_flow_list_from_set_by_sw(&g_fabric_impl_flow_list_set,node->sw);
//    if(list == NULL){
//    	list = create_fabric_impl_flow_list(node->sw);
//    	insert_fabric_impl_flow_list_to_set(&g_fabric_impl_flow_list_set,list);
//    }
//    insert_fabric_impl_flow_node_to_list(list,node);
//    return;
//};
//void insert_delete_fabric_impl_flow_node(p_fabric_impl_flow_node node){
//	insert_fabric_impl_flow_node_to_list(&g_fabric_impl_flow_list_delete,node);
//};
//
//void clear_delete_fabric_impl_flow(){
//	p_fabric_impl_flow_node temp = NULL;
//	temp = g_fabric_impl_flow_list_delete.list;
//	while(temp != NULL){
//		delete_fabric_impl_flow(temp->sw,temp->port_no,temp->table_id,temp->tag);
//		temp = delete_fabric_impl_flow_node(temp);
//	}
//	g_fabric_impl_flow_list_delete.list = NULL;
//	return;
//};
//
//void clear_delete_fabric_impl_flow_old(){
//	p_fabric_impl_flow_list list = NULL;
//	p_fabric_impl_flow_node node = NULL;
//	list = clear_fabric_impl_flow_list_set(&g_fabric_impl_flow_list_set_old);
//	while(list != NULL){
//		node = list->list;
//		while(node != NULL){
//			delete_fabric_impl_flow(node->sw,node->port_no,node->table_id,node->tag);
//			node = node->next;
//		}
//		list = delete_fabric_impl_flow_list(list);
//	}
//};
/////////////////////////////////////////////////////////
// register functions
/////////////////////////////////////////////////////////
void register_fabric_functions_into_event(){
	if(g_register_state == 0){
		register_add_switch_function(of131_fabric_impl_add_sws);
		register_delete_switch_function(of131_fabric_impl_remove_sws);
		register_add_switch_port_function(of131_fabric_impl_add_sw_ports);
		register_delete_switch_port_function(of131_fabric_impl_delete_sw_ports);
		g_register_state = 1;
	}
	return;
}



void unregister_fabric_functions_into_event(){
	if(g_register_state == 1){
		unregister_add_switch_function(of131_fabric_impl_add_sws);
		unregister_delete_switch_function(of131_fabric_impl_remove_sws);
		unregister_add_switch_port_function(of131_fabric_impl_add_sw_ports);
		unregister_delete_switch_port_function(of131_fabric_impl_delete_sw_ports);
		g_register_state = 0;
	}
}



/////////////////////////////////////////////////////////
// test functions
/////////////////////////////////////////////////////////
void of131_test_update(){
	UINT4 i = 0,remove_num = 0,add_num = 0;
	gn_switch_t* remove_sw_list[100]={NULL};
	gn_switch_t* add_sw_list[100]={NULL};
	gn_switch_t* sw = NULL;
	p_fabric_sw_node node = NULL;

	for(i = 0; i < g_server.max_switch; i++){
		sw = &g_server.switches[i];
		node = get_fabric_sw_list_by_sw(&g_fabric_sw_list_total,sw);
		if(sw->state && node == NULL){
			add_sw_list[add_num] = sw;
			add_num++;
		}else if(!sw->state && node != NULL){
			remove_sw_list[remove_num] = sw;
			remove_num++;
		}else if(sw->state && node->sw->dpid != sw->dpid){
			remove_sw_list[remove_num] = sw;
			remove_num++;
			add_sw_list[add_num] = sw;
			add_num++;
		}
	}
	of131_fabric_impl_remove_sws(remove_sw_list,remove_num);
	of131_fabric_impl_add_sws(add_sw_list,add_num);
}



//void print_list_flow_num(){
//	p_fabric_impl_flow_list list = NULL;
//	UINT4 flow_num = 0;
//	printf("****************************\n");
//	//printf("new num is: %d\n",g_fabric_impl_flow_list_set.num);
//	list = g_fabric_impl_flow_list_set.list;
//	while(list!= NULL){
//		flow_num += print_list_flow_ones(list);
//		list = list->next;
//	}
//	printf("flow total num is: %d\n",flow_num);
//
//	flow_num = print_list_flow_ones(&g_fabric_impl_flow_list_delete);
//	printf("flow delete num is: %d\n",flow_num);
////	printf("old num is: %d\n",g_fabric_impl_flow_list_set_old.num);
////	list = g_fabric_impl_flow_list_set_old.list;
////	while(list!= NULL){
////		print_list_flow_ones(list);
////		list = list->next;
////	}
//	printf("****************************\n");
//}
//UINT4 print_list_flow_ones(p_fabric_impl_flow_list list){
//	p_fabric_impl_flow_node head = list->list;
//	UINT4 i = 0;
////	printf("===============================\n");
//	while(head != NULL){
////		printf("flow node: tag:%d,table:%d,port:%d | addr:0x%x\n",head->tag,head->table_id,head->port_no,head);
//		head = head->next;
//		i++;
//	}
////	printf("list num is: %d | addr:0x%x\n",i,list);
////	printf("===============================\n");
//	return i;
//}

// get the output port of dpids
UINT4 get_out_port_between_switch(UINT8 src_dpid, UINT8 dst_dpid)
{
    INT1 src_str[48] = {0};
    INT1 dst_str[48] = {0}; 
        
	p_fabric_path path = NULL;
	p_fabric_path_node path_node = NULL;

	path = of131_fabric_get_path(src_dpid, dst_dpid);
    
    dpidUint8ToStr(src_dpid, src_str);
    dpidUint8ToStr(dst_dpid, dst_str);

    if ((0 == strlen(src_str)) || (0 == strlen(dst_str))) {
        LOG_PROC("INFO", "Fail to get path. Src dpid or Dst dpid is emtpy");
        return 0;
    }

	if (NULL == path) {
		LOG_PROC("INFO", "The path from %s to %s is not exist", src_str, dst_str);
		return 0;
	}

	path_node = path->node_list;
	if (NULL == path_node) {
		LOG_PROC("INFO", "The path from %s to %s has no node", src_str, dst_str);
		return 0;
	}

	if (NULL == path_node->port) {
		LOG_PROC("INFO", "The path from %s to %s has no port", src_str, dst_str);
		return 0;
	}

	// LOG_PROC("INFO", "The port of path from %llu to %llu is %d", src_dpid, dst_dpid, path_node->port->port_no);
	return path_node->port->port_no;
}

// this function is used to get port no between switch and host ip
UINT4 get_port_no_between_sw_ip(gn_switch_t* sw, UINT4 dst_ip)
{
	p_fabric_host_node host = get_fabric_host_from_list_by_ip(dst_ip);
	
	if ((NULL == host) || (NULL == host->sw)) {
		return 0;
	}

	UINT4 port_no = 0;
	
	if (sw == host->sw) {
		// port_no = host->port;
	}
	else {
		port_no = get_out_port_between_switch(sw->dpid, host->sw->dpid);
	}

	return port_no;
}


p_fabric_sw_node get_fabric_sw_node_by_dpid(UINT8 dpid)
{
    p_fabric_sw_node sw_node = g_fabric_sw_list.node_list;
    while (NULL != sw_node && NULL != sw_node->sw)
    {
        if (sw_node->sw->dpid == dpid)
        {
            return sw_node;
        }

        sw_node = sw_node->next;
    }

    return NULL;
}


UINT4 adjust_fabric_sw_node_list(UINT8 pre_dpid, UINT8 dpid)
{
    p_fabric_sw_node t_node = NULL;
    p_fabric_sw_node sw_node = g_fabric_sw_list.node_list;

    p_fabric_sw_node pre_node = NULL;
    p_fabric_sw_node cur_node = NULL;
    p_fabric_sw_node cur_pre_node = NULL;

    while (NULL != sw_node && NULL != sw_node->sw)
    {
        if (sw_node->sw->dpid == dpid)
        {
            cur_node = sw_node;
            cur_pre_node = t_node;
        } 
        else if (sw_node->sw->dpid == pre_dpid)
        {
            pre_node = sw_node;
        }
    
        t_node = sw_node;
        sw_node = sw_node->next;
    }

    if (NULL == cur_node)
    {
        return 0;
    }
    
    if (0 == pre_dpid) //如果是第一个
    {
        if (NULL != cur_pre_node)
        {
            cur_pre_node->next = cur_node->next;
            cur_node->next = g_fabric_sw_list.node_list;
            g_fabric_sw_list.node_list = cur_node;
        }

        return 1;
    }

    if (NULL != pre_node) //如果不是第一个
    {
        if (cur_pre_node != pre_node)
        {
            cur_pre_node->next = cur_node->next;
            cur_node->next = pre_node->next;
            pre_node->next = cur_node;
        }

        return 1;
    }

    return 0;
}


