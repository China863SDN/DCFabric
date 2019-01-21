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
*   File Name   : COpenstackMgr.h           *
*   Author      : bnc xflu                  *
*   Create Date : 2016-8-31                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACKMGR_H
#define _COPENSTACKMGR_H
#include "bnc-type.h"
#include "COpenstack.h"
#include "CTimer.h"
#include "CResource_mgr.h"

/*
 * 用来管理Openstack信息
 */
class COpenstackMgr : public CResourceMgr
{
public:
	
	/*
	  * 默认析构函数
	 */
	~COpenstackMgr();
    static COpenstackMgr* getInstance();

    /*
     * 获取external信息
     *
     * @param:UINT4                         外部接口IP
     * @param:UINT1*                        外部接口mac
     * @param:UINT4&                        外部网关IP
     * @param:std::string&                  网络ID
     *
     * @return：BOOL                        返回是否get成功
     */
//    BOOL getExternalInfo(UINT4 outer_ip,UINT1* outer_Mac,UINT4& gateway_ip,std::string& network_id );


    /*
     * 获取external信息
     *
     * @param:UINT4                         外部接口IP
     * @param:UINT1*                        外部接口mac
     *
     *
     */
    COpenstack* getOpenstack()
    {
        return m_openstack;
    }

    /*
     * 重新读取Openstack信息
     *
     * @param: param            传入参数指针
     *
     * @return: void*           指针
     */
    static void* reloadResource(void* param);

private:
 
    /*
     * 初始化Openstack服务器
     */
    void init();

    /*
     * 获取实例
     */
    static COpenstackMgr* m_pInstance;


private:
    /*
     * 默认构造函数
     */
    COpenstackMgr();

    COpenstack* m_openstack;            ///< openstack对象指针(现在是只启用一个OPenstack,所以只存了一个指针)

    pthread_t m_reloadThreadId;         ///< 重新读取OPenstack信息的线程
};


#endif
