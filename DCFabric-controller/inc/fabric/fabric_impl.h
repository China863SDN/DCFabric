/*
 * fabric_impl.h
 *
 *  Created on: Apr 3, 2015
 *      Author: joe
 */

#ifndef INC_FABRIC_FABRIC_IMPL_H_
#define INC_FABRIC_FABRIC_IMPL_H_

//#include "gnflush-types.h"
#include "fabric_path.h"

#define FABRIC_START_TAG 4

void of131_fabric_impl_setup_by_dpids(UINT8* dpids,UINT4 len);
void of131_fabric_impl_setup();
void of131_fabric_impl_delete();

UINT4 of131_fabric_impl_get_tag_sw(gn_switch_t *sw);
UINT4 of131_fabric_impl_get_tag_dpid(UINT8 dpid);
gn_switch_t* of131_fabric_impl_get_sw_tag(UINT4 tag);
UINT8 of131_fabric_impl_get_dpid_tag(UINT4 tag);

UINT1 get_fabric_state();

p_fabric_path of131_fabric_get_path(UINT8 src_dpid,UINT8 dst_dpid);
#endif /* INC_FABRIC_FABRIC_IMPL_H_ */
