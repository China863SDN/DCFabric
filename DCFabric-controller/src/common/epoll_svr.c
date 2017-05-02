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
*   File Name   : epoll_svr.c           *
*   Author      : BNC Administrator           *
*   Create Date : 2017-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "common.h"
#include "event.h"
#include "gnflush-types.h"
#include "epoll_svr.h"

//注册四种事件对应函数
stEpollsvr_Sockfuncptr * register_EpollSock_Handle(epollsvr_handler_t func_Read ,
						epollsvr_handler_t func_Write,epollsvr_handler_t func_Error )
{
	stEpollsvr_Sockfuncptr *pst_handlefuncptr = NULL ;
	pst_handlefuncptr = gn_malloc(sizeof(stEpollsvr_Sockfuncptr));
	if(NULL == pst_handlefuncptr)
	{
		return NULL;
	}
	pst_handlefuncptr->epollsvr_handler_func[0] = func_Read;
	pst_handlefuncptr->epollsvr_handler_func[1] = func_Write;
	pst_handlefuncptr->epollsvr_handler_func[2] = func_Error;
	return pst_handlefuncptr;
}
/*************************************************************************
typedef union epoll_data {  
    void *ptr;  
    int fd;  
    __uint32_t u32;  
    __uint64_t u64;  
} epoll_data_t;  
 //感兴趣的事件和被触发的事件  
struct epoll_event {  
    __uint32_t events; // Epoll events  
    epoll_data_t data; //User data variable  
};  
//添加句柄，设置事件触发，并挂载指针 epoll_event 进行维护sockfd和func
//区分socket句柄和pipe管道句柄
*************************************************************************/
INT4 EpollSvr_AddSockFd( INT4 iEpollFd, INT4 iSockFd, BOOL bIsEt, void *funptr )
{
    struct epoll_event Ev;
	stEpollsvr_Sockfuncptr *pst_handlefuncptr = NULL ;
	
	if((iEpollFd <0)||(iSockFd < 0) || (NULL == funptr))
	{
		return GN_ERR;
	}
	
    //Ev.data.fd = iSockFd;
    Ev.events = EPOLLIN| EPOLLOUT|EPOLLRDHUP | EPOLLHUP |EPOLLERR ;
    if( bIsEt )
	{
        Ev.events = EPOLLIN | EPOLLOUT |EPOLLET|EPOLLRDHUP | EPOLLHUP | EPOLLERR ; //add error  ET
	}
	//Ev.data.u32 = bIsSock;
	//Ev.data.u64 = bIsListen;
	pst_handlefuncptr = (stEpollsvr_Sockfuncptr *)funptr;
	pst_handlefuncptr->iSockFd = iSockFd;
	pst_handlefuncptr->uiMagicNum = 0xABABCDCD;
	Ev.data.ptr = funptr;

	
	fcntl(iSockFd, F_SETFL, fcntl(iSockFd, F_GETFD, 0)| O_NONBLOCK);
    epoll_ctl(iEpollFd, EPOLL_CTL_ADD, iSockFd, &Ev);

    //LOG_PROC("Info","sock %d added to Epoll tool",fd);

	return GN_OK;
}

//删除句柄，释放内存funcptr
void EpollSvr_DelSockFd( INT4 iEpollFd, INT4 iFd, BOOL bIsEt )
{
	struct epoll_event Ev;
	Ev.data.fd = iFd;
	Ev.events = EPOLLIN| EPOLLOUT| EPOLLERR ;
	if( bIsEt )
	{
		Ev.events = EPOLLIN | EPOLLOUT |EPOLLET | EPOLLERR; //add error	ET
	}
	epoll_ctl(iEpollFd, EPOLL_CTL_DEL, iFd, &Ev);

	//LOG_PROC("Info","sock %d del from Epoll tool",fd);
}

//创建epoll server, 返回全局句柄
INT4 EpollSvr_Create(INT4 *iEpollFd)
{
	INT4 iEpfd =0;
	
	iEpfd = epoll_create(EPOLL_SIZE);    
	if(iEpfd < 0)	
	{	   	
		 LOG_PROC("ERROR", "epoll_create() error!");		
		 return GN_ERR;	
	}
	*iEpollFd = iEpfd;
	return GN_OK;
}

//主体,循环epoll wait检测触发事件
INT4 EpollSvr_Loop(INT4 iEpollFd, BOOL bRunCond)
{
	INT4   i = 0;
	INT4   iErrno = 0;
	INT4   iEpoll_Event_Num = 0;
	struct epoll_event EpollEvents[EPOLL_SIZE] ; 
	stEpollsvr_Sockfuncptr *pst_handlefuncptr = NULL ;

	if((iEpollFd < 0)||(!bRunCond))	
	{	   	
		 LOG_PROC("ERROR", "iEpollFd error! %s",FN);		
		 return GN_ERR;	
	}
	while(bRunCond)
	{
		iEpoll_Event_Num = epoll_wait(iEpollFd, EpollEvents, EPOLL_SIZE, -1);//0 --- return 1---infinite
        if(iEpoll_Event_Num <= 0) 
        {
            iErrno = errno;
            if(EINTR != iErrno)
            {
                LOG_PROC("ERROR","epoll_wait() fail! Err: %d",iErrno);
            }

            continue;
        }
	
	    for(i = 0; i < iEpoll_Event_Num; i++)
		{
			
			pst_handlefuncptr = (stEpollsvr_Sockfuncptr *)EpollEvents[i].data.ptr;

			//if(0xABABCDCD == pst_handlefuncptr->uiMagicNum)
			{
				//LOG_PROC("INFO", "EpollEvents[i].events=0x%x EpollEvents[%d].data.fd=%d",i,EpollEvents[i].events,i,pst_handlefuncptr->iSockFd);
				if(EpollEvents[i].events & ( EPOLLERR | EPOLLRDHUP | EPOLLHUP ))
				{
					if(NULL != pst_handlefuncptr->epollsvr_handler_func[2])
							pst_handlefuncptr->epollsvr_handler_func[2](pst_handlefuncptr->iSockFd);
					continue;
				}
				if(EpollEvents[i].events & EPOLLIN)
				{

					if(NULL != pst_handlefuncptr->epollsvr_handler_func[0])
							pst_handlefuncptr->epollsvr_handler_func[0](pst_handlefuncptr->iSockFd);
				}
				if(EpollEvents[i].events & EPOLLOUT)
				{
					if(NULL != pst_handlefuncptr->epollsvr_handler_func[1])
							pst_handlefuncptr->epollsvr_handler_func[1](pst_handlefuncptr->iSockFd);
				}
				
			}
				
		}
	
	}
	return GN_OK;	
}

