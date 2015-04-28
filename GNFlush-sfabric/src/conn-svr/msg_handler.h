/******************************************************************************
*                                                                             *
*   File Name   : msg_handler.h           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-13           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef MSG_HANDLER_H_
#define MSG_HANDLER_H_

#include "gnflush-types.h"

extern convertter_t of10_convertter;
extern convertter_t of13_convertter;
extern msg_handler_t of_message_handler[];
extern msg_handler_t of10_message_handler[];
extern msg_handler_t of13_message_handler[];

void message_process(gn_switch_t *sw, UINT1 *ofmsg);
#endif /* MSG_HANDLER_H_ */
