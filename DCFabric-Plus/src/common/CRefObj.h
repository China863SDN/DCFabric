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
*   File Name   : CRefObj.h                                                   *
*   Author      : bnc bojiang                                                 *
*   Create Date : 2017-10-30                                                  *
*   Version     : 1.0                                                         *
*   Function    :                                                             *
*                                                                             *
******************************************************************************/
#ifndef __CREFOBJ_H
#define __CREFOBJ_H

#include "bnc-type.h"

class CMutex;

/*
 * 引用计数�?
 */
class CRefObj
{
public:
    CRefObj(BOOL isThreadSafe=TRUE);
    ~CRefObj();

    INT4 GetRefCount() const;
    INT4 AddRefCount();
    INT4 SubRefCount();
    void ResetRefCount();

private:
    INT4 m_iRefCount;
    CMutex* m_pMutex;
    BOOL m_bIsThreadSafe;
};

#endif
