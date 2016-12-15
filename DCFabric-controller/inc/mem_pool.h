/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
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
*   File Name   : mem_pool.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef MEM_POOL_H_
#define MEM_POOL_H_

#include "common.h"


/*
 *   Author      : DengChao
 *   Create Date : 2014-12-22
 *   Input Args  : block: block counts; len: memory size of each block;
 *   Output Args :
 *   Return      : memory pool
 *   History     :
 */
void *mem_create(UINT4 block , UINT4 len);

/*
 *   Author      : DengChao
 *   Create Date : 2014-12-22
 *   Input Args  : pool: memory pool
 *   Output Args :
 *   Return      : memory unit/block
 *   History     :
 */
void *mem_get(void *pool);

/*
 *   Author      : DengChao
 *   Create Date : 2014-12-22
 *   Input Args  : pool: memory pool; data: the memory unit/bloc to free;
 *   Output Args :
 *   Return      : 0 - succeed; !0 - failed
 *   History     :
 */
int mem_free(void *pool ,void *data);

/*
 *   Author      : DengChao
 *   Create Date : 2014-12-22
 *   Input Args  : pool: memory pool;
 *   Output Args :
 *   Return      :
 *   History     :
 */
void mem_destroy(void *pool);

/*
 *   Author      : DengChao
 *   Create Date : 2014-12-22
 *   Input Args  : pool: memory pool;
 *   Output Args :
 *   Return      : the availiable blocks of the pool
 *   History     :
 */
UINT4 mem_num(void *pool);

#endif /* MEM_POOL_H_ */
