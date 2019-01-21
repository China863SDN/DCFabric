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
*   File Name   : CLFHashMap.h                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef _CLFHASHMAP_H
#define _CLFHASHMAP_H

#include "bnc-type.h"
#include "comm-util.h"
#include "concurrent_hash_map.h"

/*
 * TBB提供的一种无锁hashmap
 * 来自Intel并行计算库
 */
template<class CKey, class CData, class CHashCompare>
class CLFHashMap
{
public:
    typedef std::pair<const CKey, CData> CPair;
    typedef std::allocator<CPair> CAllocator;
    typedef typename tbb::concurrent_hash_map<CKey, CData, CHashCompare, CAllocator> CHashMap;
    typedef typename CHashMap::accessor CAccessor;
    typedef typename CHashMap::iterator CIterator;

public:
    CLFHashMap() {}
    CLFHashMap(UINT4 bucketNum):m_hashMap(bucketNum) {}
    virtual ~CLFHashMap() {}

    void clear() {m_hashMap.clear();}

    BOOL insert(const CKey& key, const CData& data, CPair** item=NULL) 
    {
        erase(key);
        
        CPair value(key, data);
        CAccessor access;
        if (m_hashMap.insert(access, value))
        {
            if (NULL != item)
                *item = &(*access);
            return TRUE;
        }
        return FALSE;
    }

    BOOL find(const CKey& key, CPair** item=NULL) 
    {
        CAccessor access;
        if (m_hashMap.find(access, key))
        {
            if (NULL != item)
                *item = &(*access);
            return TRUE;
        }
        return FALSE;
    }

    void erase(const CKey& key) {m_hashMap.erase(key);}
    size_t size() const {return m_hashMap.size();}
    BOOL empty() const {return m_hashMap.empty();}
    CIterator begin() {return m_hashMap.begin();}
    CIterator end() {return m_hashMap.end();}

private:
    CHashMap m_hashMap;
};

class StringHashCompare
{
public:
    static size_t hash(const std::string& x)  
    {  
        return hashString(x);  
    }  
  
    static BOOL equal(const std::string& x, const std::string& y)  
    {  
        return (x == y);  
    }  
};

class Uint32HashCompare
{
public:
    static size_t hash(const UINT4& x)  
    {  
        return hashUint32(x);  
    }  
  
    static BOOL equal(const UINT4& x, const UINT4& y)  
    {  
        return (x == y);  
    }  
};

class Uint64HashCompare
{
public:
    static size_t hash(const UINT8& x)  
    {  
        return hashUint64(x);  
    }  
  
    static BOOL equal(const UINT8& x, const UINT8& y)  
    {  
        return (x == y);  
    }  
};

class Uint64PairHashCompare
{
public:
    static size_t hash(const std::pair<UINT8,UINT8>& x)  
    {  
        return hashUint64(x.first) + hashUint64(x.second);  
    }  
  
    static BOOL equal(const std::pair<UINT8,UINT8>& x, const std::pair<UINT8,UINT8>& y)  
    {  
        return ((x.first == y.first) && (x.second == y.second));  
    }  
};

template <class Data>
class CStringLFHashMap : public CLFHashMap<std::string, Data, StringHashCompare>
{
public:
    CStringLFHashMap() {}
    CStringLFHashMap(UINT4 bucketNum):CLFHashMap<std::string, Data, StringHashCompare>(bucketNum) {    }
    ~CStringLFHashMap() {}
};

template <class Data>
class CUint32LFHashMap : public CLFHashMap<UINT4, Data, Uint32HashCompare>
{
public:
    CUint32LFHashMap() {}
    CUint32LFHashMap(UINT4 bucketNum):CLFHashMap<UINT4, Data, Uint32HashCompare>(bucketNum) {    }
    ~CUint32LFHashMap() {}
};

template <class Data>
class CUint64LFHashMap : public CLFHashMap<UINT8, Data, Uint64HashCompare>
{
public:
    CUint64LFHashMap() {}
    CUint64LFHashMap(UINT4 bucketNum):CLFHashMap<UINT8, Data, Uint64HashCompare>(bucketNum) {    }
    ~CUint64LFHashMap() {}
};

template <class Data>
class CUint64PairLFHashMap : public CLFHashMap<std::pair<UINT8,UINT8>, Data, Uint64PairHashCompare>
{
public:
    CUint64PairLFHashMap() {}
    CUint64PairLFHashMap(UINT4 bucketNum):CLFHashMap<std::pair<UINT8,UINT8>, Data, Uint64PairHashCompare>(bucketNum) {}
    ~CUint64PairLFHashMap() {}
};

#endif
