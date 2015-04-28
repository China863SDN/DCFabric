/******************************************************************************
*                                                                             *
*   File Name   : json_server.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-28           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef JSON_SERVER_H_
#define JSON_SERVER_H_

#include "restful-svr.h"
#include "app_impl.h"

INT4 init_json_server();
INT1 *json_to_reply(json_t *obj, INT4 code);
INT1 *proc_restful_request(UINT1 type, const INT1 *url, json_t *root);

#endif /* JSON_SERVER_H_ */
