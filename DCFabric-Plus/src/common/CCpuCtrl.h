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
*   File Name   : CCpuCtrl.h                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CCPUCTRL_H
#define __CCPUCTRL_H

#include "bnc-type.h"

/*
 * 互斥锁
 */
class CCpuCtrl
{
public:
    static CCpuCtrl* getInstance();    

    ~CCpuCtrl();

    INT4 getQuota(std::string owner);
    INT4 occupyCpu(std::string owner, pthread_t tid);

private:
    CCpuCtrl();

    INT4 init();
    INT4 getCpuId();

private:
    static CCpuCtrl* m_instance;       

	volatile INT4 m_cpuId;
	INT4 m_cpuTotal;
    std::map<std::string, INT4> m_quotas;
    std::map<std::string, std::vector<INT4> > m_occupies;
};


#endif
