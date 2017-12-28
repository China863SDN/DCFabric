#ifndef SRC_OPENSTACK_OPENSTACK_FIREWALL_APP_H_
#define SRC_OPENSTACK_OPENSTACK_FIREWALL_APP_H_

#include "gnflush-types.h"
#include "openstack_host.h"
#include "fabric_firewall.h"

fabric_firewall_rule_p	openstack_firewall_GenerateFirewallPolicy(void);


#endif 
