/******************************************************************************
*                                                                             *
*   File Name   : restful-svr.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef RESTFUL_SVR_H_
#define RESTFUL_SVR_H_

#include "microhttpd.h"
#include "gnflush-types.h"

#define HTTP_TIMEOUT 5
#define REST_CAPACITY 60
#define POSTBUFFERSIZE 512
#define REST_BUFF_LEN 100   //55 * 1024 = 56320

extern UINT4 g_restful_port;

extern restful_handles_t g_restful_get_handles[];
extern restful_handles_t g_restful_post_handles[];
extern restful_handles_t g_restful_put_handles[];
extern restful_handles_t g_restful_delete_handles[];


INT4 init_restful_svr();

#endif /* RESTFUL_SVR_H_ */
