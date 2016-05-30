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

#include "../inc/ini.h"
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>

#define ADD_ELEMENT_TO_LIST(list, element_type, element)    \
do                                                          \
{                                                           \
    element_type *tmp = list;                               \
    if(NULL == tmp)                                         \
    {                                                       \
        list = element;                                     \
    }                                                       \
    else                                                    \
    {                                                       \
        while(tmp->next)                                    \
        {                                                   \
            tmp = tmp->next;                                \
        }                                                   \
        tmp->next = element;                                \
        element->pre = tmp;                                 \
    }                                                       \
}while(0);

static void* gn_malloc (size_t size)
{
    void* mem = malloc(size);
    memset(mem, 0, size);
    return mem;
}

static void gn_free(void **ptr)
{
    free(*ptr);
    *ptr = NULL;
}

//read all the configuration from the ".ini" file
ini_file_t* read_ini(const char *path)
{
    char *pos = NULL;
    char buf[1024] = {0};
    ini_file_t *doc = NULL;
    ini_selection_t *sec = NULL;
    ini_item_t *item = NULL;
    ini_comment_t *comm_list = NULL;
    ini_comment_t *comm = NULL;

    FILE *fp = fopen(path, "r");
    if (NULL == fp)
    {
        return NULL;
    }

    doc = (ini_file_t*)gn_malloc(sizeof(ini_file_t));
    if (NULL == doc)
    {
        return NULL;
    }

    doc->fd = fp;
    while(fgets(buf, 1023, fp))
    {
        for(pos = buf; '\0' != *pos; pos++)
        {
           if(('\n' == *pos) || ('\r' == *pos))
           {
               *pos = '\0';
           }
        }

        switch(buf[0])
        {
            case '\n':
            {
                break;
            }
            case '\0':
            {
                break;
            }
            case '#':
            {
                comm = (ini_comment_t *)gn_malloc(sizeof(ini_comment_t));
                if(NULL == comm)
                {
                    goto EXCEPTION_EXIT;
                }

                comm->desc = strdup(buf);
                ADD_ELEMENT_TO_LIST(comm_list, ini_comment_t, comm);
                break;
            }
            case ';':
            {
                comm = (ini_comment_t *)gn_malloc(sizeof(ini_comment_t));
                if(NULL == comm)
                {
                    goto EXCEPTION_EXIT;
                }

                comm->desc = strdup(buf);
                ADD_ELEMENT_TO_LIST(comm_list, ini_comment_t, comm);
                break;
            }
            case '[':
            {
                sec = (ini_selection_t*)gn_malloc(sizeof(ini_selection_t));
                if(NULL == sec)
                {
                    goto EXCEPTION_EXIT;
                }

                sec->selection = strdup(buf);
                sec->comments = comm_list;
                comm_list = NULL;

                ADD_ELEMENT_TO_LIST(doc->selections, ini_selection_t, sec);
                break;
            }
            default:
            {
                if(NULL == sec)
                {
                    goto EXCEPTION_EXIT;
                }

                item = (ini_item_t *)gn_malloc(sizeof(ini_item_t));
                if(NULL == item)
                {
                    goto EXCEPTION_EXIT;
                }

                pos = buf;
                do
                {
                    if('\0' == *pos++)
                    {
                        goto EXCEPTION_EXIT;
                    }
                }while('=' != *pos);

                *pos = '\0';
                pos++;

                item->name = strdup(buf);
                item->value = strdup(pos);
                item->comments = comm_list;
                comm_list = NULL;

                ADD_ELEMENT_TO_LIST(sec->ini_items, ini_item_t, item);
                break;
            }
        }
    }

    return doc;

EXCEPTION_EXIT:
    close_ini(&doc);
    return NULL;
}

//close the ".ini" file
void close_ini(ini_file_t **ini_file)
{
    ini_file_t *doc = *ini_file;
    ini_selection_t *sec_tmp = NULL;
    ini_comment_t *comm_tmp = NULL;
    ini_item_t *item_tmp = NULL;

    if((NULL == ini_file) || (NULL == doc))
    {
        return;
    }

    if(doc->selections)
    {
        while(NULL != doc->selections)
        {
            sec_tmp = doc->selections;
            doc->selections = doc->selections->next;

            while(NULL != sec_tmp->comments)
            {
                comm_tmp = sec_tmp->comments;
                sec_tmp->comments = sec_tmp->comments->next;

                gn_free((void**)(&(comm_tmp->desc)));
                gn_free((void**)(&comm_tmp));
            }

            while(NULL != sec_tmp->ini_items)
            {
                item_tmp = sec_tmp->ini_items;
                sec_tmp->ini_items = sec_tmp->ini_items->next;

                while(NULL != item_tmp->comments)
                {
                    comm_tmp = item_tmp->comments;
                    item_tmp->comments = item_tmp->comments->next;

                    gn_free((void**)(&(comm_tmp->desc)));
                    gn_free((void**)(&comm_tmp));
                }

                gn_free((void**)(&(item_tmp->name)));
                gn_free((void**)(&(item_tmp->value)));
                gn_free((void**)(&item_tmp));
            }

            gn_free((void**)(&(sec_tmp->selection)));
            gn_free((void**)(&sec_tmp));
        }
    }

    fclose(doc->fd);
    gn_free((void**)ini_file);
    *ini_file = NULL;
}

void free_selections(ini_selection_t* selection_p)
{
	ini_selection_t* sec_tmp = selection_p;	
    ini_comment_t *comm_tmp = NULL;
    ini_item_t *item_tmp = NULL;

	while(NULL != sec_tmp->ini_items)
	{
		item_tmp = sec_tmp->ini_items;
		sec_tmp->ini_items = sec_tmp->ini_items->next;

		while(NULL != item_tmp->comments)
		{
			comm_tmp = item_tmp->comments;
			item_tmp->comments = item_tmp->comments->next;

			gn_free((void**)(&(comm_tmp->desc)));
			gn_free((void**)(&comm_tmp));
		}

		gn_free((void**)(&(item_tmp->name)));
		gn_free((void**)(&(item_tmp->value)));
		gn_free((void**)(&item_tmp));
	}

	gn_free((void**)(&(sec_tmp->selection)));
    gn_free((void**)(&sec_tmp));

}


int remove_selection(ini_file_t *ini_file, const char *selection)
{
	ini_selection_t *sec_tmp = NULL;

	if((NULL == ini_file) || (NULL == selection))
	{
		return 0;
	}

	if (ini_file->selections)
	{
		sec_tmp = ini_file->selections;
		while(sec_tmp)
		{
			if (0 == strcmp(sec_tmp->selection, selection)) {
				printf("[INFO] Remove selection: %s\n", selection);
				if (sec_tmp->pre) {
					sec_tmp->pre->next = sec_tmp->next;
				}
				else {
					ini_file->selections = sec_tmp->next;
				}

				free_selections(sec_tmp);
				sec_tmp = NULL;
				return 1;
			}
			sec_tmp = sec_tmp->next;
		}
	}
	return 0;
}

char *get_selection_by_selection(ini_file_t *ini_file, const char *name)
{
    ini_selection_t *sec_tmp = NULL;

    if ((NULL == ini_file) || (NULL == name))
    {
        return NULL;
    }

    if(ini_file->selections)
    {
        sec_tmp = ini_file->selections;
        while(sec_tmp)
        {
            if ((sec_tmp->selection) && (strlen(sec_tmp->selection)) && (0 == strcmp(name, sec_tmp->selection)))
		    {
		    	// printf("found %s\n", name);
		        return sec_tmp->selection;
		    }
			
            sec_tmp = sec_tmp->next;
        }
    }
	
	// printf("not found %s\n", name);
    return NULL;
}

char *get_selection_by_name_value(const ini_file_t *ini_file, const char *name, const char *value)
{
    ini_selection_t *sec_tmp = NULL;
    ini_item_t *item_tmp = NULL;

    if((NULL == ini_file) || (NULL == name) || (NULL == value))
    {
        return NULL;
    }

    if(ini_file->selections)
    {
        sec_tmp = ini_file->selections;
        while(sec_tmp)
        {
            item_tmp = sec_tmp->ini_items;
			while(item_tmp)
			{
			    if ((0 == strcmp(name, item_tmp->name)) && (0 == strcmp(value, item_tmp->value)))
			    {
			        return sec_tmp->selection;
			    }

			    item_tmp = item_tmp->next;
			}
            sec_tmp = sec_tmp->next;
        }
    }

    return NULL;
}


void print_value(const ini_file_t *ini_file, const char *selection, const char *item)
{
	ini_selection_t *sec_tmp = NULL;
	ini_item_t *item_tmp = NULL;

	if((NULL == ini_file) || (NULL == selection) || (NULL == item))
	{
		return ;
	}

	if (ini_file->selections)
	{
		sec_tmp = ini_file->selections;
		while(sec_tmp)
		{
			printf("%s\n", sec_tmp->selection);
			item_tmp = sec_tmp->ini_items;
			while(item_tmp)
			{
				printf("%s:%s\n", item_tmp->name, item_tmp->value);
				item_tmp = item_tmp->next;
			}

			sec_tmp = sec_tmp->next;
		}
	}
}



//get the value of the configure item
char *get_value(const ini_file_t *ini_file, const char *selection, const char *item)
{
    ini_selection_t *sec_tmp = NULL;
    ini_item_t *item_tmp = NULL;

    if((NULL == ini_file) || (NULL == selection) || (NULL == item))
    {
        return NULL;
    }

    if(ini_file->selections)
    {
        sec_tmp = ini_file->selections;
        while(sec_tmp)
        {
            if(0 == strcmp(selection, sec_tmp->selection))
            {
                item_tmp = sec_tmp->ini_items;
                while(item_tmp)
                {
                    if(0 == strcmp(item, item_tmp->name))
                    {
                        return item_tmp->value;
                    }

                    item_tmp = item_tmp->next;
                }

                return NULL;
            }

            sec_tmp = sec_tmp->next;
        }
    }

    return NULL;
}

// set the value of the configure item
int set_value(const ini_file_t *ini_file, const char *selection, const char *item, char* save_value)
{
    ini_selection_t *sec_tmp = NULL;
    ini_item_t *item_tmp = NULL;
    ini_selection_t *sec_new = NULL;
    ini_file_t *doc = (ini_file_t *)ini_file;
    ini_item_t *item_new = NULL;

    if((NULL == ini_file) || (NULL == selection) || (NULL == item))
    {
        return 0;
    }

    // judge the selection exists
    if (ini_file->selections) {
    	sec_tmp = ini_file->selections;
    	while (sec_tmp) {
    		if(0 == strcmp(selection, sec_tmp->selection))
			{
				break;
			}
			sec_tmp = sec_tmp->next;
    	}
    }

    if (NULL == sec_tmp) {
    	// add selections to list
    	sec_new = (ini_selection_t*)gn_malloc(sizeof(ini_selection_t));
		if(NULL == sec_new)
		{
			return 0;
		}

		sec_new->selection = strdup(selection);
		ADD_ELEMENT_TO_LIST(doc->selections, ini_selection_t, sec_new);
    }

    if(ini_file->selections)
    {
        sec_tmp = ini_file->selections;
        while(sec_tmp)
        {
            if(0 == strcmp(selection, sec_tmp->selection))
            {
                item_tmp = sec_tmp->ini_items;
                while(item_tmp)
                {
                    if(0 == strcmp(item, item_tmp->name))
                    {
                        strcpy(item_tmp->value, save_value);
                        return 1;
                    }

                    item_tmp = item_tmp->next;
                }

                // add item
                if (NULL == item_tmp) {

                	item_new = (ini_item_t *)gn_malloc(sizeof(ini_item_t));
					if(NULL == item_new)
					{
						return 0;
					}

					item_new->name = strdup(item);
					item_new->value = strdup(save_value);
					item_new->comments = NULL;

					ADD_ELEMENT_TO_LIST(sec_tmp->ini_items, ini_item_t, item_new);
                }
            }

            sec_tmp = sec_tmp->next;
        }
    }

    return 0;
}

// set_value_by_int
int set_value_int(const ini_file_t *ini_file, const char *selection, const char *item, unsigned long long int save_value)
{
	int return_value = 0;
	char temp_value[64];
	sprintf(temp_value, "%llu", save_value);
	return_value = set_value(ini_file, selection, item, temp_value);
	return return_value;
}

// set value ip
int set_value_ip(const ini_file_t *ini_file, const char *selection, const char *item, unsigned long int save_value)
{
	int return_value = 0;
	struct in_addr addr;
	memcpy(&addr, &save_value, 4);
	return_value = set_value(ini_file, selection, item, inet_ntoa(addr));
	return return_value;
}

// set value mac
int set_value_mac(const ini_file_t *ini_file, const char *selection, const char *item, unsigned char* save_value)
{
	int return_value = 0;
	char temp[16] = {0};
	sprintf(temp, "%02x:%02x:%02x:%02x:%02x:%02x",save_value[0], save_value[1],\
			save_value[2],save_value[3],save_value[4],save_value[5]);
	return_value = set_value(ini_file, selection, item, temp);
	return return_value;
}

//write ini_file to the file
ini_file_t* save_ini(ini_file_t *ini_file, const char *path)
{
    ini_file_t *doc = (ini_file_t *)ini_file;
    ini_selection_t *sec_tmp = NULL;
    ini_comment_t *comm_tmp = NULL;
    ini_item_t *item_tmp = NULL;
    char buf[1024] = {0};

    if((NULL == ini_file) || (NULL == ini_file->fd) || (NULL == path))
    {
    	printf("Ini save file error! The parameter is empty or configure file is wrong!\n");
        return NULL;
    }

    fclose(ini_file->fd);

    FILE *fp = fopen(path, "w");
	if (NULL == fp)
	{
		return NULL;
	}
	ini_file->fd = fp;

    if(doc->selections)
    {
        while(NULL != doc->selections)
        {
            sec_tmp = doc->selections;
            doc->selections = doc->selections->next;

            while(NULL != sec_tmp->comments)
            {
                comm_tmp = sec_tmp->comments;
                sec_tmp->comments = sec_tmp->comments->next;

                fputs(comm_tmp->desc, ini_file->fd);
                fputs("\n", ini_file->fd);
            }
            fputs(sec_tmp->selection, ini_file->fd);
            fputs("\n", ini_file->fd);

            while(NULL != sec_tmp->ini_items)
            {
                item_tmp = sec_tmp->ini_items;
                sec_tmp->ini_items = sec_tmp->ini_items->next;
                while(NULL != item_tmp->comments)
                {
                    comm_tmp = item_tmp->comments;
                    item_tmp->comments = item_tmp->comments->next;

                    fputs(comm_tmp->desc, ini_file->fd);
                    fputs("\n", ini_file->fd);
                }

                strcpy(buf,item_tmp->name);
                strcat(buf,"=");
                strcat(buf,item_tmp->value);
                strcat(buf,"\n");
                fputs(buf , ini_file->fd);
            }

            fputs("\n", ini_file->fd);
        }
    }

    fflush(ini_file->fd);
    close_ini(&doc);

    ini_file_t *new_doc = read_ini(path);
    ini_file = new_doc;
    return new_doc;
}
