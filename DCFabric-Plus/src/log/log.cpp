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
*   File Name   : log.cpp           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "log.h"
#include "configurator.h"
#include "fileappender.h"  
#include "layout.h"

log4cplus::Logger logger;

void initLogger()
{    
    //log4cplus::PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT("../conf/log4cplus.properties"));
    logger = log4cplus::Logger::getRoot();
}

const INT1* fileName(const INT1* file)
{
    const INT1* ptr = strrchr(file, '/');
    return (NULL != ptr) ? (ptr + 1) : file;
}

