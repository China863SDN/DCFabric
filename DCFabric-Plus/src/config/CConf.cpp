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
*   File Name   : CConf.cpp                                                   *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-31                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CConf.h"
#include "log.h"
#include "bnc-error.h"
#include "comm-util.h"

#define MSG_HOLD_MINUTE_DEFAULT    30
#define MSG_HOLD_NUMBER_DEFAULT    30
#define RECV_THREAD_NUMBER_DEFAULT 1

const static std::string CONFIGURE_FILE = "./conf/controller.conf";

static INT1* skipLeadingBlanks(INT1* buf)
{
    while ((' ' == *buf) || ('\t' == *buf))
        ++ buf;
    return buf;
}

CConf* CConf::m_pInstance = NULL;

CConf* CConf::getInstance()
{
    if (NULL == m_pInstance) 
    {
        m_pInstance = new CConf();
        if (NULL == m_pInstance)
        {
            exit(-1);
        }
    }

    return (m_pInstance);
}

CConf::CConf():
    m_file(CONFIGURE_FILE),
    m_msgHoldNumber(0),
    m_recvThreadNumber(0),
    m_topoDiscoverProtocol(TOPO_DISCOVER_LLDP),
    m_securityGroupOn(FALSE)
{
}

CConf::~CConf()
{
#if 0
    std::map<std::string, std::map<std::string, std::string>* >::iterator iter;
    std::map<std::string, std::string>::iterator it;
    for (iter = m_mapConfigs.begin(); iter != m_mapConfigs.end(); iter++)
    {
        iter->second->erase(iter->second->begin(), iter->second->end());
        delete iter->second;
    }
    m_mapConfigs.erase(m_mapConfigs.begin(), m_mapConfigs.end());
#endif
}

#if 0
INT4 CConf::loadConf()
{
    if (m_sFilename.empty())
    {
        m_sFilename = CONFIGURE_FILE;
    }

    LOG_INFO(">> CConf::loadConf ...");

    std::ifstream file(m_sFilename.c_str());
	if (NULL == file)
	{
		LOG_ERROR(">> CConf::loadConf fail! conf file not exist...");
		return BNC_ERR;
	}

    char buf[256] = {0};
    std::map<std::string, std::string>* block = NULL;
    while (!file.eof())
    {
        memset(buf, 0, 256);
        file.getline(buf, 256);
        std::string line(buf);
        switch (buf[0])
        {
            case '\n':
            case '\0':
            case '#':
            case ' ':
            case '\t':
            {
                break;
            }
            case '[':
            {
                LOG_INFO_FMT("case: %s", buf);
                block = new std::map<std::string, std::string>();
                m_mapConfigs.insert(std::pair<std::string, std::map<std::string, std::string>* >(line.substr(1, line.find(']') - 1), block));
                break;
            }
            default:
            {
                LOG_INFO_FMT("default: %s", buf);
                std::string::size_type pos = line.find('=');
                if (std::string::npos != pos)
                {
                    block->insert(std::pair<std::string, std::string>(line.substr(0, pos), line.substr(pos + 1, line.size())));
                }
                else
                {
                    block->insert(std::pair<std::string, std::string>(line, line));
                }
                break;
            }
        }
    }

    file.close();
    LOG_INFO_FMT("<< CConf::loadConf: load %s success", m_sFilename.c_str());

	const INT1* pConf = getConfig("msgtree_conf", "msg_hold_number");
    m_msgHoldNumber = (NULL == pConf) ? MSG_HOLD_NUMBER_DEFAULT : atol(pConf);
    LOG_INFO_FMT("msg_hold_number %u", m_msgHoldNumber);

    pConf = getConfig("controller", "recv_thread_num");
    m_recvThreadNumber = (NULL == pConf) ? RECV_THREAD_NUMBER_DEFAULT : atol(pConf);
    LOG_INFO_FMT("recv_thread_num %u", m_recvThreadNumber);

    m_topoDiscoverProtocol = TOPO_DISCOVER_LLDP;
    pConf = getConfig("controller", "topo_discover_protocol");
    if ((NULL != pConf) && (strcasecmp(pConf, "IP") == 0))
        m_topoDiscoverProtocol = TOPO_DISCOVER_IP;
    LOG_INFO_FMT("topo_discover_protocol %d", m_topoDiscoverProtocol);

    m_securityGroupOn = FALSE;
    pConf = getConfig("openstack", "security_group_on");
    if ((NULL != pConf) && (atol(pConf) > 0))
        m_securityGroupOn = TRUE;
    LOG_INFO_FMT("security_group_on %d", m_securityGroupOn);

	return BNC_OK;
}

const INT1* CConf::getConfig(const INT1* block, const INT1* key)
{
    std::map<std::string, std::map<std::string, std::string>* >::iterator iter = m_mapConfigs.find(std::string(block));
    if (iter != m_mapConfigs.end())
    {
        std::map<std::string, std::string>::iterator it = iter->second->find(std::string(key));
        if (it != iter->second->end())
        {
            return (it->second).c_str();
        }
    }

    return NULL;
}

std::map<std::string, std::string>* CConf::getConfig(const INT1* block)
{
    std::map<std::string, std::map<std::string, std::string>* >::iterator iter = m_mapConfigs.find(std::string(block));
    if (iter != m_mapConfigs.end())
    {
        return iter->second;
    }

    return NULL;
}

void CConf::SetConfig(const INT1* block, const INT1* key, const INT1* value)
{
    std::map<std::string, std::map<std::string, std::string>* >::iterator iter = m_mapConfigs.find(std::string(block));
    if (iter != m_mapConfigs.end())
    {
        std::map<std::string, std::string>::iterator it = iter->second->find(std::string(key));
        if (it != iter->second->end())
        {
           it->second = value;
        }
		else
		{
			iter->second->insert(std::make_pair(std::string(key), std::string((value))));
		}
    }
	else
	{
		//m_mapConfigs.insert(std::make_pair(std::string(block), std::make_pair(std::string(key), std::string(value))));
	}
}

#else

INT4 CConf::loadConf()
{
    LOG_INFO_FMT(">> CConf::loadConf: conf file %s", m_file.c_str());

    std::ifstream file(m_file.c_str());
	if (NULL == file)
	{
		LOG_ERROR_FMT("<< CConf::loadConf: conf file %s not exist", m_file.c_str());
		return BNC_ERR;
	}

    INT1 buf[256] = {0}, *ptr = NULL;
    CConfigBlock block;
    CConfigItem  item;

    while (!file.eof())
    {
        memset(buf, 0, 256);
        file.getline(buf, 256);
        ptr = skipLeadingBlanks(buf);
        std::string line(ptr);

        switch (ptr[0])
        {
            case '\r':
            case '\n':
            case '\0':
            {
                break;
            }
            case '[':
            {
                if (!block.block.empty())
                    m_blocks.push_back(block);
                block.clear();
                item.clear();

                LOG_INFO_FMT("block: %s", ptr);
                block.block = line.substr(1, line.find(']')-1);
                break;
            }
            case '#':
            {
                LOG_INFO_FMT("comment: %s", ptr);
                item.comment.push_back(line);
                break;
            }            
            default:
            {
                LOG_INFO_FMT("item: %s", ptr);
                std::string::size_type pos = line.find('=');
                if (std::string::npos != pos)
                {
                    item.key = line.substr(0, pos);
                    item.val = line.substr(pos+1, line.size());
                }
                else
                {
                    item.key = line;
                }
                block.items.push_back(item);
                item.clear();
                break;
            }
        }
    }

    m_blocks.push_back(block);

    file.close();
    LOG_INFO_FMT("<< CConf::loadConf: load %s success", m_file.c_str());

	const INT1* pConf = getConfig("msgtree_conf", "msg_hold_number");
    m_msgHoldNumber = (NULL == pConf) ? MSG_HOLD_NUMBER_DEFAULT : atol(pConf);
    LOG_INFO_FMT("msg_hold_number %u", m_msgHoldNumber);

    pConf = getConfig("controller", "recv_thread_num");
    m_recvThreadNumber = (NULL == pConf) ? RECV_THREAD_NUMBER_DEFAULT : atol(pConf);
    LOG_INFO_FMT("recv_thread_num %u", m_recvThreadNumber);

    m_topoDiscoverProtocol = TOPO_DISCOVER_LLDP;
    pConf = getConfig("controller", "topo_discover_protocol");
    if ((NULL != pConf) && (strcasecmp(pConf, "IP") == 0))
        m_topoDiscoverProtocol = TOPO_DISCOVER_IP;
    LOG_INFO_FMT("topo_discover_protocol %d", m_topoDiscoverProtocol);

    m_securityGroupOn = FALSE;
    pConf = getConfig("openstack", "security_group_on");
    if ((NULL != pConf) && (atol(pConf) > 0))
        m_securityGroupOn = TRUE;
    LOG_INFO_FMT("security_group_on %d", m_securityGroupOn);

    pConf = getConfig("mininet_conf", "mininet_list");
    if (NULL != pConf)
    {
        std::list<std::string> singleList;
        split(pConf, ",", singleList);

        STL_FOR_LOOP(singleList, it)
        {
            m_mininetList.push_back(ntohl(ip2number((*it).c_str())));
        }
    }

	return BNC_OK;
}

const INT1* CConf::getConfig(const INT1* block, const INT1* key)
{
    STL_FOR_LOOP(m_blocks, blockIt)
    {
        CConfigBlock& blck = *blockIt;
        if (blck.block.compare(block) == 0)
        {
            STL_FOR_LOOP(blck.items, itemIt)
            {
                CConfigItem& item = *itemIt;
                if (item.key.compare(key) == 0)
                    return item.val.c_str();
            }
        }
    }

    return NULL;
}

void CConf::setConfig(const INT1* block, const INT1* key, const INT1* val)
{
    STL_FOR_LOOP(m_blocks, blockIt)
    {
        CConfigBlock& blck = *blockIt;
        if (blck.block.compare(block) == 0)
        {
            STL_FOR_LOOP(blck.items, itemIt)
            {
                CConfigItem& item = *itemIt;
                if (item.key.compare(key) == 0)
                    item.val = val;
            }
        }
    }
}

void CConf::saveConf()
{
    LOG_INFO_FMT(">> CConf::saveConf: conf file %s", m_file.c_str());

    std::ofstream file(m_file.c_str());
	if (NULL == file)
	{
		LOG_ERROR_FMT("<< CConf::saveConf: open conf file %s failed !", m_file.c_str());
		return;
	}

    STL_FOR_LOOP(m_blocks, blockIt)
    {
        CConfigBlock& block = *blockIt;
        file << "[" << block.block.c_str() << "]" << std::endl;

        STL_FOR_LOOP(block.items, itemIt)
        {
            CConfigItem& item = *itemIt;
            STL_FOR_LOOP(item.comment, commIt)
            {
                file << (*commIt).c_str() << std::endl;
            }
            file << item.key.c_str() << "=" << item.val.c_str() << std::endl;
        }
        file << std::endl;
    }

    file.close();
    LOG_INFO("<< CConf::saveConf: success");
}


BOOL CConf::isMininet(UINT4 ip)
{
    STL_FOR_LOOP(m_mininetList, it)
    {
        if (*it == ip)
            return TRUE;
    }
    return FALSE;
}

#endif
