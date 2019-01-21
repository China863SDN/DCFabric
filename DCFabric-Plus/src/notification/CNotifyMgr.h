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
*   File Name   : CNotifyMgr.h              *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-13                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _CNOTIFY_MGR_H
#define _CNOTIFY_MGR_H

#include "CNotifyHost.h"
#include "CNotifyFlow.h"
#include "CNotifyOpenstack.h"
#include "CNotifyPath.h"
#include "CNotifyTopo.h"
#include "CNotifyExternal.h"
#include "CNotifyNetwork.h"
#include "CNotifySubnet.h"
#include "CNotifyFloatingIp.h"
#include "CNotifyBaseNetworking.h"

/*
 * 管理所有通知的类
 *
 * 使用中介者模式来定义了各个模块间通知关系
 * 每个模块都有对应的通知类,
 * 在此处初始化了各个模块之间的对应关系.
 *
 * 比如: Path - Flow
 * 在初始化时调用addNotifyHandler(Path, Flow)
 * 动态地绑定了对应关系,
 * 并且在Center和各自的通知类中编写相应的处理函数notifyAddPath
 *
 * 当Path发生变化后, 调用Path中的处理函数notifyAddPath
 * 将会调用Flow中相应的处理函数notifyAddPath
 * 具体的处理逻辑就在Flow的notifyAddPath中
 *
 * 这样做的目的是为了解耦,
 * 使各个模块之间不存在相互依赖关系,
 * 全部都通过Center类进行通信,
 * 而每个模块只需要处理自己相关的内容
 */
class CNotifyMgr
{
public:
	 /*
     * 默认析构函数
     */
    ~CNotifyMgr();
    /*
     * 获取实例
     *
     * @return: CNotifyMgr*     实例指针
     */
    static CNotifyMgr* getInstance();

    /*
     * 获取Host通知
     *
     * @return: CNotifyHandler*     Host通知指针
     */
    CNotifyHandler* getNotifyHost() { return m_notifyHost; }

	/*
     * 获取Network通知
     *
     * @return: CNotifyHandler*     Network通知指针
     */
    CNotifyHandler* getNotifyNetwork() { return m_notifyNetwork; }

	
	/*
	 * 获取Subnet通知
	 *
	 * @return: CNotifyHandler* 	Subnet通知指针
	 */
	CNotifyHandler* getNotifySubnet() { return m_notifySubnet; }
	

	/*
	 * 获取ExternalPort通知
	 *
	 * @return: CNotifyHandler* 	ExternalPort通知指针
	 */
	//CNotifyHandler* getNotifyExternalPort() { return m_notifyExternalPort; }

	
	/*
	 * 获取FloatingIp通知
	 *
	 * @return: CNotifyHandler* 	FloatingIp通知指针
	 */
	CNotifyHandler* getNotifyFloatingIp() { return m_notifyFloatingIp; }
	
    /*
     * 获取openstackport通知
     *
     * @return: CNotifyHandler*     OpenstackPort通知指针
     */
    CNotifyHandler* getNotifyOpenstack()
                   { return m_notifyOpenstack; }

	 /*
     * 获取BaseNetworking通知
     *
     * @return: CNotifyHandler*     BaseNetworking通知指针
     */
    CNotifyHandler* getNotifyBaseNetworking()
                   { return m_notifyBaseNetworking; }

    /*
     * 获取openstackExternal通知
     *
     * @return: CNotifyHandler*     OpenstackExternal通知指针
     */
    CNotifyHandler* getNotifyOpenstackExternal()
                   { return m_notifyExternal; }

    /*
     * 获取Flow通知
     *
     * @return: CNotifyHandler*     Flow通知指针
     */
    CNotifyHandler* getNotifyFlow() { return m_notifyFlow; }

    /*
     * 获取Path通知
     *
     * @return: CNotifyHandler*     Path通知指针
     */
    CNotifyHandler* getNotifyPath() { return m_notifyPath; }

private:
    /*
     * 默认构造函数
     */
    CNotifyMgr();

    /*
     * 初始化
     */
    void init();

    static CNotifyMgr* m_pInstance;             ///< 静态实例

    CNotifyHandler* m_notifyHost;               ///< Host通知指针
    CNotifyHandler* m_notifyNetwork;               ///< Network通知指针
    CNotifyHandler* m_notifySubnet;               ///< Subnet通知指针
    //CNotifyHandler* m_notifyExternalPort;               ///< ExternalPort通知指针
    CNotifyHandler* m_notifyFloatingIp;               ///< FloatingIp通知指针
    CNotifyHandler* m_notifyOpenstack;      ///< OpenstackPort通知指针
    CNotifyHandler* m_notifyBaseNetworking;      ///< BaseNetworking通知指针
    CNotifyHandler* m_notifyFlow;               ///< Flow通知指针
    CNotifyHandler* m_notifyPath;               ///< Path通知指针
    CNotifyHandler* m_notifyTopo;               ///< Topo通知指针
    CNotifyHandler* m_notifyExternal;           ///< External通知指针

    CNotificationCenter* m_center;          ///< 通知处理中心类
};


#endif
