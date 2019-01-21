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
*   File Name   : CEtherArpHandler.cpp           *
*   Author      : bnc zgzhao, cyyang           *
*   Create Date : 2016-7-6           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "log.h"
#include "bnc-error.h"
#include "bnc-inet.h"
#include "openflow-common.h"
#include "CConf.h"
#include "CControl.h"
#include "COfMsgUtil.h"
#include "CEtherArpHandler.h"
#include "CServiceMgr.h"

#if 0
static UINT8 total = 0;
static time_t timeStart = 0;
static UINT8 totalPeriod = 0;
static time_t timePeriod = 0;

#define procStats() \
{\
    if (0 == total)\
    {\
        timeStart = time(NULL);\
        timePeriod = timeStart;\
    }\
    if (++total%20000 == 0)\
    {\
        time_t tmCurr = time(NULL);\
        time_t tmDlta = tmCurr - timeStart;\
        if (tmDlta > 1)\
        {\
            time_t pdDlta = tmCurr - timePeriod;\
            if (pdDlta < 10)\
            {\
                LOG_WARN_FMT("[%s:%s:%d] received %llu pkts with average speed %llu pkts/s ...", \
                    toString(),__FUNCTION__, __LINE__, total, (UINT8)(total/tmDlta));\
            }\
            else\
            {\
                LOG_WARN_FMT("[%s:%s:%d] received %llu pkts with average speed %llu pkts/s, and current speed %llu pkts/s ...", \
                    toString(),__FUNCTION__, __LINE__, total, (UINT8)(total/tmDlta), (UINT8)((total-totalPeriod)/pdDlta));\
                timePeriod = tmCurr;\
                totalPeriod = total;\
            }\
        }\
    }\
}
#endif

CEtherArpHandler::CEtherArpHandler()
{
}

CEtherArpHandler::~CEtherArpHandler()
{
}

INT4 CEtherArpHandler::onregister()
{
	const INT1* pNum = CConf::getInstance()->getConfig("controller", "handler_thread_num");
    UINT4 num = (pNum == NULL) ? 1 : atol(pNum);
    LOG_INFO_FMT("handler_thread_num %u", num);

    if (0 == num)
        num = 1;

    CMsgPath path1 = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_ARP, 1);
    if (CMsgHandler::onregister(path1, num, TRUE, THREAD_TYPE_OCCUPY_CORE))
    {
        LOG_WARN_FMT("CEtherArpHandler register to path[%s] failed !", path1.c_str());
        return BNC_ERR;
    }

    CMsgPath path2 = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_ARP, 2);
    if (CMsgHandler::onregister(path2, 1))
    {
        LOG_WARN_FMT("CEtherArpHandler register to path[%s] failed !", path2.c_str());
        CMsgHandler::deregister(path1);
        return BNC_ERR;
    }

    return BNC_OK;

}

void CEtherArpHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_ARP, 1);
    CMsgHandler::deregister(path);

    path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_ARP, 2);
    CMsgHandler::deregister(path);
}

BOOL CEtherArpHandler::isRequest(arp_t* arp_pkt)
{
	LOG_IF_RETURN (NULL == arp_pkt, FALSE, "arp pointer is NULL");

	BOOL value = (arp_pkt->opcode == htons(1)) ? TRUE : FALSE;

	return value;
}

BOOL CEtherArpHandler::isReply(arp_t* arp_pkt)
{
	LOG_IF_RETURN (NULL == arp_pkt, FALSE, "arp pointer is NULL");

	BOOL value = (arp_pkt->opcode == htons(2)) ? TRUE : FALSE;

	return value;
}

INT4 CEtherArpHandler::handle(CSmartPtr<CMsgQueue> queue)
{
	CSmartPtr<CMsgCommon> msg;
	
	while(!queue->empty())
	{
		queue->pop(msg);
		if (msg.isNotNull())
            handle(msg);
	}

    queue->setBusy(FALSE);
    
	return BNC_OK;
}

INT4 CEtherArpHandler::handle(CSmartPtr<CMsgCommon> msg)
{
    //procStats();

	if(!CControl::getInstance()->isDelayTimeReach())
	{
		return BNC_ERR;
	}

	INT4 ret = BNC_OK;
	
    CMsg* ofmsg = (CMsg*)msg.getPtr();

    LOG_DEBUG_FMT("%s[%p] handle ARP msg of path[%s] from sockfd[%d] ...", 
        toString(), this, ofmsg->getPath().c_str(), ofmsg->getSockfd());
	
	ret = CServiceMgr::getInstance()->ArpProcess(ofmsg);

	return ret;
}

const char* CEtherArpHandler::toString()
{
    return "CEtherArpHandler";
}

CMsgHandler* CEtherArpHandler::clone()
{
    CMsgHandler* instance = new CEtherArpHandler();
    *instance = *this;
    return instance;
}

void CEtherArpHandler::printArp(arp_t* pkt)
{
    INT1 str_ip[32] = {0};
    number2ip(pkt->targetip, str_ip);
    LOG_INFO_FMT("Arp target ip: target ip is %s, type is %d", str_ip, ntohs(pkt->opcode));
}

