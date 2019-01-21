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
*   File Name   : COpenstackDefine.h    *
*   Author      : bnc xflu              *
*   Create Date : 2016-9-05             *
*   Version     : 1.0                   *
*   Function    : .                     *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_DEFINE_H
#define _COPENSTACK_DEFINE_H
#include <string>
#include "bnc-type.h"

namespace bnc
{
    namespace openstack
    {
        enum openstack_version
        {
            juno,
            kilo,
            liberty,
            mitaka
        };

        enum version
        {
            empty_version,
            v2
        };

        enum network_type
        {
            versions,
            extensions,
            networks,
            subnets,
            ports,
            subnetpools,
            routers,
            floatingips,
            securitygroups,
            securitygrouprules,
            quotas,
            serviceproviders,
            flavors,
            tags,
            networkipavailability,
            qos,
            qosrules,
            metering,
            lbaas,
            lbaas_v1,
            firewallpolicies,
        };

        enum device_type
        {
            device_unknown = 0,
            device_host,
            device_interface,
            device_gateway,
            device_dhcp,
            device_loadbalance,
            device_floatingip,
            device_cloadblance_ha,
            device_clloadblance_vip,
        };

    }
}

/*
 * 提供了openstack所定义的参数
 */
class COpenstackDefine
{
public:
    COpenstackDefine();
    ~COpenstackDefine();

    /*
     * 获取实例
     */
    static COpenstackDefine* getInstance();

    /*
     * 根据版本号和network类型来读取字符串
     *
     * @param: version      版本号
     * @param: type         类型
     * @param: str          根据版本号和类型生成的字符串
     *
     * @return: BOOL        TRUE:成功; FALSE:失败
     */
    BOOL getStrFromNetwork(bnc::openstack::version version,
            bnc::openstack::network_type type, std::string& str);

    /*
     * port device owner类型转换
     *
     * @param: str              字符串
     *
     * @return: device_type     主机类型
     */
    bnc::openstack::device_type getDeviceTypeFromStr(const std::string & str) const;

private:
    /*
     * 获取版本号对应的字符串
     *
     * @param: ver          版本号
     * @param: str          根据版本号生成的字符串(将会将结果存入)
     *
     * @return: BOOL        TRUE:成功; FALSE:失败
     */
    BOOL getStrFromVersion(bnc::openstack::version ver, std::string& str);

    /*
     * 获取类型对应的字符串
     *
     * @param: type         类型
     * @param: str          根据类型生成的字符串(将会将结果存入)
     *
     * @return: BOOL        TRUE:成功; FALSE:失败
     */
    BOOL getStrFromType(bnc::openstack::network_type type, std::string& str);

    /*
     * 判断该字符串是否可用
     *
     * @param: version          版本
     * @param: str              字符串
     *
     * @return: BOOL            TRUE:可用; FALSE:不可用
     */
    BOOL isValid(UINT4 openstack_version, const std::string & str) const;

    static COpenstackDefine* m_pInstance;       ///< 实例
};


#endif
