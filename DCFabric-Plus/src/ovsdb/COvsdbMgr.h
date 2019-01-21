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
*   File Name   : COvsdbMgr.h                                                 *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __COVSDBMGR_H
#define __COVSDBMGR_H

#include "bnc-type.h"
#include "CRefObj.h"
#include "CSwitchMgr.h"
#include "CQosQueue.h"

typedef struct open_vswitch_s
{
    INT1 uuid[64];
}open_vswitch_t;

typedef struct ovs_bridge_s
{
    INT1  name[16];
    INT1  uuid[64];
    UINT8 dpid;
    INT4  proto;
}ovs_bridge_t;

class COvsdbClient : public CRefObj
{
public:
    COvsdbClient();
    COvsdbClient(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port);
    ~COvsdbClient();

    INT4 getSockfd() {return m_sockfd;}
    UINT4 getIp() {return m_ip;}
    UINT2 getPort() {return m_port;}

    open_vswitch_t& getOpenVSwitch() {return m_sw;}
    void saveBridge(const ovs_bridge_t& bridge);
    std::vector<ovs_bridge_t>& getBridges() {return m_bridges;}
    void clearBridges();

private:
    INT4                      m_sockfd;
    INT1                      m_mac[MAC_LEN];
    UINT4                     m_ip;
    UINT2                     m_port;
    open_vswitch_t            m_sw;
    std::vector<ovs_bridge_t> m_bridges;
};

//typedef std::map<INT4/*sockfd*/, CSmartPtr<COvsdbClient> > COvsdbClientMap;
typedef CUint32LFHashMap<CSmartPtr<COvsdbClient> > COvsdbClientHMap;

/*
 * OVSDB管理类
 */
class COvsdbMgr
{
public:
    /*
     * 获取CClusterMgr实例
     *
     * @return: CClusterMgr*   CClusterMgr实例
     */
    static COvsdbMgr* getInstance();

    /*
     * 默认析构函数
     */
    ~COvsdbMgr();

    /*
     * 初始化
     *
     * @return: 成功 or 失败
     */
    INT4 init();

    /*
     * 从收到的消息中找出一条完整的JSON
     *
     * @param: buffer         消息内容
     * @param: len            消息长度
     *
     * @return: 完整JSON的长度
     */
    UINT4 genJson(const INT1* buffer, UINT4 len);

    /*
     * OVSDB消息处理
     *
     * @param: sockfd         交换机连接sockfd
     * @param: msg            消息内容
     * @param: len            消息长度
     *
     * @return: 成功 or 失败
     */
    INT4 process(INT4 sockfd, const INT1* msg, UINT4 len);

    INT4 addClient(INT4 sockfd, const INT1* mac, UINT4 ip, UINT2 port);
    void delClient(INT4 sockfd);
    CSmartPtr<COvsdbClient> getClient(INT4 sockfd);
    CSmartPtr<COvsdbClient> getClient(UINT4 ip);

    void saveBridge(INT4 sockfd, const ovs_bridge_t& bridge);

private:
    /*
     * 默认构造函数
     */
    COvsdbMgr();

    void setController(INT4 sockfd, const INT1* uuid);
    void addPort(INT4 sockfd, const INT1* uuid, const INT1* name);
    void addBridge(INT4 sockfd, const INT1* name, const INT1* uuid, const INT1* failMode, const INT1* ofproto);
    void addTunnel(INT4 sockfd, const ovs_bridge_t& bridge);
    void addPortAndPortOption(INT4 sockfd, const INT1* brUuid, const INT1* name, 
        const INT1* localIp, const INT1* remoteIp, const INT1* optionKey, const INT1* type);

    void replyEcho(INT4 sockfd);
    void handleOpenvswitchTable(INT4 sockfd, const json_t* result);
    BOOL handleControllerTable(INT4 sockfd, const json_t* result);
    void handleBridgeTable(INT4 sockfd, const json_t* result, BOOL haveController);

    void setFailmodAndOfver(INT4 sockfd, const INT1* brUuid, const INT1* failMode, const INT1* ofproto);
    void handleSearchHostByMac(INT4 sockfd, const json_t* result);

    void searchAllTableFromOvsdb(INT4 sockfd);

private:
    static const UINT4 hash_bucket_number = 10240;
    static COvsdbMgr* m_instance;      

    BOOL             m_tunnelOn; 
    INT4             m_tunnelType;
    INT4             m_ofVersion;
    COvsdbClientHMap m_clients;
    COvsdbClientHMap m_ipMap;
    CQosQueue        m_qosQueue;
};

#endif
