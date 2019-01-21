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
*   File Name   : comm-util.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef __COMMON_UTIL_H
#define __COMMON_UTIL_H

#include "bnc-type.h"

#define ALIGN_8(length) (((length) + 7) & ~7)

// 宏, 用于容器的使用简化, 代替了原来的定义和for循环
#define STL_FOR_LOOP(container, it) \
	for(typeof((container).begin()) it = (container).begin(); it != (container).end(); ++it)

#define STL_FOR_MAP(container, it) \
		for(typeof((container).begin()) it = (container).begin(); it != (container).end(); )


// 宏, 用于容器的使用简化, 代替了原来的判断是否存在
#define STL_EXIST(container, value)	\
	((container).find(value) != (container).end())


/*
 * 获取cpu总数
 */
INT4 getTotalCpu();

/*
 * 绑定线程到某CPU
 */
INT4 setCpuAffinity(pthread_t tid, INT4 cpuId);

/*
 * 解绑线程与CPU的绑定
 */
INT4 clearCpuAffinity(pthread_t tid);

/*
 * 将线程优先级设置最大
 */
INT4 maxThreadPriority(pthread_attr_t& attr);

/*
 * 设置无阻塞
 * 为epoll使用
 */
INT4 setNonBlocking(INT4 fd);

/*
 * 增加fd
 * 为epoll使用
 */
INT4 addFd(INT4 epollfd, INT4 fd);

/*
 * 删除fd
 * 为epoll使用
 */
void delFd(INT4 epollfd, INT4 fd);

/*
 * UINT8的主机序转网络序
 */
UINT8 htonll(UINT8 n);

/*
 * UINT8的网络序转主机序
 */
UINT8 ntohll(UINT8 n);

/*
 * ip字符串转UINT4
 */
UINT4 ip2number(const INT1* ip);

/*
 * ip UINT4转字符串
 */
INT1* number2ip(INT4 ip_num, INT1* ip);

/*
 * ip UINT4转计算值
 */
UINT4 ipnumber2value(UINT4 ip_num);

/*
 * mac str转换
 */
char *mac2str(const UINT1 *hex, char *result);

/*
 * mac str转换
 */
UINT1 *macstr2hex(const char *macstr, UINT1 *result);

UINT1 *strmac2hex(const char *macstr, UINT1 *result);

/*
 * 字符串分割函数
 * @param str: 			原始字符串
 * @param seperator: 	分隔符
 * @param out:			用来存储新字符串的列表
 */
void split(const std::string& str, const std::string& separator, std::list<std::string>& out);

/*
 * 发送消息
 */
BOOL sendMsgOut(INT4 fd, const INT1* data, INT4 len);

/*
*str to unsigned char
*/
int dpid_str_to_bin( char *str, unsigned char *dpid);

/*
*str to UINT8
*/
UINT8 uc8_to_ulli64(const unsigned char *dpid_str, UINT8 *dpid);

UINT1 *ulli64_to_uc8(UINT8 dpid, unsigned char *DpidStr);

/*
 * UINT8转字符串
 */
UINT1 *uint8ToStr(UINT8 value, unsigned char *str);

/*
 * 字符串拼接
 * 将element添加到str中, 并且换行
 * 主要为for_each使用
 */
void strAppend(std::string element, std::string & str);

/*
 * 获得CIDR格式IP地址的子网掩码
 */
UINT4 cidr_to_subnet_mask(UINT4 num);

/*
 * 字符串转换成UINT8
 */
INT4 dpidStr2Uint8(const INT1 *dpid, UINT8 *ret);

/*
 * 把dpid转换成str
 */
INT1* dpidUint8ToStr(UINT8 ori_dpid, INT1* str);

/*
 * 根据配置的interface获取controller的MAC
 */
INT4 getControllerMac(const INT1* itf, UINT1* mac);

/*
 * 根据配置的interface获取controller的IP
 */
INT4 getControllerIp(const INT1* itf, UINT4* ip);

/*
 * 根据socket获取对端的MAC
 */
INT4 getPeerMac(INT4 sockfd, const struct sockaddr_in& peer, const INT1* itf, INT1* mac);

std::string to_string(UINT8 num);

uint32_t randUint32();

BOOL isValidMac(const INT1* mac);

UINT4 hashUint32(const UINT4& key);

UINT4 hashUint64(const UINT8& key);

UINT4 hashString(const std::string& key);

UINT2 calc_ip_checksum(UINT2 *buffer, UINT4 size);

BOOL isDigitalString(const INT1* str);

UINT8 string2Uint8(const INT1* str);

UINT4 string2Uint4(const INT1* str);

char* inet_htoa(UINT4 ip);

INT4 cidr_str_to_ip_prefix(const char* ori_cidr, UINT4* ip, UINT4* mask);

#endif
