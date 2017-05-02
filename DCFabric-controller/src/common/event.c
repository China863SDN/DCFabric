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
#include "common.h"
#include "event.h"
#include "gnflush-types.h"

stEvent g_stEventTbl[EVENT_NUM] = {0};

UINT4  Create_Event(UINT8 EventNo)
{
	INT4 iRet = GN_OK;
	if(EventNo >= EVENT_NUM) return GN_ERR;
	if(g_stEventTbl[EventNo].bUsedFlag)
		return GN_ERR;
	iRet = sem_init(&g_stEventTbl[EventNo].semEvent,0,0);
	g_stEventTbl[EventNo].bUsedFlag = TRUE;
	
	return iRet;
}

UINT4  Write_Event(UINT8 EventNo)
{
	INT4 iRet = GN_OK;
	if(EventNo >= EVENT_NUM) return GN_ERR;
	if(!g_stEventTbl[EventNo].bUsedFlag)
		return GN_ERR;
	iRet = sem_post(&g_stEventTbl[EventNo].semEvent);
	return iRet;
}
//block wait
UINT4  Read_Event(UINT8 EventNo )
{
	INT4 iRet = GN_OK;
	if(EventNo >= EVENT_NUM) return GN_ERR;
	if(!g_stEventTbl[EventNo].bUsedFlag)
		return GN_ERR;
	iRet = sem_wait(&g_stEventTbl[EventNo].semEvent);
	return iRet;
}

UINT4 Destory_Event(UINT8 EventNo)
{
	INT4 iRet = GN_OK;
	if(EventNo >= EVENT_NUM) return GN_ERR;
	if(!g_stEventTbl[EventNo].bUsedFlag)
		return GN_ERR;
	iRet = sem_destroy(&g_stEventTbl[EventNo].semEvent);
	if(iRet != GN_OK)
	{
		return GN_ERR;
	}
	g_stEventTbl[EventNo].bUsedFlag = FALSE;
	
	return GN_OK;
}