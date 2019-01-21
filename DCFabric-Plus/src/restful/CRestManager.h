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
*   File Name   : CRestManager.h		*
*   Author      : bnc xflu          *
*   Create Date : 2016-7-21         *
*   Version     : 1.0               *
*   Function    : .                 *
*                                                                             *
******************************************************************************/
#ifndef _CRestManager_H
#define _CRestManager_H

#include <map>
#include <string>
#include <list>

#include "bnc-type.h"
#include "CRestApiMgr.h"
#include "CRestDefine.h"

/*
 * manage all Rest service
 * 管理所有的Rest相关服务
 */
class CRestManager
{
public:
	~CRestManager();
	/*
	 * get RestManager instance
	 *
	 * @return: RestMananger instance
	 */
	static CRestManager* getInstance();

	/*
	 * get RestApi instance
	 *
	 * @return: RestApi instance
	 */
	CRestApiMgr* getRestApiMgr();

	/*
	 * get RestDefine instance
	 *
	 * @return RestDefine instance
	 */
	CRestDefine* getRestDefine();

private:
	/*
	 * avoid default constructor
	 */
	CRestManager();
	

	/*
	 * 初始化Rest Manger
	 *
	 * @return TRUE: success; FALSE: fail
	 */
	BOOL init();

	static CRestManager* m_pInstance;	///< RestManager instance
};

#endif
