/*
 * DCFabric GPL Source Code
 * Copyright (C) 2015, BNC <DCFabric-admin@bnc.org.cn>
 *
 * This file is part of the DCFabric SDN Controller. DCFabric SDN
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

/*
 *  debug_svr.c
 *
 *  Created on: Feb 29, 2016
 *  Author: BNC administrator
 *  E-mail: DCFabric-admin@bnc.org.cn
 */
#include "json_server.h"


INT1 *fabric_debug_get_all_host();

INT1 *fabric_debug_get_all_sw();

INT1 *fabric_debug_get_all_path();

INT1 *fabric_debug_get_all_loadbalance_pool();

INT1 *fabric_debug_get_all_loadbalance_member();

INT1 *fabric_debug_get_all_loadbalance_listener();

INT1 *fabric_debug_get_all_securitygroup();

INT1 *fabric_debug_get_all_hostsecurity();

INT1 *fabric_debug_clear_all_security(const INT1 *url, json_t *root);

INT1 *fabric_debug_reload_all_security(const INT1 *url, json_t *root);

INT1 *fabric_debug_clear_all_loadbalance(const INT1 *url, json_t *root);

INT1 *fabric_debug_reload_all_loadbalance(const INT1 *url, json_t *root);



 

