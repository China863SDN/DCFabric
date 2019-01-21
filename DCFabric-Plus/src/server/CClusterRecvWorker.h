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
*   File Name   : CClusterRecvWorker.h                                        *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CCLUSTERRECVWORKER__H
#define __CCLUSTERRECVWORKER__H

#include "CRecvWorker.h"
#include "CLoopBuffer.h"
#include "CSmartPtr.h"

class CClusterRecvWorker : public CRecvWorker
{
public:
    CClusterRecvWorker();
    virtual ~CClusterRecvWorker();

    virtual INT4 process(INT4 sockfd, INT1* buffer, UINT4 len);
    virtual void processPeerDisconn(INT4 sockfd);

    virtual const char* toString() {return "CClusterRecvWorker";}

public:
    INT4 processMsg(INT4 sockfd, INT1* buffer);

private:
    INT4  m_size;
    INT1* m_buffer;
    CSmartPtr<CLoopBuffer> m_loopBuffer;
};

#endif
