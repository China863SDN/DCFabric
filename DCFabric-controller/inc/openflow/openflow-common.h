/* Copyright (c) 2008 The Board of Trustees of The Leland Stanford
* Junior University
* Copyright (c) 2011, 2012 Open Networking Foundation
*
* We are making the OpenFlow specification and associated documentation
* (Software) available for public use and benefit with the expectation
* that others will use, modify and enhance the Software and contribute
* those enhancements back to the community. However, since we would
* like to make the Software available for broadest use, with as few
* restrictions as possible permission is hereby granted, free of
* charge, to any person obtaining a copy of this Software to deal in
* the Software under the copyrights without restriction, including
* without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to
* permit persons to whom the Software is furnished to do so, subject to
* the following conditions:
*
* The above copyright notice and this permission notice shall be
* included in all copies or substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
* EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
* MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT.  IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
* BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
* ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
* CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
* SOFTWARE.
*
* The name and trademarks of copyright holder(s) may NOT be used in
* advertising or publicity pertaining to the Software or any
* derivatives without specific, written prior permission.
*/
#ifndef __OPENFLOW_COMMON_H__
#define __OPENFLOW_COMMON_H__

#ifdef __KERNEL__
#include <linux/types.h>
#else
#include <stdint.h>
#endif

#ifdef SWIG
#define OFP_ASSERT(EXPR)        /* SWIG can't handle OFP_ASSERT. */
#elif !defined(__cplusplus)
/* Build-time assertion for use in a declaration context. */
#define OFP_ASSERT(EXPR)                                                \
        extern int (*build_assert(void))[ sizeof(struct {               \
                    unsigned int build_assert_failed : (EXPR) ? 1 : -1; })]
#else /* __cplusplus */
#define OFP_ASSERT(_EXPR) typedef int build_assert_failed[(_EXPR) ? 1 : -1]
#endif /* __cplusplus */

#ifndef SWIG
#define OFP_PACKED __attribute__((packed))
#else
#define OFP_PACKED              /* SWIG doesn't understand __attribute. */
#endif

/* Version number:
 * Non-experimental versions released: 0x01
 * Experimental versions released: 0x81 -- 0x99
 */
/* The most significant bit being set in the version field indicates an
 * experimental OpenFlow version.
 */

#define OFP_VERSION_MLAPI 0xff
#define OFP_SB_VERSION (OFP_VERSION)

#define OFP_MAX_TYPE ((1<<8)-1)
#define OFP_MAX_PAYLOAD (2*8192)

#define OFP_MAX_TABLE_NAME_LEN 32
#define OFP_MAX_PORT_NAME_LEN  16

#define OFP_TCP_PORT  6633
#define OFP_SSL_PORT  6633

#define OFP_NO_BUFFER (0xffffffff)

#define OFP_ETH_ALEN 6          /* Bytes in an Ethernet address. */
#define OFP_HEADER_LEN  8

enum ofp_version {
    OFP10_VERSION = 0x01,
    OFP11_VERSION = 0x02,
    OFP12_VERSION = 0x03,
    OFP13_VERSION = 0x04,
    OFP14_VERSION = 0x05,
    OFP15_VERSION = 0x06
};

enum ofp_max_msg
{
    OFP10_MAX_MSG = 22,
    OFP13_MAX_MSG = 30
};

/* Header on all OpenFlow packets. */
struct ofp_header {
    uint8_t version;    /* OFP_VERSION. */
    uint8_t type;       /* One of the OFPT_ constants. */
    uint16_t length;    /* Length including this ofp_header. */
    uint32_t xid;       /* Transaction id associated with this packet.
                           Replies use the same id as was in the request
                           to facilitate pairing. */
};
OFP_ASSERT(sizeof(struct ofp_header) == 8);

/* The match type indicates the match structure (set of fields that compose the
* match) in use. The match type is placed in the type field at the beginning
* of all match structures. The "standard" type corresponds to ofp_match and
* must be supported by all OpenFlow switches. Extensions that define other
* match types may be published on the OpenFlow wiki. Support for extensions is
* optional.
*/
enum ofp_match_type {
    OFPMT_STANDARD, /* Deprecated */
    OFPMT_OXM = 1,  /* OpenFlow Extensible Match */
};

#define OFP_MAX_OXM_TLVS (32)
#define HTON_OXM_HDR(oxm) (*(uint32_t *)(oxm) = htonl(*(uint32_t *)(oxm)))
#define NTOH_OXM_HDR(oxm) (*(uint32_t *)(oxm) = ntohl(*(uint32_t *)(oxm)))
#define ASSIGN_OXM_HDR(oxm_d, oxm_r) (*(uint32_t *)(oxm_d) = *(uint32_t *)(oxm_r))

struct ofp_oxm_header
{
    uint16_t oxm_class;
    uint8_t  oxm_field_hm;
    uint8_t  length;
    uint8_t  data[0];
};


struct _ofp_oxm_header {
    uint8_t length;
#define OFP_OXM_GHDR_FIELD(oxm) (((oxm)->oxm_field_hm >> 1) & 0xff)
#define OFP_OXM_GHDR_HM(oxm) ((oxm)->oxm_field_hm & 0x1)
#define OFP_OXM_GHDR_TYPE(oxm)  ((oxm)->oxm_class << 15 | OFP_OXM_GHDR_FIELD(oxm))
#define OFP_OXM_SHDR_FIELD(oxm, field) ((oxm)->oxm_field_hm = ((field & 0x7f) << 1) | \
                                                             OFP_OXM_GHDR_HM(oxm))
#define OFP_OXM_SHDR_HM(oxm, hm) ((oxm)->oxm_field_hm = (hm & 0x1)| OFP_OXM_GHDR_FIELD(oxm))
    uint8_t oxm_field_hm;
    uint16_t oxm_class;
    uint8_t  data[0];
};


/* OXM Class IDs.
* The high order bit differentiate reserved classes from member classes.
* Classes 0x0000 to 0x7FFF are member classes, allocated by ONF.
* Classes 0x8000 to 0xFFFE are reserved classes, reserved for standardisation.
*/
enum ofp_oxm_class {
    OFPXMC_NXM_0 = 0x0000, /* Backward compatibility with NXM */
    OFPXMC_NXM_1 = 0x0001, /* Backward compatibility with NXM */
    OFPXMC_OPENFLOW_BASIC = 0x8000, /* Basic class for OpenFlow */
    OFPXMC_EXPERIMENTER = 0xFFFF, /* Experimenter class */
};

#define OFPXMT_OFB_IN_PORT_SZ (4)
#define OFPXMT_OFB_METADATA_SZ (8)
#define OFPXMT_OFB_ETH_SZ (6)
#define OFPXMT_OFB_ETH_TYPE_SZ (2)
#define OFPXMT_OFB_IPV4_SZ (4)
#define OFPXMT_OFB_IPV6_SZ (16)
#define OFPXMT_OFB_VLAN_VID_SZ (2)
#define OFPXMT_OFB_VLAN_PCP_SZ (1)
#define OFPXMT_OFB_IP_PROTO_SZ (1)
#define OFPXMT_OFB_L4_PORT_SZ (2)
#define OFPXMT_OFB_IP_DSCP_SZ (1)
#define OFPXMT_OFB_MPLS_LABEL_SZ (4)

#define ETH_TYPE_VLAN_8021Q    0x8100
#define ETH_TYPE_MPLS_8021Q    0x8847

/* OXM Flow match field types for OpenFlow basic class. */
enum oxm_ofb_match_fields {
    OFPXMT_OFB_IN_PORT = 0, /* Switch input port. */
    OFPXMT_OFB_IN_PHY_PORT = 1, /* Switch physical input port. */
    OFPXMT_OFB_METADATA = 2, /* Metadata passed between tables. */
    OFPXMT_OFB_ETH_DST = 3, /* Ethernet destination address. */
    OFPXMT_OFB_ETH_SRC = 4, /* Ethernet source address. */
    OFPXMT_OFB_ETH_TYPE = 5, /* Ethernet frame type. */
    OFPXMT_OFB_VLAN_VID = 6, /* VLAN id. */
    OFPXMT_OFB_VLAN_PCP = 7, /* VLAN priority. */
    OFPXMT_OFB_IP_DSCP = 8, /* IP DSCP (6 bits in ToS field). */
    OFPXMT_OFB_IP_ECN = 9, /* IP ECN (2 bits in ToS field). */
    OFPXMT_OFB_IP_PROTO = 10, /* IP protocol. */
    OFPXMT_OFB_IPV4_SRC = 11, /* IPv4 source address. */
    OFPXMT_OFB_IPV4_DST = 12, /* IPv4 destination address. */
    OFPXMT_OFB_TCP_SRC = 13, /* TCP source port. */
    OFPXMT_OFB_TCP_DST = 14, /* TCP destination port. */
    OFPXMT_OFB_UDP_SRC = 15, /* UDP source port. */
    OFPXMT_OFB_UDP_DST = 16, /* UDP destination port. */
    OFPXMT_OFB_SCTP_SRC = 17, /* SCTP source port. */
    OFPXMT_OFB_SCTP_DST = 18, /* SCTP destination port. */
    OFPXMT_OFB_ICMPV4_TYPE = 19, /* ICMP type. */
    OFPXMT_OFB_ICMPV4_CODE = 20, /* ICMP code. */
    OFPXMT_OFB_ARP_OP = 21, /* ARP opcode. */
    OFPXMT_OFB_ARP_SPA = 22, /* ARP source IPv4 address. */
    OFPXMT_OFB_ARP_TPA = 23, /* ARP target IPv4 address. */
    OFPXMT_OFB_ARP_SHA = 24, /* ARP source hardware address. */
    OFPXMT_OFB_ARP_THA = 25, /* ARP target hardware address. */
    OFPXMT_OFB_IPV6_SRC = 26, /* IPv6 source address. */
    OFPXMT_OFB_IPV6_DST = 27, /* IPv6 destination address. */
    OFPXMT_OFB_IPV6_FLABEL = 28, /* IPv6 Flow Label */
    OFPXMT_OFB_ICMPV6_TYPE = 29, /* ICMPv6 type. */
    OFPXMT_OFB_ICMPV6_CODE = 30, /* ICMPv6 code. */
    OFPXMT_OFB_IPV6_ND_TARGET = 31, /* Target address for ND. */
    OFPXMT_OFB_IPV6_ND_SLL = 32, /* Source link-layer for ND. */
    OFPXMT_OFB_IPV6_ND_TLL = 33, /* Target link-layer for ND. */
    OFPXMT_OFB_MPLS_LABEL = 34, /* MPLS label. */
    OFPXMT_OFB_MPLS_TC = 35, /* MPLS TC. */
    OFPXMT_OFP_MPLS_BOS = 36, /* MPLS BoS bit. */
    OFPXMT_OFB_PBB_ISID = 37, /* PBB I-SID. */
    OFPXMT_OFB_TUNNEL_ID = 38, /* Logical Port Metadata. */
    OFPXMT_OFB_IPV6_EXTHDR = 39, /* IPv6 Extension Header pseudo-field */
    OFPXMT_OFB_IPV4_SRC_PREFIX = 40,
    OFPXMT_OFB_IPV4_DST_PREFIX = 41,
    OFPXMT_OFB_IPV6_SRC_PREFIX = 42,
    OFPXMT_OFB_IPV6_DST_PREFIX = 43,
    OFPXMT_OFB_METADATA_MASK = 44
};

/* The VLAN id is 12-bits, so we can use the entire 16 bits to indicate
* special conditions.
*/
enum ofp_vlan_id {
    OFPVID_PRESENT = 0x1000, /* Bit that indicate that a VLAN id is set */
    OFPVID_NONE = 0x0000, /* No VLAN id was set. */
};

#define OFP_MAX_INSTRUCTIONS (6)
#define OFP_MAX_ACTIONS (32)

enum ofp_instruction_type {
    OFPIT_GOTO_TABLE = 1, /* Setup the next table in the lookup
                             pipeline */
    OFPIT_WRITE_METADATA = 2, /* Setup the metadata field for use later in
                                 pipeline */
    OFPIT_WRITE_ACTIONS = 3, /* Write the action(s) onto the datapath action
                                set */
    OFPIT_APPLY_ACTIONS = 4, /* Applies the action(s) immediately */
    OFPIT_CLEAR_ACTIONS = 5, /* Clears all actions from the datapath
                                action set */
    OFPIT_METER = 6, /* Apply meter (rate limiter) */
    OFPIT_EXPERIMENTER = 0xFFFF /* Experimenter instruction */
};

/* Instruction header that is common to all instructions. The length includes
* the header and any padding used to make the instruction 64-bit aligned.
* NB: The length of an instruction *must* always be a multiple of eight. */
struct ofp_instruction {
    uint16_t type; /* Instruction type */
    uint16_t len; /* Length of this struct in bytes. */
};
OFP_ASSERT(sizeof(struct ofp_instruction) == 4);

/* Instruction structure for OFPIT_GOTO_TABLE */
struct ofp_instruction_goto_table {
    uint16_t type; /* OFPIT_GOTO_TABLE */
    uint16_t len; /* Length of this struct in bytes. */
    uint8_t table_id; /* Set next table in the lookup pipeline */
    uint8_t pad[3]; /* Pad to 64 bits. */
};
OFP_ASSERT(sizeof(struct ofp_instruction_goto_table) == 8);

/* Instruction structure for OFPIT_WRITE_METADATA */
struct ofp_instruction_write_metadata {
    uint16_t type; /* OFPIT_WRITE_METADATA */
    uint16_t len; /* Length of this struct in bytes. */
    uint8_t pad[4]; /* Align to 64-bits */
    uint64_t metadata; /* Metadata value to write */
    uint64_t metadata_mask; /* Metadata write bitmask */
};
OFP_ASSERT(sizeof(struct ofp_instruction_write_metadata) == 24);

#define OFP_ACT_HDR_SZ (4)

/* Action header that is common to all actions.  The length includes the
 * header and any padding used to make the action 64-bit aligned.
 * NB: The length of an action *must* always be a multiple of eight. */
struct ofp_action_header {
    uint16_t type;                  /* One of OFPAT_*. */
    uint16_t len;                   /* Length of action, including this
                                       header.  This is the length of action,
                                       including any padding to make it
                                       64-bit aligned. */
    uint8_t pad[4];
};
OFP_ASSERT(sizeof(struct ofp_action_header) == 8);

/* Instruction structure for OFPIT_WRITE/APPLY/CLEAR_ACTIONS */
struct ofp_instruction_actions {
    uint16_t type; /* One of OFPIT_*_ACTIONS */
    uint16_t len; /* Length of this struct in bytes. */
    uint8_t pad[4]; /* Align to 64-bits */
    struct ofp_action_header actions[0]; /* Actions associated with
                                            OFPIT_WRITE_ACTIONS and
                                            OFPIT_APPLY_ACTIONS */
};
OFP_ASSERT(sizeof(struct ofp_instruction_actions) == 8);

/* Instruction structure for OFPIT_METER-OF1.3.1+ */
struct ofp_instruction_meter {
    uint16_t type; /* OFPIT_METER */
    uint16_t len; /* Length is 8. */
    uint32_t meter_id; /* Meter instance. */
};
OFP_ASSERT(sizeof(struct ofp_instruction_meter) == 8);

/* Instruction structure for experimental instructions-OF1.3.1+ */
struct ofp_instruction_experimenter {
    uint16_t type; /* OFPIT_EXPERIMENTER */
    uint16_t len; /* Length of this struct in bytes */
    uint32_t experimenter; /* Experimenter ID which takes the same form
                              as in struct ofp_experimenter_header. */
    /* Experimenter-defined arbitrary additional data. */
};
OFP_ASSERT(sizeof(struct ofp_instruction_experimenter) == 8);

enum ofp_controller_max_len {
    OFPCML_MAX = 0xffe5, /* maximum max_len value which can be used
                            to request a specific byte length. */
    OFPCML_NO_BUFFER = 0xffff /* indicates that no buffering should be
                                applied and the whole packet is to be
                                sent to the controller. */
};

/* Action structure for OFPAT_GROUP-OF1.3.1+ */
struct ofp_action_group {
    uint16_t type; /* OFPAT_GROUP. */
    uint16_t len; /* Length is 8. */
    uint32_t group_id; /* Group identifier. */
};
OFP_ASSERT(sizeof(struct ofp_action_group) == 8);

struct ofp_action_set_queue{
	uint16_t type; /* OFPAT_SET_QUEUE. */
	uint16_t len; /* Len is 8. */
	uint32_t queue_id; /* Queue id for the packets. */

};
OFP_ASSERT(sizeof(struct ofp_action_set_queue) == 8);

/* Action structure for OFPAT_SET_MPLS_TTL-OF1.3.1+ */
struct ofp_action_mpls_ttl {
    uint16_t type; /* OFPAT_SET_MPLS_TTL. */
    uint16_t len; /* Length is 8. */
    uint8_t mpls_ttl; /* MPLS TTL */
    uint8_t pad[3];
};
OFP_ASSERT(sizeof(struct ofp_action_mpls_ttl) == 8);

/* Action structure for OFPAT_SET_NW_TTL-OF1.3.1+ */
struct ofp_action_nw_ttl {
    uint16_t type; /* OFPAT_SET_NW_TTL. */
    uint16_t len; /* Length is 8. */
    uint8_t nw_ttl; /* IP TTL */
    uint8_t pad[3];
};
OFP_ASSERT(sizeof(struct ofp_action_nw_ttl) == 8);

/* Action structure for OFPAT_PUSH_VLAN/MPLS/PBB-OF1.3.1+ */
struct ofp_action_push {
    uint16_t type; /* OFPAT_PUSH_VLAN/MPLS/PBB. */
    uint16_t len; /* Length is 8. */
    uint16_t ethertype; /* Ethertype */
    uint8_t pad[2];
};
OFP_ASSERT(sizeof(struct ofp_action_push) == 8);

/* Action structure for OFPAT_POP_MPLS. */
struct ofp_action_pop_mpls {
    uint16_t type; /* OFPAT_POP_MPLS. */
    uint16_t len; /* Length is 8. */
    uint16_t ethertype; /* Ethertype */
    uint8_t pad[2];
};
OFP_ASSERT(sizeof(struct ofp_action_pop_mpls) == 8);

#define OFP_ACT_SETF_HDR_SZ (4)

/* Action structure for OFPAT_SET_FIELD. */
struct ofp_action_set_field {
    uint16_t type; /* OFPAT_SET_FIELD. */
    uint16_t len; /* Length is padded to 64 bits. */
    /* Followed by:
    * - Exactly oxm_len bytes containing a single OXM TLV, then
    * - Exactly ((oxm_len + 4) + 7)/8*8 - (oxm_len + 4) (between 0 and 7)
    * bytes of all-zero bytes
    */
    uint8_t field[4]; /* OXM TLV - Make compiler happy */
};
OFP_ASSERT(sizeof(struct ofp_action_set_field) == 8);

/* Action header for OFPAT_EXPERIMENTER-OF1.3.1+
* The rest of the body is experimenter-defined. */
struct ofp_action_experimenter_header {
    uint16_t type; /* OFPAT_EXPERIMENTER. */
    uint16_t len; /* Length is a multiple of 8. */
    uint32_t experimenter; /* Experimenter ID which takes the same
                              form as in struct
                              ofp_experimenter_header. */
};
OFP_ASSERT(sizeof(struct ofp_action_experimenter_header) == 8);

/* Switch configuration. */
struct ofp10_switch_config {
    struct ofp_header header;
    uint16_t flags;             /* OFPC_* flags. */
    uint16_t miss_send_len;     /* Max bytes of new flow that datapath should
                                   send to the controller. */
};
OFP_ASSERT(sizeof(struct ofp10_switch_config) == 12);

#define OFP_DEFAULT_MISS_SEND_LEN   128

enum ofp_config_flags {
    /* Handling of IP fragments. */
    OFPC_FRAG_NORMAL   = 0,  /* No special handling for fragments. */
    OFPC_FRAG_DROP     = 1,  /* Drop fragments. */
    OFPC_FRAG_REASM    = 2,  /* Reassemble (only if OFPC_IP_REASM set). */
    OFPC_FRAG_MASK     = 3
};

/* Table numbering. Tables can use any number up to OFPT_MAX. */
enum ofp_table {
    /* Last usable table number. */
    OFPTT_MAX = 0xfe,
    /* Fake tables. */
    OFPTT_ALL = 0xff /* Wildcard table used for table config,
                        flow stats and flow deletes. */
};

/* Configure/Modify behavior of a flow table */
struct ofp_table_mod {
    struct ofp_header header;
    uint8_t table_id; /* ID of the table, OFPTT_ALL indicates all tables */
    uint8_t pad[3]; /* Pad to 32 bits */
    uint32_t config; /* Bitmap of OFPTC_* flags */
};
OFP_ASSERT(sizeof(struct ofp_table_mod) == 16);

/* Flags to configure the table. Reserved for future use. */
enum ofp_table_config {
    OFPTC_DEPRECATED_MASK = 3, /* Deprecated bits */
};

#define OFPX_MATCH_HDR_SZ (4)

/* Fields to match against flows */
struct ofpx_match { /* Corresponding to ofp_match OF1.3.1 */
    uint16_t type; /* One of OFPMT_* */
    uint16_t length; /* Length of ofp_match (excluding padding) */
    /* Followed by:
     * - Exactly (length - 4) (possibly 0) bytes containing OXM TLVs, then
     * - Exactly ((length + 7)/8*8 - length) (between 0 and 7) bytes of
     * all-zero bytes
     * In summary, ofp_match is padded as needed, to make its overall size
     * a multiple of 8, to preserve alignement in structures using it.
    */
    uint8_t oxm_fields[4]; /* OXMs start here - Make compiler happy */
};
OFP_ASSERT(sizeof(struct ofpx_match) == 8);

enum ofp_flow_mod_command {
    OFPFC_ADD,              /* New flow. */
    OFPFC_MODIFY,           /* Modify all matching flows. */
    OFPFC_MODIFY_STRICT,    /* Modify entry strictly matching wildcard */
    OFPFC_DELETE,           /* Delete all matching flows. */
    OFPFC_DELETE_STRICT    /* Strictly match wildcards and priority. */
};

/* Bucket for use in groups. -OF1.3.1+  */
struct ofp_bucket {
    uint16_t len; /* Length the bucket in bytes, including
                     this header and any padding to make it
                     64-bit aligned. */
    uint16_t weight; /* Relative weight of bucket. Only
                        defined for select groups. */
    uint32_t watch_port; /* Port whose state affects whether this
                            bucket is live. Only required for fast
                            failover groups. */
    uint32_t watch_group; /* Group whose state affects whether this
                             bucket is live. Only required for fast
                             failover groups. */
    uint8_t pad[4];
    struct ofp_action_header actions[0]; /* The action length is inferred
                                            from the length field in the
                                            header. */
};
OFP_ASSERT(sizeof(struct ofp_bucket) == 16);


/* Group setup and teardown (controller -> datapath). -OF1.3.1+ */
struct ofp_group_mod {
    struct ofp_header header;
    uint16_t command; /* One of OFPGC_*. */
    uint8_t type; /* One of OFPGT_*. */
    uint8_t pad; /* Pad to 64 bits. */
    uint32_t group_id; /* Group identifier. */
    struct ofp_bucket buckets[0]; /* The length of the bucket array is inferred
                                     from the length field in the header. */
};
OFP_ASSERT(sizeof(struct ofp_group_mod) == 16);

/* Group commands OF1.3.1+ */
enum ofp_group_mod_command {
    OFPGC_ADD = 0, /* New group. */
    OFPGC_MODIFY = 1, /* Modify all matching groups. */
    OFPGC_DELETE = 2, /* Delete all matching groups. */
};

/* Group types. Values in the range [128, 255] are reserved for experimental
* use - OF1.3.1+. */
enum ofp_group_type {
    OFPGT_ALL = 0, /* All (multicast/broadcast) group. */
    OFPGT_SELECT = 1, /* Select group. */
    OFPGT_INDIRECT = 2, /* Indirect group. */
    OFPGT_FF = 3, /* Fast failover group. */
};

/* Group numbering. Groups can use any number up to OFPG_MAX -OF1.3.1+ */
enum ofp_group {
    /* Last usable group number. */
    OFPG_MAX = 0xffffff00,
    /* Fake groups. */
    OFPG_ALL = 0xfffffffc, /* Represents all groups for group delete
                              commands. */
    OFPG_ANY = 0xffffffff /* Wildcard group used only for flow stats
                             requests. Selects all flows regardless of
                             group (including flows with no group).
                           */
};

/* Common header for all meter bands */
struct ofp_meter_band_header {
    uint16_t type; /* One of OFPMBT_*. */
    uint16_t len; /* Length in bytes of this band. */
    uint32_t rate; /* Rate for this band. */
    uint32_t burst_size; /* Size of bursts. */
};
OFP_ASSERT(sizeof(struct ofp_meter_band_header) == 12);

/* Meter configuration. OFPT_METER_MOD. -OF1.3.1+ */
struct ofp_meter_mod {
    struct ofp_header header;
    uint16_t command; /* One of OFPMC_*. */
    uint16_t flags; /* One of OFPMF_*. */
    uint32_t meter_id; /* Meter instance. */
    struct ofp_meter_band_header bands[0]; /* The bands length is
                                              inferred from the length field
                                                in the header. */
};
OFP_ASSERT(sizeof(struct ofp_meter_mod) == 16);

/* Meter numbering. Flow meters can use any number up to OFPM_MAX. - OF1.3.1+*/
enum ofp_meter {
    /* Last usable meter. */
    OFPM_MAX = 0xffff0000,
    /* Virtual meters. */
    OFPM_SLOWPATH = 0xfffffffd, /* Meter for slow datapath. */
    OFPM_CONTROLLER = 0xfffffffe, /* Meter for controller connection. */
    OFPM_ALL = 0xffffffff, /* Represents all meters for stat requests
                              commands. */
};

/* Meter commands -OF1.3.1+ */
enum ofp_meter_mod_command {
    OFPMC_ADD, /* New meter. */
    OFPMC_MODIFY, /* Modify specified meter. */
    OFPMC_DELETE, /* Delete specified meter. */
};

/* Meter configuration flags -OF1.3.1+  */
enum ofp_meter_flags {
    OFPMF_KBPS = 1 << 0, /* Rate value in kb/s (kilo-bit per second). */
    OFPMF_PKTPS = 1 << 1, /* Rate value in packet/sec. */
    OFPMF_BURST = 1 << 2, /* Do burst size. */
    OFPMF_STATS = 1 << 3, /* Collect statistics. */
};

/* Meter band types -OF1.3.1+ */
enum ofp_meter_band_type {
    OFPMBT_DROP = 1, /* Drop packet. */
    OFPMBT_DSCP_REMARK = 2, /* Remark DSCP in the IP header. */
    OFPMBT_EXPERIMENTER = 0xFFFF /* Experimenter meter band. */
};


/* OFPMBT_DROP band - drop packets - OF1.3.1+ */
struct ofp_meter_band_drop {
    uint16_t type; /* OFPMBT_DROP. */
    uint16_t len; /* Length in bytes of this band. */
    uint32_t rate; /* Rate for dropping packets. */
    uint32_t burst_size; /* Size of bursts. */
    uint8_t pad[4];
};
OFP_ASSERT(sizeof(struct ofp_meter_band_drop) == 16);

/* OFPMBT_DSCP_REMARK band - Remark DSCP in the IP header - OF1.3.1+ */
struct ofp_meter_band_dscp_remark {
    uint16_t type; /* OFPMBT_DSCP_REMARK. */
    uint16_t len; /* Length in bytes of this band. */
    uint32_t rate; /* Rate for remarking packets. */
    uint32_t burst_size; /* Size of bursts. */
    uint8_t prec_level; /* Number of drop precedence level to add. */
    uint8_t pad[3];
};
OFP_ASSERT(sizeof(struct ofp_meter_band_dscp_remark) == 16);

/* OFPMBT_EXPERIMENTER band - Write actions in action set OF1.3.1+ */
struct ofp_meter_band_experimenter {
    uint16_t type; /* One of OFPMBT_*. */
    uint16_t len; /* Length in bytes of this band. */
    uint32_t rate; /* Rate for this band. */
    uint32_t burst_size; /* Size of bursts. */
    uint32_t experimenter; /* Experimenter ID which takes the same
                              form as in struct
                              ofp_experimenter_header. */
};
OFP_ASSERT(sizeof(struct ofp_meter_band_experimenter) == 16);

struct ofp_multipart_request {
    struct ofp_header header;
    uint16_t type; /* One of the OFPMP_* constants. */
    uint16_t flags; /* OFPMPF_REQ_* flags. */
    uint8_t pad[4];
    uint8_t body[0]; /* Body of the request. */
};
OFP_ASSERT(sizeof(struct ofp_multipart_request) == 16);



struct ofp131_group_stats_request {
    uint32_t   group_id; /* All groups if OFPG_ALL. */
    uint8_t    pad[4];
};
OFP_ASSERT(sizeof(struct ofp131_group_stats_request) == 8);


enum ofp_multipart_request_flags {
    OFPMPF_REQ_MORE = 1 << 0 /* More requests to follow. */
};

struct ofp_multipart_reply {
    struct ofp_header header;
    uint16_t type; /* One of the OFPMP_* constants. */
    uint16_t flags; /* OFPMPF_REPLY_* flags. */
    uint8_t pad[4];
    uint8_t body[0]; /* Body of the reply. */
};
OFP_ASSERT(sizeof(struct ofp_multipart_reply) == 16);

enum ofp_multipart_reply_flags {
    OFPMPF_REPLY_MORE = 1 << 0 /* More replies to follow. */
};

enum ofp_multipart_types {
    /* Description of this OpenFlow switch.
    * The request body is empty.
    * The reply body is struct ofp_desc. */
    OFPMP_DESC = 0,
    /* Individual flow statistics.
    * The request body is struct ofp_flow_stats_request.
    * The reply body is an array of struct ofp_flow_stats. */
    OFPMP_FLOW = 1,
    /* Aggregate flow statistics.
    * The request body is struct ofp_aggregate_stats_request.
    * The reply body is struct ofp_aggregate_stats_reply. */
    OFPMP_AGGREGATE = 2,
    /* Flow table statistics.
    * The request body is empty.
    * The reply body is an array of struct ofp_table_stats. */
    OFPMP_TABLE = 3,
    /* Port statistics.
    * The request body is struct ofp_port_stats_request.
    * The reply body is an array of struct ofp_port_stats. */
    OFPMP_PORT_STATS = 4,
    /* Queue statistics for a port
    * The request body is struct ofp_queue_stats_request.
    * The reply body is an array of struct ofp_queue_stats */
    OFPMP_QUEUE = 5,
    /* Group counter statistics.
    * The request body is struct ofp_group_stats_request.
    * The reply is an array of struct ofp_group_stats. */
    OFPMP_GROUP = 6,
    /* Group description.
    * The request body is empty.
    * The reply body is an array of struct ofp_group_desc_stats. */
    OFPMP_GROUP_DESC = 7,
    /* Group features.
    * The request body is empty.
    * The reply body is struct ofp_group_features. */
    OFPMP_GROUP_FEATURES = 8,
    /* Meter statistics.
    * The request body is struct ofp_meter_multipart_requests.
    * The reply body is an array of struct ofp_meter_stats. */
    OFPMP_METER = 9,
    /* Meter configuration.
    * The request body is struct ofp_meter_multipart_requests.
    * The reply body is an array of struct ofp_meter_config. */
    OFPMP_METER_CONFIG = 10,
    /* Meter features.
    * The request body is empty.
    * The reply body is struct ofp_meter_features. */
    OFPMP_METER_FEATURES = 11,
    /* Table features.
    * The request body is either empty or contains an array of
    * struct ofp_table_features containing the controller's
    * desired view of the switch. If the switch is unable to
    * set the specified view an error is returned.
    * The reply body is an array of struct ofp_table_features. */
    OFPMP_TABLE_FEATURES = 12,
    /* Port description.
    * The request body is empty.
    * The reply body is an array of struct ofp_port. */
    OFPMP_PORT_DESC = 13,
    /* Experimenter extension.
    * The request and reply bodies begin with
    * struct ofp_experimenter_multipart_header.
    * The request and reply bodies are otherwise experimenter-defined. */
    OFPMP_EXPERIMENTER = 0xffff
};

#define DESC_STR_LEN   256
#define SERIAL_NUM_LEN 32
/* Body of reply to OFPST_DESC request.  Each entry is a NULL-terminated
 * ASCII string. */
 struct ofp_desc_stats {
    char mfr_desc[DESC_STR_LEN];       /* Manufacturer description. */
    char hw_desc[DESC_STR_LEN];        /* Hardware description. */
    char sw_desc[DESC_STR_LEN];        /* Software description. */
    char serial_num[SERIAL_NUM_LEN];   /* Serial number. */
    char dp_desc[DESC_STR_LEN];        /* Human readable description of datapath. */
};
OFP_ASSERT(sizeof(struct ofp_desc_stats) == 1056);

#define OFP_MAX_TABLE_PROPS (16)

/* Common header for all Table Feature Properties */
struct ofp_table_feature_prop_header {
    uint16_t type; /* One of OFPTFPT_*. */
    uint16_t length; /* Length in bytes of this property. */
};
OFP_ASSERT(sizeof(struct ofp_table_feature_prop_header) == 4);

/* Body for ofp_multipart_request of type OFPMP_TABLE_FEATURES./
* Body of reply to OFPMP_TABLE_FEATURES request. */
struct ofp_table_features {
    uint16_t length; /* Length is padded to 64 bits. */
    uint8_t table_id; /* Identifier of table. Lower numbered tables
                         are consulted first. */
    uint8_t pad[5]; /* Align to 64-bits. */
    char name[OFP_MAX_TABLE_NAME_LEN];
    uint64_t metadata_match; /* Bits of metadata table can match. */
    uint64_t metadata_write; /* Bits of metadata table can write. */
    uint32_t config; /* Bitmap of OFPTC_* values */
    uint32_t max_entries; /* Max number of entries supported. */
    /* Table Feature Property list */
    struct ofp_table_feature_prop_header properties[0];
};
OFP_ASSERT(sizeof(struct ofp_table_features) == 64);

/* Table Feature property types.
* Low order bit cleared indicates a property for a regular Flow Entry.
* Low order bit set indicates a property for the Table-Miss Flow Entry.
*/
enum ofp_table_feature_prop_type {
    OFPTFPT_INSTRUCTIONS = 0, /* Instructions property. */
    OFPTFPT_INSTRUCTIONS_MISS = 1, /* Instructions for table-miss. */
    OFPTFPT_NEXT_TABLES = 2, /* Next Table property. */
    OFPTFPT_NEXT_TABLES_MISS = 3, /* Next Table for table-miss. */
    OFPTFPT_WRITE_ACTIONS = 4, /* Write Actions property. */
    OFPTFPT_WRITE_ACTIONS_MISS = 5, /* Write Actions for table-miss. */
    OFPTFPT_APPLY_ACTIONS = 6, /* Apply Actions property. */
    OFPTFPT_APPLY_ACTIONS_MISS = 7, /* Apply Actions for table-miss. */
    OFPTFPT_MATCH = 8, /* Match property. */
    OFPTFPT_WILDCARDS = 10, /* Wildcards property. */
    OFPTFPT_WRITE_SETFIELD = 12, /* Write Set-Field property. */
    OFPTFPT_WRITE_SETFIELD_MISS = 13, /* Write Set-Field for table-miss. */
    OFPTFPT_APPLY_SETFIELD = 14, /* Apply Set-Field property. */
    OFPTFPT_APPLY_SETFIELD_MISS = 15, /* Apply Set-Field for table-miss. */
    OFPTFPT_EXPERIMENTER = 0xFFFE, /* Experimenter property. */
    OFPTFPT_EXPERIMENTER_MISS = 0xFFFF, /* Experimenter for table-miss. */
};

/* Instructions property */
struct ofp_table_feature_prop_instructions {
    uint16_t type; /* One of OFPTFPT_INSTRUCTIONS,
                      OFPTFPT_INSTRUCTIONS_MISS. */
    uint16_t length; /* Length in bytes of this property. */
    /* Followed by:
    * - Exactly (length - 4) bytes containing the instruction ids, then
    * - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
    * bytes of all-zero bytes */
    struct ofp_instruction instruction_ids[0]; /* List of instructions */
};
OFP_ASSERT(sizeof(struct ofp_table_feature_prop_instructions) == 4);

/* Next Tables property */
struct ofp_table_feature_prop_next_tables {
    uint16_t type; /* One of OFPTFPT_NEXT_TABLES,
                      OFPTFPT_NEXT_TABLES_MISS. */
    uint16_t length; /* Length in bytes of this property. */
    /* Followed by:
    * - Exactly (length - 4) bytes containing the table_ids, then
    * - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
    * bytes of all-zero bytes */
    uint8_t next_table_ids[0];
};
OFP_ASSERT(sizeof(struct ofp_table_feature_prop_next_tables) == 4);

/* Actions property */
struct ofp_table_feature_prop_actions {
    uint16_t type; /* One of OFPTFPT_WRITE_ACTIONS,
                      OFPTFPT_WRITE_ACTIONS_MISS,
                      OFPTFPT_APPLY_ACTIONS,
                      OFPTFPT_APPLY_ACTIONS_MISS. */
    uint16_t length; /* Length in bytes of this property. */
    /* Followed by:
    * - Exactly (length - 4) bytes containing the action_ids, then
    * - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
    * bytes of all-zero bytes */
    struct ofp_action_header action_ids[0]; /* List of actions */
};
OFP_ASSERT(sizeof(struct ofp_table_feature_prop_actions) == 4);

/* Match, Wildcard or Set-Field property */
struct ofp_table_feature_prop_oxm {
    uint16_t type; /* One of OFPTFPT_MATCH,
                      OFPTFPT_WILDCARDS,
                      OFPTFPT_WRITE_SETFIELD,
                      OFPTFPT_WRITE_SETFIELD_MISS,
                      OFPTFPT_APPLY_SETFIELD,
                      OFPTFPT_APPLY_SETFIELD_MISS. */
    uint16_t length; /* Length in bytes of this property. */
    /* Followed by:
    * - Exactly (length - 4) bytes containing the oxm_ids, then
    * - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
    * bytes of all-zero bytes */
    uint32_t oxm_ids[0]; /* Array of OXM headers */
};
OFP_ASSERT(sizeof(struct ofp_table_feature_prop_oxm) == 4);

/* Experimenter table feature property */
struct ofp_table_feature_prop_experimenter {
    uint16_t type; /* One of OFPTFPT_EXPERIMENTER,
                      OFPTFPT_EXPERIMENTER_MISS. */
    uint16_t length; /* Length in bytes of this property. */
    uint32_t experimenter; /* Experimenter ID which takes the same
                              form as in struct
                              ofp_experimenter_header. */
    uint32_t exp_type; /* Experimenter defined. */
    /* Followed by:
     * - Exactly (length - 12) bytes containing the experimenter data, then
     * - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
     * bytes of all-zero bytes */
    uint32_t experimenter_data[0];
};
OFP_ASSERT(sizeof(struct ofp_table_feature_prop_experimenter) == 12);

/* Role request and reply message. */
struct ofp_role_request {
    struct ofp_header header; /* Type OFPT_ROLE_REQUEST/OFPT_ROLE_REPLY. */
    uint32_t role; /* One of NX_ROLE_*. */
    uint8_t pad[4]; /* Align to 64 bits. */
    uint64_t generation_id; /* Master Election Generation Id */
};
OFP_ASSERT(sizeof(struct ofp_role_request) == 24);

/* Controller roles. */
enum ofp_controller_role {
    OFPCR_ROLE_NOCHANGE = 0, /* Don't change current role. */
    OFPCR_ROLE_EQUAL = 1, /* Default role, full access. */
    OFPCR_ROLE_MASTER = 2, /* Full access, at most one master. */
    OFPCR_ROLE_SLAVE = 3, /* Read-only access. */
};

/* Asynchronous message configuration. */
struct ofp_async_config {
    struct ofp_header header; /* OFPT_GET_ASYNC_REPLY or OFPT_SET_ASYNC. */
    uint32_t packet_in_mask[2]; /* Bitmasks of OFPR_* values. */
    uint32_t port_status_mask[2]; /* Bitmasks of OFPPR_* values. */
    uint32_t flow_removed_mask[2];/* Bitmasks of OFPRR_* values. */
};
OFP_ASSERT(sizeof(struct ofp_async_config) == 32);

/* Why is this packet being sent to the controller? */
enum ofp_packet_in_reason {
    OFPR_NO_MATCH = 0, /* No matching flow (table-miss flow entry). */
    OFPR_ACTION = 1, /* Action explicitly output to controller. */
    OFPR_INVALID_TTL = 2, /* Packet has invalid TTL */
};

/* Why was this flow removed? */
enum ofp_flow_removed_reason {
    OFPRR_IDLE_TIMEOUT = 0, /* Flow idle time exceeded idle_timeout. */
    OFPRR_HARD_TIMEOUT = 1, /* Time exceeded hard_timeout. */
    OFPRR_DELETE = 2, /* Evicted by a DELETE flow mod. */
    OFPRR_GROUP_DELETE = 3, /* Group was removed. */
};

/* What changed about the physical port */
enum ofp_port_reason {
    OFPPR_ADD,              /* The port was added. */
    OFPPR_DELETE,           /* The port was removed. */
    OFPPR_MODIFY            /* Some attribute of the port has changed. */
};

/* OFPT_ERROR: Error message (datapath -> controller). */
struct ofp_error_msg {
    struct ofp_header header;

    uint16_t type;
    uint16_t code;
    uint8_t data[0];          /* Variable-length data.  Interpreted based
                                 on the type and code. */
};
OFP_ASSERT(sizeof(struct ofp_error_msg) == 12);


/* ofp_error_msg 'code' values for OFPET_HELLO_FAILED.  'data' contains an
 * ASCII text string that may give failure details. */
enum ofp_hello_failed_code {
    OFPHFC_INCOMPATIBLE,        /* No compatible version. */
    OFPHFC_EPERM                /* Permissions error. */
};

/* ofp_error_msg 'code' values for OFPET_TABLE_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp_table_mod_failed_code {
    OFPTMFC_BAD_TABLE = 0, /* Specified table does not exist. */
    OFPTMFC_BAD_CONFIG = 1, /* Specified config is invalid. */
    OFPTMFC_EPERM = 2, /* Permissions error. */
};

/* ofp_error_msg 'code' values for OFPET_SWITCH_CONFIG_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp_switch_config_failed_code {
    OFPSCFC_BAD_FLAGS = 0, /* Specified flags is invalid. */
    OFPSCFC_BAD_LEN = 1, /* Specified len is invalid. */
    OFPSCFC_EPERM = 2, /* Permissions error. */
};

/* ofp_error_msg 'code' values for OFPET_ROLE_REQUEST_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp_role_request_failed_code {
    OFPRRFC_STALE = 0, /* Stale Message: old generation_id. */
    OFPRRFC_UNSUP = 1, /* Controller role change unsupported. */
    OFPRRFC_BAD_ROLE = 2, /* Invalid role. */
};

/* ofp_error_msg 'code' values for OFPET_METER_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp_meter_mod_failed_code {
    OFPMMFC_UNKNOWN = 0, /* Unspecified error. */
    OFPMMFC_METER_EXISTS = 1, /* Meter not added because a Meter ADD
                               * attempted to replace an existing Meter. */
    OFPMMFC_INVALID_METER = 2, /* Meter not added because Meter specified
                                * is invalid. */
    OFPMMFC_UNKNOWN_METER = 3, /* Meter not modified because a Meter
                                  MODIFY attempted to modify a non-existent
                                  Meter. */
    OFPMMFC_BAD_COMMAND = 4, /* Unsupported or unknown command. */
    OFPMMFC_BAD_FLAGS = 5, /* Flag configuration unsupported. */
    OFPMMFC_BAD_RATE = 6, /* Rate unsupported. */
    OFPMMFC_BAD_BURST = 7, /* Burst size unsupported. */
    OFPMMFC_BAD_BAND = 8, /* Band unsupported. */
    OFPMMFC_BAD_BAND_VALUE = 9, /* Band value unsupported. */
    OFPMMFC_OUT_OF_METERS = 10, /* No more meters available. */
    OFPMMFC_OUT_OF_BANDS = 11, /* The maximum number of properties
                                * for a meter has been exceeded. */
};

/* ofp_error_msg 'code' values for OFPET_TABLE_FEATURES_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp_table_features_failed_code {
    OFPTFFC_BAD_TABLE = 0, /* Specified table does not exist. */
    OFPTFFC_BAD_METADATA = 1, /* Invalid metadata mask. */
    OFPTFFC_BAD_TYPE = 2, /* Unknown property type. */
    OFPTFFC_BAD_LEN = 3, /* Length problem in properties. */
    OFPTFFC_BAD_ARGUMENT = 4, /* Unsupported property value. */
    OFPTFFC_EPERM = 5, /* Permissions error. */
};

/* Hello elements types.
*/
enum ofp_hello_elem_type {
    OFPHET_VERSIONBITMAP = 1, /* Bitmap of version supported. */
};

/* Common header for all Hello Elements */
struct ofp_hello_elem_header {
    uint16_t type; /* One of OFPHET_*. */
    uint16_t length; /* Length in bytes of this element. */
};
OFP_ASSERT(sizeof(struct ofp_hello_elem_header) == 4);

/* Version bitmap Hello Element */
struct ofp_hello_elem_versionbitmap {
    uint16_t type; /* OFPHET_VERSIONBITMAP. */
    uint16_t length; /* Length in bytes of this element. */
    /* Followed by:
    * - Exactly (length - 4) bytes containing the bitmaps, then
    * - Exactly (length + 7)/8*8 - (length) (between 0 and 7)
    * bytes of all-zero bytes */
    uint32_t bitmaps[0]; /* List of bitmaps - supported versions */
};
OFP_ASSERT(sizeof(struct ofp_hello_elem_versionbitmap) == 4);

/* OFPT_HELLO. This message includes zero or more hello elements having
* variable size. Unknown elements types must be ignored/skipped, to allow
* for future extensions. */
struct ofp_hello {
    struct ofp_header header;
    /* Hello element list */
    struct ofp_hello_elem_header elements[0];
};
OFP_ASSERT(sizeof(struct ofp_hello) == 8);

/* Experimenter extension. */
struct ofp_experimenter_header {
    struct ofp_header header; /* Type OFPT_EXPERIMENTER. */
    uint32_t experimenter; /* Experimenter ID:
    * - MSB 0: low-order bytes are IEEE OUI.
    * - MSB != 0: defined by ONF. */
    uint32_t exp_type; /* Experimenter defined. */
    /* Experimenter-defined arbitrary additional data. */
};
OFP_ASSERT(sizeof(struct ofp_experimenter_header) == 16);

#endif
