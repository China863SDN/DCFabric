/******************************************************************************
*                                                                             *
*   File Name   : event-service.h           *
*   Author      :            *
*   Create Date : 2015-4-29           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#ifndef INC_FABRIC_FABRIC_ARP_H_
#define INC_FABRIC_FABRIC_ARP_H_

#include "gnflush-types.h"
#define
typedef void (*foo) (gn_switch_t* sw,UINT4 num); 
void *fabric_thread_foo(void *arg);
void fabric_register(void *fabric_register_method);
void fabric_thread_start();

void put_into_array(gn_switch_t* p);
void delete_from_array(gn_switch_t* p);
void put_into_port_array(gn_switch_t* p);
void delete_from_port_array(gn_switch_t* p);

#endif
