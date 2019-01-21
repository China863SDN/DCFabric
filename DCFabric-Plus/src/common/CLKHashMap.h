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
*   File Name   : CLKHashMap.h                                                *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CLKHASHMAP_H
#define __CLKHASHMAP_H

#include "bnc-type.h"
#include "comm-util.h"
#include "CMutex.h"

template <class HASHKEY, class HASHDATA>
class CLKHashMap
{
public:
    typedef std::map<HASHKEY, HASHDATA> CBucket;
    typedef std::pair<HASHKEY, HASHDATA> CBucketPair;
    typedef typename CBucket::iterator CBucketIterator;
    typedef UINT4 (HashFunc)(const HASHKEY& key);
    typedef CBucket CReference;
    typedef CBucketIterator CReferenceIterator;
    typedef CBucketPair CReferencePair;

public:
    CLKHashMap():m_bucketNum(0),m_hashFunc(NULL) {}
    CLKHashMap(UINT4    bucketNum, HashFunc* hashFunc):
        m_bucketNum(bucketNum),m_buckets(bucketNum),m_mutexs(bucketNum),m_hashFunc(hashFunc) {}
    virtual ~CLKHashMap()    {}

    HASHDATA* insert(UINT4 index, const HASHKEY& key, const HASHDATA& data)
    {
        remove(index, key);

        HASHDATA* ret = NULL;

        CBucket* buck = bucket(index);
        if (NULL != buck)
        {
            std::pair<CBucketIterator, bool> result = buck->insert(CBucketPair(key, data));
            if (result.second)
            {
                ret = &(result.first->second);
                m_mutex.lock();
                m_reference.insert(CReferencePair(key, data));
                m_mutex.unlock();
            }
        }
    
        return ret;
    }

    void remove(UINT4 index, const HASHKEY& key)
    {
        CBucket* buck = bucket(index);
        if (NULL != buck)
            buck->erase(key);

        m_mutex.lock();
        m_reference.erase(key);
        m_mutex.unlock();
    }

    BOOL pop(UINT4 index, const HASHKEY& key, HASHDATA& data)
    {
        BOOL done = FALSE;

        CBucket* buck = bucket(index);
        if (NULL != buck)
        {
            CBucketIterator it = buck->find(key);
            if (it != buck->end())
            {
                data = it->second;
                buck->erase(it);
                m_mutex.lock();
                m_reference.erase(key);
                m_mutex.unlock();
                done = TRUE;
            }
        }

        return done;
    }

    HASHDATA* find(UINT4 index, const HASHKEY& key)
    {
        HASHDATA* found = NULL;

        CBucket* buck = bucket(index);
        if (NULL != buck)
        {
            CBucketIterator it = buck->find(key);
            if (it != buck->end())
                found = &(it->second);
        }
        
        return found;
    }
        
    void clear()
    {
        m_reference.clear();
        STL_FOR_LOOP(m_buckets, vit)
        {
            CBucket& bucket = *vit;
            bucket.clear();
        }
    }

    size_t size()  {return m_reference.size();}
    BOOL empty()   {return m_reference.empty();}
    UINT4 bucketNum()    {return m_bucketNum;}
    UINT4 bucketIndex(const HASHKEY& key)    
    {
        return m_hashFunc(key) & (m_bucketNum - 1);
    }
    CBucket* bucket(UINT4 index)
    {
        return (index < m_bucketNum) ? &m_buckets[index] : NULL;
    }
    int  bucketLock(UINT4 index)
    {
        return (index < m_bucketNum) ? m_mutexs[index].lock() : EINVAL;
    }
    int  bucketTrylock(UINT4 index)
    {
        return (index < m_bucketNum) ? m_mutexs[index].trylock() : EINVAL;
    }
    void bucketUnlock(UINT4 index)
    {
        if (index < m_bucketNum) 
            m_mutexs[index].unlock();
    }

    CReferenceIterator begin() {return m_reference.begin();}
    CReferenceIterator end() {return m_reference.end();}

private:
    UINT4                m_bucketNum; /* bucket number of hash table */
    std::vector<CBucket> m_buckets;   /* hash table */
    std::vector<CMutex>  m_mutexs;    /* mutex for every bucket */
    HashFunc*            m_hashFunc;  /* key hash function */
    CReference           m_reference; /* for traverse purpose */
    CMutex               m_mutex;     /* mutex for reference */
};

template <class HASHDATA>
class CUint32LKHashMap : public CLKHashMap<UINT4, HASHDATA>
{
public:
    CUint32LKHashMap():CLKHashMap<UINT4, HASHDATA>(1, hashUint32) {    }

    CUint32LKHashMap(UINT4 bucketNum):CLKHashMap<UINT4, HASHDATA>(bucketNum, hashUint32) {}

    ~CUint32LKHashMap() {}
};

template <class HASHDATA>
class CUint64LKHashMap : public CLKHashMap<UINT8, HASHDATA>
{
public:
    CUint64LKHashMap():CLKHashMap<UINT8, HASHDATA>(1, hashUint64) {    }

    CUint64LKHashMap(UINT4 bucketNum):CLKHashMap<UINT8, HASHDATA>(bucketNum, hashUint64) {}

    ~CUint64LKHashMap() {}
};

template <class HASHDATA>
class CStringLKHashMap : public CLKHashMap<std::string, HASHDATA>
{
public:
    CStringLKHashMap():CLKHashMap<std::string, HASHDATA>(1, hashString) {    }

    CStringLKHashMap(UINT4 bucketNum):CLKHashMap<std::string, HASHDATA>(bucketNum, hashString) {}

    ~CStringLKHashMap() {}
};

#endif /* __CHASHMAP_H */
