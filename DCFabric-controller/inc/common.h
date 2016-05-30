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
*   File Name   : common.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef COMMON_H_
#define COMMON_H_


#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <malloc.h>
#include <unistd.h>
#include <errno.h>
#include <sys/timeb.h>
#include <fcntl.h>
#include <dirent.h>
#include <sys/stat.h>
#include <assert.h>
#include <setjmp.h>

#include <sys/procfs.h>
#include <sys/fcntl.h>
//#include <stropts.h>

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>

#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
//#include <net/if.h>
#include <netdb.h>

#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/resource.h>

#include <netinet/in_systm.h>
#include <netinet/tcp.h>
//#include <net/bpf.h>
#include <net/ethernet.h>

#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>

#ifndef __USE_GNU
#define __USE_GNU
#endif


#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/sem.h>
#include <sys/ipc.h>
//#include <syslog.h>
#include <signal.h>
#include <semaphore.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
//#include <libnet.h>

typedef signed   char     BOOL;
typedef char              INT1;
typedef short             INT2;
typedef int               INT4;
typedef unsigned char     UINT1;
typedef unsigned short    UINT2;
typedef unsigned int      UINT4;
typedef unsigned int      UINT;

#ifdef X86_64
typedef long              INT8;
typedef unsigned long     UINT8;
#else
typedef long long int     INT8;
typedef unsigned long long int UINT8;
#endif

#include "gn_inet.h"

#define TRUE 1
#define FALSE 0

#define FN __FUNCTION__

#ifndef NULL
#define NULL 0
#endif

#define CONFIGURE_FILE "./config/sdn-controller.conf"

#define ALIGN_8(len) (((len) + 7) & ~7)

#define LIST_ADD_FRONT(list, element)                   \
{                                                       \
    element->next = list;                               \
    list = element;                                     \
}

#define LIST_ADD_BACK(list, element_type, element)      \
{                                                       \
    element_type *p_element = list;                     \
    if(NULL == list)                                    \
    {                                                   \
        list = element;                                 \
    }                                                   \
    else                                                \
    {                                                   \
        while(p_element->next)                          \
        {                                               \
            p_element = p_element->next;                \
        }                                               \
                                                        \
        p_element->next = element;                      \
        list = element;                                 \
    }                                                   \
}

#define LIST_FREE(list, element_type, element_free_func)\
{                                                       \
    element_type *p_element = list;                     \
    while(p_element)                                    \
    {                                                   \
        list = p_element->next;                         \
        element_free_func(p_element);                   \
        p_element = list;                               \
    }                                                   \
}


// 1
#define LOG_PROC(log_level, format, arguments...) \
    printf("[%s] ", log_level);    \
    printf(format, ##arguments);    \
    printf("\n");
/*
// 2
UINT1 g_log_level;   //设置日志显示级别
#define GN_LOG(log_level, format, arguments...) \
{                                       \
    if(log_level >= g_log_level)        \
    {                                   \
        switch(log_level)               \
        {                               \
            case LOG_DEBUG:             \
                printf("[DEBUG] ");     \
                break;                  \
            case LOG_INFO:              \
                printf("[INFO] ");      \
                break;                  \
            case LOG_WARNING:           \
                printf("[WARNING] ");   \
                break;                  \
            case LOG_ERROR:             \
                printf("[ERROR] ");     \
                break;                  \
            case LOG_FATAL:             \
                printf("[FATAL] ");     \
                break;                  \
            default:                    \
                printf("[WARNING] ");   \
                break;                  \
        }                               \
        printf(format, ##arguments);    \
        printf("\n");                   \
    }                                   \
}

// 3
#define LOG_INFO(log_level, format, arguments...) \
    printf("[INFO] ");    \
    printf(format, ##arguments);    \
    printf("\n");

#define LOG_DEBUG(log_level, format, arguments...) \
    printf("[DEBUG] ");    \
    printf(format, ##arguments);    \
    printf("\n");

#define LOG_WARNING(log_level, format, arguments...) \
    printf("[WARNING] ");    \
    printf(format, ##arguments);    \
    printf("\n");

#define LOG_ERROR(log_level, format, arguments...) \
    printf("[ERROR] ");    \
    printf(format, ##arguments);    \
    printf("\n");

enum LOG_LEVEL
{
    LOG_DEBUG = 0,
    LOG_INFO = 1,
    LOG_WARNING = 2,
    LOG_ERROR = 3,
    LOG_FATAL = 4
};
*/
#pragma pack(1)
typedef struct key_value
{
    INT1 *key;
    INT1 *value;
}key_value_t;

typedef struct net_mask
{
    UINT4 ip;       //网络字节序
    UINT4 prefix;
    UINT4 minip;
    UINT4 maxip;
}net_mask_t;
#pragma pack()

INT1* strcut(INT1 *org, INT1 delim, INT1 **next);

//malloc and memset
void* gn_malloc(size_t size);

//free and set value to null
void gn_free(void **ptr);

UINT8 gn_htonll(UINT8 n);

UINT8 gn_ntohll(UINT8 n);

UINT4 random_uint32();

//convert the unsigned long long int to unsigned char array
UINT1 *ulli64_to_uc8(UINT8 dpid, unsigned char *DpidStr);

//converts the unsigned char array to convert unsigned long long int
UINT8 uc8_to_ulli64(const unsigned char *dpid_str, UINT8 *dpid);

//convert the mac string to unsigned char array
INT4 mac_str_to_bin( char *str, unsigned char *mac);

//calc hash index
UINT4 mac_hash(UINT1 *mac, UINT4 size);

//converts the IPv4 host address addr to a string
INT1 *inet_htoa(UINT4 ip);

//masked ip address to ip and mask
void masked_ip_parser(INT1 *masked_ip, net_mask_t *net_mask);

//converts the unsigned char array to mac string
INT1 *mac2str(UINT1 *hex, INT1 *result);

//converts the mac string to unsigned char array
UINT1 *macstr2hex(char *macstr, UINT1 *result);

//get the host cpu counts
INT4 get_total_cpu();

//set the cpu affinity of the current thread
INT4 set_cpu_affinity(UINT4 cpu_id);

// convert ip to number
UINT4 ip2number(const INT1* ip);

// convert number to ip
INT1* number2ip(INT4 ip_num, INT1* ip);

// convert ipv6 to number
UINT1* ipv6_str_to_number(char* str, UINT1* ipv6);

// show ipv4 log
void nat_show_ip(UINT4 ip);

// show mac address log
void nat_show_mac(UINT1* mac);

// show ipv6 log
void nat_show_ipv6(UINT1* ip);

//convert str dpid to uint8
INT4 dpidStr2Uint8(const INT1 *dpid, UINT8 *ret);
BOOL is_digit(const INT1 *str, INT4 base);

/*
 * 计算校验和
 */
UINT2 calc_ip_checksum(UINT2 *buffer, UINT4 size);

UINT2 calc_tcp_checksum(ip_t* p_ip, tcp_t* p_tcp);

INT1* dpidUint8ToStr(UINT8 dpid, INT1* str);

/*
 * 定义了几个比较函数:
 * 如果双方均为空,返回1;均不为空且内容一致,返回1;
 * 其他情况返回0
 */
INT4 compare_str(char* str1, char* str2);

INT4 compare_array(void* str1, void* str2, INT4 num);

INT4 compare_pointer(void* ptr1, void* ptr2);

/*
 * 输入CIDR 字符串 , 转换为IP 和位数的组合
 * 比如:输入192.168.53.0/24 转换为192.168.53.0 和24
*/

INT4 cidr_str_to_ip_prefix(char* ori_cidr, UINT4* ip, UINT4* mask);
/*
 * 输入不小于32的数字,输出对应的子网掩码
 * 比如:输入24, 输出 FFFFFFF00
*/
UINT4 cidr_to_subnet_mask(UINT4 num);
#endif /* COMMON_H_ */
