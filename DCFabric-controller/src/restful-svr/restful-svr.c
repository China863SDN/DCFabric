/******************************************************************************
*                                                                             *
*   File Name   : restful-svr.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "restful-svr.h"
#include "json_server.h"
#include "timer.h"

pthread_t g_restful_pid;
UINT4 g_restful_port = 8081;
static INT1 *g_rest_reply = NULL;

restful_handles_t g_restful_get_handles[REST_CAPACITY];
restful_handles_t g_restful_post_handles[REST_CAPACITY];
restful_handles_t g_restful_put_handles[REST_CAPACITY];
restful_handles_t g_restful_delete_handles[REST_CAPACITY];

struct connection_info
{
    UINT4 connection_type;
    UINT8 connection_time;
};

static BOOL is_json(const INT1 *json_string, UINT4 string_len)
{
    UINT4 brace_left_cnt = 0;
    UINT4 brace_right_cnt = 0;
    UINT4 offset = 0;

    for(; offset < string_len; offset++)
    {
        if(json_string[offset] == '{')
        {
            brace_left_cnt++;
        }
        else if(json_string[offset] == '}')
        {
            brace_right_cnt++;
        }
    }

    if((brace_left_cnt > 0) && (brace_left_cnt == brace_right_cnt))
    {
        return TRUE;
    }

    return FALSE;
}

static void request_completed(void *cls, struct MHD_Connection *connection,
        void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct connection_info *conn_info = *con_cls;

    free(conn_info);
    *con_cls = NULL;
}

static INT4 send_page (struct MHD_Connection *connection, const INT1 *page, INT4 status_code)
{
    INT4 ret;
    struct MHD_Response *response;

    if (NULL == page)
    {
        return MHD_NO;
    }

    response = MHD_create_response_from_buffer(strlen(page), (void *) page, MHD_RESPMEM_MUST_COPY);
    if (!response)
    {
        return MHD_NO;
    }

    ret = MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret = MHD_queue_response(connection, status_code, response);

    MHD_destroy_response(response);
    return ret;
}

static INT1 *proc_rest_msg(const INT1 *method, const INT1 *url, const INT1 *upload_data)
{
    json_t *root = NULL;
    INT4 parse_type = 0;
    INT1 *reply = NULL;

    if (upload_data)
    {
        parse_type = json_parse_document(&root, upload_data);
        if (parse_type != JSON_OK)
        {
            return json_to_reply(NULL, EC_RESTFUL_INVALID_REQ);
        }
    }

    if (strncmp(method, "GET", 3) == 0)
    {
        reply = proc_restful_request(HTTP_GET, url, root);
    }
    else if (strncmp(method, "POST", 4) == 0)
    {
        reply = proc_restful_request(HTTP_POST, url, root);
    }
    else if (strncmp(method, "PUT", 3) == 0)
    {
        reply = proc_restful_request(HTTP_PUT, url, root);
    }
    else if (strncmp(method, "DELETE", 6) == 0)
    {
        reply = proc_restful_request(HTTP_DELETE, url, root);
    }

    return reply;
}

static int answer_to_connection(void *cls, struct MHD_Connection *connection,
        const INT1 *url, const char *method, const INT1 *version,
        const INT1 *upload_data, size_t *upload_data_size, void **con_cls)
{
    INT4 ret = 0;
    UINT4 idx = 0;
    if (NULL == *con_cls)
    {
        struct connection_info *conn_info = (struct connection_info *)gn_malloc(sizeof(struct connection_info));
        if(NULL == conn_info)
        {
            return MHD_NO;
        }

        conn_info->connection_time = g_cur_sys_time.tv_sec;
        *con_cls = (void *)conn_info;
        return MHD_YES;
    }

    if (0 == strcmp(method, "GET"))
    {
        if (0 != strcmp(url, "/gn/cluster/query/json")) {
            LOG_PROC("INFO", "Restful[%s]: [%s]", method, url);
        }
           
        g_rest_reply = proc_rest_msg(method, url, upload_data);
        //todo 判空
        ret = send_page(connection, g_rest_reply, MHD_HTTP_OK);     //回复消息
        memset((char *) upload_data, 0x0, *upload_data_size);    //清空缓冲区
        *upload_data_size = 0;
        upload_data = NULL;

        free(g_rest_reply);
        g_rest_reply = NULL;
        return ret;
    }

    if (0 == strcmp(method, "POST") || 0 == strcmp(method, "DELETE")
            || 0 == strcmp(method, "PUT") || 0 == strcmp(method, "PATCH")
            || 0 == strcmp(method, "HEAD"))
    {
        struct connection_info *conn_info = *con_cls;
        conn_info->connection_type = HTTP_POST;

        if (*upload_data_size)
        {
            if(is_json(upload_data, *upload_data_size))
            {
                INT1 *tmp = (char*) upload_data;
                for (idx = *upload_data_size; idx > 0; --idx)
                {
                    if ('}' == tmp[idx])
                    {
                        tmp[idx + 1] = '\0';
                        break;
                    }
                }

                LOG_PROC("INFO", "Restful[%s]: [%s] %s\n", method, url, upload_data);
                g_rest_reply = proc_rest_msg(method, url, upload_data);

                memset((char *) upload_data, 0x0, *upload_data_size);    //清空缓冲区
                *upload_data_size = 0;
            }
            else
            {
                if(conn_info->connection_time - g_cur_sys_time.tv_sec > HTTP_TIMEOUT)
                {
                    memset((char *) upload_data, 0x0, *upload_data_size);    //清空缓冲区
                    *upload_data_size = 0;
                    return MHD_NO;
                }
            }

            return MHD_YES;
        }
        else
        {
            ret = send_page(connection, g_rest_reply, MHD_HTTP_OK);
            memset((char *) upload_data, 0x0, *upload_data_size);
            *upload_data_size = 0;
            upload_data = NULL;
            if (g_rest_reply)
            {
                
                gn_free((void **)&g_rest_reply);
            }
            return ret;
        }
    }

    return ret;
}

static void *start_httpd_service(void *para)
{
    struct MHD_Daemon *daemon;
    daemon = MHD_start_daemon(MHD_USE_SELECT_INTERNALLY, g_restful_port, NULL,
            NULL, &answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED,
            request_completed, NULL, MHD_OPTION_END);

    if (NULL == daemon)
    {
        return NULL;
    }

    while(1)
    {
        usleep(5000);
    }

    (void)getchar();
    MHD_stop_daemon(daemon);

    return NULL;
}

INT4 init_restful_svr()
{
    UINT4 i;
    INT1 *value = get_value(g_controller_configure, "[controller]", "restful_service_port");
    g_restful_port = (NULL == value ? 8081 : atoll(value));

    for(i = 0; i < REST_CAPACITY; i++)
    {
        g_restful_get_handles[i].used = 0;
        g_restful_post_handles[i].used = 0;
        g_restful_delete_handles[i].used = 0;
    }

    init_json_server();
    if (pthread_create(&g_restful_pid, NULL, start_httpd_service, NULL))
    {
        LOG_PROC("ERROR", "Restful service state failed");
        return GN_ERR;
    }
    return GN_OK;
}
