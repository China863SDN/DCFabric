/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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
*   File Name   : CClusterService.cpp                                         *
*   Author      : jiang bo                                                    *
*   Create Date : 2018-5-25                                                   *
*   Version     : 1.0                                                         *
*   Function    : .                                                           *
*                                                                             *
******************************************************************************/
#include "CClusterDefine.h"
#include "CClusterService.h"
#include "CClusterInterface.h"
#include "CClusterKeepalive.h"
#include "CClusterElect.h"
#include "CClusterSync.h"
#include "CConf.h"
#include "CServer.h"
#include "CControl.h"
#include "COfMsgUtil.h"
#include "bnc-error.h"
#include "openflow-common.h"
#include "log.h"
#include "comm-util.h"

#define DEFAULT_CLUSTER_SERVICE_PORT    55555
#define CLUSTER_DEL_VIP_COMMAND         "sh ./tools/controller-tools/delete_virtual_ip.sh"
#define CLUSTER_ADD_VIP_COMMAND         "sh ./tools/controller-tools/add_virtual_ip.sh"

extern UINT1 BROADCAST_MAC[6];


CClusterService* CClusterService::m_instance = NULL;

CClusterService* CClusterService::getInstance()
{
    if (NULL == m_instance) 
    {
        m_instance = new CClusterService();
        if (NULL == m_instance)
        {
            exit(-1);
        }
    }

    return m_instance;
}

CClusterService::CClusterService():
    m_clusterOn(FALSE),
    m_controllerRole(OFPCR_ROLE_EQUAL),
    m_virtualIp(0),
    m_ip(0),
    m_port(0)
{
}

CClusterService::~CClusterService()
{
    clear();
}

INT4 CClusterService::init()
{
    const INT1* pConf = CConf::getInstance()->getConfig("cluster_conf", "cluster_on");
    m_clusterOn = (NULL != pConf) ? (atoi(pConf) > 0) : FALSE;
	LOG_INFO_FMT("cluster_on: %s", m_clusterOn ? "true" : "false");

    if (!m_clusterOn)
        return BNC_OK;

    if (initVirtualIp() != BNC_OK)
        return BNC_ERR;

    if (initServers() != BNC_OK)
        return BNC_ERR;

    if (initControllers() != BNC_OK)
        return BNC_ERR;

    if (CClusterKeepalive::init() != BNC_OK)
        return BNC_ERR;

    if (CClusterSync::init() != BNC_OK)
        return BNC_ERR;

    connectRemotes();

    usleep(500*1000); //500ms
    if (CLUSTER_ELECTION_INIT == CClusterElection::getState())
        if (CClusterElection::start() != BNC_OK)
            return BNC_ERR;

    return BNC_OK;
}

void CClusterService::clear()
{
    STL_FOR_LOOP(m_controllers, it)
    {
        cluster_controller_t& controller = *it;
        if (-1 != controller.sockfd)
        {
            close(controller.sockfd);
            controller.sockfd = -1;
        }
    }

    CClusterSync::clear();
}

INT4 CClusterService::initVirtualIp()
{
    const INT1* conf = CConf::getInstance()->getConfig("cluster_conf", "virtual_ip");
    if (NULL != conf)
    {
        INT1 str[48] = {0}, *ptr = NULL;
        strncpy(str, conf, 47);
        if ((ptr = strchr(str, '/')) != NULL)
            *ptr = '\0';
        m_virtualIp = ntohl(ip2number(str));
    }

    return BNC_OK;
}

INT4 CClusterService::initServers()
{
    m_ip = CServer::getInstance()->getControllerIp();

    const INT1* pPort = CConf::getInstance()->getConfig("controller", "cluster_service_port");
    m_port = (NULL == pPort) ? DEFAULT_CLUSTER_SERVICE_PORT : atoi(pPort);
    LOG_INFO_FMT("cluster_service_port %u", m_port);

    if (m_recvWorker.init() != BNC_OK)
    {
        LOG_ERROR("init CClusterRecvWorker failed !");
        return BNC_ERR;
    }

    new(&m_listener) CClusterTcpListener(m_ip, m_port);
    if (m_listener.init() != BNC_OK)
    {
        LOG_ERROR("init CClusterTcpListener failed !");
        return BNC_ERR;
    }

    return BNC_OK;
}

INT4 CClusterService::initControllers()
{
    const INT1* conf = CConf::getInstance()->getConfig("cluster_conf", "cluster_controller_list");
    if (NULL != conf)
    {
        INT1 *ptr = NULL;
        INT1 controllers[1024] = {0};
        strncpy(controllers, conf, 1024);

        for (ptr = strtok(controllers, ","); NULL != ptr; ptr = strtok(NULL, ","))
        {
            cluster_controller_t controller;
            controller.sockfd = -1;
            controller.ip = ntohl(ip2number(ptr));
            controller.port = 0;
            controller.state = CLUSTER_CONTROLLER_INIT;
            controller.karetry = 0;

            m_controllers.push_back(controller);
        }
    }

    return BNC_OK;
}

void CClusterService::connectRemotes()
{
    STL_FOR_LOOP(m_controllers, it)
    {
        cluster_controller_t& controller = *it;
        if (CServer::getInstance()->getControllerIp() != controller.ip)
            connectRemote(controller.ip);
    }
}

INT4 CClusterService::connectRemote(UINT4 ip)
{
    INT4 sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
       LOG_ERROR("Create cluster client socket failed !");
       return BNC_ERR;
    }

    struct sockaddr_in addr;
    bzero(&addr, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(m_port);
    addr.sin_addr.s_addr = htonl(ip);

    INT4 ret = connect(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    INT4 errnum = errno;
    if (ret == 0)
    {
        LOG_WARN_FMT("-- connect to CLUSTER remote[%s:%d] success, sockfd[%d] --", inet_htoa(ip), m_port, sockfd);
        if (createRemote(sockfd, ip, m_port) != BNC_OK)
        {
            close(sockfd);
            return BNC_ERR;
        }
    }
    else if ((ret < 0) && (EINPROGRESS == errnum))
    {
        //connect is in progress
        fd_set set;  
        FD_ZERO(&set);  
        FD_SET(sockfd, &set);

        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 10000; //10 ms' latency

        ret = select(sockfd+1, NULL, &set, NULL, &timeout);
        if (ret < 0)
        {
            LOG_WARN_FMT("connect to CLUSTER remote[%s:%u], select failed", inet_htoa(ip), m_port);
            close(sockfd);
            return BNC_ERR;
        }
        else if (ret == 0)
        {
            LOG_WARN_FMT("connect to CLUSTER remote[%s:%u], select timeout", inet_htoa(ip), m_port);
            close(sockfd);
            return BNC_ERR;
        }
        else
        {
            if (!FD_ISSET(sockfd, &set))
            {
                LOG_WARN_FMT("connect to CLUSTER remote[%s:%u], unready sockfd[%d]", inet_htoa(ip), m_port, sockfd);
                close(sockfd);
                return BNC_ERR;
            }

            int error = 0;  
            socklen_t len = sizeof(error); 
            if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR, &error, &len) != 0)
            {
                LOG_WARN_FMT("connect to CLUSTER remote[%s:%u], getsockopt failed", inet_htoa(ip), m_port);
                close(sockfd);
                return BNC_ERR;
            }
            if (0 != error)
            {
                LOG_WARN_FMT("connect to CLUSTER remote[%s:%u], getsockopt error[%d]", inet_htoa(ip), m_port, error);
                close(sockfd);
                return BNC_ERR;
            }

            LOG_WARN_FMT("--- connect to CLUSTER remote[%s:%d] success, sockfd[%d] ---", inet_htoa(ip), m_port, sockfd);
            if (createRemote(sockfd, ip, m_port) != BNC_OK)
            {
                close(sockfd);
                return BNC_ERR;
            }
        }
    }
    else
    {
        LOG_WARN_FMT("connect to CLUSTER remote[%s:%u] failed[%d]", inet_htoa(ip), m_port, errnum);
        close(sockfd);
        return BNC_ERR;
    }

    CClusterElection::sendHelloReq(sockfd);

    return BNC_OK;
}

INT4 CClusterService::remoteConnect(INT4 sockfd, UINT4 ip, UINT2 port)
{
    LOG_WARN_FMT("--- new CLUSTER remote[%s:%d]sockfd[%d] connected ---", inet_htoa(ip), port, sockfd);

    if (createRemote(sockfd, ip, port) != BNC_OK)
    {
        close(sockfd);
        return BNC_ERR;
    }

    return BNC_OK;
}

void CClusterService::remoteDisconnect(INT4 sockfd)
{
    UINT4 ip = 0;

    STL_FOR_LOOP(m_controllers, it)    
    {
        cluster_controller_t& controller = *it;
        if (sockfd == controller.sockfd)
        {
            LOG_WARN_FMT("$$$ remote CLUSTER %s[%s]sockfd[%d] disconnected $$$", 
                (CClusterElection::getMasterIp()==controller.ip)?"MASTER":"SLAVE",
                inet_htoa(controller.ip), sockfd);
            m_recvWorker.delConnFd(controller.sockfd);
            close(controller.sockfd);
            controller.sockfd = -1;
            controller.port = 0;
            controller.state = CLUSTER_CONTROLLER_DISCONNECTED;
            controller.karetry = 0;
            ip = controller.ip;
            break;
        }
    }

    if (ip > 0)
    {
        INT4 ret = connectRemote(ip);
        if ((BNC_OK != ret) && (CClusterElection::getMasterIp() == ip))
        {
            CClusterElection::start();
        }
    }
    else
    {
        close(sockfd);
    }
}

INT4 CClusterService::createRemote(INT4 sockfd, UINT4 ip, UINT2 port)
{
    BOOL update = FALSE;
    cluster_controller_t* node = NULL;

	m_mutex.lock();

    STL_FOR_LOOP(m_controllers, it)    
    {
        cluster_controller_t& controller = *it;
        if (controller.ip == ip)
        {
            if (-1 != controller.sockfd) 
            {
                //close old connection
                m_recvWorker.delConnFd(controller.sockfd);
                close(controller.sockfd);
                controller.sockfd = -1;
            }
            
            //keep new connection
            controller.sockfd = sockfd;
            controller.port = port;
            controller.state = CLUSTER_CONTROLLER_CONNECTED;
            controller.karetry = 0;
            m_recvWorker.addConnFd(sockfd);

            node = &controller;
            //LOG_INFO("after update, m_controllers.size = %u", m_controllers.size());            
            break;
        }
    }

    if (NULL == node)
    {
        cluster_controller_t controller;
        controller.sockfd = sockfd;
        controller.ip = ip;
        controller.port = port;
        controller.state = CLUSTER_CONTROLLER_CONNECTED;
        controller.karetry = 0;

        m_controllers.push_back(controller);
        m_recvWorker.addConnFd(sockfd);

        //LOG_INFO("after insert, m_controllers.size = %u", m_controllers.size());            

        //update config
        update = TRUE;
    }

    m_mutex.unlock();

    if (update)
        updateConfig();

	return BNC_OK;
}

INT4 CClusterService::updateRole(INT4 role)
{
    if (role == m_controllerRole)
        return BNC_OK;

    if (OFPCR_ROLE_MASTER == role)
    {
        system(CLUSTER_ADD_VIP_COMMAND);
        sendGratuitousArp();
        CClusterSync::syncAll();
    }
    else if (OFPCR_ROLE_SLAVE == role)
    {
        system(CLUSTER_DEL_VIP_COMMAND);
    }

    m_controllerRole = role;

    CSwitchHMap& map = CControl::getInstance()->getSwitchMgr().getSwitchMap();
	CControl::getInstance()->getSwitchMgr().lock();

    STL_FOR_LOOP(map, it)
    {
		CSmartPtr<CSwitch> sw = it->second;
        if (sw.isNull())
            continue;
        
		if ((OFP13_VERSION == sw->getVersion()) &&
            (SW_STATE_STABLE == sw->getState()))
		{
            COfMsgUtil::sendOfp13RoleRequest(sw->getSockfd(), role);
        }
    }

	CControl::getInstance()->getSwitchMgr().unlock();

    return BNC_OK;
}

void CClusterService::updateConfig()
{
	const INT1* block = "cluster_conf";
	const INT1* key   = "cluster_controller_list";
    INT1 value[256] = {0};

    for (UINT4 i = 0; i < m_controllers.size(); ++i)
    {
        if (0 != m_controllers[i].ip)
        {
            INT1* ipStr = inet_htoa(m_controllers[i].ip);
            strncat(value, ipStr, sizeof(value)-strlen(value)-1);
            if ((i < m_controllers.size()-1) && (strlen(value) < sizeof(value)-1))
                strcat(value, ",");
        }
    }

    LOG_INFO_FMT("CLUSTER save cluster_controller_list[%s]", value);
    CConf::getInstance()->setConfig(block, key, value);
    CConf::getInstance()->saveConf();
}

INT4 CClusterService::sendGratuitousArp()
{
    if (0 == m_virtualIp)
        return BNC_ERR;

	INT1 mac_str[48] = {0};
	mac2str(CServer::getInstance()->getControllerMac(), mac_str);

    LOG_WARN_FMT("send gratuitous arp with mac[%s]vip[%s]", mac_str, inet_htoa(m_virtualIp));

    INT4 sockfd = socket(AF_PACKET, SOCK_RAW, htons(ETH_P_ARP));
    if (sockfd < 0)
    {
        LOG_ERROR_FMT("Create raw socket failed[%d] !", errno);
        return BNC_ERR;
    }

    struct ifreq ifr;
    bzero(&ifr, sizeof(ifr));
    strcpy(ifr.ifr_name, CServer::getInstance()->getControllerEth().c_str());
    if (ioctl(sockfd, SIOCGIFINDEX, &ifr) != 0)
    {
        LOG_ERROR_FMT("ioctl get interface index for %s failed[%d] !", 
            CServer::getInstance()->getControllerEth().c_str(), errno);
        close(sockfd);
        return BNC_ERR;
    }

#if 0
    struct sockaddr_ll{ 
        unsigned short sll_family; /* 总是 AF_PACKET */ 
        unsigned short sll_protocol; /* 物理层的协议 */ 
        int sll_ifindex; /* 接口号 */ 
        unsigned short sll_hatype; /* 报头类型 */ 
        unsigned char sll_pkttype; /* 分组类型 */ 
        unsigned char sll_halen; /* 地址长度 */ 
        unsigned char sll_addr[8]; /* 物理层地址 */ 
    };
#endif
    struct sockaddr_ll sll;
    memset(&sll, 0, sizeof(sll));
    sll.sll_family = AF_PACKET;
    sll.sll_protocol = htons(ETH_P_ARP);
    sll.sll_ifindex = ifr.ifr_ifindex;
    //sll.sll_halen = ETHER_ADDR_LEN;
    //memcpy(sll.sll_addr, BROADCAST_MAC, ETHER_ADDR_LEN);

    arp_t arp = {0};
    memcpy(arp.eth_head.dest, BROADCAST_MAC, 6);
    memcpy(arp.eth_head.src, CServer::getInstance()->getControllerMac(), 6);
    arp.eth_head.proto = htons(ETHER_ARP);
    arp.hardwaretype = htons(1);
    arp.prototype = htons(ETHER_IP);
    arp.hardwaresize = 0x6;
    arp.protocolsize = 0x4;
    arp.opcode = htons(1);
    arp.sendip = htonl(m_virtualIp);
    arp.targetip = htonl(m_virtualIp);
    memcpy(arp.sendmac, CServer::getInstance()->getControllerMac(), 6);
    memcpy(arp.targetmac, BROADCAST_MAC, 6);

    INT4 count = 0;
    for (; count < 5; ++count)
    {
        if (sendto(sockfd, &arp, sizeof(arp), 0, (struct sockaddr*)&sll, sizeof(sll)) < 0)  
            LOG_ERROR_FMT("send gratuitous arp via interface %s failed[%d] !", 
                CServer::getInstance()->getControllerEth().c_str(), errno);
    }

    close(sockfd);

    return BNC_OK;
}

