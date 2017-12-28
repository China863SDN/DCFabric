#include "openstack_routers.h"
#include <unistd.h>
#include "common.h"
#include "fabric_openstack_external.h"

struct _openstack_router * g_openstack_router_list = NULL;

struct _openstack_port_forward *  g_openstack_forward_list = NULL;
UINT1 visit_openstack_router_list_find_unvalid(void)
{
	openstack_router_p pTempRouter = g_openstack_router_list;
	while(pTempRouter)
	{		
		if(pTempRouter->status ==0)
		{//发现未更新的,即需被删除的节点
			remove_external_port_by_outer_interface_ip(pTempRouter->external_fixed_ips);
		}
		pTempRouter = pTempRouter->next;
	}
	return 0;
}
UINT1 reset_openstack_router_status(void)
{
	openstack_router_p pTempRouter = g_openstack_router_list;
	while(pTempRouter)
	{		
		pTempRouter->status =0;
		pTempRouter = pTempRouter->next;
	}
	return 0;
}

openstack_router_p create_openstack_router(char* router_id,UINT4 external_fixed_ips)
{
	openstack_router_p pRouterNode = (openstack_router_p)gn_malloc(sizeof(openstack_router));
	if(NULL == pRouterNode)
	{
		return NULL;
	}

    strcpy(pRouterNode->router_id, router_id);
	pRouterNode->external_fixed_ips =external_fixed_ips;
	pRouterNode->status =1;
	return pRouterNode;
}


void destory_openstack_router(openstack_router_p router_node)
{
	if(NULL == router_node)
	{
		return;
	}
	
	gn_free((void*)(&router_node));
}

bool update_openstack_router_list(openstack_router_p * header, char* router_id, UINT4 fixed_ip, char* network_id)
{
	if(NULL == router_id ||  0 == strcmp(router_id, ""))
	{
		return false;
	}

	openstack_router_p pRouterNode = get_openstack_router_by_router_id(*header, router_id);
	if(NULL == pRouterNode)
	{
		pRouterNode = create_openstack_router(router_id,fixed_ip);  
		if(NULL == pRouterNode)
		{
			return false;
		}

		pRouterNode->next = *header;
		*header = pRouterNode;
	}
	else
	{
		pRouterNode->status =1;
	}

	return true;
}

openstack_router_p get_openstack_router_by_router_id(openstack_router_p header, char* router_id)
{
	openstack_router_p pTempRouter = header;
	while(pTempRouter)
	{
		if(0 == strcmp(pTempRouter->router_id, router_id))
		{
			return pTempRouter;
		}		

		pTempRouter = pTempRouter->next;
	}

	return NULL;
}


openstack_port_forward_p create_openstack_portfoward(int protol, int status, char* network_id, char* src_ip, char* dst_ip, char* in_port, char* outside_port)
{
	openstack_port_forward_p pPortForwardNode = (openstack_port_forward_p )gn_malloc(sizeof(openstack_port_forward));
	if(NULL == pPortForwardNode)
	{
		return NULL;
	}
      
    pPortForwardNode->proto = protol;
    pPortForwardNode->state = status;
    strcpy(pPortForwardNode->src_ip, src_ip);
    strcpy(pPortForwardNode->dst_ip, dst_ip);
    strcpy(pPortForwardNode->network_id, network_id);
    pPortForwardNode->n_src_ip = ip2number(src_ip);
    pPortForwardNode->n_dst_ip = ip2number(dst_ip);

    //ensure start <= end for src port range
    UINT2 portStart = atoi(outside_port);
    UINT2 portEnd   = portStart;
    char* ptr = strchr(outside_port, '-');
    if (NULL != ptr) { //src port range
        portEnd = atoi(ptr+1);
        if (portEnd < portStart) {
            UINT2 portSwap = portStart;
            portStart = portEnd;
            portEnd = portSwap;
        }
    }
    pPortForwardNode->src_port_start = portStart;
    UINT2 srcDelta = portEnd - portStart;

    //ensure start <= end for dst port range
    portStart = atoi(in_port);
    portEnd   = portStart;
    ptr = strchr(in_port, '-');
    if (NULL != ptr) { //dst port range
        portEnd = atoi(ptr+1);
        if (portEnd < portStart) {
            UINT2 portSwap = portStart;
            portStart = portEnd;
            portEnd = portSwap;
        }
    }
    pPortForwardNode->dst_port_start = portStart;
    UINT2 dstDelta = portEnd - portStart;

    //ensure consistency b/t src and dst port range
    UINT2 delta = (srcDelta <= dstDelta) ? srcDelta : dstDelta;
    pPortForwardNode->src_port_end = pPortForwardNode->src_port_start + delta;
    pPortForwardNode->dst_port_end = pPortForwardNode->dst_port_start + delta;
    
	return pPortForwardNode;
}

openstack_port_forward_p copy_openstack_portfoward(openstack_port_forward_p src_node)
{
	if(NULL == src_node)
	{
		return NULL;
	}

	openstack_port_forward_p newNode = (openstack_port_forward_p)gn_malloc(sizeof(openstack_port_forward));
	if(NULL != newNode)
	{
		memcpy(newNode, src_node, sizeof(openstack_port_forward));
		newNode->next = NULL;
	}

	return newNode;
}

bool update_opstack_portforward_list(openstack_port_forward_p* header, char* protol, char* status, char* network_id, char* src_ip, char* dst_ip, char *in_port, char* outside_port)
{
	LOG_PROC("INFO", "%s start", FN);
	if(NULL == protol || NULL == status || NULL == network_id || NULL == src_ip || NULL == dst_ip || NULL == in_port || NULL == outside_port)
	{
		return false;
	}

    if(0 == strcmp(network_id, ""))
    {
       return false;
    }

    if(0 == strcmp(src_ip, ""))
    {
        return false;
    }

	int nStatus = 0;
	if(0 == strcmp(status, "enable") || 0 == strcmp(status, "ENABLE"))
	{
		nStatus = 1;
	}
	else
	{
		return false;
	}

	int nPro = 0;
	if(0 == strcmp(protol, "tcp"))
	{
		nPro = IPPROTO_TCP; 
	} 
	else if(0 == strcmp(protol, "udp"))
	{
		nPro = IPPROTO_UDP;
	}
	else
	{
		return false;
	}

	openstack_port_forward_p pPortForwardNode = get_opstack_portforward_by_src_ip(*header, src_ip, atoi(outside_port));
	if(NULL != pPortForwardNode)
	{//正常情况下不会存在的
		return true;
	} 

	pPortForwardNode =  create_openstack_portfoward(nPro, nStatus, network_id,  src_ip, dst_ip, in_port, outside_port);
	if(NULL == pPortForwardNode)
	{
		return false;
	}

	pPortForwardNode->next = *header;
	*header = pPortForwardNode;
	
    LOG_PROC("INFO", "%s %s %s %s %s", network_id, src_ip, dst_ip, in_port, outside_port);
    
    LOG_PROC("INFO", "%s END", FN);

	return true;
}

openstack_port_forward_p get_opstack_portforward_by_src_ip(openstack_port_forward_p  header, char* src_ip, int src_port)
{
	openstack_port_forward_p tempNode = header;
	while(tempNode)
	{
		if((0 == strcmp(tempNode->src_ip, src_ip)) && 
           ((tempNode->src_port_start <= src_port) && 
            (tempNode->src_port_end >= src_port)))
		{
			return tempNode;
		}

		tempNode = tempNode->next;
	}

	return NULL;
}

void destory_port_forward_list(openstack_port_forward_p header)
{
    openstack_port_forward_p temp = header;
    openstack_port_forward_p pre = temp;
    
    while(temp)
    {
       pre = temp;
       temp = temp->next;
       gn_free(&pre);
    }
}
