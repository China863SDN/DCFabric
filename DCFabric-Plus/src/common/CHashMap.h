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
*   File Name   : CHashMap.h                                                  *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CHASHMAP_H
#define __CHASHMAP_H

#include "bnc-type.h"
#include "comm-util.h"
//#include "CMutex.h"

template <class HASHKEY, class HASHDATA>
class CHashMap
{
public:
    typedef std::map<HASHKEY, HASHDATA> CBucket;
    typedef std::pair<HASHKEY, HASHDATA> CBucketPair;
    typedef typename CBucket::iterator CBucketIterator;
    typedef UINT4 (HashFunc)(const HASHKEY& key);
    typedef void (CopyFunc)(HASHDATA& rhs, HASHDATA& lhs);

public:
    CHashMap():m_bucketNum(0),m_count(0),m_hashFunc(NULL),m_copyFunc(NULL) {}
    CHashMap(UINT4   bucketNum, HashFunc* hashFunc, CopyFunc* copyFunc=NULL):
        m_bucketNum(bucketNum),m_buckets(bucketNum),m_count(0),m_hashFunc(hashFunc),
        m_copyFunc(copyFunc) {}
    virtual ~CHashMap()    {}

    CHashMap& operator=(const CHashMap& rhs)
    {
        if (m_bucketNum == rhs.m_bucketNum)
        {
            m_count = rhs.m_count;
            m_hashFunc = rhs.m_hashFunc;
            m_copyFunc = rhs.m_copyFunc;

            if (NULL != m_copyFunc)
            {
                for (UINT4 i = 0; i < m_bucketNum; i++)
                {
                    if (rhs.m_buckets[i].empty())
                        continue;

                    STL_FOR_LOOP(rhs.m_buckets[i], it)
                    {
                        HASHDATA data;
                        m_copyFunc((HASHDATA&)it->second, data);
                        m_buckets[i].insert(CBucketPair(it->first, data));
                    }
                }
            }
            else
            {
                m_buckets = rhs.m_buckets;
            }
        }

        return *this;
    }

    HASHDATA* insert(const HASHKEY& key, const HASHDATA& data)
    {
        HASHDATA* ret = NULL;

        if (NULL != m_hashFunc)
        {
            UINT4 bucketNo = m_hashFunc(key) & (m_bucketNum - 1);
    
            //m_mutexs[bucketNo].lock();

            std::pair<CBucketIterator, bool> result =
                m_buckets[bucketNo].insert(CBucketPair(key, data));

            //m_mutexs[bucketNo].unlock();

            if (result.second)
            {
                m_count ++;
                ret = &(result.first->second);
            }
        }
    
        return ret;
    }

    void remove(const HASHKEY& key)
    {
        if (NULL != m_hashFunc)
        {
            UINT4 bucketNo = m_hashFunc(key) & (m_bucketNum - 1);    
            //m_mutexs[bucketNo].lock();
            m_buckets[bucketNo].erase(key);
            m_count --;
            //m_mutexs[bucketNo].unlock();
        }
    }

    void remove_front()
    {
        BOOL done = FALSE;

        for (UINT4 i = 0; i < m_bucketNum; i++)
        {
            //m_mutexs[i].lock();

            if (!m_buckets[i].empty())
            {
                CBucketIterator it = m_buckets[i].begin();
                m_buckets[i].erase(it);
                m_count --;
                done = TRUE;
            }

            //m_mutexs[i].unlock();
            
            if (done)
                break;
        }
    }

    BOOL pop(const HASHKEY& key, HASHDATA& data)
    {
        BOOL done = FALSE;

        if (NULL != m_hashFunc)
        {
            UINT4 bucketNo = m_hashFunc(key) & (m_bucketNum - 1);
    
            //m_mutexs[bucketNo].lock();

            CBucketIterator it = m_buckets[bucketNo].find(key);
            if (it != m_buckets[bucketNo].end())
            {
                data = it->second;
                m_buckets[bucketNo].erase(it);
                m_count --;
                done = TRUE;
            }
        
            //m_mutexs[bucketNo].unlock();
        }

        return done;
    }

    BOOL pop_front(HASHKEY& key, HASHDATA& data)
    {
        BOOL done = FALSE;

        for (UINT4 i = 0; i < m_bucketNum; i++)
        {
            //m_mutexs[i].lock();

            if (!m_buckets[i].empty())
            {
                CBucketIterator it = m_buckets[i].begin();
                key = it->first;
                data = it->second;
                m_buckets[i].erase(it);
                m_count --;
                done = TRUE;
            }

            //m_mutexs[i].unlock();
            
            if (done)
                break;
        }
        
        return done;
    }

    HASHDATA* find(const HASHKEY& key)
    {
        HASHDATA* found = NULL;

        if (NULL != m_hashFunc)
        {
            UINT4 bucketNo = m_hashFunc(key) & (m_bucketNum - 1);
    
            //m_mutexs[bucketNo].lock();

            CBucketIterator it = m_buckets[bucketNo].find(key);
            if (it != m_buckets[bucketNo].end())
                found = &(it->second);
            
            //m_mutexs[bucketNo].unlock();
        }
        
        return found;
    }
    
    HASHDATA* front(HASHKEY& key)
    {
        HASHDATA* found = NULL;

        for (UINT4 i = 0; i < m_bucketNum; i++)
        {
            //m_mutexs[i].lock();

            if (!m_buckets[i].empty())
            {
                CBucketIterator it = m_buckets[i].begin();
                key = it->first;
                found = &(it->second);
            }

            //m_mutexs[i].unlock();

            if (NULL != found)
                break;
        }
    
        return found;
    }
    
    void clear()
    {
        STL_FOR_LOOP(m_buckets, vit)
        {
            CBucket& bucket = *vit;
            bucket.clear();
        }
        m_count = 0;
    }

    size_t size()  {return m_count;}
    BOOL empty()   {return (m_count == 0);}
    UINT4 bucketNum()    {return m_bucketNum;}
    CBucket* bucket(UINT4 no)
    {
        return (no < m_bucketNum) ? &m_buckets[no] : NULL;
    }

private:
    UINT4                m_bucketNum; /* bucket number of hash table */
    std::vector<CBucket> m_buckets;   /* hash table */
    UINT4                m_count;     /* data count */
    //std::vector<CMutex>  m_mutexs;    /* mutex for every map */
    HashFunc*            m_hashFunc;  /* key hash function */
    CopyFunc*            m_copyFunc;  /* data copy function */
};

template <class HASHDATA>
class CUint32HashMap : public CHashMap<UINT4, HASHDATA>
{
public:
    CUint32HashMap():CHashMap<UINT4, HASHDATA>(1, hashUint32) {    }

    CUint32HashMap(UINT4 bucketNum):CHashMap<UINT4, HASHDATA>(bucketNum, hashUint32) {}

    ~CUint32HashMap() {}
};

template <class HASHDATA>
class CUint64HashMap : public CHashMap<UINT8, HASHDATA>
{
public:
    CUint64HashMap():CHashMap<UINT8, HASHDATA>(1, hashUint64) {    }

    CUint64HashMap(UINT4 bucketNum):CHashMap<UINT8, HASHDATA>(bucketNum, hashUint64) {}

    ~CUint64HashMap() {}
};

template <class HASHDATA>
class CStringHashMap : public CHashMap<std::string, HASHDATA>
{
public:
    CStringHashMap():CHashMap<std::string, HASHDATA>(1, hashString) {    }

    CStringHashMap(UINT4 bucketNum):CHashMap<std::string, HASHDATA>(bucketNum, hashString) {}

    ~CStringHashMap() {}
};

#endif /* __CHASHMAP_H */
