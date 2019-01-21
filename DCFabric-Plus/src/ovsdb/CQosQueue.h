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
*   File Name   : CQosQueue.h                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CQOSQUEUE_H
#define __CQOSQUEUE_H

#include "bnc-type.h"
#include "json.h"
#include "CSwitch.h"
#include "CSmartPtr.h"

/*
 * 互斥锁
 */
class CQosQueue
{
public:
    CQosQueue();
    ~CQosQueue();

    INT4 clearQosInPortTable(INT4 sockfd, const INT1* portUuid);
    INT4 clearQosInQosTable(INT4 sockfd);
    INT4 clearQueueInQueueTable(INT4 sockfd);

    INT4 notifyReceviceQueueUuid(const json_t* queue);
    INT4 notifyReceviceQosUuid(const json_t* qos);
    INT4 notifyReceviceInterfaceUuid(const json_t* interface);
    INT4 notifyRecevicePortUuid(const json_t* port);
    INT4 notifyReceiveSearchBridgeUuid(INT4 sockfd, const json_t* bridge);

private:
    INT4 setQueueUuid(CSmartPtr<CSwitch> sw, UINT4 port_no, UINT4 queue_id, const INT1* uuid);
    INT4 setQosUuid(CSmartPtr<CSwitch> sw, UINT4 port_no, const INT1* qos_uuid);
    INT4 setPortUuid(const INT1* port_uuid, const INT1* interface_uuid);
    INT4 setInterfaceUuid(CSmartPtr<CSwitch> sw, UINT4 port_no, const INT1* interface_uuid);
    INT4 searchInterfaceByPortNo(INT4 sockfd, UINT4 port_no);
    INT4 searchPortByInterface(INT4 sockfd, const INT1* interface_uuid);
    INT4 sendQuery(INT4 sockfd, const INT1* id, const INT1* content);
    INT4 addQueueToQosTable(INT4 sockfd, const INT1* qos_uuid, UINT4 queue_id, const INT1* queue_uuid);
    INT4 addQosToPortTable(INT4 sockfd, const INT1* port_uuid, const INT1* qos_uuid);
};

#endif
