#ifndef INC_OPENSTACK_QOS_APP_H_
#define INC_OPENSTACK_QOS_APP_H_

#include "gnflush-types.h"

/**********************************/
//字段长度
#define QOS_NAME_LEN 			60
#define QOS_PORT_LEN 			60
#define QOS_DESCRIPTION_LEN		210
#define QOS_TENANT_LEN			60
#define QOS_ID_LEN				60
#define QOS_PROTOCOL_LEN		10
#define QOS_TYPE_LEN			10
#define QOS_DEVICEOWNER_LEN		50
#define QOS_SUBNETID_LEN		60
#define QOS_ROUTERID_LEN		60
#define QOS_IP_LEN				60
/**********************************/
#define QOS_OPERATION_NOTHING			"NOTHING"
#define QOS_OPERATION_METER				"METER"
#define QOS_OPERATION_QUEUE				"QUEUE"
/**********************************/



//QOS结构体

/* by:yhy
 * QOS规则
 */
typedef struct _openstack_qos_rule 
{
	char 	id			[QOS_ID_LEN];				
	char 	name		[QOS_NAME_LEN];			
	char 	description	[QOS_DESCRIPTION_LEN];		
	char 	tenant_id	[QOS_TENANT_LEN];					
	char 	protocol	[QOS_PROTOCOL_LEN];	
	char 	type		[QOS_TYPE_LEN];	
	UINT1 	shared;	
	UINT8 	max_rate;							//注意单位:bit/s
	struct _openstack_qos_rule* pre;
	struct _openstack_qos_rule* next;
} openstack_qos_rule, *openstack_qos_rule_p;

/* by:yhy
 * QOS映射
 */
typedef struct _openstack_qos_binding
{
	char 	id	[QOS_ID_LEN];	
	char 	qos_id	[QOS_ID_LEN];
	char 	tenant_id[QOS_TENANT_LEN];	
	char	device_owner[QOS_DEVICEOWNER_LEN];
	char	subnet_id[QOS_SUBNETID_LEN];				//这个字段存放network_id
	char	router_id[QOS_ROUTERID_LEN];
	UINT4	ip;
	struct _openstack_qos_binding* pre;
	struct _openstack_qos_binding* next;				
} openstack_qos_binding, *openstack_qos_binding_p;

typedef struct _openstack_qos_instance
{
	char 	qos_rule_id	[QOS_ID_LEN];	
	char 	qos_binding_id	[QOS_ID_LEN];
	char 	qos_operation_type[10];
	UINT8 	switch_dpid;
	UINT4	meter_id;
	UINT4	table_id;
	struct _openstack_qos_instance* pre;
	struct _openstack_qos_instance* next;				
} openstack_qos_instance, *openstack_qos_instance_p;

/************************************************/
//外部可引用变量
/**********************************/
extern openstack_qos_binding_p 	G_openstack_qos_floating_binding_List 		;	
extern openstack_qos_binding_p 	G_openstack_qos_router_binding_List 		;
extern openstack_qos_binding_p 	G_openstack_qos_loadbalancer_binding_List 	;
/************************************************/
extern openstack_qos_instance_p 	G_openstack_qos_instance_List;
extern openstack_qos_rule_p		G_openstack_qos_rule_List;
/**********************************/
//外部可引用函数
/**********************************/
void openstack_qos_InitBindingList(openstack_qos_binding_p * Binding_List);
void openstack_qos_Init(void);
void openstack_qos_ReloadRules(void);
void openstack_qos_rule_ConventDataFromJSON(char *jsonString);


openstack_qos_rule_p    openstack_qos_C_rule(openstack_qos_rule_p *qos_rule_list,
		char * id,
		char * name,
		char * description,
		char * tenant_id,
		char * protocol,
		char * type,
		UINT1  shared,
		UINT8  max_rate
										    );
openstack_qos_rule_p    openstack_qos_R_rule(openstack_qos_rule_p *qos_rule_list,char * id);
openstack_qos_rule_p    openstack_qos_U_rule(openstack_qos_rule_p *qos_rule_list,
		char * id,
		char * name,
		char * description,
		char * tenant_id,
		char * protocol,
		char * type,
		UINT1  shared,
		UINT8  max_rate
										    );
openstack_qos_rule_p    openstack_qos_D_rule(openstack_qos_rule_p *qos_rule_list,char * id);
UINT1    openstack_qos_Compare_rule(openstack_qos_rule_p *rule_list_new,openstack_qos_rule_p *rule_list_old);
void openstack_qos_D_rule_list(openstack_qos_rule_p qos_rule_list);

openstack_qos_binding_p    openstack_qos_C_binding(openstack_qos_binding_p *qos_binding_list,char * id,char * qos_id,char * tenant_id,char * device_owner,char * subnet_id,char * router_id,UINT4 ip);
openstack_qos_binding_p    openstack_qos_R_binding(openstack_qos_binding_p *qos_binding_list,char * id);
openstack_qos_binding_p    openstack_qos_U_binding(openstack_qos_binding_p *qos_binding_list,char * id,char * qos_id,char * tenant_id,char * device_owner,char * subnet_id,char * router_id,UINT4 ip);
openstack_qos_binding_p    openstack_qos_D_binding(openstack_qos_binding_p *qos_binding_list,char * id);
openstack_qos_binding_p    openstack_qos_Compare_binding(openstack_qos_binding_p* qos_binding_list_new,openstack_qos_binding_p *qos_binding_list_old);
void openstack_qos_D_binding_list(openstack_qos_binding_p qos_binding_list);

openstack_qos_instance_p    openstack_qos_C_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id,char * qos_rule_id,char *qos_operation_type,UINT8 	switch_dpid,UINT4	meter_id,UINT4	table_id);
openstack_qos_instance_p    openstack_qos_R_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id);
openstack_qos_instance_p    openstack_qos_U_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id,char * qos_rule_id,char *qos_operation_type,UINT8 	switch_dpid,UINT4	meter_id,UINT4	table_id);
openstack_qos_instance_p    openstack_qos_D_instance(openstack_qos_instance_p *qos_instance_list,char * qos_binding_id);



#endif
