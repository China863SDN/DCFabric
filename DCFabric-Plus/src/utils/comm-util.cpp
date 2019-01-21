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
*   File Name   : comm-util.cpp           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include "comm-util.h"
#include "log.h"
#include "bnc-error.h"

INT4 getTotalCpu()
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

INT4 setCpuAffinity(pthread_t tid, INT4 cpuId)
{
    if (cpuId >= getTotalCpu())
    {
        LOG_ERROR("invalid cpu id");
        return -1;
    }

    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpuId, &cpu_set);
    return pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpu_set);
}

INT4 clearCpuAffinity(pthread_t tid)
{
    cpu_set_t cpu_set;
    CPU_ZERO(&cpu_set);
    return pthread_setaffinity_np(tid, sizeof(cpu_set_t), &cpu_set);
}

INT4 maxThreadPriority(pthread_attr_t& attr)
{
    struct sched_param param;	

    pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED);      
    pthread_attr_setscope(&attr, PTHREAD_SCOPE_SYSTEM);
    pthread_attr_setschedpolicy(&attr, SCHED_FIFO);
    pthread_attr_getschedparam(&attr, &param);
    param.sched_priority = sched_get_priority_max(SCHED_FIFO);

    return pthread_attr_setschedparam(&attr, &param);        
}

INT4 setNonBlocking(INT4 fd)
{
    INT4 old_flag = fcntl(fd, F_GETFL);
    INT4 new_flag = old_flag | O_NONBLOCK;
    fcntl(fd, F_SETFL, new_flag);
    return old_flag;
}

INT4 addFd(INT4 epollfd, INT4 fd)
{
    setNonBlocking(fd);
    epoll_event event;
    event.data.fd = fd;
    event.events = EPOLLIN | EPOLLET | EPOLLERR | EPOLLHUP | EPOLLRDHUP;
    if (epoll_ctl(epollfd, EPOLL_CTL_ADD, fd, &event) < 0)
    {
        LOG_ERROR_FMT("epoll_ctl add failed[%d]!", errno);
        return BNC_ERR;
    }

    return BNC_OK;
}

void delFd(INT4 epollfd, INT4 fd)
{
    epoll_ctl(epollfd, EPOLL_CTL_DEL, fd, NULL);
}

UINT8 htonll(UINT8 n)
{
    return htonl(1) == 1 ? n : ((UINT8) htonl(n) << 32) | htonl(n >> 32);
}

UINT8 ntohll(UINT8 n)
{
    return htonl(1) == 1 ? n : ((UINT8) ntohl(n) << 32) | ntohl(n >> 32);
}

UINT4 ip2number(const INT1* ip)
{
   return inet_addr(ip);
}

INT1* number2ip(INT4 ip_num, INT1* ip)
{
   strcpy(ip, inet_ntoa(*(struct in_addr*)&ip_num));
   return ip;
}

UINT4 ipnumber2value(UINT4 ip_num)
{
	return (((ip_num>>24)&0xff) + ((ip_num&0xff)<<24) + ((ip_num &0xff00)<<8) + ((ip_num>>8)&0xff00)); 
}

char *mac2str(const UINT1 *hex, char *result)
{
    sprintf(result, "%02x:%02x:%02x:%02x:%02x:%02x",hex[0], hex[1],\
            hex[2],hex[3],hex[4],hex[5]);

    return result;
}

UINT1 *macstr2hex(const char *macstr, UINT1 *result)
{
    int i;
    char *s, *e;

    if ((macstr == NULL) || (result == NULL))
    {
        return result;
    }

    s = (char *) macstr;
    for (i = 0; i < 6; ++i)
    {
        result[i] = s ? strtoul (s, &e, 16) : 0;
        if (s)
           s = (*e) ? e + 1 : e;
    }

    return result;
}

static unsigned char SwitchChar(char chStr)  
{  
    if (chStr >= '0' && chStr <= '9')  
    {  
        return (chStr - '0');  
    }  
    else if (chStr >= 'A' && chStr <= 'f')  
    {  
        return (chStr - 'A' + 10);  
    }  
    else if (chStr >= 'a' && chStr <= 'f')  
    {  
        return (chStr - 'a' + 10);  
    }  
    else  
    {  
        return 0;  
    }  
}  

UINT1 *strmac2hex(const char *macstr, UINT1 *result)
{
	int  nTotal = 0;  
    if ((macstr == NULL) || (result == NULL))
    {
        return result;
    }

	char *pchStr = strtok((char*)macstr, ":");  

	   
	while (NULL != pchStr)	
	{  
		result[nTotal++] = (SwitchChar(*pchStr) << 4) | SwitchChar(*(pchStr+1));  
		pchStr = strtok(NULL, ":");  
	}  

    return result;
}

void split(const std::string& str, const std::string& separator, std::list<std::string>& out)
{
    if (str.empty())
    {
        return;
    }
    
    std::size_t pos = 0;
    std::size_t cur = 0;
    std::size_t length = str.size();
    while (std::string::npos != (cur = str.find(separator, pos)))
    {
        out.push_back(str.substr(pos, cur - pos));
        pos = cur + 1;
    }

    out.push_back(str.substr(pos, length - pos));
}

BOOL sendMsgOut(INT4 fd, const INT1* data, INT4 length)
{
    try
    {
        INT4 writeLen = 0;

        if (length > 0)
        {
            INT4 ret = 0;

            while (writeLen < length)
            {
                ret = write(fd, data+writeLen, length-writeLen);
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
                    //LOG_WARN_FMT("sendMsgOut: sockfd[%d], length[%d], sent[%d], errno[%d]", 
                     //   fd, length, writeLen, errno);
                    break;
                }
                writeLen += ret;
            }
        }

        return ((writeLen < length) ? FALSE : TRUE);
    }
    catch (...)
    {
        LOG_ERROR("Catch write exception");
        return FALSE;
    }
}
int dpid_str_to_bin( char *str, unsigned char *dpid)
{
    int i;
    char *s, *e;

    if ((NULL == dpid) || (NULL == str))
    {
        return -1;
    }

    s = (char *) str;
    for (i = 0; i < 8; ++i)
    {
        dpid[i] = s ? strtoul (s, &e, 16) : 0;
        if (s)
           s = (*e) ? e + 1 : e;
    }
    return 0;
}
//unsigned char [8] ---->unsigned long long int
UINT8 uc8_to_ulli64(const unsigned char *dpid_str, UINT8 *dpid)
{
    unsigned long long int dpid_tmp = 0;
    UINT4 idx = 0;
    while(idx < 8)
    {
        dpid_tmp <<= 8;
        dpid_tmp |= (*(dpid_str + idx++) & 0x00000000000000ff);
    }

    *dpid = dpid_tmp;
    return dpid_tmp;
}
//unsigned long long int ---->unsigned char [8]
UINT1 *ulli64_to_uc8(UINT8 dpid, unsigned char *DpidStr)
{
    DpidStr[0]= (unsigned char)((dpid>>56) & 0x00000000000000ff);
    DpidStr[1]= (unsigned char)((dpid>>48) & 0x00000000000000ff);
    DpidStr[2]= (unsigned char)((dpid>>40) & 0x00000000000000ff);
    DpidStr[3]= (unsigned char)((dpid>>32) & 0x00000000000000ff);
    DpidStr[4]= (unsigned char)((dpid>>24) & 0x00000000000000ff);
    DpidStr[5]= (unsigned char)((dpid>>16) & 0x00000000000000ff);
    DpidStr[6]= (unsigned char)((dpid>> 8) & 0x00000000000000ff);
    DpidStr[7]= (unsigned char)((dpid>> 0) & 0x00000000000000ff);

    return DpidStr;
}

INT1* dpidUint8ToStr(UINT8 ori_dpid, INT1* str)
{
	UINT1 dpid[8];
	uint8ToStr(ori_dpid, dpid);
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0], dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
    return str;
}

UINT1 *uint8ToStr(UINT8 value, unsigned char *str)
{
    str[0]= (unsigned char)((value>>56) & 0x00000000000000ff);
    str[1]= (unsigned char)((value>>48) & 0x00000000000000ff);
    str[2]= (unsigned char)((value>>40) & 0x00000000000000ff);
    str[3]= (unsigned char)((value>>32) & 0x00000000000000ff);
    str[4]= (unsigned char)((value>>24) & 0x00000000000000ff);
    str[5]= (unsigned char)((value>>16) & 0x00000000000000ff);
    str[6]= (unsigned char)((value>> 8) & 0x00000000000000ff);
    str[7]= (unsigned char)((value>> 0) & 0x00000000000000ff);

    return str;
}

INT4 dpidStr2Uint8(const INT1 *dpid, UINT8 *ret)
{
    if (NULL == dpid)
    {
        return -1;
    }

    INT4 length = strlen(dpid);
    if (length < 23)
    {
        return -1;
    }

    INT4 i = 0;
    INT4 j = 0;
    UINT1 tmp[17] = {0};
    for (; i <23 && j < 16; i++)
    {
        if (':' != dpid[i])
        {
            if (!((dpid[i] <= '9' && dpid[i] >= '0')
                || (dpid[i] <= 'f' && dpid[i] >= 'a')
                || (dpid[i] <= 'F' && dpid[i] >= 'A')))
            {
                return -1;
            }
            tmp[j] = dpid[i];
            j++;
        }
    }

    *ret = strtoul((const char*)tmp, NULL, 16);

    return 0;
}

void strAppend(std::string element, std::string & str)
{
	str.append(element + "\n");
}

UINT4 cidr_to_subnet_mask(UINT4 num)
{
	UINT4 mask = 0;

	if (32 >= num) {
	    int bits = sizeof(UINT4) * 8;

	    mask = ~0;
	    bits -= num;
	    mask <<= bits;
	}

    return htonl(mask);
}

INT4 getControllerMac(const INT1* itf, UINT1* mac)
{
    if ((itf == NULL) || (mac == NULL) || (*itf == '\0'))
    {
        return BNC_ERR;
    }

	//linux netlink
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return BNC_ERR;
    }

	//linux net card struct
    struct ifreq ifr;    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, itf);

	//get netcard interface mac address
    int ret = ioctl(sock, SIOCGIFHWADDR, &ifr, sizeof(ifr));
    if (ret == 0)
    {
        memcpy(mac, ifr.ifr_hwaddr.sa_data, 6);
    }

    close(sock);
    return (ret == 0) ? BNC_OK : BNC_ERR;
}

INT4 getControllerIp(const INT1* itf, UINT4* ip)
{
    if ((itf == NULL) || (ip == NULL) || (*itf == '\0'))
    {
        return BNC_ERR;
    }

	//linux netlink
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0)
    {
        return BNC_ERR;
    }

	//linux net card struct
    struct ifreq ifr;    
    memset(&ifr, 0, sizeof(ifr));
    strcpy(ifr.ifr_name, itf);

	//get netcard interface ip address
    int ret = ioctl(sock, SIOCGIFADDR, &ifr, sizeof(ifr));
    if (ret == 0)
    {
        struct sockaddr_in* in = (struct sockaddr_in*)&ifr.ifr_addr;
        *ip = ntohl(*(UINT4*)&in->sin_addr);
    }

    close(sock);
    return (ret == 0) ? BNC_OK : BNC_ERR;
}

INT4 getPeerMac(INT4 sockfd, const struct sockaddr_in& peer, const INT1* itf, INT1* mac)
{
    if ((-1 == sockfd) || (NULL == itf) || (NULL == mac))
    {
        return BNC_ERR;
    }

    struct arpreq arp; 
    memset(&arp, 0, sizeof(struct arpreq)); 
    memcpy((struct sockaddr_in*)&arp.arp_pa, &peer, sizeof(struct sockaddr_in)); 
    strcpy(arp.arp_dev, itf); 
    arp.arp_ha.sa_family = AF_UNSPEC; 

    int ret = ioctl(sockfd, SIOCGARP, &arp);
    if (ret == 0)
    {
        memcpy(mac, (INT1*)arp.arp_ha.sa_data, 6);
    }

    return (ret == 0) ? BNC_OK : BNC_ERR;
}

std::string to_string(UINT8 num)
{
    char str[50] = {0};
    snprintf(str, 50, "%llx", num);
    return std::string(str);
}

static void randInit()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0)
        srand(tv.tv_sec ^ tv.tv_usec);
}

static void randBytes(void* _p, size_t n)
{
    uint8_t* p = (uint8_t*)_p;
    randInit();
    while (n--) {
        *p++ = rand();
    }
}

uint32_t randUint32()
{
#ifndef UINT32_MAX
#define UINT32_MAX ((uint32_t) 4294967295)
#endif
#ifndef INT32_MAX
#define INT32_MAX ((int32_t) 2147483647)
#endif

    if (RAND_MAX >= UINT32_MAX)
    {
        randInit();
        return rand();
    }
    else if (RAND_MAX == INT32_MAX)
    {
        randInit();
        return rand() | ((rand() & 1u) << 31);
    }
    else
    {
        uint32_t x;
        randBytes(&x, sizeof(x));
        return x;
    }
}

BOOL isValidMac(const INT1* mac)
{
    INT1 invalidMac[MAC_LEN] = {0};
    return ((NULL != mac) && (memcmp(invalidMac, mac, MAC_LEN) != 0));
}

UINT4 hashUint32(const UINT4& key)
{
    UINT4 hash = key;

    hash += ~(hash << 15);
    hash ^=  (hash >> 10);
    hash +=  (hash << 3);
    hash ^=  (hash >> 6);
    hash += ~(hash << 11);
    hash ^=  (hash >> 16);

    return hash;
}

UINT4 hashUint64(const UINT8& key)
{
    UINT8 hash = key;

    hash += ~(hash << 32);
    hash ^=  (hash >> 22);
    hash += ~(hash << 13);
    hash ^=  (hash >> 8);
    hash +=  (hash << 3);
    hash ^=  (hash >> 15);
    hash += ~(hash << 27);
    hash ^=  (hash >> 31);

    return (UINT4)hash;
}

UINT4 hashString(const std::string& key)
{
    const INT1* str = key.c_str();
    UINT4 len = key.size();
    UINT4 i, j = len & ~3;
    UINT4 hash = 0;
    
    for (i = 0; i < j; i += 4)
    {
        hash += (hash << 3) + str[i];
        hash += (hash << 3) + str[i + 1];
        hash += (hash << 3) + str[i + 2];
        hash += (hash << 3) + str[i + 3];
    }
    
    for (; i < len; i++)
        hash += (hash << 3) + str[i];
    
    return hash;
}

// calculate checksum
UINT2 calc_ip_checksum(UINT2 *buffer, UINT4 size)
{
	UINT4 cksum = 0;
	while(size >1)
	{
		cksum += *buffer++;
		size -= sizeof(UINT2);
	}
	if(size )
	{
		cksum += *(UINT1*)buffer;
	}

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >>16);
	return (UINT2)(~cksum);
}

BOOL isDigitalString(const INT1* str)
{
    for (UINT4 pos = 0; str[pos] != '\0'; pos++) 
    	if ((str[pos] < '0') || (str[pos] > '9')) 
            return FALSE;

    return TRUE;
}

UINT8 string2Uint8(const INT1* str)
{
    return strtoull(str, 0, 10); //10 base
}

UINT4 string2Uint4(const INT1* str)
{
    return strtoul(str, 0, 10); //10 base
}

char* inet_htoa(UINT4 ip)
{
    struct in_addr in = {0};
    in.s_addr = htonl(ip);
    return inet_ntoa(in);
}

INT4 cidr_str_to_ip_prefix(const char* ori_cidr, UINT4* ip, UINT4* mask)
{
	char cidr[48] = {0};
	memcpy(cidr, ori_cidr, 48);
	
	char* token = NULL;
	char* buf = cidr; 
	INT4 count = 0;

	while((token = strsep(&buf , "/")) != NULL)
	{
		if (0 == count) 
		{
			// printf("cidr ip: %s\n", token);
			*ip = ip2number(token);
			// printf("%u\n", cidr_ip);
		}
		else 
		{
			// printf("mask is: %s\n", token);
			*mask = (0 == strcmp("0", token)) ? 0:atoll(token);
			// printf("%u\n", cidr_mask);
		}
		count++;
	}

	return *ip;
}

