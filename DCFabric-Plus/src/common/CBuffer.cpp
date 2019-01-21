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
*   File Name   : CBuffer.cpp           *
*   Author      : bnc zgzhao           *
*   Create Date : 2016-5-25           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/
#include "CBuffer.h"
#include "bnc-error.h"
#include "log.h"

#define BUFFER_DEF_SIZE    1024*100 //100K
#define BUFFER_EXT_SIZE    1024*100 //100K

CBuffer::CBuffer():m_size(BUFFER_DEF_SIZE),m_len(0)
{
    m_buffer = (INT1*)malloc(BUFFER_DEF_SIZE);
    if (NULL == m_buffer)
        LOG_ERROR_FMT("CBuffer alloc %d bytes failed!", BUFFER_DEF_SIZE);
}

CBuffer::CBuffer(UINT4 size):m_size(size),m_len(0)
{
    m_buffer = (INT1*)malloc(size);
    if (NULL == m_buffer)
        LOG_ERROR_FMT("CBuffer alloc %d bytes failed!", size);
}

CBuffer::~CBuffer()
{
    if (NULL != m_buffer)
    {
        free(m_buffer);
        m_buffer = NULL;
    }
}

UINT4 CBuffer::write(const INT1* data, UINT4 len)
{
    if (0 == len)
        return 0;
    
	if (m_len + len > m_size)
	{
        UINT4 moreSpace = m_len + len - m_size;
		moreSpace = (moreSpace > BUFFER_EXT_SIZE) ? moreSpace : BUFFER_EXT_SIZE;

        LOG_WARN_FMT("CBuffer realloc %d bytes ...", m_size + moreSpace);

        m_buffer = (INT1*)realloc(m_buffer, m_size + moreSpace);
		if (NULL == m_buffer)
		{
            LOG_ERROR_FMT("CBuffer realloc %d bytes failed !", m_size + moreSpace);
			return 0;
		}
	}

    memcpy(m_buffer+m_len, data, len);
    return len;
}

UINT4 CBuffer::read(INT1* buffer, UINT4 len)
{
    UINT4 ret = 0;

    if (0 == len)
        return ret;
    
	if (m_len > len)
	{
        memcpy(buffer, m_buffer, len);
        memcpy(m_buffer, m_buffer+len, m_len-len);
        ret = len;
	}
    else
	{
        memcpy(buffer, m_buffer, m_len);
	    ret = m_len;
        m_len = 0;
	}

    return ret;
}

