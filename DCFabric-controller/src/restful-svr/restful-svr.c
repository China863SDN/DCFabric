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
#include <sys/prctl.h>   

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

//by:yhy 判断json_string是否符合json格式
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
//by:yhy 垃圾回收机制.固定用法参见libmicrohttpd
static void request_completed(void *cls, struct MHD_Connection *connection,
        void **con_cls, enum MHD_RequestTerminationCode toe)
{
    struct connection_info *conn_info = *con_cls;

    free(conn_info);
    *con_cls = NULL;
}
//by:yhy 生成响应信息并发送.固定用法参见libmicrohttpd
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
	//by:yhy 具体用法请详细阅读libmicrohttpd
    ret = MHD_add_response_header(response, MHD_HTTP_HEADER_CONTENT_TYPE, "application/json");
    ret = MHD_queue_response(connection, status_code, response);

    MHD_destroy_response(response);
    return ret;
}

//by:yhy 根据method,url,upload_data处理restful消息
static INT1 *proc_rest_msg(const INT1 *method, const INT1 *url, const INT1 *upload_data)
{
    json_t *root = NULL;
    INT4 parse_type = 0;
    INT1 *reply = NULL;

	//by:yhy 根据upload_data构建json节点
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

//by:yhy 固定用法参见libmicrohttpd中Processing POST data中Request handling
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
        //by:yhy 处理rest消息
        g_rest_reply = proc_rest_msg(method, url, upload_data);
        //by:yhy 发送对rest消息的响应
        ret = send_page(connection, g_rest_reply, MHD_HTTP_OK);     
        memset((char *) upload_data, 0x0, *upload_data_size);   
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

                memset((char *) upload_data, 0x0, *upload_data_size);    
                *upload_data_size = 0;
            }
            else
            {
				//by:yhy 如果upload_data非json格式,根据连接时间与当前时间差判断是否超时,其实就是判断rest消息是否接收完毕
                if(conn_info->connection_time - g_cur_sys_time.tv_sec > HTTP_TIMEOUT)
                {
                    memset((char *) upload_data, 0x0, *upload_data_size);    
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

//by:yhy restful服务初始化
INT4 init_restful_svr()
{
    UINT4 i;
	struct MHD_Daemon *daemon;
    INT1 *value = get_value(g_controller_configure, "[controller]", "restful_service_port");
    g_restful_port = (NULL == value ? 8081 : atoll(value));

    for(i = 0; i < REST_CAPACITY; i++)
    {
        g_restful_get_handles[i].used = 0;
        g_restful_post_handles[i].used = 0;
        g_restful_delete_handles[i].used = 0;
    }
	
	//by:yhy 初始化针对各个url的rest服务请求的handle
    init_json_server();

	//by:yhy 固定用法参见libmicrohttpd中Processing POST data
    daemon = MHD_start_daemon(MHD_USE_EPOLL_INTERNALLY_LINUX_ONLY, g_restful_port, NULL,
							  NULL, &answer_to_connection, NULL, MHD_OPTION_NOTIFY_COMPLETED,
							  request_completed, NULL, MHD_OPTION_END);
    if (NULL == daemon)
    {
        LOG_PROC("ERROR", "Restful service state failed");
        return GN_ERR;
    }
    return GN_OK;
}
