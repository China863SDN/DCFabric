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
*   File Name   : COfConnectMgr.cpp                                           *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-error.h"
#include "log.h"
#include "comm-util.h"
#include "COfMsgUtil.h"
#include "CControl.h"
#include "COfConnectMgr.h"
#include "CServer.h"
#include "CSwitchEventReportor.h"
#include "COfRecvWorker.h"

CTimer COfConnectMgr::m_timer;
std::map<UINT8, CSmartPtr<CSwitch> > COfConnectMgr::m_sws;
CMutex COfConnectMgr::m_mutex;

void COfConnectMgr::processPeerConnect(CSmartPtr<CSwitch>& sw)
{
    if (sw.isNotNull() && (sw->getState() < SW_STATE_NEW_ACCEPT))
    {
        //上报交换机事件SWITCH_CONNECT
        CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_CONNECT, EVENT_REASON_NONE);
    }
}

void COfConnectMgr::processPeerQuit(CSmartPtr<CSwitch>& sw, INT4 reason)
{
    if (sw.isNotNull())
    {
        //连接信息需要从相应的COfRecvWorker中清�?
        CSmartPtr<COfRecvWorker> worker = CServer::getInstance()->mapOfRecvWorker(sw->getSockfd());
        if (worker.isNotNull())
            worker->processConnClose(sw->getSockfd());

        if (sw->getState() < SW_STATE_CLOSING)
        {
            sw->setState(SW_STATE_CLOSING);

            //上报交换机事件SWITCH_QUIT
            CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_QUIT, reason);
        }
        
        //删除交换机，更新TOPO
        CControl::getInstance()->getSwitchMgr().delSwitch(sw->getSockfd());
    }
}

void COfConnectMgr::processPeerEnterStable(CSmartPtr<CSwitch>& sw)
{
    if (sw.isNotNull())
    {
        //上报交换机事件ENTER_STABLE
        CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_ENTER_STABLE, EVENT_REASON_NONE);
    }
}

void COfConnectMgr::processPeerEnterUnreach(CSmartPtr<CSwitch>& sw, INT4 reason)
{
    if (sw.isNotNull() && (sw->getState() < SW_STATE_UNREACHABLE))
    {
        if (isValidMac(sw->getSwMac()))
        {
            sw->setState(SW_STATE_UNREACHABLE);

            //上报交换机事件ENTER_UNREACH
            CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_ENTER_UNREACH, reason);

            //启动掉电检�?
            startPowerOffCheck(sw);
        }
        else
        {
            //seems mininet sws have all zero mac and same ip        
            processPeerQuit(sw, reason);
        }
    }
}

void COfConnectMgr::processPeerExitUnreach(CSmartPtr<CSwitch>& sw, INT4 reason)
{
    if (sw.isNotNull() && (sw->getState() == SW_STATE_UNREACHABLE))
    {
        //交换机状态恢复为STABLE
        sw->setState(SW_STATE_STABLE);

        //上报交换机事件EXIT_UNREACH
        CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_EXIT_UNREACH, reason);

        //停止掉电检�?
        stopPowerOffCheck(sw);
    }
}

void COfConnectMgr::processPeerDisconn(CSmartPtr<CSwitch>& sw)
{
    if (sw.isNotNull() && (sw->getState() < SW_STATE_DISCONNECTED))
    {
        if (isValidMac(sw->getSwMac()))
        {
            sw->setState(SW_STATE_DISCONNECTED);
            
            //上报交换机事件SWITCH_DISCONNECT
            CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_DISCONNECT, EVENT_REASON_SWITCH_DISCONNECT);

            //启动掉电检�?
            startPowerOffCheck(sw);
        }
        else
        {
            //seems mininet sws have all zero mac and same ip
            processPeerQuit(sw, EVENT_REASON_SWITCH_DISCONNECT);
        }
    }
}

void COfConnectMgr::processPeerPowerOff(CSmartPtr<CSwitch>& sw)
{
    if (sw.isNotNull() && (sw->getState() < SW_STATE_POWER_OFF))
    {
        sw->setState(SW_STATE_POWER_OFF);
        
        //上报交换机事件POWER_OFF
        CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_POWER_OFF, EVENT_REASON_SWITCH_POWER_OFF);
        
        //停止掉电检�?
        stopPowerOffCheck(sw);

        processPeerQuit(sw, EVENT_REASON_SWITCH_POWER_OFF);
    }
}

void COfConnectMgr::processPeerReconn(CSmartPtr<CSwitch>& sw, switch_dupl_conn_t& duplConn)
{
    //重复的连接信息需要从相应的COfRecvWorker中清�?
    CSmartPtr<COfRecvWorker> worker = CServer::getInstance()->mapOfRecvWorker(sw->getSockfd());
    if (worker.isNotNull())
        worker->processConnClose(duplConn.sockfd);

    //上报交换机事件SWITCH_RECONNECT
    CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_RECONNECT, EVENT_REASON_SWITCH_RECONNECT, &duplConn);
    
    //如果该交换机之前启动了掉电检测，停止掉电检�?
    stopPowerOffCheck(sw);
}

void COfConnectMgr::processSystemFailure(INT4 sockfd)
{
    //连接信息需要从相应的COfRecvWorker中清�?
    CSmartPtr<COfRecvWorker> worker = CServer::getInstance()->mapOfRecvWorker(sockfd);
    if (worker.isNotNull())
        worker->processConnClose(sockfd);

    CSmartPtr<CSwitch> sw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (sw.isNotNull())
    {
        sw->setState(SW_STATE_CLOSING);

        //上报交换机事件SWITCH_QUIT
        CSwitchEventReportor::getInstance()->report(sw, EVENT_TYPE_SWITCH_QUIT, EVENT_REASON_SYSTEM_FAILURE);
        
        //删除交换机，更新TOPO
        CControl::getInstance()->getSwitchMgr().delSwitch(sockfd);
    }
}

BOOL COfConnectMgr::needPowerOffCheck(CSmartPtr<CSwitch>& sw)
{
    if (sw.isNull() || (sw->getState() >= SW_STATE_POWER_OFF))
        return FALSE;

    CPortMap& ports = sw->getPorts();
    STL_FOR_LOOP(ports, it)
    {
        gn_port_t& port = it->second;
        if (PORT_STATE_UP == port.state)
        {
            //if (PORT_TYPE_EXTERNAL == port.type)
            //    return TRUE;
            if (PORT_TYPE_SWITCH == port.type)
                return TRUE;
        }
    }

    return FALSE;
}

void COfConnectMgr::startPowerOffCheck(CSmartPtr<CSwitch>& sw)
{
    if (!needPowerOffCheck(sw))
        return;

    if (checkPowerOff(sw))
    {
        processPeerPowerOff(sw);
        return;
    }

    m_mutex.lock();
    std::map<UINT8, CSmartPtr<CSwitch> >::iterator it = m_sws.find(sw->getDpid());
    if (it == m_sws.end())
    {
        m_sws.insert(std::pair<UINT8, CSmartPtr<CSwitch> >(sw->getDpid(), sw));
        LOG_WARN_FMT("start power-off check on switch[%llx] with sockfd[%d], checking number[%lu]", 
            sw->getDpid(), sw->getSockfd(), m_sws.size());        
    }
    m_mutex.unlock();

    if (m_timer.isStop())
    {
        INT4 ret = m_timer.schedule(power_off_check_interval, power_off_check_interval, checkPowerOff, NULL);
        if (BNC_OK != ret)
            LOG_WARN_FMT("schedule checkPowerOff failed !");
    }
}

void COfConnectMgr::stopPowerOffCheck(CSmartPtr<CSwitch>& sw)
{
    if (sw.isNull())
        return;

    m_mutex.lock();
    m_sws.erase(sw->getDpid());
    m_mutex.unlock();
}

void COfConnectMgr::checkPowerOff(void* param)
{
    std::list<CSmartPtr<CSwitch> > powerOffs;

    m_mutex.lock();

    std::map<UINT8, CSmartPtr<CSwitch> >::iterator cur;
    std::map<UINT8, CSmartPtr<CSwitch> >::iterator next = m_sws.begin();
    while (next != m_sws.end())
    {
        cur = next++;
        CSmartPtr<CSwitch>& sw = cur->second;
        if (sw.isNotNull() && (sw->getState() < SW_STATE_POWER_OFF))
        {
            if (checkPowerOff(sw))
            {
                powerOffs.push_back(sw);

                m_sws.erase(cur);
            }
        }
        else
        {
            m_sws.erase(cur);
        }
    }

    m_mutex.unlock();

    if (m_sws.size() > 0)
        LOG_WARN_FMT("after checkPowerOff, checking number[%lu]", m_sws.size());

    STL_FOR_LOOP(powerOffs, it)
    {
        processPeerPowerOff(*it);
    }
}

BOOL COfConnectMgr::checkPowerOff(CSmartPtr<CSwitch>& sw)
{
    CPortMap& ports = sw->getPorts();
    STL_FOR_LOOP(ports, it)
    {
        gn_port_t& port = it->second;
        if (PORT_STATE_UP == port.state)
        {
            //if (PORT_TYPE_EXTERNAL == port.type)
            //    return FALSE;
            if (PORT_TYPE_SWITCH == port.type)
                if (CControl::getInstance()->getTopoMgr().isLinkUp(sw->getDpid(), port.port_no))
                    return FALSE;
        }
    }

    LOG_WARN_FMT("switch[%llx] power-off: none up neighbors !", sw->getDpid());
    return TRUE;
}


