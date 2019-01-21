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
*   File Name   : CEvent.h                                                    *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CEVENT_H
#define __CEVENT_H

#include "bnc-type.h"
#include "CMsgCommon.h"

typedef std::string CEventDesc;

enum event_state_e
{
    EVENT_STATE_INIT,
    EVENT_STATE_ACTIVE,
    EVENT_STATE_DROP,

    //...
    EVENT_STATE_MAX
};

enum event_type_e
{
    EVENT_TYPE_NONE,

    //switch events
    EVENT_TYPE_SWITCH_CONNECT         = 0x100,
    EVENT_TYPE_SWITCH_QUIT            = 0x101,
    EVENT_TYPE_SWITCH_ENTER_STABLE    = 0x102,
    EVENT_TYPE_SWITCH_ENTER_UNREACH   = 0x103,
    EVENT_TYPE_SWITCH_EXIT_UNREACH    = 0x104,
    EVENT_TYPE_SWITCH_DISCONNECT      = 0x105,
    EVENT_TYPE_SWITCH_POWER_OFF       = 0x106,
    EVENT_TYPE_SWITCH_RECONNECT       = 0x107,

    //port events
    EVENT_TYPE_PORT_UP                = 0x200,
    EVENT_TYPE_PORT_DOWN              = 0x201,

    //topo link events
    EVENT_TYPE_TOPO_LINK_ESTABLISH    = 0x300,
    EVENT_TYPE_TOPO_LINK_DELETE       = 0x301,
    EVENT_TYPE_TOPO_LINK_UNALIVE      = 0x302,
    EVENT_TYPE_TOPO_LINK_RECOVER      = 0x303,

    //networking events
	NETWORKING_EVENT_FLOATING_C       = 0x400,
	NETWORKING_EVENT_FLOATING_R       = 0x401,
	NETWORKING_EVENT_FLOATING_U       = 0x402,
	NETWORKING_EVENT_FLOATING_D       = 0x403,
	NETWORKING_EVENT_FLOATING_DS      = 0x404,
	
	NETWORKING_EVENT_NETWORK_C        = 0x410,
	NETWORKING_EVENT_NETWORK_R        = 0x411,
	NETWORKING_EVENT_NETWORK_U        = 0x412,
	NETWORKING_EVENT_NETWORK_D        = 0x413,
	NETWORKING_EVENT_NETWORK_DS       = 0x414,
	
	NETWORKING_EVENT_ROUTER_C         = 0x420,
	NETWORKING_EVENT_ROUTER_R         = 0x421,
	NETWORKING_EVENT_ROUTER_U         = 0x422,
	NETWORKING_EVENT_ROUTER_D         = 0x423,
	NETWORKING_EVENT_ROUTER_DS        = 0x424,
	
	NETWORKING_EVENT_SUBNET_C         = 0x430,
	NETWORKING_EVENT_SUBNET_R         = 0x431,
	NETWORKING_EVENT_SUBNET_U         = 0x432,
	NETWORKING_EVENT_SUBNET_D         = 0x433,
	NETWORKING_EVENT_SUBNET_DS        = 0x434,
	
	NETWORKING_EVENT_PORT_C           = 0x440,
	NETWORKING_EVENT_PORT_R           = 0x441,
	NETWORKING_EVENT_PORT_U           = 0x442,
	NETWORKING_EVENT_PORT_D           = 0x443,
	NETWORKING_EVENT_PORT_DS          = 0x444,

    //security group events
    EVENT_TYPE_SECURITY_GROUP_ATTACH  = 0x500,
    EVENT_TYPE_SECURITY_GROUP_DETACH  = 0x501,

    EVENT_TYPE_SECURITY_GROUP_RULE_C  = 0x510,
    EVENT_TYPE_SECURITY_GROUP_RULE_R  = 0x511,
    EVENT_TYPE_SECURITY_GROUP_RULE_U  = 0x512,
    EVENT_TYPE_SECURITY_GROUP_RULE_D  = 0x513,
    EVENT_TYPE_SECURITY_GROUP_RULE_DS = 0x514,

    //portforward events
    EVENT_TYPE_PORTFORWARD_RULE_C     = 0x600,
    EVENT_TYPE_PORTFORWARD_RULE_R     = 0x601,
    EVENT_TYPE_PORTFORWARD_RULE_U     = 0x602,
    EVENT_TYPE_PORTFORWARD_RULE_D     = 0x603,
    EVENT_TYPE_PORTFORWARD_RULE_DS    = 0x604,

	//qos events
	EVENT_TYPE_QOS_ATTCH			  = 0x700,
	EVENT_TYPE_QOS_UPDATE  			  = 0x701,

	EVENT_TYPE_QOS_RULE_C			  = 0x710,
	EVENT_TYPE_QOS_RULE_R			  = 0x711,
	EVENT_TYPE_QOS_RULE_U			  = 0x712,
	EVENT_TYPE_QOS_RULE_D			  = 0x713,
	EVENT_TYPE_QOS_RULE_DS			  = 0x714,

    //flow table events
    EVENT_TYPE_FLOW_TABLE_ADD         = 0x800,
    EVENT_TYPE_FLOW_TABLE_MOD         = 0x801,
    EVENT_TYPE_FLOW_TABLE_MOD_STRICT  = 0x802,
    EVENT_TYPE_FLOW_TABLE_DEL         = 0x803,
    EVENT_TYPE_FLOW_TABLE_DEL_STRICT  = 0x804,
    EVENT_TYPE_FLOW_TABLE_RECOVER     = 0x805,

	EVENT_TYPE_TAG_FLOW_ADD           = 0x900,

    //...
    EVENT_TYPE_MAX                    = 0xFFFF
};

enum event_reason_e
{
    EVENT_REASON_NONE,

    //switch events
    EVENT_REASON_NEGO_VER_FAIL          = 0x100,
    EVENT_REASON_SEND_MSG_FAIL          = 0x101,
    EVENT_REASON_HEARTBEAT_TIMEOUT      = 0x102,
    EVENT_REASON_CONNECT_RECOVER        = 0x103,
    EVENT_REASON_SWITCH_DISCONNECT      = 0x104,
    EVENT_REASON_SWITCH_POWER_OFF       = 0x105,
    EVENT_REASON_SWITCH_RECONNECT       = 0x106,

    //port events
    EVENT_REASON_PORT_UP                = 0x200,
    EVENT_REASON_PORT_DOWN              = 0x201,

    //topo link events
    EVENT_REASON_TOPO_LINK_ESTABLISH    = 0x300,
    EVENT_REASON_TOPO_LINK_UNALIVE      = 0x301,
    EVENT_REASON_TOPO_LINK_RECOVER      = 0x302,

    //networking events
	EVENT_REASON_NETWORKING_FLOATING_C  = 0x400,
	EVENT_REASON_NETWORKING_FLOATING_R  = 0x401,
	EVENT_REASON_NETWORKING_FLOATING_U  = 0x402,
	EVENT_REASON_NETWORKING_FLOATING_D  = 0x403,
	EVENT_REASON_NETWORKING_FLOATING_DS = 0x404,
	
	EVENT_REASON_NETWORKING_NETWORK_C   = 0x410,
	EVENT_REASON_NETWORKING_NETWORK_R   = 0x411,
	EVENT_REASON_NETWORKING_NETWORK_U   = 0x412,
	EVENT_REASON_NETWORKING_NETWORK_D   = 0x413,
	EVENT_REASON_NETWORKING_NETWORK_DS  = 0x414,
	
	EVENT_REASON_NETWORKING_ROUTER_C    = 0x420,
	EVENT_REASON_NETWORKING_ROUTER_R    = 0x421,
	EVENT_REASON_NETWORKING_ROUTER_U    = 0x422,
	EVENT_REASON_NETWORKING_ROUTER_D    = 0x423,
	EVENT_REASON_NETWORKING_ROUTER_DS   = 0x424,
	
	EVENT_REASON_NETWORKING_SUBNET_C    = 0x430,
	EVENT_REASON_NETWORKING_SUBNET_R    = 0x431,
	EVENT_REASON_NETWORKING_SUBNET_U    = 0x432,
	EVENT_REASON_NETWORKING_SUBNET_D    = 0x433,
	EVENT_REASON_NETWORKING_SUBNET_DS   = 0x434,
	
	EVENT_REASON_NETWORKING_PORT_C      = 0x440,
	EVENT_REASON_NETWORKING_PORT_R      = 0x441,
	EVENT_REASON_NETWORKING_PORT_U      = 0x442,
	EVENT_REASON_NETWORKING_PORT_D      = 0x443,
	EVENT_REASON_NETWORKING_PORT_DS     = 0x444,

    //security group events
    EVENT_REASON_SECURITY_GROUP_ATTACH  = 0x500,
    EVENT_REASON_SECURITY_GROUP_DETACH  = 0x501,

    EVENT_REASON_SECURITY_GROUP_RULE_C  = 0x510,
    EVENT_REASON_SECURITY_GROUP_RULE_R  = 0x511,
    EVENT_REASON_SECURITY_GROUP_RULE_U  = 0x512,
    EVENT_REASON_SECURITY_GROUP_RULE_D  = 0x513,
    EVENT_REASON_SECURITY_GROUP_RULE_DS = 0x514,

    //portforward events
    EVENT_REASON_PORTFORWARD_RULE_C     = 0x600,
    EVENT_REASON_PORTFORWARD_RULE_R     = 0x601,
    EVENT_REASON_PORTFORWARD_RULE_U     = 0x602,
    EVENT_REASON_PORTFORWARD_RULE_D     = 0x603,
    EVENT_REASON_PORTFORWARD_RULE_DS    = 0x604,

	//qos events
	EVENT_REASON_QOS_ATTCH			    = 0x700,
	EVENT_REASON_QOS_UPDATE  			= 0x701,

	EVENT_REASON_QOS_RULE_C			    = 0x710,
	EVENT_REASON_QOS_RULE_R			    = 0x711,
	EVENT_REASON_QOS_RULE_U			    = 0x712,
	EVENT_REASON_QOS_RULE_D			    = 0x713,
	EVENT_REASON_QOS_RULE_DS			= 0x714,

    //flow table events
    EVENT_REASON_FLOW_TABLE_ADD         = 0x800,
    EVENT_REASON_FLOW_TABLE_MOD         = 0x801,
    EVENT_REASON_FLOW_TABLE_MOD_STRICT  = 0x802,
    EVENT_REASON_FLOW_TABLE_DEL         = 0x803,
    EVENT_REASON_FLOW_TABLE_DEL_STRICT  = 0x804,
    EVENT_REASON_FLOW_TABLE_RECOVER     = 0x805,
	
	EVENT_REASON_TAG_FLOW_ADD			= 0x900,

    //inner reasons
    EVENT_REASON_SYSTEM_FAILURE         = 0xFF00,

    //...
    EVENT_REASON_MAX                    = 0xFFFF
};

/*
 * 事件封装�?
 */
class CEvent : public CMsgCommon
{
public:
    CEvent();
    CEvent(INT4 oper, INT4 event, INT4 reason);
    virtual ~CEvent();

    /*
     * 获取事件状�?
     *
     * @return: INT4        返回事件状�?
     */
    INT4 getState() {return m_state;}

    /*
     * 设置事件状�?
     *
     * @param: state         事件状�?
     *
     * @return: None
     */
    void setState(INT4 state) {m_state = state;}

    /*
     * 获取事件类型
     *
     * @return: INT4        返回事件类型
     */
    INT4 getEvent() {return m_event;}

    /*
     * 设置事件类型
     *
     * @param: type         事件类型
     *
     * @return: None
     */
    void setEvent(INT4 event) {m_event = event;}

    /*
     * 设置事件原因
     *
     * @param: reason         事件原因
     *
     * @return: None
     */
    void setReason(INT4 reason) {m_reason = reason;}

    /*
     * 获取事件原因
     *
     * @return: INT4        返回事件原因
     */
    INT4 getReason() {return m_reason;}

    /*
     * 获取端口事件描述
     *
     * @return: CEventDesc        返回端口事件描述
     */ 
    CEventDesc getDesc() {return m_desc;}

    /*
     * 设置端口事件描述
     *
     * @param: desc         端口事件描述
     *
     * @return: None
     */
    void setDesc(CEventDesc& desc)    {m_desc = desc;}

private:
    INT4       m_state;  ///< 事件状�?
    INT4       m_event;  ///< 事件类型
    INT4       m_reason; ///< 事件原因
    CEventDesc m_desc;   ///< 事件描述
};

#endif
