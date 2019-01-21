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
*   File Name   : bnc-type.h           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#ifndef __BNC_TYPE_H
#define __BNC_TYPE_H

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
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <sys/mman.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <sys/resource.h>
#include <netinet/in_systm.h>
#include <netinet/tcp.h>
#include <net/ethernet.h>
#include <linux/if_packet.h>
#include <linux/if_ether.h>
#include <linux/if_arp.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/wait.h>
#include <sys/syscall.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <signal.h>
#include <semaphore.h>
#include <limits.h>
#include <string.h>
#include <pthread.h>
#include <sched.h>
#include <sys/epoll.h>

#include <string>
#include <map>
#include <list>
#include <queue>
#include <iterator>
#include <iostream>
#include <fstream>
#include <vector>
#include <exception>
#include <set>
#include <stack>
#include <memory>

typedef bool                   BOOL;
typedef char                   INT1;
typedef short                  INT2;
typedef int                    INT4;
typedef unsigned char          UINT1;
typedef unsigned short         UINT2;
typedef unsigned int           UINT4;
typedef unsigned int           UINT;
typedef double                 DOUBLE;
typedef float                  FLOAT;

#ifdef X86_64
typedef long                   INT8;
typedef unsigned long          UINT8;
#else
typedef long long int          INT8;
typedef unsigned long long int UINT8;
#endif

#define TRUE 1
#define FALSE 0
#define MAC_LEN 6
#define IPV6_LEN 16

#define FN __FUNCTION__
#define LN __LINE__

#define bnc_malloc(X) 		malloc(X)
#define bnc_free(X)			free(X)

#endif
