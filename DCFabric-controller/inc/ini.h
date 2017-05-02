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
*   File Name   : ini.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef INI_H_
#define INI_H_

#include <stdio.h>

typedef struct ini_comment
{
    char *desc;
    struct ini_comment *pre;
    struct ini_comment *next;
}ini_comment_t;

typedef struct ini_item
{
    char *name;
    char *value;
    ini_comment_t *comments;
    struct ini_item *pre;
    struct ini_item *next;
}ini_item_t;

typedef struct ini_selection
{
    char *selection;
    ini_comment_t *comments;
    ini_item_t *ini_items;
    struct ini_selection *pre;
    struct ini_selection *next;
}ini_selection_t;

typedef struct ini_file
{
    void *fd;
    ini_selection_t *selections;
}ini_file_t;



/*
 *   Author      : dengchao
 *   Create Date : 2014-12-12
 *   Input Args  : path - the absolute path and filename of the ".ini" configure file
 *   Output Args :
 *   Return      : return the fd of the ".ini" configure file
 *   History     :
 */
ini_file_t* read_ini(const char *path);

/*
 *   Author      : dengchao
 *   Create Date : 2014-12-12
 *   Input Args  : ini_file - the ".ini" file pointer
 *   Output Args :
 *   Return      :
 *   History     :
 */
void close_ini(ini_file_t **ini_file);

/*
 *   Author      : dengchao
 *   Create Date : 2014-12-12
 *   Input Args  : ini_file - the ".ini" file pointer
 *                 selection - the name of the selection
 *                 item - the name of the item
 *   Output Args :
 *   Return      :
 *   History     :
 */

int remove_selection(ini_file_t *ini_file, const char *selection);
char *get_selection_by_selection(ini_file_t *ini_file, const char *name);
char *get_selection_by_name_value(const ini_file_t *ini_file, const char *name, const char *value);
char *get_value(const ini_file_t *ini_file, const char *selection, const char *item);

int set_value(const ini_file_t *ini_file, const char *selection, const char *item, char* save_value);

int set_value_int(const ini_file_t *ini_file, const char *selection, const char *item, unsigned long long int save_value);

int set_value_ip(const ini_file_t *ini_file, const char *selection, const char *item, unsigned long int save_value);

int set_value_mac(const ini_file_t *ini_file, const char *selection, const char *item, unsigned char* save_value);
/*
 *   Author      : dengchao
 *   Create Date : 2014-12-12
 *   Input Args  : ini_file - the ".ini" file pointer
 *                 path - the absolute path and filename of the ".ini" configure file
 *   Output Args :
 *   Return      : return the fd of the ".ini" configure file
 *   History     :
 *   Attention   : the input param ini_file will be invaild, please use the return value as the ini_file object pointer.
 */
ini_file_t* save_ini(ini_file_t *ini_file, const char *path);

#endif /* INI_H_ */
