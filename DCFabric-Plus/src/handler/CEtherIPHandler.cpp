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
*   File Name   : CEtherIPHandler.cpp           *
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
#include "COfMsgUtil.h"
#include "CHost.h"
#include "CHostMgr.h"
#include "BasePort.h"
#include "BasePortManager.h"
#include "CFlowMgr.h"
#include "CControl.h"
#include "CServer.h"
#include "CRecvWorker.h"
#include "CProxyConnectMgr.h"
#include "COpenstackMgr.h"
#include "COpenstackExternal.h"
#include "COpenstackExternalMgr.h"
#include "CEtherIPHandler.h"
#include "CServiceMgr.h"

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
    if (++total%100000 == 0)\
    {\
        time_t tmCurr = time(NULL);\
        time_t pdDlta = tmCurr - timePeriod;\
        if (pdDlta >= 10)\
        {\
            UINT8 currSpeed = (UINT8)((total - totalPeriod) / pdDlta);\
            if (currSpeed > 0)\
            {\
                LOG_WARN_FMT("%s received %llu pkts with average speed %llu pkts/s, and current speed %llu pkts/s ...", \
                    toString(), total, (UINT8)(total/(tmCurr-timeStart)), currSpeed);\
                timePeriod = tmCurr;\
                totalPeriod = total;\
            }\
        }\
    }\
}


CEtherIPHandler::CEtherIPHandler()
{
}

CEtherIPHandler::~CEtherIPHandler()
{
}

INT4 CEtherIPHandler::onregister()
{
	const INT1* pNum = CConf::getInstance()->getConfig("controller", "handler_thread_num");
    UINT4 num = (pNum == NULL) ? 1 : atol(pNum);
    LOG_INFO_FMT("handler_thread_num %u", num);

    if (0 == num)
        num = 1;

    std::string path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_IP);
    return CMsgHandler::onregister(path, num, FALSE, THREAD_TYPE_OCCUPY_CORE);
}

void CEtherIPHandler::deregister()
{
	std::string path = COfMsgUtil::getMsgPath(OFP13_VERSION, OFPT13_PACKET_IN, ETHER_IP);
    CMsgHandler::deregister(path);
}

void CEtherIPHandler::processInternalPacket(CSmartPtr<CHost>& srcHost, CSmartPtr<CHost>& dstHost,
        const CSmartPtr<CSwitch> & srcSw, ip_t* pkt, packet_in_info_t* packetIn)
{

    LOG_INFO("Process Internal Packet");

    // 如果目的主机交换机不存在, 广播
    if (dstHost.isNull() || dstHost->getSw().isNull() || (0 == dstHost->getPortNo()))
    {
        /* 临时代码, 后续添加到专门处理广播的线程�?*/
        LOG_INFO("Fail to find dest switch. start flood");
        // flood(srcSw, packetIn);
       // floodInternal(srcSw, packetIn);
    }
    // 如果目的主机交换机存�?
    else
    {
        // 如果源和目标都是主机
        if (srcHost.isNotNull() && srcHost->isHost() && dstHost->isHost())
        {
            // 取得网关信息
            CSmartPtr<CHost> srcGateway = CHostMgr::getInstance()->getHostGateway(srcHost);
            CSmartPtr<CHost> dstGateway = CHostMgr::getInstance()->getHostGateway(dstHost);

            // 取得交换机信�?
            UINT8 srcDpid = srcHost->getSw()->getDpid();
            UINT8 dstDpid = dstHost->getSw()->getDpid();

            // 如果同网�?
            if (srcGateway == dstGateway)
            {
                // 如果不同交换�?
                if (srcDpid != dstDpid)
                {
                    CFlowMgr::getInstance()->install_different_switch_flow(srcHost, dstHost);
                    CFlowMgr::getInstance()->install_different_switch_flow(dstHost, srcHost);
                }
            }
            // 如果不同网段
            else
            {
                // 修改包中的目标主机Mac地址(修改前是源主机所在网关Mac)
                memcpy(pkt->eth_head.dest, dstHost->getMac(), 6);

                // 如果同交换机
                if (srcDpid == dstDpid)
                {
                    CFlowMgr::getInstance()->install_same_switch_gateway_flow(dstHost, srcGateway);
                    CFlowMgr::getInstance()->install_same_switch_gateway_flow(srcHost, dstGateway);
                }
                // 如果不同交换�?
                else
                {
                    CFlowMgr::getInstance()->install_different_switch_flow(srcHost, dstHost, srcGateway);
                    CFlowMgr::getInstance()->install_different_switch_flow(dstHost, srcHost, dstGateway);
                }
            }

            // 转发�?
            LOG_INFO("forward packet");
           // forward(dstHost->getSw(), dstHost->getPortNo(), packetIn);

            // 下发到主机的流表
            CFlowMgr::getInstance()->install_local_host_flows(srcHost);
            CFlowMgr::getInstance()->install_local_host_flows(dstHost);
        }
    }
}

void CEtherIPHandler::processInPacket(CSmartPtr<CHost>& dstHost, ip_t* pkt, packet_in_info_t* packetIn)
{
    LOG_INFO("Process Ip In");

    // 如果目标是代理主�?
    if (dstHost.isNotNull() && dstHost->isProxyHost())
    {
        // 获取ExternalSw �?Portno
        const CSmartPtr<CSwitch>& sw = CHostMgr::getInstance()->getExternalHostSw(dstHost);
        UINT4 port = CHostMgr::getInstance()->getExternalHostPort(dstHost);

        // 如果不存�?
        if ((0 == port) || sw.isNull())
        {
            LOG_INFO("Can't get External Switch and port info");
            return ;
        }

        // 获取代理连接信息
        CProxyConnect* proxyConnect = CProxyConnectMgr::getInstance()->findProxyConnect(
                                        pkt->src, pkt->dest, getDstPortNo(pkt), pkt->proto);

        // 如果存在连接
        if (proxyConnect)
        {
            // 下发流表
            CFlowMgr::getInstance()->install_proxy_flows(proxyConnect);
            // 将包转发到交换机, 重新处理
        }
        // 如果不存在连�?
        else
        {
            // 判断目的主机是不是FloatingIP, 只有FloatingIP才接受从外部的访�?
            UINT4 fixedIp = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getFixedipByFloatingip(dstHost->getIp());

            if (fixedIp)
            {
                CSmartPtr<CHost> fixedHost = CHostMgr::getInstance()->findHostByIp(fixedIp);
                if (fixedHost.isNotNull())
                {
                    COpenstackExternal* external = COpenstackExternalMgr::getInstance()->findOpenstackExternalAny();

                    if (external)
                    {
                        LOG_INFO("Create new floating ip connect.");
                        // 创建连接
                        proxyConnect = CProxyConnectMgr::getInstance()->addProxyConnect(
                                                        pkt->src, pkt->dest, fixedIp,
                                                        external->getExternalGatewayMac(), dstHost->getMac(),fixedHost->getMac(),
                                                        0, 0, pkt->proto);

                        if (proxyConnect)
                        {
                            CFlowMgr::getInstance()->install_proxy_flows(proxyConnect);
                        }
                    }
                }
            }
            else
            {
                LOG_INFO("Fail to find fixedIp");
            }

        }
    }
    else
    {
        LOG_INFO("Is not proxy Host");
    }
}

void CEtherIPHandler::processOutPacket(const CSmartPtr<CSwitch> & srcSw, CSmartPtr<CHost>& srcHost, ip_t* pkt, packet_in_info_t* packetIn)
{
    LOG_INFO("Process Ip out");
    UINT4 extIp = pkt->dest;
    UINT1 proto = pkt->proto;

    // 其他存储信息
    UINT4 fixedIp = pkt->src;
    UINT1 extMac[6] = {0};
    UINT1 fixedMac[6] = {0};
    UINT1 proxyMac[6] = {0};
    UINT4 proxyIp = 0;

    // 判断TCP/UDP�? 获取Src的PortNo
    UINT2 proxyPortNo = getSrcPortNo(pkt);

    // 判断TCP/UDP�? 获取Dst的PortNo
    UINT2 extPortNo = getDstPortNo(pkt);
    // 判断TCP/UDP�? 获取Src的PortNo
    UINT2 fixedPortNo = getSrcPortNo(pkt);

    memcpy(fixedMac, pkt->eth_head.src, 6);

    // 如果是FloatingIP
    UINT4 floatingip = COpenstackMgr::getInstance()->getOpenstack()->getResource()->getFloatingipbyFixedIp(srcHost->getIp());
        // 获取Floating的ProxyIp
    if (floatingip)
    {
        LOG_INFO("is floating ip");
        CSmartPtr<CHost> floatingHost = CHostMgr::getInstance()->findHostByIp(floatingip);
        if (floatingHost.isNotNull())
        {
            LOG_INFO("find floating host");
            COpenstackExternal* external = COpenstackExternalMgr::getInstance()->findOpenstackExternalAny();

            if (external)
            {
                LOG_INFO("find external");

                proxyIp = floatingip;
                memcpy(proxyMac, floatingHost->getMac(), 6);
                memcpy(extMac, external->getExternalGatewayMac(), 6);
                proxyPortNo = 0;    // 对于floatingIp, 不需要匹配PortNo, 主要满足IP就可�?
                extPortNo = 0;
                fixedPortNo = 0;
            }
        }
    }
    else
    // 如果不是, 获取NAT的ExternalIp
    {
        LOG_INFO("is Nat");
        COpenstackExternal* external = COpenstackExternalMgr::getInstance()->findOpenstackExternalAny();

        if (external)
        {
            proxyIp = external->getExternalOuterInterfaceIp();
            memcpy(proxyMac, external->getExternalOuterInterfaceMac(), 6);
            memcpy(extMac, external->getExternalGatewayMac(), 6);
        }

        /* 暂时注释掉了, 后续再Coding */
        return ;
    }

    // 在代理连接中查找是否存在
    CProxyConnect* proxyConnect = CProxyConnectMgr::getInstance()->findProxyConnect(extIp, proxyIp, proxyPortNo, proto);

    // 如果不存�? 创建代理连接
    if (NULL == proxyConnect)
    {
        proxyConnect = CProxyConnectMgr::getInstance()->addProxyConnect(
                extIp, proxyIp, fixedIp, extMac, proxyMac, fixedMac, extPortNo, fixedPortNo, proto);
    }

    // 判断是否存在, 下发流表
    if (proxyConnect)
    {
        // 下发流表
        CFlowMgr::getInstance()->install_proxy_flows(proxyConnect);

        // 将包转发到交换机, 重新处理
       // forward(srcSw, OFPP13_TABLE, packetIn);
    }
}


INT4 CEtherIPHandler::handle(CSmartPtr<CMsgCommon> msg)
{
    procStats();

	INT4 ret = BNC_OK;	
	if(!CControl::getInstance()->isDelayTimeReach())
	{
		return BNC_ERR;
	}
    CMsg* ofmsg = (CMsg*)msg.getPtr();

    LOG_DEBUG_FMT("%s[%p] handle %d bytes IP msg of path[%s] from sockfd[%d] ...", 
        toString(), this, ofmsg->length(), ofmsg->getPath().c_str(), ofmsg->getSockfd());
	ret = CServiceMgr::getInstance()->IpProcess(ofmsg);
#if 0
    INT4 sockfd = msg->getSockfd();
    CSmartPtr<CSwitch> srcSw = CControl::getInstance()->getSwitchMgr().getSwitch(sockfd);
    if (srcSw.isNull())
    {
        LOG_WARN_FMT("CSwitch not created for sockfd[%d] !", sockfd);
        CRecvWorker* worker = CServer::getInstance()->mapOfRecvWorker(sockfd);
        if (NULL != worker)
            worker->processConnClosed(sockfd);
        return BNC_ERR;
    }

    INT1* data = msg->getData();
    if (NULL == data)
    {
        LOG_WARN_FMT("%s[%p] received IP msg with no data from sockfd[%d] !", 
            toString(), this, sockfd);
        return BNC_ERR;
    }

    packet_in_info_t packetIn = {0};
    COfMsgUtil::ofpPacketInConvertter(data, packetIn);

	
	ip_t* pkt = (ip_t*)packetIn.data;
	
	// 打印收到的信�?
	printPacketInfo(pkt);

#endif

	#if 0
    LOG_INFO_FMT("%s recv new msg ..., queue size = %lu", toString().c_str(), queue->size());

    CSmartPtr<CMsg> msg;
    while (queue->size() > 0)
    {
        queue->pop(msg);

		INT4 sockfd = msg->getSockfd();
		const CSmartPtr<CSwitch>& srcSw = CServer::getInstance()->getSwitchMgr()->findSwBySockFd(sockfd);

		packet_in_info_t* packetIn = msg->getPacketIn();
		ip_t *pkt = (ip_t *)packetIn->data;

		// 打印收到的包的信�?
		printPacketInfo(pkt);

		// 存储源主�?
		CHost* srcHost = CHostMgr::getInstance()->addHost(srcSw, packetIn->inport, pkt->eth_head.src, pkt->src);

		// 查找目的主机
		CHost* dstHost = CHostMgr::getInstance()->findHostByIp(pkt->dest);

		// 检测防火墙/安全�?防DDOS过载策略

        // 如果是广播包
        if (isFloodPacket(pkt->src, pkt->dest))
        {
            /* 临时代码, 后续添加到专门处理广播的线程�?*/
            LOG_INFO("Receive Ip flood packet");
            floodInternal(srcSw, packetIn);
        }
		// 如果源主机和目的主机都存�? 直接转发
        else if ((NULL != srcHost) && (NULL != dstHost))
		{
		    // 如果目的主机是代理主�?
		    if (dstHost->isProxyHost())
		    {
		        // 目的主机是Virtual IP或者FloatingIP, 逻辑后续添加
		    }
		    // 如果目的主机不是代理主机
		    else
		    {
		        // 内部主机之间的通信
		        processInternalPacket(srcHost, dstHost, srcSw, pkt, packetIn);
		    }
		}
        // 如果源主机为�? 目标主机存在, 那么这是从外部网络进来的�?
        else if ((NULL == srcHost) && (NULL != dstHost))
        {
            processInPacket(dstHost, pkt, packetIn);
        }
        // 如果源主机不为空, 目标主机为空, 那么这是从内部发往外部网络的包
        else if ((NULL != srcHost) && (NULL == dstHost))
        {
            // 判断访问的是不是禁用的IP
                // 如果�? 不处�?
            // 判断是不是DHCP�?--> 这里可能不需要处�? 因为之前的广播已经处理了
                // 如果是DHCP�? 广播

            // 其他情况, 处理发送给外网
            // else
            {
                processOutPacket(srcSw, srcHost, pkt, packetIn);
            }
        }
		// 其他情况
		else
		{
		    // 不处�?
		}
	}
	#endif
	return ret;
}

const char* CEtherIPHandler::toString()
{
    return "CEtherIPHandler";
}

CMsgHandler* CEtherIPHandler::clone()
{
    CMsgHandler* instance = new CEtherIPHandler();
    *instance = *this;
    return instance;
}

UINT4 CEtherIPHandler::getSrcPortNo(ip_t* pkt)
{
    UINT4 portNo = 0;

    if (NULL == pkt)
    {
        // do nothing
    }
    if (IPPROTO_TCP == pkt->proto)
    {
        tcp_t* tcp_pkt = (tcp_t*)pkt->data;
        if (tcp_pkt) {
            portNo = tcp_pkt->sport;
        }
    }
    else if (IPPROTO_UDP == pkt->proto)
    {
        udp_t* udp_pkt = (udp_t*)pkt->data;
        if (udp_pkt) {
            portNo = udp_pkt->sport;
        }
    }
    else
    {
        // do nothing
    }

    return portNo;
}

UINT4 CEtherIPHandler::getDstPortNo(ip_t* pkt)
{
    UINT4 portNo = 0;

    if (NULL == pkt)
    {
        // do nothing
    }
    else if (IPPROTO_TCP == pkt->proto)
    {
        tcp_t* tcp_pkt = (tcp_t*)pkt->data;
        if (tcp_pkt) {
            portNo = tcp_pkt->dport;
        }
    }
    else if (IPPROTO_UDP == pkt->proto)
    {
        udp_t* udp_pkt = (udp_t*)pkt->data;
        if (udp_pkt) {
            portNo = udp_pkt->dport;
        }
    }
    else
    {
        // do nothing
    }

    return portNo;
}

void CEtherIPHandler::printPacketInfo(ip_t* pkt)
{
    INT1 str_src[24] = {0};
    INT1 str_dst[24] = {0};
    number2ip(pkt->src, str_src);
    number2ip(pkt->dest, str_dst);

    LOG_INFO_FMT("src ip is %s, dst ip is %s", str_src, str_dst);
}

BOOL CEtherIPHandler::isFloodPacket(UINT4 src_ip, UINT4 dst_ip)
{
    // 判断源IP�?0.0.0.0", 并且目的主机�?255.255.255.255"
    if ((0 == src_ip) && ((UINT4)-1 == dst_ip))
    {
        return TRUE;
    }

    return FALSE;
}

