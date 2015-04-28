/*
 * fabric_arp.h
 *
 *  Created on: Apr 2, 2015
 *      Author: joe
 */

#ifndef INC_FABRIC_FABRIC_ARP_H_
#define INC_FABRIC_FABRIC_ARP_H_


void fabric_arp_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
void fabric_ip_handle(gn_switch_t *sw, packet_in_info_t *packet_in);
#endif /* INC_FABRIC_FABRIC_ARP_H_ */
