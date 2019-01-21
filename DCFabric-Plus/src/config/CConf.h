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
*   File Name   : CConf.h                                                     *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-31                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CCONF__H
#define __CCONF__H

#include "bnc-type.h"

enum TOPO_DISCOVER_PROTOCOL
{
    TOPO_DISCOVER_LLDP = 0,
    TOPO_DISCOVER_IP   = 1
};

class CConfigItem
{
public:
    std::string key;
    std::string val;
    std::vector<std::string> comment;

    void clear() {
        key.clear();
        val.clear();
        comment.clear();
    }
};

class CConfigBlock
{
public:
    std::string block;
    std::vector<CConfigItem> items;

    void clear() {
        block.clear();
        items.clear();
    }
};

/*
 * 配置类
 */
class CConf
{
public:
	 ~CConf();
    /*
     * 获取CConf实例
     *
     * @return: CConf*            CConf实例
     */
    static CConf* getInstance();

    /*
     * 加载配置文件
     */
    INT4 loadConf();

    /*
     * 获取配置项
     * param block: 配置块名称
     * param key: 配置项名称
     * ret: 返回配置项
     */
    const INT1* getConfig(const INT1* block, const INT1* key);

#if 0
    /*
     * 获取块配置项
     * param block: 配置块名称
     * ret: 返回块配置项
     */
    std::map<std::string, std::string>* getConfig(const INT1* block);
#endif

	 /*
     * 设置块配置项
     * param block: 配置块名称
     * ret: 
     */
	void setConfig(const INT1* block, const INT1* key, const INT1* val);
	
    /*
     * 更新配置文件
     */
    void saveConf();

    UINT4 getMsgHoldNumber() {return m_msgHoldNumber;}
    UINT4 getRecvThreadNumber() {return m_recvThreadNumber;}
    INT4  getTopoDiscoverProtocol() {return m_topoDiscoverProtocol;}
    BOOL  isSecurityGroupOn() {return m_securityGroupOn;}
    BOOL  isMininet(UINT4 ip);

private:
    CConf();

private:
    static CConf* m_pInstance;       
    
    std::string m_file;
#if 0
    std::map<std::string, std::map<std::string, std::string>* > m_mapConfigs;
#else
    std::vector<CConfigBlock> m_blocks;
#endif

    UINT4 m_msgHoldNumber;
    UINT4 m_recvThreadNumber;
    INT4  m_topoDiscoverProtocol;
    BOOL  m_securityGroupOn;
    std::vector<UINT4> m_mininetList;
};

#endif
