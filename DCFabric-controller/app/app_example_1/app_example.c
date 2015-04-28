/******************************************************************************
*                                                                             *
*   File Name   : app_example.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-3-24           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/


#include "app_impl.h"
#include "gn_inet.h"
#include "openflow-common.h"
#include "openflow-10.h"
#include "openflow-13.h"

static INT4 _of10_msg_packet_in_handler(gn_switch_t *sw, UINT1 *of_msg)
{
    LOG_PROC("INFO", "%s", FN);
    return GN_OK;
}

static INT4 _of13_msg_packet_in_handler(gn_switch_t *sw, UINT1 *of_msg)
{
    LOG_PROC("INFO", "%s", FN);
    return GN_OK;
}

static INT4 _of13_msg_packet_in_ipv4_handler(gn_switch_t *sw, packet_in_info_t *packet_in_info)
{
    LOG_PROC("INFO", "%s", FN);
    return GN_OK;
}

static INT1 *_get_app_example(const INT1 *url, json_t *root)
{
    INT1 *reply = NULL;
    INT4 ret = GN_OK;
    INT1 json_tmp[32];
    json_t *obj, *key, *value;

    obj = json_new_object();

    key = json_new_string("myReply");
    value = json_new_string("Hello World");
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("retCode");
    sprintf(json_tmp, "%d", ret);
    value = json_new_number(json_tmp);
    json_insert_child(key, value);
    json_insert_child(obj, key);

    key = json_new_string("retMsg");
    value = json_new_string(get_error_msg(ret));
    json_insert_child(key, value);
    json_insert_child(obj, key);

    json_tree_to_string(obj, &reply);
    json_free_value(&obj);

    LOG_PROC("INFO", "Reply: %s", reply);
    return reply;
}


void app_example_init()
{
    LOG_PROC("INFO", "%s", FN);
    register_restful_handler(HTTP_GET, "/app/example/json", _get_app_example);
    return;
}
app_init(app_example_init);

void app_example_proc(gn_switch_t *sw)
{
    LOG_PROC("INFO", "%s> A new switch found", FN);
    if(sw->ofp_version == OFP10_VERSION)
    {
        register_of_msg_hander(sw, OFPT_PACKET_IN, _of10_msg_packet_in_handler);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        register_of_msg_hander(sw, OFPT13_PACKET_IN, _of13_msg_packet_in_handler);
    }

    return;
}
app_proc(app_example_proc);

void app_example_fini()
{
    LOG_PROC("INFO", "%s", FN);
    return;
}
app_init(app_example_fini);
