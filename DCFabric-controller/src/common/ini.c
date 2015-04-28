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

//write ini_file to the file
int save_ini(const ini_file_t *ini_file, const char *path)
{
    ini_file_t *doc = (ini_file_t *)ini_file;
    ini_selection_t *sec_tmp = NULL;
    ini_comment_t *comm_tmp = NULL;
    ini_item_t *item_tmp = NULL;
    char buf[1024] = {0};

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

                fputs(buf , ini_file->fd);
            }

            fputs("\n", ini_file->fd);
        }
    }

    fflush(ini_file->fd);
    return 0;
}
