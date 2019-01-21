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
*   File Name   : CMsgCommon.h                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CMSGCOMMON_H
#define __CMSGCOMMON_H

#include "bnc-type.h"
#include "CRefObj.h"

typedef enum {
    MSG_OPER_NONE = 0,
    MSG_OPER_REALTIME,  //消息直接交给消费者实时处理
    MSG_OPER_HOLD,      //消息会保留一段时间或达到一定数目
    MSG_OPER_OVERRIDE,  //消息直接被后来者覆盖
    MSG_OPER_PASSON,    //所有消息将传递给消费者

    MSG_OPER_INVALID
}msg_operation_e;

typedef std::string CMsgPath;
typedef std::string CMsgKey;

/*
 * 消息的基类定义
 */
class CMsgCommon : public CRefObj
{
public:
    CMsgCommon();
    CMsgCommon(INT4 oper);
    virtual ~CMsgCommon();

    /*
     * 获取消息处理方式
     *
     * @return: INT4        返回消息处理方式
     */
    INT4 getOperation() {return m_oper;}

    /*
     * 设置消息处理方式
     *
     * @param: oper        消息处理方式
     *
     * @return: None
     */
    void setOperation(INT4 oper) {m_oper = oper;}

    /*
     * 获取消息路径
     *
     * @return: CMsgPath&      返回消息路径引用
     */
    CMsgPath& getPath() {return m_path;}

    /*
     * 设置消息路径
     *
     * @param: path         消息路径
     *
     * @return: None
     */
    void setPath(CMsgPath& path) {m_path = path;}
    
    /*
     * 获取消息关键字
     *
     * @return: std::string&      返回消息关键字引用
     */
    CMsgKey& getKey() {return m_key;}

    /*
     * 设置消息关键字
     *
     * @param: key         消息关键字
     *
     * @return: None
     */
    void setKey(CMsgKey& key) {m_key = key;}

    /*
     * 获取消息产生的时间
     *
     * @return: time_t        返回消息产生的时间
     */
    time_t getTime() {return m_time;}

private:
    INT4     m_oper; ///< 消息处理方式 @msg_level_e
    CMsgPath m_path; ///< 消息路径
    CMsgKey  m_key;  ///< 消息关键字
    time_t   m_time; ///< 消息产生的时间
};

#endif
