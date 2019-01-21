/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2016, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the BNC SDN Controller. BNC SDN
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

/******************************************************************************
*                                                                             *
*   File Name   : CQosQueue.cpp                                               *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CQosQueue.h"
#include "bnc-error.h"
#include "CClusterService.h"
#include "log.h"
#include "CControl.h"
#include "CServer.h"

#define STR_LEN_MAX                 1500
#define STR_LEN_EXTERNAL            50

#ifndef CREATE_OVSDB_QUEUE
#define CREATE_OVSDB_QUEUE	        "08"
#endif
#ifndef SEARCH_INTERFACE_BY_PORT_NO
#define SEARCH_INTERFACE_BY_PORT_NO "09"
#endif
#ifndef SEARCH_PORT_BY_INTERFACE
#define SEARCH_PORT_BY_INTERFACE    "10"
#endif

CQosQueue::CQosQueue()
{
}

CQosQueue::~CQosQueue()
{
}

//向ovsdb内port表内清除绑定qos�?
INT4 CQosQueue::clearQosInPortTable(INT4 sockfd, const INT1* portUuid)
{
	if (NULL == portUuid) 
		return BNC_ERR;

	if (CClusterService::getInstance()->isClusterOn() && 
        (OFPCR_ROLE_MASTER != CClusterService::getInstance()->getControllerRole()))
        return BNC_OK;

	INT1 content[STR_LEN_MAX] = {0};
	INT1 text[] = {"[\"Open_vSwitch\",{\"op\":\"update\",\"table\":\"Port\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],\"row\":{\"qos\":[\"set\",[]]}}]"};	
	sprintf(content, text, portUuid);
	
	return sendQuery(sockfd, CREATE_OVSDB_QUEUE, content);
}

//向ovsdb内qos表内清空qos�?
INT4 CQosQueue::clearQosInQosTable(INT4 sockfd)
{	
	INT1 content[] = {"[\"Open_vSwitch\",{\"op\":\"delete\",\"table\":\"QoS\",\"where\":[]}]"};
	return sendQuery(sockfd, CREATE_OVSDB_QUEUE, content);
}

/* by:yhy
 * 向ovsdb内queue表清空queue
 */
INT4 CQosQueue::clearQueueInQueueTable(INT4 sockfd)
{	
	if (CClusterService::getInstance()->isClusterOn() && 
        (OFPCR_ROLE_MASTER != CClusterService::getInstance()->getControllerRole()))
        return BNC_OK;

	INT1 content[] = {"[\"Open_vSwitch\",{\"op\":\"delete\",\"table\":\"Queue\",\"where\":[]}]"};
	return sendQuery(sockfd, CREATE_OVSDB_QUEUE, content);
}

//set queue uuid
INT4 CQosQueue::setQueueUuid(CSmartPtr<CSwitch> sw, UINT4 port_no, UINT4 queue_id, const INT1* queue_uuid)
{
	if ((0 == port_no) || (NULL == queue_uuid) || (0 == strlen(queue_uuid))) 
		return BNC_ERR;

	INT1* qos_uuid = NULL;
	
#if 0
	gn_qos_t* qos = sw->qos_entries;
	while (qos) 
	{
		if (qos->port_no == port_no)
        {      
			qos_uuid = qos->qos_uuid;
            break;
        }

		qos = qos->next;
	}

	gn_queue_t* queue = sw->queue_entries;
	while (queue)
	{
		if ((queue->port_no == port_no) && (queue->queue_id == queue_id))
		{
			strcpy(queue->queue_uuid, queue_uuid);
            break;
		}

		queue = queue->next;
	}
#endif

    if (NULL != qos_uuid)
        return addQueueToQosTable(sw->getSockfd(), qos_uuid, queue_id, queue_uuid);

	return BNC_ERR;
}

//set qos uuid
INT4 CQosQueue::setQosUuid(CSmartPtr<CSwitch> sw, UINT4 port_no, const INT1* qos_uuid)
{
	if ((0 == port_no) || (NULL == qos_uuid) || (0 == strlen(qos_uuid))) 
		return BNC_ERR;

#if 0
	gn_qos_t* qos = sw->qos_entries;
	while (qos)
	{
		if (qos->port_no == port_no)
		{
			strcpy(qos->qos_uuid, qos_uuid);
			if (0 == strlen(qos->interface_uuid)) 
			{
				searchInterfaceByPortNo(sw->getSockfd(), port_no);
			}
            break;
		}

		qos = qos->next;
	}

	gn_queue_t* queue = sw->queue_entries;
	while (queue)
	{
		if (queue->port_no == port_no) 
		{
			return addQueueToQosTable(sw->getSockfd(), qos_uuid, queue->queue_id, queue->queue_uuid);
		}
		
		queue = queue->next;
	}
#endif

	return BNC_ERR;
}

//set port uuid
INT4 CQosQueue::setPortUuid(const INT1* port_uuid, const INT1* interface_uuid)
{
	if ((NULL == port_uuid) || (NULL == interface_uuid) || (0 == strlen(interface_uuid))) 
		return BNC_ERR;

#if 0
	BOOL bFindFlag = FALSE;
	UINT2 offset = 0;
	p_fabric_sw_list list = NULL;
	list = &g_fabric_sw_list;

	p_fabric_sw_node sw_node = list->node_list;

	INT1 port_qos_uuid[STR_LEN_MAX] = {0};
	while (sw_node) 
	{
		gn_switch_t* sw = sw_node->sw;
		gn_qos_t* qos_next = sw->qos_entries;
		while (sw && qos_next) 
		{
			if (0 == strcmp(interface_uuid, qos_next->interface_uuid)) 
			{
				LOG_PROC("INFO", "%s port_uuid=%s interface_uuid=%s sw->qos_entries->port_uuid=%s sw->sw_ip=0x%x",FN, port_uuid, interface_uuid,sw->qos_entries->port_uuid, sw->sw_ip);
				if ((sw) && (strlen(port_uuid)) && (strlen(qos_next->qos_uuid))) 
				{
					sw->qos_entries->status = 1;
					if(0 == strlen(port_qos_uuid))
						offset = sprintf(port_qos_uuid, "%s", qos_next->qos_uuid);
					else
						offset += sprintf(port_qos_uuid + offset, ", %s", qos_next->qos_uuid);
					addQosToPortTable(sw, port_uuid, sw->qos_entries->qos_uuid);
					bFindFlag = TRUE;
				}
			}
			qos_next = qos_next->next;
		}
		if(TRUE == bFindFlag)
		{
			addQosToPortTable(sw, port_uuid, port_qos_uuid);
			LOG_PROC("INFO", "%s port_uuid=%s interface_uuid=%s port_qos_uuid=%s sw->sw_ip=0x%x",FN, port_uuid, interface_uuid,port_qos_uuid, sw->sw_ip);
		
		}
		offset = 0;
		memset(port_qos_uuid, 0x00, STR_LEN_MAX);
		bFindFlag = FALSE;
		sw_node = sw_node->next;
	}
#endif

	return BNC_ERR;
}

//set interface uuid
INT4 CQosQueue::setInterfaceUuid(CSmartPtr<CSwitch> sw, UINT4 port_no, const INT1* interface_uuid)
{
	if ((0 == port_no) || (NULL == interface_uuid) || (0 == strlen(interface_uuid))) 
		return BNC_ERR;

#if 0
	gn_qos_t* qos = sw->qos_entries;
	while (qos)
	{
		if (qos->port_no == port_no)
		{
			strcpy(qos->interface_uuid, interface_uuid);
			if (0 == strlen(qos->port_uuid)) 
			{
				searchPortByInterface(sw->getSockfd(), interface_uuid);
			}
            break;
		}

		qos = qos->next;
	}
#endif

	return BNC_ERR;
}

//向ovsdb内qos表内qos项绑定queue
INT4 CQosQueue::addQueueToQosTable(INT4 sockfd, const INT1* qos_uuid, UINT4 queue_id, const INT1* queue_uuid)
{
	/*
     * example
     * ["Open_vSwitch",{"op":"mutate","table":"QoS","where":[["_uuid","==",["uuid","a86f68bc-334f-411c-a44e-6b4e9ea24f8d"]]]
     * ,"mutations":[["queues","insert",["map",[[1,["uuid","1ba592f7-6845-4a2e-8127-3aa1426805ca"]]]]]]}]
     */

	if ((NULL == qos_uuid) || (NULL == queue_uuid) || (0 == strlen(qos_uuid)) || (0 == strlen(queue_uuid)))
    	return BNC_ERR;

	INT1 content[STR_LEN_MAX] = {0};
	INT1 text[] = {"[\"Open_vSwitch\",{\"op\":\"mutate\",\"table\":\"QoS\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],"
		"\"mutations\":[[\"queues\",\"insert\",[\"map\",[[%d,[\"uuid\",\"%s\"]]]]]]}]"};
	sprintf(content, text, qos_uuid, queue_id, queue_uuid);

	return sendQuery(sockfd, CREATE_OVSDB_QUEUE, content);
}

//向ovsdb内port表内绑定qos�?
INT4 CQosQueue::addQosToPortTable(INT4 sockfd, const INT1* port_uuid, const INT1* qos_uuid)
{
	/*
	 * example
	 * [\"Open_vSwitch\",{\"op\":\"update\",\"table\":\"Port\",\"where\":[[\"_uuid\",\"==
	 * \",[\"uuid\",\"917d741f-f0c9-4cc0-a94c-7536e48689fe\"]]],\"row\":{\"qos\":[\"set\",
	 * [[\"uuid\",\"7a61aacf-d7e8-423b-917a-3f83011561d0\"]]]}}]
	 */

	if ((NULL == port_uuid) || (NULL == qos_uuid) || (0 == strlen(port_uuid)) || (0 == strlen(qos_uuid)))
    	return BNC_ERR;

	if (CClusterService::getInstance()->isClusterOn() && 
        (OFPCR_ROLE_MASTER != CClusterService::getInstance()->getControllerRole()))
        return BNC_OK;

	INT1 content[STR_LEN_MAX] = {0};
	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"update\",\"table\":\"Port\","
		"\"where\":[[\"_uuid\",\"==\",[\"uuid\",\"%s\"]]],"
		"\"row\":{\"qos\":[\"set\",[[\"uuid\",\"%s\"]]]}}]"};
	sprintf(content, text, port_uuid, qos_uuid);

	return sendQuery(sockfd, CREATE_OVSDB_QUEUE, content);
}

//通知:收到queue的uuid回复
INT4 CQosQueue::notifyReceviceQueueUuid(const json_t* queue)
{
	if ((NULL == queue) || 
        (NULL == queue->child) || 
        (NULL == queue->child->child) || 
        (NULL == queue->child->child->child) || 
        (NULL == queue->child->child->child->child) || 
        (NULL == queue->child->child->child->child->child))
		return BNC_ERR;
	
	json_t* queue_json = json_find_first_label(queue->child->child->child->child->child, "external_ids");
	if ((NULL == queue_json) || 
        (NULL == queue_json->child) || 
        (NULL == queue_json->child->child) || 
        (NULL == queue_json->child->child->next) || 
		(NULL == queue_json->child->child->next->child) || 
		(NULL == queue_json->child->child->next->child->next)) 
        return BNC_ERR;

	//port_no
	json_t* port_json = queue_json->child->child->next->child->child;
	if ((NULL == port_json) || 
        (NULL == port_json->next) || 
        (NULL == port_json->next->text)) 
        return BNC_ERR;

	//queue id
	json_t* id_json = queue_json->child->child->next->child->next->child;
	if ((NULL == id_json) || 
        (NULL == id_json->next) || 
        (NULL == id_json->next->text)) 
        return BNC_ERR;

	//sw dpid
    if (NULL == queue_json->child->child->next->child->next->next)
        return BNC_ERR;
	json_t* sw_json = queue_json->child->child->next->child->next->next->child;
	if ((NULL == sw_json) || 
        (NULL == sw_json->text) || 
        (NULL == sw_json->next->text))
        return BNC_ERR;

	// queue_id
	// printf("%s : %s\n", port_json->text, port_json->next->text);
	
	// queue_id
	// printf("%s : %s\n", id_json->text, id_json->next->text);

	// sw dpid
	// printf("%s : %s\n", sw_json->text, sw_json->next->text);

	// queue uuid
	// printf("uuid: %s\n", queue->child->child->text);

	UINT4 port_no = atoi(port_json->next->text);
	UINT4 queue_id = atoi(id_json->next->text);
	UINT8 sw_dpid = 0;
	dpidStr2Uint8(sw_json->next->text, &sw_dpid);	

	CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(sw_dpid);
	if (sw.isNull()) 
		return BNC_ERR;

	return setQueueUuid(sw, port_no, queue_id, queue->child->child->text);
}

//通知:收到qos的uuid回复
INT4 CQosQueue::notifyReceviceQosUuid(const json_t* qos)
{
	if ((NULL == qos) || 
        (NULL == qos->child) || 
        (NULL == qos->child->child) || 
        (NULL == qos->child->child->child) || 
        (NULL == qos->child->child->child->child) || 
        (NULL == qos->child->child->child->child->child))
		return BNC_ERR;
	
	json_t* qos_json = json_find_first_label(qos->child->child->child->child->child, "external_ids");
	if ((NULL == qos_json) || 
        (NULL == qos_json->child) || 
        (NULL == qos_json->child->child) || 
        (NULL == qos_json->child->child->next) || 
		(NULL == qos_json->child->child->next->child) || 
		(NULL == qos_json->child->child->next->child->next)) 
        return BNC_ERR;

	//port_no
	json_t* port_json = qos_json->child->child->next->child->child;
	if ((NULL == port_json) || 
        (NULL == port_json->next) || 
        (NULL == port_json->next->text)) 
        return BNC_ERR;

	//sw dpid
	json_t* sw_json = qos_json->child->child->next->child->next->child;
	if ((NULL == sw_json) || 
        (NULL == sw_json->text) || 
        (NULL == sw_json->next->text))
        return BNC_ERR;

	// port no
	// printf("%s : %s\n", port_json->text, port_json->next->text);

	// sw dpid
	// printf("%s : %s\n", sw_json->text, sw_json->next->text);

	// qos uuid
	// printf("uuid: %s\n", qos->child->child->text);

	UINT4 port_no = atoi(port_json->next->text);
	UINT8 sw_dpid = 0;
	dpidStr2Uint8(sw_json->next->text, &sw_dpid);	

	CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByDpid(sw_dpid);
	if (sw.isNull()) 
		return BNC_ERR;

	return setQosUuid(sw, port_no, qos->child->child->text);
}

//通知:收到interface的uuid回复
INT4 CQosQueue::notifyReceviceInterfaceUuid(const json_t* interface)
{
	if ((NULL == interface) || 
        (NULL == interface->child) || 
        (NULL == interface->child->child))
		return BNC_ERR;
	
	json_t* row = json_find_first_label(interface->child->child, "rows");
	if ((NULL == row) || 
        (NULL == row->child) || 
        (NULL == row->child->child)) 
        return BNC_ERR;

	json_t* tmp = row->child->child;
    if ((NULL == tmp->child) ||
        (NULL == tmp->child->child) ||
        (NULL == tmp->child->child->text) ||
        (NULL == tmp->child->next) ||
        (NULL == tmp->child->next->next) ||
        (NULL == tmp->child->next->next->child) ||
        (NULL == tmp->child->next->next->child->text) ||
        (NULL == tmp->child->next->child) ||
        (NULL == tmp->child->next->child->child) ||
        (NULL == tmp->child->next->child->child->next) ||
        (NULL == tmp->child->next->child->child->next->text))
        return BNC_ERR;

    // mac in use
    //printf("mac:%s\n", tmp->child->child->text);

    // port no
    //printf("port:%s\n", tmp->child->next->next->child->text);

    // uuid
    //printf("uuid:%s\n", tmp->child->next->child->child->next->text);

	UINT4 port_no = atoi(tmp->child->next->next->child->text);
    UINT1 phy_mac[6] = {0};
    macstr2hex(tmp->child->child->text, phy_mac);

	CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitchByMac((INT1*)phy_mac);
	if (sw.isNull()) 
		return BNC_ERR;

	return setInterfaceUuid(sw, port_no, tmp->child->next->child->child->next->text);
}

//通知:收到port的uuid回复
INT4 CQosQueue::notifyRecevicePortUuid(const json_t* port)
{
	if ((NULL == port) ||
        (NULL == port->child) ||
        (NULL == port->child->child))
		return BNC_ERR;

	json_t* row = json_find_first_label(port->child->child, "rows");
	if ((NULL == row) ||
        (NULL == row->child) ||
        (NULL == row->child->child) ||
        (NULL == row->child->child->child) ||
        (NULL == row->child->child->child->child) ||
        (NULL == row->child->child->child->child->child) ||
        (NULL == row->child->child->child->child->child->next) ||
        (NULL == row->child->child->child->child->child->next->text) ||
        (NULL == row->child->child->child->next) ||
        (NULL == row->child->child->child->next->child) ||
        (NULL == row->child->child->child->next->child->child) ||
        (NULL == row->child->child->child->next->child->child->next) ||
        (NULL == row->child->child->child->next->child->child->next->text))
		return BNC_ERR;

    // uuid
    // printf("%s\n", row->child->child->child->child->child->next->text);

    // interface uuid
    // printf("%s\n", row->child->child->child->next->child->child->next->text);

    setPortUuid(row->child->child->child->child->child->next->text, 
                row->child->child->child->next->child->child->next->text);

	return BNC_OK;
}

//clear qos
INT4 CQosQueue::notifyReceiveSearchBridgeUuid(INT4 sockfd, const json_t* bridge)
{
	if ((NULL == bridge) || 
        (NULL == bridge->child) || 
        (NULL == bridge->child->child))
		return BNC_ERR;

    json_t* row = json_find_first_label(bridge->child->child, "rows");
	if ((NULL == row) || 
        (NULL == row->child) || 
        (NULL == row->child->child)) 
        return BNC_ERR;

    json_t* ports = json_find_first_label(row->child->child, "ports");
	if ((NULL == ports) || 
        (NULL == ports->child) || 
        (NULL == ports->child->child) ||
        (NULL == ports->child->child->next) ||
        (NULL == ports->child->child->next->child)) 
        return BNC_ERR;

	INT1 port_uuid[128] = {0};
    for (json_t* port = ports->child->child->next->child; NULL != port; port = port->next)
    {
        memset(port_uuid, 0, sizeof(port_uuid));
        if ((NULL != port->child) && 
            (NULL != port->child->text) &&
            (0 == strcmp(port->child->text, "uuid")) &&
            (NULL != port->child->next->text))
        {
            strcpy(port_uuid, port->child->next->text);
            clearQosInPortTable(sockfd, port_uuid);
        }
    }

	clearQosInQosTable(sockfd);
	clearQueueInQueueTable(sockfd);

	return BNC_OK;
}

//ovsdb内根据端口号查询interface
INT4 CQosQueue::searchInterfaceByPortNo(INT4 sockfd, UINT4 port_no)
{
	/* 
	 * example
	*  [\"Open_vSwitch\",{\"op\":\"select\",\"table\":\"Interface\",\"where\":[[\"ofport\",\"==\",25]],\"columns\":[\"_uuid\",\"ofport\"]}]
	 */

	if (0 == port_no) 
        return BNC_ERR;

	INT1 content[STR_LEN_MAX] = {0};
	INT1 text[] = {"[\"Open_vSwitch\",{"
		"\"op\":\"select\",\"table\":\"Interface\","
		"\"where\":[[\"ofport\",\"==\",%d]],\"columns\":[\"_uuid\",\"ofport\",\"mac_in_use\"]}]"};
	sprintf(content, text, port_no);

	return sendQuery(sockfd, SEARCH_INTERFACE_BY_PORT_NO, content);
}

//ovsdb内根据interface查询port
INT4 CQosQueue::searchPortByInterface(INT4 sockfd, const INT1* interface_uuid)
{
	/*
	 * example
	 * [\"Open_vSwitch\",{\"op\":\"select\",\"table\":\"Port\",\"where\":[[\"interfaces\",\"==\",
	 * [\"uuid\",\"10f326ab-dae4-4acc-8136-24036f26e807\"]]],\"columns\":[\"_uuid\",\"interfaces\"]}]
	 */

	if ((NULL == interface_uuid) || (0 == strlen(interface_uuid))) 
        return BNC_ERR;

	INT1 content[STR_LEN_MAX] = {0};
	INT1 text[] = {"[\"Open_vSwitch\",{\"op\":\"select\",\"table\":\"Port\","
		"\"where\":[[\"interfaces\",\"==\",[\"uuid\",\"%s\"]]],\"columns\":[\"_uuid\",\"interfaces\"]}]"};
	sprintf(content, text, interface_uuid);

	return sendQuery(sockfd, SEARCH_PORT_BY_INTERFACE, content);
}

INT4 CQosQueue::sendQuery(INT4 sockfd, const INT1* id, const INT1* content)
{
	INT1 query[STR_LEN_MAX] = {0};
	INT1 transact[] = {"{\"id\":\"%s\",\"method\":\"transact\",\"params\":%s}"};
	sprintf(query, transact, id, content); 			

    LOG_INFO(query);

    if (!sendMsgOut(sockfd, query, strlen(query)))
    {
        CSmartPtr<COvsdbRecvWorker> worker = CServer::getInstance()->mapOvsdbRecvWorker(sockfd);
        if (worker.isNotNull())
            worker->processPeerDisconn(sockfd);
        return BNC_ERR;
    }

	return BNC_OK;
}

