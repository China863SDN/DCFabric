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
*   File Name   : CLoopBuffer.cpp                                             *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#include "CLoopBuffer.h"
#include "bnc-error.h"
#include "log.h"

#define LOOP_BUFFER_SIZE        1024*100 //100K
#define LOOP_BUFFER_EXT_SIZE    1024*100 //100K

CLoopBuffer::CLoopBuffer(BOOL isThreadSafe):
    m_isThreadSafe(isThreadSafe),
    m_totalLen(LOOP_BUFFER_SIZE),
    m_curLen(0)
{
    m_buffer = (INT1*)malloc(LOOP_BUFFER_SIZE);
    if (NULL == m_buffer)
        LOG_ERROR_FMT("CLoopBuffer alloc %d bytes failed!", LOOP_BUFFER_SIZE);
    m_head = m_buffer;
}

CLoopBuffer::CLoopBuffer(UINT4 size, BOOL isThreadSafe):
    m_isThreadSafe(isThreadSafe),
    m_totalLen(size),
    m_curLen(0)
{
    m_buffer = (INT1*)malloc(size);
    if (NULL == m_buffer)
        LOG_ERROR_FMT("CLoopBuffer alloc %u bytes failed!", size);
    m_head = m_buffer;
}

CLoopBuffer::~CLoopBuffer()
{
    if (NULL != m_buffer)
    {
        free(m_buffer);
        m_buffer = NULL;
    }
}

BOOL CLoopBuffer::write(const INT1* data, UINT4 len)
{
    if (len == 0)
        return FALSE;
    
    if (m_isThreadSafe)
        m_mutex.lock();

	if (m_curLen + len > m_totalLen)
	{
	    INT1 *old = m_buffer;
        UINT4 moreSpace = m_curLen + len - m_totalLen;
		moreSpace = (moreSpace > LOOP_BUFFER_EXT_SIZE) ? moreSpace : LOOP_BUFFER_EXT_SIZE;

        LOG_WARN_FMT("CLoopBuffer realloc %d bytes ...", m_totalLen + moreSpace);

        m_buffer = (INT1*)malloc(m_totalLen + moreSpace);
		if (NULL == m_buffer)
		{
            LOG_ERROR_FMT("CLoopBuffer realloc %d bytes failed!", m_totalLen + moreSpace);
            m_buffer = old;
            if (m_isThreadSafe)
                m_mutex.unlock();
			return FALSE;
		}

		if (m_head + m_curLen >= old + m_totalLen)
		{
		    UINT4 secondLen = m_head + m_curLen - (old + m_totalLen);
			memcpy(m_buffer, m_head, (m_curLen - secondLen));
		    memcpy(m_buffer + m_curLen - secondLen, old, secondLen);
	    }
		else
		{
		    memcpy(m_buffer, m_head, m_curLen);	
		}

		m_totalLen += moreSpace;
        m_head = m_buffer;

        free(old);
	}

	if (m_head + m_curLen + len <= m_buffer + m_totalLen)
    {
        memcpy(m_head + m_curLen, data, len);
    }
    else
	{
		if (m_head + m_curLen >= m_buffer + m_totalLen)
		{
			UINT4 pos = (m_head + m_curLen) - (m_buffer + m_totalLen);
			memcpy(m_buffer + pos, data, len);
		}
		else
		{
			UINT4 firstLen = m_buffer + m_totalLen - (m_head + m_curLen);
			if (firstLen >= len)
            {
                memcpy(m_head + m_curLen, data, len);
            }
            else
            {
                memcpy(m_head + m_curLen, data, firstLen);
                memcpy(m_buffer, data + firstLen, len - firstLen);
            }
		}
	}

	m_curLen += len;

    if (m_isThreadSafe)
        m_mutex.unlock();

    return TRUE;
}

BOOL CLoopBuffer::read(INT1* buffer, UINT4 len, BOOL peek)
{
	if (NULL == buffer)
	{
        LOG_WARN("buffer is null when read CLoopBuffer !");
		return FALSE;
	}

	if (m_curLen < len)
	{
        LOG_INFO_FMT("len[%u] exceed m_curLen[%u] when read CLoopBuffer !", len, m_curLen);
		return FALSE;
	}

    if (m_isThreadSafe)
        m_mutex.lock();

	if (m_head + m_curLen > m_buffer + m_totalLen)
	{//回头
		//LOG_INFO("-----------------buffer circle-----------------");
		UINT4 secondLen = m_head + m_curLen - (m_buffer + m_totalLen);
	    UINT4 firstLen = m_curLen - secondLen;
		
		if (len >= firstLen)
		{//回头读取
			memcpy(buffer, m_head, firstLen);
			memcpy(buffer + firstLen, m_buffer, len - firstLen);
			if (!peek)
			{
				m_head = m_buffer + len - firstLen;
				m_curLen -= len;
			}	
		}
		else
		{//尾部读取
			memcpy(buffer, m_head, len);
			if (!peek)
			{
				m_head += len;
                m_curLen -= len;
			}	
		}
	}
	else
	{
        memcpy(buffer, m_head, len);
		if (!peek)
		{
            m_head += len;
			if (m_head == (m_buffer + m_totalLen))
			{
				m_head = m_buffer;	
			}
            m_curLen -= len;
		}	
	}
	
    if (m_isThreadSafe)
        m_mutex.unlock();
	
	return TRUE;
}

void CLoopBuffer::reset()
{
    if (m_isThreadSafe)
        m_mutex.lock();

    m_curLen = 0;
    m_head = m_buffer;

    if (m_isThreadSafe)
        m_mutex.unlock();
}

void CLoopBuffer::clear(UINT4 len)
{
	if (m_curLen <= len)
    {
        reset();
        return;
    }

    if (m_isThreadSafe)
        m_mutex.lock();

	if (m_head + m_curLen > m_buffer + m_totalLen)
	{//回头
		//LOG_INFO("-----------------buffer circle-----------------");
		UINT4 secondLen = m_head + m_curLen - (m_buffer + m_totalLen);
	    UINT4 firstLen = m_curLen - secondLen;
		
		if (len >= firstLen)
		{//回头清除
            m_head = m_buffer + len - firstLen;
            m_curLen -= len;
		}
		else
		{//尾部清除
            m_head += len;
            m_curLen -= len;
		}
	}
	else
	{
        m_head += len;
        if (m_head == (m_buffer + m_totalLen))
        {
            m_head = m_buffer;  
        }
        m_curLen -= len;
	}
	
    if (m_isThreadSafe)
        m_mutex.unlock();
}

