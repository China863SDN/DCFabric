/*
 * BNC SDN Controller GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
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
 *   File Name   : CLog.h                                                      *
 *   Author      : bnc bojiang                                                 *
 *   Create Date : 2017-10-30                                                  *
 *   Version     : 1.0                                                         *
 *   Function    :                                                             *
 *                                                                             *
 ******************************************************************************/
#ifndef _CLOG_H_
#define _CLOG_H_

#include "bnc-type.h"
#include "CLoopBuffer.h"
#include "CSemaphore.h"
#include "CThread.h"

extern INT4 g_logLevel;

class CLog
{
public:
    static CLog* getInstance();

    ~CLog();

    INT4 init();

    INT4 write(const INT1* data, UINT4 len);
    INT4 waitWriteFile();
    void writeFile();

private:
    CLog();

    std::string createFile();

private:
    static CLog* m_instance;       

    std::string m_file;
    CLoopBuffer m_buffer;
    CThread     m_thread;
    CSemaphore  m_sem;
};

#endif /* LOG_H_ */

