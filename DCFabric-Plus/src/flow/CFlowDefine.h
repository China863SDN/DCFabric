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
*   File Name   : CFlowDefine.h		*
*   Author      : bnc xflu          *
*   Create Date : 2016-8-1          *
*   Version     : 1.0           	*
*   Function    : .           		*
*                                                                             *
******************************************************************************/
#ifndef _CFLOWDEFINE_H
#define _CFLOWDEFINE_H
#include "bnc-type.h"


#define FABRIC_FLOATING_FLOW_HARD_TIME_OUT 0
#define FABRIC_FLOATING_FLOW_IDLE_TIME_OUT 100
#define FABRIC_IMPL_HARD_TIME_OUT 0
#define FABRIC_IMPL_IDLE_TIME_OUT 0
#define FABRIC_ARP_HARD_TIME_OUT 0
#define FABRIC_ARP_IDLE_TIME_OUT 30	

#define FABRIC_NAT_IDLE_TIME_OUT 	30*2
#define FABRIC_NAT_HARD_TIME_OUT 	0	

#define FABRIC_FIREWALL_HARD_TIME_OUT 0
#define FABRIC_FIREWALL_IDLE_TIME_OUT 0
#define FABRIC_FIREWALL_EPHEMERAL_FLOW_HARD_TIME_OUT 0
#define FABRIC_FIREWALL_EPHEMERAL_FLOW_IDLE_TIME_OUT 100
#define FABRIC_PRIORITY_FIREWALL_IN_FLOW  10
#define FABRIC_PRIORITY_FIREWALL_OUT_FLOW 10

#define FABRIC_PORTFORWARD_HARD_TIME_OUT 0
#define FABRIC_PORTFORWARD_IDLE_TIME_OUT 0
#define FABRIC_PORTFORWARD_EPHEMERAL_FLOW_HARD_TIME_OUT 0
#define FABRIC_PORTFORWARD_EPHEMERAL_FLOW_IDLE_TIME_OUT 100
#define FABRIC_PRIORITY_PORTFORWARD_IN_FLOW  10
#define FABRIC_PRIORITY_PORTFORWARD_OUT_FLOW 10

#define FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_IDLE_TIME_OUT 0
#define FABRIC_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW_HARD_TIME_OUT 0
#define FABRIC_PRIORITY_OUTPUT_TABLE_FLOATING_TO_FIX_FLOW	20
#define FABRIC_PRIORITY_OUTPUT_TABLE_FLOATING_INVM_FLOW		20



#define FABRIC_PRIORITY_FLOATING_EXTERNAL_HOST_SUBNET_FLOW 7
#define FABRIC_PRIORITY_FLOATING_INTERNAL_SUBNET_FLOW 9



static const UINT1  FABRIC_TABLE_INPUT	=	0;

static const UINT1  FABRIC_TABLE_FIREWALL_OUTTOR =	 1;
static const UINT1  FABRIC_TABLE_QOS_OUTTOR = 2;
static const UINT1  FABRIC_TABLE_PORTFORWARD_OUTTOR	= 3;
static const UINT1  FABRIC_TABLE_NAT_OUTTOR	 =  4;
static const UINT1  FABRIC_TABLE_FLOATINGIP_OUTTOR = 5;

static const UINT1  FABRIC_TABLE6_REV = 6;
static const UINT1  FABRIC_TABLE7_REV = 7;
static const UINT1  FABRIC_TABLE8_REV = 8;

static const UINT1  FABRIC_TABLE_VM = 9;
static const UINT1  FABRIC_TABLE_TAGFORWARD_TOR = 10;

static const UINT1  FABRIC_TABLE_PORTFORWARD_INVM = 11;
static const UINT1  FABRIC_TABLE_NAT_INVM = 12;
static const UINT1  FABRIC_TABLE_FLOATINGIP_INVM = 13;

static const UINT1  FABRIC_TABLE14_REV = 14;
static const UINT1  FABRIC_TABLE15_REV = 15;
static const UINT1  FABRIC_TABLE16_REV = 16;


static const UINT1  FABRIC_TABLE_FIREWALL_INVM = 17;
static const UINT1  FABRIC_TABLE_QOS_INVM = 18;
static const UINT1  FABRIC_TABLE_FORWARD_INVM = 19;


static const UINT1  FABRIC_TABLE_FIREWALL_INEXT =	 1;
static const UINT1  FABRIC_TABLE_QOS_INEXT = 2;
static const UINT1  FABRIC_TABLE_PORTFORWARD_INEXT	= 3;
static const UINT1  FABRIC_TABLE_NAT_INEXT	 =  4;
static const UINT1  FABRIC_TABLE_FLOATINGIP_INEXT = 5;


static const UINT1  FABRIC_TABLE_TAGFORWARD_EXT = 10;

static const UINT1  FABRIC_TABLE_FIREWALL_OUTEXT = 11;
static const UINT1  FABRIC_TABLE_QOS_OUTEXT = 12;

static const UINT1  FABRIC_TABLE13_REV = 13;

static const UINT1  FABRIC_TABLE17_REV = 17;
static const UINT1  FABRIC_TABLE18_REV = 18;


static const UINT1  FABRIC_TABLE_MAC_FORWARD = 19;


static const UINT1  FABRIC_TABLE0 = 0;
static const UINT1  FABRIC_TABLE1 = 1;
static const UINT1  FABRIC_TABLE2 = 2;
static const UINT1  FABRIC_TABLE3 = 3;
static const UINT1  FABRIC_TABLE4 = 4;
static const UINT1  FABRIC_TABLE5 = 5;
static const UINT1  FABRIC_TABLE6 = 6;
static const UINT1  FABRIC_TABLE7 = 7;
static const UINT1  FABRIC_TABLE8 = 8;
static const UINT1  FABRIC_TABLE9 = 9;
static const UINT1  FABRIC_TABLE10 = 10;
static const UINT1  FABRIC_TABLE11 = 11;
static const UINT1  FABRIC_TABLE12 = 12;
static const UINT1  FABRIC_TABLE13 = 13;
static const UINT1  FABRIC_TABLE14 = 14;
static const UINT1  FABRIC_TABLE15 = 15;
static const UINT1  FABRIC_TABLE16 = 16;
static const UINT1  FABRIC_TABLE17 = 17;
static const UINT1  FABRIC_TABLE18 = 18;
static const UINT1  FABRIC_TABLE19 = 19;
static const UINT1  FABRIC_TABLE20 = 20;



/*
 * 定义了一些流表所需要使用的定义
 */

namespace bnc
{
	namespace flow
	{

	    /*
	     * 定义了各流表Table的�?
	     * 可以根据实际配置情况,修改流表�?
	     */
	    /*
        static UINT1 TABLE_PROTOCAL = 0;
        static UINT1 TABLE_SECURITY = 1;
        static UINT1 TABLE_GROUP    = 2;
        static UINT1 TABLE_INTERNAL = 3;
        static UINT1 TABLE_EXTERNAL = 4;
        static UINT1 TABLE_VLAN     = 5;
        static UINT1 TABLE_OUTPUT   = 6;
        */

        /*
         * 测试1: 使用4层流表结�?
         */
        static UINT1 TABLE_PROTOCAL = 0;
        static UINT1 TABLE_SECURITY = 10;
        static UINT1 TABLE_GROUP    = 11;
        static UINT1 TABLE_INTERNAL = 1;
        static UINT1 TABLE_EXTERNAL = 12;
        static UINT1 TABLE_VLAN     = 2;
        static UINT1 TABLE_OUTPUT   = 3;

        //
	    /*
		#define FABRIC_IMPL_HARD_TIME_OUT 0
		#define FABRIC_IMPL_IDLE_TIME_OUT 0
		#define FABRIC_ARP_HARD_TIME_OUT 0
		#define FABRIC_ARP_IDLE_TIME_OUT 100
		#define FABRIC_FIND_HOST_IDLE_TIME_OUT 100

		#define FABRIC_PRIORITY_HOST_INPUT_FLOW 5
		#define FABRIC_PRIORITY_SWITCH_INPUT_FLOW 10
		#define FABRIC_PRIORITY_ARP_MISSMATCH_INPUT_FLOW 10
		#define FABRIC_PRIORITY_MISSMATCH_PUSHTAG_FLOW 0
		#define FABRIC_PRIORITY_SWAPTAG_FLOW 20

		#define FABRIC_PRIORITY_FLOATING_EXTERNAL_HOST_SUBNET_FLOW 7
		#define FABRIC_PRIORITY_FLOATING_EXTERNAL_GROUP_SUBNET_FLOW 8
		#define FABRIC_PRIORITY_FLOATING_INTERNAL_SUBNET_FLOW 9
		#define FABRIC_PRIORITY_ARP_FLOW 15
		#define FABRIC_PRIORITY_FLOATING_FLOW 16
		#define FABRIC_PRIORITY_NAT_FLOW 17
		#define FABRIC_PRIORITY_LOADBALANCE_FLOW 18
		#define FABRIC_PRIORITY_DENY_FLOW 19
		#define FABRIC_PRIORITY_FLOATING_LBAAS_FLOW 21

		#define FABRIC_INPUT_TABLE 0
		#define FABRIC_PUSHTAG_TABLE 10
		#define FABRIC_SWAPTAG_TABLE 20
		#define FABRIC_OUTPUT_TABLE 30
		*/
	}
}

#endif
