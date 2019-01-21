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
*   File Name   : CSwitchSimulator.cpp                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "bnc-type.h"
#include "bnc-inet.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"
#include "CSwitchSimulator.h"

UINT4 g_ctrlIp = 0;
UINT2 g_ctrlPort = 0;
INT4 g_proto = 0;
INT4 g_switchNumber = 0;
UINT4 g_hostNumber = 1;
UINT4 g_hostStartIp = 1;

static const INT4 g_helloSize = 16;
static const INT4 g_ipSize = 100;
static const INT4 g_arpReqSize = 200;
static const INT4 g_sleep_us = 50;

#define STL_FOR_LOOP(container, it) \
	for(typeof((container).begin()) it = (container).begin(); it != (container).end(); ++it)

typedef struct {
    INT4 sockfd;
    CTimer* timer;
    UINT4 startIp;
}sockfd_timer_t;

static BOOL sendMsgOut(INT4 fd, const INT1* data, INT4 len)
{
    try
    {
        INT4 writeLen = 0;

        if (len > 0)
        {
            INT4 ret = 0;

            while (writeLen < len)
            {
                ret = write(fd, data+writeLen, len-writeLen);
                if (ret <= 0)
                {
                    INT4 iErrno = errno;
                    if (EAGAIN == iErrno)
                    {
                        usleep(500);
                        continue;
                    }
                    else if (EINTR == iErrno)
                    {
                        usleep(500);
                        continue;
                    }
                    printf("sendMsgOut: sockfd[%d], length[%d], sent[%d], errno[%d]\n", 
                        fd, len, writeLen, errno);
                    break;
                }
                writeLen += ret;
            }
        }

        return ((writeLen < len) ? FALSE : TRUE);
    }
    catch (...)
    {
        printf("Catch write exception");
        return FALSE;
    }
}

static INT4 setNonBlocking(INT4 fd)
{
    INT4 old_flag = fcntl(fd, F_GETFL);
    INT4 new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
}

static void initOfHeader(INT1* buffer, INT4 size, INT4 sockfd)
{
    struct ofp_header* header = (struct ofp_header *)buffer;
    header->version = OFP13_VERSION;
    header->type = OFPT13_HELLO;
    header->length = htons(size);
    header->xid = htonl(sockfd);
}

static void initPktinIp(INT1* buffer, INT4 size, INT4 sockfd, UINT4 src)
{
    struct ofp13_packet_in* of_pkt = (struct ofp13_packet_in *)buffer;
    of_pkt->header.version = OFP13_VERSION;
    of_pkt->header.type = OFPT13_PACKET_IN;
    of_pkt->header.length = htons(size);
    of_pkt->header.xid = htonl(1);
    of_pkt->buffer_id = 0xffffffff;
    of_pkt->total_len = htons(size - 42);
    of_pkt->match.type = htons(1);
    of_pkt->match.length = htons(12);

    ip_t* ip = (ip_t *)(buffer + 42);
    ip->eth_head.proto = htons(ETHER_IP);
    ip->proto = 17; //UDP
    ip->src = htonl(src);
    ip->dest = 0x0a00000a;

    udp_t* udp = (udp_t*)(ip + 1);
    udp->sport = htons(43210);
    udp->dport = htons(43211);
}

static void initPktinArpReq(INT1* buffer, INT4 size, INT4 sockfd, UINT4 src)
{
    struct ofp13_packet_in* of_pkt = (struct ofp13_packet_in *)buffer;
    of_pkt->header.version = OFP13_VERSION;
    of_pkt->header.type = OFPT13_PACKET_IN;
    of_pkt->header.length = htons(size);
    of_pkt->header.xid = htonl(1);
    of_pkt->buffer_id = 0xffffffff;
    of_pkt->total_len = htons(size - 42);
    of_pkt->match.type = htons(1);
    of_pkt->match.length = htons(12);

    arp_t* arp = (arp_t *)(buffer + 42);
    arp->eth_head.proto = htons(ETHER_ARP);
    arp->opcode = htons(1);
    arp->sendip = htonl(src);
    arp->targetip = 0x0a00000a;
}


void CSwitchSimulator::test(void* param)
{
    sockfd_timer_t* sockfdTimer = (sockfd_timer_t*)param;
    if (NULL == sockfdTimer)
        return;

    if (sockfdTimer->sockfd <= 0)
        return;

    INT1 buffer[1500] = {0};
    INT4 size = 0;

    switch (g_proto)
    {
        case TEST_PROTO_HELLO:
            size = g_helloSize;
            break;
        case TEST_PROTO_PKTIN_IP:
            size = g_ipSize;
            break;
        case TEST_PROTO_PKTIN_ARP_REQ:
            size = g_arpReqSize;
            break;
        default:
            break;
    }

    UINT4 src = 1;

    while (1)
    {
        if (src > g_hostNumber)
            src = 1;

        switch (g_proto)
        {
            case TEST_PROTO_HELLO:
                initOfHeader(buffer, size, sockfdTimer->sockfd);
                break;
            case TEST_PROTO_PKTIN_IP:
                initPktinIp(buffer, size, sockfdTimer->sockfd, sockfdTimer->startIp*10000+src++);
                break;
            case TEST_PROTO_PKTIN_ARP_REQ:
                initPktinArpReq(buffer, size, sockfdTimer->sockfd, sockfdTimer->startIp*10000+src++);
                break;
            default:
                break;
        }

        if (!sendMsgOut(sockfdTimer->sockfd, buffer, size))
        {
            sockfdTimer->timer->stop();
            free(sockfdTimer);
            break;
        }

        usleep(g_sleep_us);
    }
}

CSwitchSimulator::CSwitchSimulator()
{
}

CSwitchSimulator::~CSwitchSimulator()
{
    STL_FOR_LOOP(m_sockfds, it)
    if (*it > 0)
        close(*it);
}

void CSwitchSimulator::start()
{
    printf("\nwill start simulating %d switch%s ...\n", g_switchNumber, (g_switchNumber>1)?"es":"");

    INT4 sockfd =-1;
    struct sockaddr_in servaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(g_ctrlPort);
    servaddr.sin_addr.s_addr = g_ctrlIp;
    
    for (INT4 i = 0; i < g_switchNumber; ++i)
    {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            printf("%s socket failed[%d] !", toString(), errno);
            return;
        }
        setNonBlocking(sockfd);

        INT4 sockopt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (char *)&sockopt, sizeof(sockopt));
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, (char *)&sockopt, sizeof(sockopt));

        connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr));

        m_sockfds.push_back(sockfd);

        m_timers.push_back(CTimer(THREAD_TYPE_PRIOR));

        sockfd_timer_t* param = (sockfd_timer_t*)malloc(sizeof(sockfd_timer_t));
        param->sockfd = m_sockfds[i];
        param->timer = &(m_timers[i]);
        param->startIp = g_hostStartIp++;
        m_timers[i].schedule(3, 0, CSwitchSimulator::test, param);

        printf("... %d started ...\n", i+1);
        sleep(5);
    }
}

