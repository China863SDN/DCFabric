/******************************************************************************
*                                                                             *
*   File Name   : msg_handler_common.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-13           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "msg_handler.h"
#include "app_impl.h"
#include "openflow-common.h"

msg_handler_t of_message_handler[1];
msg_handler_t of13_message_handler[OFP13_MAX_MSG];

//app��Ϣ����ע�ắ��
INT4 register_of_msg_hander(gn_switch_t *sw, UINT1 type, msg_handler_t msg_handler)
{
    msg_handler_t *new_msg_handers = NULL;
    if(sw->ofp_version == OFP10_VERSION)
    {
        new_msg_handers = (msg_handler_t *)malloc(sizeof(msg_handler_t) * OFP10_MAX_MSG);
        memcpy(new_msg_handers, of10_message_handler, sizeof(msg_handler_t) * OFP10_MAX_MSG);
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        new_msg_handers = (msg_handler_t *)malloc(sizeof(msg_handler_t) * OFP13_MAX_MSG);
        memcpy(new_msg_handers, of13_message_handler, sizeof(msg_handler_t) * OFP13_MAX_MSG);
    }
    else
    {
        LOG_PROC("ERROR", "OpenFlow verion [%d] is unsupported!", sw->ofp_version);
        return GN_ERR;
    }

    sw->msg_driver.msg_handler = new_msg_handers;
    sw->msg_driver.msg_handler[type] = msg_handler;

    return GN_OK;
}

//app��Ϣ����ע�ắ��
INT4 unregister_of_msg_hander(gn_switch_t *sw, UINT1 type)
{
    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.msg_handler[type] = of13_message_handler[type];
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.msg_handler[type] = of13_message_handler[type];
    }
    else
    {
        LOG_PROC("[ERROR]", "OpenFlow verion [%d] is unsupported!", sw->ofp_version);
        return GN_ERR;
    }

    return GN_OK;
}

//����__start_appproc_sec��__stop_appproc_sec֮����ڵĺ���ָ��app_proc(x)
static void mod_proccalls(gn_switch_t *sw)
{
    proccall_t *p_proc;   //����ָ�����

    p_proc = &__start_appproc_sec;
    do
    {
        (*p_proc)(sw);
        p_proc++;
    } while (p_proc < &__stop_appproc_sec);
}

INT4 of_msg_hello(gn_switch_t *sw, UINT1 *of_msg)
{
    struct ofp_header *header = (struct ofp_header*)of_msg;

    sw->ofp_version = header->version;
    if(sw->ofp_version == OFP10_VERSION)
    {
        sw->msg_driver.type_max = OFP10_MAX_MSG;
        sw->msg_driver.msg_handler = of10_message_handler;
        sw->msg_driver.convertter = &of10_convertter;
    }
    else if(sw->ofp_version == OFP13_VERSION)
    {
        sw->msg_driver.type_max = OFP13_MAX_MSG;
        sw->msg_driver.msg_handler = of13_message_handler;
        sw->msg_driver.convertter = &of13_convertter;
    }
    else
    {
        LOG_PROC("ERROR", "OpenFlow version unsupported, --version %u", header->version);
        return GN_ERR;
    }

    mod_proccalls(sw);
    return sw->msg_driver.msg_handler[0](sw, of_msg);
}

//��ʼ��Ĭ����Ϣ����
msg_handler_t of_message_handler[1] =
{
    of_msg_hello,       /* OFPT_HELLO */
};



void message_process(gn_switch_t *sw, UINT1 *ofmsg)
{
    struct ofp_header *header = (void *)ofmsg;
//    if(1)
//        printf("@@@@ Recv msg type[%d]\n", header->type);

    sw->msg_driver.msg_handler[header->type](sw, ofmsg);
}
