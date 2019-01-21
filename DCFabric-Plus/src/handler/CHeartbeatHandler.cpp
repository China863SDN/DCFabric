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
*   File Name   : CHeartbeatHandler.cpp                                       *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CHeartbeatHandler.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "CRecvWorker.h"
#include "CServer.h"
#include "CConf.h"
#include "COfConnectMgr.h"
#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"
#include "CSwitchEventTester.h"

static const UINT4 HEARTBEAT_INTERVAL_DEFAULT = 10; //s
static const UINT4 HEARTBEAT_TIMES_DEFAULT    = 3;
static const UINT4 HEARTBEAT_PAUSE_TIME       = 60; //s

//涓�娆EARTBEAT鎺㈡祴鐨勪氦鎹㈡満鏁扮洰
static const UINT4 HEARTBEAT_NUMBER_ONE_TIME = 100; //per switch

UINT4 CHeartbeatHandler::m_heartbeatInterval = 0;
UINT4 CHeartbeatHandler::m_heartbeatTimes = 0;

void CHeartbeatHandler::heartbeat(void* param)
{
    static UINT4 step = 0;
    UINT4 count = 0; //valid range is [step, step+1)*HEARTBEAT_NUMBER_ONE_TIME

    CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    if (map.size() <= step*HEARTBEAT_NUMBER_ONE_TIME)
        step = 0;

    LOG_DEBUG_FMT(">> HEARTBEAT start: step[%u]", step);
    
    STL_FOR_LOOP(map, it)
    {
		CSmartPtr<CSwitch> sw = it->second;
        if (sw.isNull())
            continue;
        
        INT4 sockfd = sw->getSockfd();

		if ((OFP13_VERSION == sw->getVersion()) &&
            ((SW_STATE_CONNECTED <= sw->getState()) && 
             (SW_STATE_UNREACHABLE >= sw->getState())))
		{
            //HEARTBEAT交互及重传机制：
            //        START
            //          | heartbeat_interval
            //        send 1
            //          | heartbeat_interval * 2^1
            //        send 2
            //          | heartbeat_interval * 2^2
            //        send 3
            //          :
            //        send heartbeat_times
            //          | heartbeat_interval * 2^heartbeat_times
            //        OVER
            //          | pause for 100s
            //        goto START
            
            if (0 == sw->m_heartbeatTimes)
            {
                if (!((step*HEARTBEAT_NUMBER_ONE_TIME <= count) && 
                      (count < (step+1)*HEARTBEAT_NUMBER_ONE_TIME)))
                {
                    ++count;
                    continue;
                }
                ++count;

                //开始新一轮HEARTBEAT交互
                LOG_DEBUG_FMT("HEARTBEAT: send OFPT13_ECHO_REQUEST to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);
                BOOL ret = COfMsgUtil::sendOfpMsg(sockfd, OFP13_VERSION, OFPT13_ECHO_REQUEST);
                if (ret)
                {
                    sw->m_heartbeatTimes ++;
                    sw->m_heartbeatInterim = 0;
                }
                else
                {
                    LOG_WARN_FMT("HEARTBEAT: send OFPT13_ECHO_REQUEST to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
                    //当发送失败后，暂停100s，然后开始新一轮HEARTBEAT交互
                    sw->m_heartbeatTimes = CHeartbeatHandler::m_heartbeatTimes + 1;
                    sw->m_heartbeatInterim = 0;
                    
                    COfConnectMgr::processPeerEnterUnreach(sw);
                }
            }
            else if (sw->m_heartbeatTimes < CHeartbeatHandler::m_heartbeatTimes)
            {
                //本轮HEARTBEAT交互发生了重传，且重传未结束
                //下一次重传是在时间heartbeat_interval*(2^sw->m_heartbeatTimes)
                sw->m_heartbeatInterim ++;
                if (sw->m_heartbeatInterim >= (UINT4)(1 << sw->m_heartbeatTimes))
                {
                    LOG_WARN_FMT("HEARTBEAT: re-send OFPT13_ECHO_REQUEST to switch[%llx] with sockfd[%d] ...", sw->getDpid(), sockfd);
                    BOOL ret = COfMsgUtil::sendOfpMsg(sockfd, OFP13_VERSION, OFPT13_ECHO_REQUEST);
                    if (ret)
                    {
                        sw->m_heartbeatTimes ++;
                        sw->m_heartbeatInterim = 0;
                    }
                    else
                    {
                        LOG_WARN_FMT("HEARTBEAT: re-send OFPT13_ECHO_REQUEST to switch[%llx] with sockfd[%d] failed !", sw->getDpid(), sockfd);
                        //当发送失败后，暂停100s，然后开始新一轮HEARTBEAT交互
                        sw->m_heartbeatTimes = CHeartbeatHandler::m_heartbeatTimes + 1;
                        sw->m_heartbeatInterim = 0;
                        
                        COfConnectMgr::processPeerEnterUnreach(sw);
                    }
                }
            }
            else if (sw->m_heartbeatTimes == CHeartbeatHandler::m_heartbeatTimes)
            {
                sw->m_heartbeatInterim ++;
                if (sw->m_heartbeatInterim >= (UINT4)(1 << sw->m_heartbeatTimes))
                {
                    //根据配置heartbeat_times，本轮HEARTBEAT交互及重传结束
                    //暂停100s，然后开始新一轮HEARTBEAT交互
                    LOG_WARN_FMT("HEARTBEAT on switch[%llx] with sockfd[%d] timeout !", sw->getDpid(), sockfd);
                    sw->m_heartbeatTimes = CHeartbeatHandler::m_heartbeatTimes + 1;
                    sw->m_heartbeatInterim = 0;
                    
                    COfConnectMgr::processPeerEnterUnreach(sw, EVENT_REASON_HEARTBEAT_TIMEOUT);
                }
            } 
            else
            {
                //下一轮HEARTBEAT交互及重传将在上轮结束后100s后开始
                sw->m_heartbeatInterim ++;
                if ((CHeartbeatHandler::m_heartbeatInterval > 0) &&
                    (sw->m_heartbeatInterim >= (HEARTBEAT_PAUSE_TIME / CHeartbeatHandler::m_heartbeatInterval)))
                {
                    sw->m_heartbeatTimes = 0;
                    sw->m_heartbeatInterim = 0;
                }
            }		
        }
    }

	CControl::getInstance()->getSwitchMgr().unlock();

    ++step;
    LOG_DEBUG_FMT("<< HEARTBEAT end: step[%u], count[%u]", step, count);

#if 0
    static CSwitchEventTester swtester;    
    //LOG_WARN_FMT("\n\n\n###### EVENT_TYPE_SWITCH_CONNECT ######");
    swtester.test(EVENT_TYPE_SWITCH_CONNECT);
    //LOG_WARN_FMT("\n\n\n###### EVENT_TYPE_SWITCH_QUIT ######");
    swtester.test(EVENT_TYPE_SWITCH_QUIT);
    //LOG_WARN_FMT("\n\n\n###### EVENT_TYPE_SWITCH_ENTER_STABLE ######");
    swtester.test(EVENT_TYPE_SWITCH_ENTER_STABLE);
    //LOG_WARN_FMT("\n\n\n###### EVENT_TYPE_SWITCH_DISCONNECT ######");
    swtester.test(EVENT_TYPE_SWITCH_DISCONNECT);
#endif
}

CHeartbeatHandler::CHeartbeatHandler()
{
    init();
}

CHeartbeatHandler::~CHeartbeatHandler()
{
}

INT4 CHeartbeatHandler::init()
{
    const INT1* pInterval = CConf::getInstance()->getConfig("heartbeat_conf", "heartbeat_interval");
    m_heartbeatInterval = (pInterval == NULL) ? HEARTBEAT_INTERVAL_DEFAULT : atol(pInterval);
    LOG_INFO_FMT("heartbeat_interval %u", m_heartbeatInterval);

    if (0 == m_heartbeatInterval)
        return BNC_OK;

    const INT1* pTimes = CConf::getInstance()->getConfig("heartbeat_conf", "heartbeat_times");
    m_heartbeatTimes = (pTimes == NULL) ? HEARTBEAT_TIMES_DEFAULT : atol(pTimes);
    LOG_INFO_FMT("heartbeat_times %u", m_heartbeatTimes);

    return m_timer.schedule(m_heartbeatInterval, m_heartbeatInterval, CHeartbeatHandler::heartbeat, this);
}

INT4 CHeartbeatHandler::onregister()
{
    CMsgPath path1 = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_ECHO_REQUEST);
    if (CMsgHandler::onregister(path1, 1))
    {
        LOG_WARN_FMT("CHeartbeatHandler register to path[%s] failed !", path1.c_str());
        return BNC_ERR;
    }

    CMsgPath path2 = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_ECHO_REPLY);
    if (CMsgHandler::onregister(path2, 1))
    {
        LOG_WARN_FMT("CHeartbeatHandler register to path[%s] failed !", path2.c_str());
        CMsgHandler::deregister(path1);
        return BNC_ERR;
    }

    return BNC_OK;
}

void CHeartbeatHandler::deregister()
{
    CMsgPath path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_ECHO_REQUEST);
    CMsgHandler::deregister(path);

    path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_ECHO_REPLY);
    CMsgHandler::deregister(path);
}

INT4 CHeartbeatHandler::handle(CSmartPtr<CMsgCommon> msg)
{
    CMsg* ofmsg = (CMsg*)msg.getPtr();

    LOG_DEBUG_FMT("%s[%p] handle new msg of path[%s] from sockfd[%d] ...", 
        toString(), this, ofmsg->getPath().c_str(), ofmsg->getSockfd());

    INT4 sockfd = ofmsg->getSockfd();
    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (sw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        COfConnectMgr::processSystemFailure(sockfd);
        return BNC_ERR;
    }

    COfConnectMgr::processPeerExitUnreach(sw);

    INT1* data = ofmsg->getData();
    if (NULL == data)
    {
        LOG_WARN_FMT("%s[%p] received msg with no data from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }

    if (ofmsg->getType() == OFPT13_ECHO_REQUEST) 
    {
        LOG_DEBUG_FMT("send OFPT13_ECHO_REPLY to switch with sockfd[%d] ...", sockfd);
        UINT4 xid = ((struct ofp_header*)data)->xid;
        BOOL ret = COfMsgUtil::sendOfpMsg(sockfd, OFP13_VERSION, OFPT13_ECHO_REPLY, xid);
        if (!ret)
        {
            LOG_WARN_FMT("send OFPT13_ECHO_REPLY to switch with sockfd[%d] failed !", sockfd);
            COfConnectMgr::processPeerEnterUnreach(sw);
            return BNC_ERR;
        }
    }
    else
    {
        //HEARTBEAT正常进行，每次的请求都能及时收到回应
        sw->m_heartbeatTimes = 0;
        sw->m_heartbeatInterim = 0;
    }

    return BNC_OK;
}

