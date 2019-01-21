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
*   File Name   : COpenstack.h              *
*   Author      : bnc xflu                  *
*   Create Date : 2016-8-31                 *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_H
#define _COPENSTACK_H

#include "bnc-type.h"
#include "string.h"
#include "COpenstackDefine.h"
#include "COpenstackResource.h"

/*
 * openstack类
 */
class COpenstack
{
public:
    /*
     * 带参数的构造函数
     *
     * @param: server_ip            服务器IP地址
     * @param: server_port_no       服务器端口号
     * @param: token_ip             token服务器IP地址
     * @param: token_port_no        token服务器端口号
     * @param: tenant               租户名(token验证用)
     * @param: user                 用户名(token验证用)
     * @param: password             密码(token验证用)
     *
     */
    COpenstack(UINT4 server_ip, UINT4 server_port_no,
            UINT4 token_ip, UINT4 token_port_no,
            const std::string & tenant,
            const std::string & user,
            const std::string & password);

    /*
     * 默认析构函数
     */
    ~COpenstack();

    /*
     * 初始化
     *
     * @return: None
     */
    void init();

    /*
     * 读取Openstack所有资源
     *
     * @return: None
     */
    void loadResources();

    /*
     * 获取OpenstackResource
     *
     * @return: COpenstackResource*         返回COpenstackResource类型
     */
    COpenstackResource* getResource()
    {
        return m_resource;
    }

private:
    /*
     * Openstack默认构造函数
     */
    COpenstack();

    /*
     * 向token服务器发送验证请求
     * 成功后, 会将认证ID存储到auth_token_id中
     *
     * @return: BOOL        TRUE:成功; FALSE:失败
     */
    BOOL authenticate();

    /*
     * 获取Openstack资源(network/subnent..)信息
     *
     * @param: version          版本号
     * @param: type             类型
     * @param: str              返回数据(成功取得后, 会将结果保存进去)
     *
     * @return: BOOL            TRUE:成功; FALSE:失败
     */
    BOOL loadResource(bnc::openstack::version version, bnc::openstack::network_type type, std::string& str);

    /*
     * 连接服务器
     *
     * @param: ip           服务器IP地址
     * @param: port_no      服务器端口号
     *
     * @return: INT4        返回值为Socket描述符; 失败的话, 返回0
     */
    INT4 connectServer(UINT4 ip, UINT4 port_no);

    /*
     * 创建认证数据
     *
     * @param: path         验证请求Path(会将生成的验证path存储进去)
     * @param: body         验证请求Body(会将生成的验证Body存储进去)
     *
     * @return: BOOL        TRUE:成功; FALSE:失败
     */
    BOOL createAuthData(std::string & path, std::string & body);

    /*
     * 解析认证数据
     * 解析成功后, 会将验证结果保存到auth_token_id
     *
     * @param: result       认证后从服务器返回的字符串(里面包含了认证结果等信息)
     *
     * @return: BOOL        TRUE:成功; FALSE:失败
     */
    BOOL parseAuthData(const std::string & result);

    /*
    * 创建资源请求数据
    *
    * @param: version           版本号
    * @param: type              类型
    * @param: path              资源请求Path(会将生成的资源请求Path存储进去)
    * @param: body              资源请求Body(会将生成的资源请求Body存储进去)
    *
    * @return: BOOL             TRUE:成功; FALSE:失败
    */
    BOOL createResourceData(bnc::openstack::version version,
           bnc::openstack::network_type type, std::string & path, std::string & body);

    /*
     * 解析资源数据
     *
     * @param: version          版本
     * @param: type             类型
     * @param: result           从服务器返回的资源数据
     *
     * @return: BOOL            TRUE:成功; FALSE:失败
     */
    BOOL parseResourceData(bnc::openstack::version version, bnc::openstack::network_type type,
                           const std::string & result);


    /*
     * 读取openstack服务器回复的数据
     *
     * @param: sockfd           socket描述符
     * @param: result           资源字符串(会将从服务器返回的资源数据存储进去)
     */
    BOOL readReplyData(INT4 sockfd, std::string & result);

    /*
     * 定时更新openstack信息
     *
     * @param: url              请求的资源的url
     */
    void reload(const std::string & url) const;

    UINT4 server_ip;                    ///< openstack服务器ip地址
    UINT4 server_port_no;               ///< openstack服务器端口
    UINT4 token_ip;                     ///< token的ip地址
    UINT4 token_port_no;                ///< token的端口号

    std::string auth_tenant_name;       ///< 认证租户名
    std::string auth_user_name;         ///< 认证用户名
    std::string auth_password;          ///< 认证用户密码

    std::string auth_token_id;          ///< 记录认证id, 每次调用authenticate生成

    COpenstackResource* m_resource;     ///< 资源管理类
};

#endif
