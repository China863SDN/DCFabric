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
*   File Name   : COpenstackResource.h      *
*   Author      : bnc xflu                  *
*   Create Date : 2016-9-7                  *
*   Version     : 1.0                       *
*   Function    : .                         *
*                                                                             *
******************************************************************************/
#ifndef _COPENSTACK_RESOURCE_MGR_H
#define _COPENSTACK_RESOURCE_MGR_H
#include "COpenstackDefine.h"
#include "COpenstackPort.h"
#include "COpenstackFloatingip.h"
#include "COpenstackSecurityGroup.h"
#include "COpenstackNetwork.h"
#include "COpenstackSubnet.h"
#include "COpenstackRouter.h"
#include "COpenstackQosRule.h"
#include "COpenstackQosBind.h"
#include "../networking/BaseSubnetManager.h"
#include "../networking/BaseFloatingManager.h"
#include "../networking/BaseNetworkManager.h"
#include "../networking/BaseRouterManager.h"
#include "NetworkingEventReporter.h"
#include <string>
#include <list>
#include "CSmartPtr.h"

typedef std::map<string, CSmartPtr<COpenstackNetwork> > 	  COpenstackNetworkMap;
typedef std::map<string, CSmartPtr<COpenstackSubnet> > 		  COpenstackSubnetMap;
typedef std::map<string, CSmartPtr<COpenstackFloatingip> > 	  COpenstackFloatingMap;
typedef std::map<string, CSmartPtr<COpenstackQosBind> > 	  COpenstackQosBindMap;
typedef std::map<string, CSmartPtr<COpenstackQosRule> > 	  COpenstackQosRuleMap;
typedef std::map<string, CSmartPtr<COpenstackPort> > 		  COpenstackPortMap;
typedef std::map<string, CSmartPtr<COpenstackSecurityGroup> > COpenstackSecurityGroupMap;
typedef std::map<string, CSmartPtr<COpenstackRouter> > 		  COpenstackRouterMap;

/*
 * openstack资源管理类
 */
class COpenstackResource
{
public:
    /*
     * 默认构造函数
     */
    COpenstackResource();

    /*
     * 默认析构函数
     */
    ~COpenstackResource();

    /*
     * 初始化
     */
    void init();

    /*
     * 通过floatingIP来获取fixedIP
     *
     * @param:UINT4                                                 fixedIP
     *
     * @return: UINT4                                               floatingIP
     */
    UINT4 getFloatingipbyFixedIp(const UINT4& fixedip);

    /*
     * 通过fixedIP来获取floatingIP
     *
     * @param:UINT4                                                 floatingIP
     *
     * @return: UINT4                                               fixedIP
     */
    UINT4 getFixedipByFloatingip(const UINT4& floatingip);

    /*
     * 获取资源列表
     *
     * @return:list<bnc::openstack::network_type>                   返回可用的openstack网络类型
     */
    const std::list<bnc::openstack::network_type>  & getResourceList() {return m_resourceList; }

    /*
     * 获取port列表
     *
     * @return:COpenstackPortMap&                                openstack port列表
     */
    const COpenstackPortMap& getOpenstackPortList() {return m_port_list;}

    /*
     * 获取Floatingip列表
     *
     * @return:list<COpenstackFloatingip*>                          openstack Floatingip列表
     */
    const std::map<string, COpenstackFloatingip*> &getOpenstackFloatingipList() {return m_floatingip_list;}

    /*
     * 获取securitygroup_list列表
     *
     * @return:list<COpenstackSecurityGroup*>                       COpenstackSecurityGroup列表
     */
    const COpenstackSecurityGroupMap& getOpenstackSecurityGroupList() {return m_securitygroup_list;}

    /*
     * 获取network_list列表
     *
     * @return:list<COpenstackNetwork*>                          openstack_network_list列表
     */
    const std::map<string, COpenstackNetwork*> &getOpenstackNetworkList() {return m_network_list;}

    /*
     * 获取subnet_list列表
     *
     * @return:list<COpenstackSubnet*>                          openstack_subnet_list列表
     */
    const std::map<string, COpenstackSubnet*> &getOpenstackSubnetList() {return m_subnet_list;}

	 /*
     * 获取qosrule_list列表
     *
     * @return:list<COpenstackQosRule*>                          qosrule_list列表
     */
	const std::map<string, COpenstackQosRule*> & getOpenstackQosRuleList(){return m_qosrule_list; }

	
	 /*
     * 获取qosbind_list列表
     *
     * @return:list<COpenstackQosBind*>                          qosbind_list列表
     */
	const std::map<string, COpenstackQosBind*> & getOpenstackQosBindList(){return m_qosbind_list; }

    /*
     * 解析数据
     * @param：bnc::openstack::network_type                        openstack网络类型
     * @param：string                                              待解析参数
     *
     * @return:BOOL                                                返回解析是否成功
     */
    BOOL parseResourceData(bnc::openstack::network_type type,
                           const std::string & result);
    /*
     * 根据IP地址查找openstack port
     *
     * @param：UINT4                                               IP地址
     *
     * @return：CSmartPtr<COpenstackPort>                          port
     */
    CSmartPtr<COpenstackPort> findOpenstackPortByIp(const UINT4 ip_address);

    /*
     * 根据IP地址查找openstack subnet
     *
     * @param：UINT4                                               IP地址
     *
     * @return：COpenstackSubnet                                   port
     */
    COpenstackSubnet* findSubnetByNetworkId(const std::string& network_id);

    /*
    * 根据IP地址查找openstack port
    *
    * @param：string                                              port id
    *
    * @return：CSmartPtr<COpenstackPort>                          port
    */
    CSmartPtr<COpenstackPort> findOpenstackPort(const std::string & id);

    /*
    * 根据IP地址查找openstack subnet
    *
    * @param：string                                              subnet id
    *
    * @return：COpenstackSubnet                                   subnet
    */
    COpenstackSubnet* findOpenstackSubnet(const std::string& id);
	
	void resetResourceData(bnc::openstack::network_type type);
	void clearResourceNode(bnc::openstack::network_type type);
	
    COpenstackFloatingip* findOpenstackFloatingip(const std::string& id);

private:
    BOOL parseNetworkData(const std::string & result);
    BOOL parseSubnetData(const std::string & result);
    BOOL parsePortData(const std::string & result);
    BOOL parseFloatingipData(const std::string & result);
    BOOL parseSecurityGroupData(const std::string & result);
    BOOL parseRouterData(const std::string & result);
	BOOL parseQosRuleData(const std::string & result);
    BOOL addOpenstackPort(CSmartPtr<COpenstackPort> port);
    BOOL addOpenstackFloatingip(COpenstackFloatingip* floatingip);
    //COpenstackFloatingip* findOpenstackFloatingip(const std::string& id);
    BOOL addOpenstackSecurityGroups(const COpenstackSecurityGroupMap& securityGroups);
    BOOL addOpenstackSecurityGroup(CSmartPtr<COpenstackSecurityGroup> securitygroup);
    CSmartPtr<COpenstackSecurityGroup> findOpenstackSecurityGroup(const std::string& id);
    BOOL addOpenstackNetwork(COpenstackNetwork* network);
    COpenstackNetwork* findOpenstackNetwork(const std::string& id);
    BOOL addOpenstackSubnet(COpenstackSubnet* subnet);
    BOOL addOpenstackRouters(const COpenstackRouterMap& routers);

	BOOL addOpenstackQosRule(COpenstackQosRule* qosRule);
	COpenstackQosRule* findOpenstackQosRule(const std::string & qos_rule_id);

	BOOL addOpenstackQosBindPort(COpenstackQosBind* qosBind);
	COpenstackQosBind* findOpenstackQosBindPort(const std::string & qosBind_portid);

	void resetOpenstackNetworkList();
	void resetOpenstackSubnetList();
	void resetOpenstackPortList();
	void resetOpenstackFloatingIpList();
	void resetOpenstackSecurityGroupList();
	void resetOpenstackRouterList();
	void resetOpenstackQosRuleList();
	void resetOpenstackQosBindList();

	void clearOpenstackNetworkNode();
	void clearOpenstackSubnetNode();
	void clearOpenstackPortNode();
	void clearOpenstackFloatingIpNode();
	void clearOpenstackSecurityGroupNode();
	void clearOpenstackRouterNode();
	void clearOpenstackQosRuleNode();
	void clearOpenstackQosBindNode();
	
	void reportSecurityGroupEvent(CSmartPtr<COpenstackPort> portNew, CSmartPtr<COpenstackPort> portOld);
    void reportSecurityGroupRuleEvent(const CSmartPtr<COpenstackSecurityGroup> securityGroupRule, INT4 eventId, INT4 reason);
    void getIPAndMaskByCidr(const std::string& cidr, UINT4& ip, UINT4& mask);
    void reportPortforwardRuleEvent(const CSmartPtr<COpenstackRouter> router, INT4 eventId, INT4 reason);
   
private:
	COpenstackPortMap m_port_list;
    std::map<string, COpenstackFloatingip*> m_floatingip_list;
	COpenstackSecurityGroupMap m_securitygroup_list;
    std::map<string, COpenstackNetwork*> m_network_list;
   	std::map<string, COpenstackSubnet*> m_subnet_list;
	COpenstackRouterMap m_router_list;
	std::map<string, COpenstackQosRule*> m_qosrule_list;
	std::map<string, COpenstackQosBind*> m_qosbind_list;

    std::list<bnc::openstack::network_type> m_resourceList;     ///< 可用的openstack网络类型
};

#endif
