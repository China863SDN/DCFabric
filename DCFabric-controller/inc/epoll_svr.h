/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2015, BNC <ywxu@bnc.org.cn>
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
*   File Name   : epoll_svr.h           *
*   Author      : BNC Administrator           *
*   Create Date : 2017-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef EPOLL_SVR_H_
#define EPOLL_SVR_H_

#define EPOLL_SIZE 					65535
#define  EPOLLSVR_EVENTTYPENUM		5
#define  EPOLLEVENT_READ					0
#define	 EPOLLEVENT_WRITE					1
#define  EPOLLEVENT_ERROR					2

typedef INT4 (*epollsvr_handler_t)(INT4 iSockFd);


typedef struct epollsvr_sockfuncptr
{
	UINT4            uiMagicNum;
	INT4       		 iSockFd;
	epollsvr_handler_t epollsvr_handler_func[EPOLLSVR_EVENTTYPENUM];
	
}stEpollsvr_Sockfuncptr;

stEpollsvr_Sockfuncptr * register_EpollSock_Handle(epollsvr_handler_t func_Read ,
						epollsvr_handler_t func_Write,epollsvr_handler_t func_Error );

INT4 EpollSvr_Create(INT4 *iEpollFd);
INT4 EpollSvr_AddSockFd( INT4 iEpollFd, INT4 iSockFd, BOOL bIsEt, void *funptr );
void EpollSvr_DelSockFd( INT4 iEpollFd, INT4 iFd, BOOL bIsEt );
INT4 EpollSvr_Loop(INT4 iEpollFd, BOOL bRunCond);
#endif /* COMMON_H_ */

