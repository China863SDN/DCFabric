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
*   File Name   : meter-mgr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-10           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef METER_MGR_H_
#define METER_MGR_H_

#include "gnflush-types.h"

INT4 add_meter_entry(gn_switch_t *sw, gn_meter_t *meter);
INT4 modify_meter_entry(gn_switch_t *sw, gn_meter_t *meter);
INT4 delete_meter_entry(gn_switch_t *sw, gn_meter_t *meter);

void clear_meter_entries(gn_switch_t *sw);

INT4 init_meter_mgr();
void fini_meter_mgr();

#endif /* METER_MGR_H_ */
