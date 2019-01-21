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
*   File Name   : CControl.h                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CCONTROL_H
#define __CCONTROL_H

#include "bnc-type.h"
#include "CMsgHandlerMgr.h"
#include "CMsgTree.h"
#include "CSwitchMgr.h"
#include "CTopoMgr.h"
#include "CTagMgr.h"

/*
 * 服务器类
 *     管理服务的公共资源
 */
class CControl
{
public:
    /*
     * 获取CControl实例
     *
     * @return: CControl*            CControl实例
     */
    static CControl* getInstance();

    /*
     * 默认析构函数
     */
    ~CControl();
	
    /*
     * 初始化
     *
     * @return: INT4        成功 or 失败
     */
    INT4 init();

    /*
     * 获取handler管理类对象
     *
     * @return: 返回handler管理类对象引用
     */
    CMsgHandlerMgr& getHandlerMgr() {return m_handlerMgr;}

    /*
     * 获取消息存储树
     *
     * @return: 返回消息存储树对象引用
     */
    CMsgTree& getMsgTree() {return m_msgTree;}

    /*
     * 获取交换机管理类对象
     *
     * @return: 返回交换机管理类对象引用
     */
    CSwitchMgr& getSwitchMgr() {return m_switchMgr;}

    /*
     * 获取拓扑管理对象
     *
     * @return: 返回拓扑管理对象引用
     */
    CTopoMgr& getTopoMgr() {return m_topoMgr;}
    
    /*
     * 获取Tag管理对象
     *
     * @return: 获取Tag管理对象引用
     */
    CTagMgr& getTagMgr() {return m_tagMgr;}


	/*
     * 获取L3 mode 模式标记
     *
     * @return: 模式标记
     */
	BOOL isL3ModeOn() { return m_L3modeon; }

	/*
     * 获取Delay Time 标记
     *
     * @return: Delay Time标记
     */
	BOOL isDelayTimeReach()  {return m_DelayFlag; }
	
	void setDelayTimeReach(BOOL bTimeOut){ m_DelayFlag = bTimeOut; }

private:
    /*
     * 默认构造函数
     */
    CControl();

	INT4 init_signal();
	INT4 set_fileandcore();
private:
    //CControl实例
    static CControl* m_pInstance;       
    
    //消息存储数管理类
    CMsgTree m_msgTree;

    //全局handler mgr对象
    CMsgHandlerMgr m_handlerMgr;

    //交换机管理类
    CSwitchMgr m_switchMgr;

    //获取topo管理类
    CTopoMgr m_topoMgr;

    //Tag管理类
    CTagMgr m_tagMgr;

private:
	const static  INT4 DELAY_TIMEOUT_SECOND = 5;
	const static  INT4 m_maxFileNo = 65535;

	BOOL 	m_L3modeon;
	BOOL 	m_DelayFlag;
	CTimer 	m_timer;
};

#endif
