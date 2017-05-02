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
#ifndef EVENT_H_
#define EVENT_H_

#define EVENT_NUM   		255

typedef struct Event_Struct
{
	sem_t  semEvent;

	BOOL   bUsedFlag;
	
}stEvent;
UINT4  Create_Event(UINT8 EventNo);
UINT4  Write_Event(UINT8 EventNo);
UINT4  Read_Event(UINT8 EventNo );
UINT4  Destory_Event(UINT8 EventNo);

#endif
