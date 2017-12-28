/*
 * openstack_dataparse.h
 *
 *  Created on: 6 20, 2017
 *      Author: yang
 */

#ifndef INC_OPENSTACK_DATAPARSE_H_
#define  INC_OPENSTACK_DATAPARSE_H_

#define OPENSTACK_STRINGLEN		48


typedef struct lb_health_check
{
	INT4	delay;
	INT4	timeout;
	INT2	fail;
	INT2    rise;
	INT1    type;
	INT1  	enabled;
}t_lb_health_check, *p_lb_health_check;
typedef struct lb_session_persistence
{
	char    type[48];
}t_lb_session_persistence, *p_lb_session_persistence;

INT4 createOpenstackQOS(char* jsonString, void* param);

INT4 createOpenstackNetwork(char* jsonString, void* param);
INT4 createOpenstackRouter(char* jsonString, void* param);
INT4 createOpenstackSubnet(char* jsonString, void* param);
INT4 createOpenstackPort(char* jsonString, void* param);
INT4 createOpenstackFloating(char* jsonString, void* param);
INT4 createOpenstackSecurity(char* jsonString, void* param);
INT4 createOpenstackPortforward(char* jsonString, void* param);
INT4 createOpenstackLbaaspools(char* jsonString, void* param);
INT4 createOpenstackLbaasvips(char* jsonString, void* param);
INT4 createOpenstackLbaasmember(char* jsonString, void* param);
INT4 createOpenstackLbaaslistener(char* jsonString, void* param);

INT4 createOpenstackClbaaspools(char* jsonString, void* param);
INT4 createOpenstackClbaasvips(char* jsonString, void* param);
INT4 createOpenstackClbaasloadbalancer(char* jsonString, void* param);
INT4 createOpenstackClbaasinterface(char* jsonString, void* param);
INT4 createOpenstackClbaaslistener(char* jsonString, void* param);
INT4 createOpenstackClbaasbackend(char* jsonString, void* param);
INT4 createOpenstackClbaasbackendListen(char* jsonString, void* param);

#endif
