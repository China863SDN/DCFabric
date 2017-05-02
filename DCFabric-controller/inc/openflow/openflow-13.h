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

/* OpenFlow: protocol between controller and datapath. */

#ifndef __OPENFLOW_13_H__
#define __OPENFLOW_13_H__ 1

#include "openflow-common.h"

enum ofp13_type {
    /* Immutable messages. */
    OFPT13_HELLO = 0,              /* Symmetric message */
    OFPT13_ERROR = 1,              /* Symmetric message */
    OFPT13_ECHO_REQUEST = 2,       /* Symmetric message */
    OFPT13_ECHO_REPLY = 3,         /* Symmetric message */
    OFPT13_EXPERIMENTER = 4,       /* Symmetric message */

    /* Switch configuration messages. */
    OFPT13_FEATURES_REQUEST = 5,   /* Controller/switch message */
    OFPT13_FEATURES_REPLY = 6,     /* Controller/switch message */
    OFPT13_GET_CONFIG_REQUEST = 7, /* Controller/switch message */
    OFPT13_GET_CONFIG_REPLY = 8,   /* Controller/switch message */
    OFPT13_SET_CONFIG = 9,         /* Controller/switch message */

    /* Asynchronous messages. */
    OFPT13_PACKET_IN = 10,         /* Async message */
    OFPT13_FLOW_REMOVED = 11,      /* Async message */
    OFPT13_PORT_STATUS = 12,       /* Async message */

    /* Controller command messages. */
    OFPT13_PACKET_OUT = 13,        /* Controller/switch message */
    OFPT13_FLOW_MOD = 14,          /* Controller/switch message */
    OFPT13_GROUP_MOD = 15,         /* Controller/switch message */
    OFPT13_PORT_MOD = 16,          /* Controller/switch message */
    OFPT13_TABLE_MOD = 17,         /* Controller/switch message */

    /* Multipart messages. */
    OFPT13_MULTIPART_REQUEST = 18, /* Controller/switch message */
    OFPT13_MULTIPART_REPLY = 19,   /* Controller/switch message */

    /* Barrier messages. */
    OFPT13_BARRIER_REQUEST = 20,   /* Controller/switch message */
    OFPT13_BARRIER_REPLY = 21,     /* Controller/switch message */

    /* Queue Configuration messages. */
    OFPT13_QUEUE_GET_CONFIG_REQUEST = 22,  /* Controller/switch message */
    OFPT13_QUEUE_GET_CONFIG_REPLY = 23,    /* Controller/switch message */

    /* Controller role change request messages. */
    OFPT13_ROLE_REQUEST = 24,      /* Controller/switch message */
    OFPT13_ROLE_REPLY = 25,        /* Controller/switch message */

    /* Asynchronous message configuration. */
    OFPT13_GET_ASYNC_REQUEST = 26, /* Controller/switch message */
    OFPT13_GET_ASYNC_REPLY = 27,   /* Controller/switch message */
    OFPT13_SET_ASYNC = 28,         /* Controller/switch message */

    /* Meters and rate limiters configuration messages. */
    OFPT13_METER_MOD = 29,         /* Controller/switch message */
};

/* Description of a port */
struct ofp13_port {
    uint32_t port_no;
    uint8_t pad[4];
    uint8_t hw_addr[OFP_ETH_ALEN];
    uint8_t pad2[2];    /* Align to 64 bits. */
    char name[OFP_MAX_PORT_NAME_LEN]; /* Null-terminated */
    uint32_t config;    /* Bitmap of OFPPC_* flags. */
    uint32_t state;     /* Bitmap of OFPPS_* flags. */

    /* Bitmaps of OFPPF_* that describe features. All bits zeroed if
     * unsupported or unavailable. */
    uint32_t curr;       /* Current features. */
    uint32_t advertised; /* Features being advertised by the port. */
    uint32_t supported;  /* Features supported by the port. */
    uint32_t peer;       /* Features advertised by peer. */
    uint32_t curr_speed; /* Current port bitrate in kbps. */
    uint32_t max_speed;  /* Max port bitrate in kbps */
};
OFP_ASSERT(sizeof(struct ofp13_port) == 64);

/* Flags to indicate behavior of the physical port. These flags are
* used in ofp_port to describe the current configuration. They are
* used in the ofp_port_mod message to configure the port's behavior.
*/
enum ofp13_port_config {
    OFPPC13_PORT_DOWN = 1 << 0,   /* Port is administratively down. */
    OFPPC13_NO_RECV = 1 << 2,     /* Drop all packets received by port. */
    OFPPC13_NO_FWD = 1 << 5,      /* Drop packets forwarded to port. */
    OFPPC13_NO_PACKET_IN = 1 << 6 /* Do not send packet-in msgs for port. */
};


/* Current state of the physical port. These are not configurable from
* the controller.
*/
enum ofp13_port_state {
    OFPPS13_LINK_DOWN = 1 << 0, /* No physical link present. */
    OFPPS13_BLOCKED = 1 << 1, /* Port is blocked */
    OFPPS13_LIVE = 1 << 2, /* Live for Fast Failover Group. */
};

/* Port numbering. Ports are numbered starting from 1. */
enum ofp13_port_no {
    /* Maximum number of physical and logical switch ports. */
    OFPP13_MAX = 0xffffff00,
    /* Reserved OpenFlow Port (fake output "ports"). */
    OFPP13_IN_PORT = 0xfffffff8, /* Send the packet out the input port. This
                                  reserved port must be explicitly used
                                  in order to send back out of the input
                                  port. */
    OFPP13_TABLE = 0xfffffff9, /* Submit the packet to the first flow table
                                NB: This destination port can only be
                                used in packet-out messages. */
    OFPP13_NORMAL = 0xfffffffa, /* Process with normal L2/L3 switching. */
    OFPP13_FLOOD = 0xfffffffb, /* All physical ports in VLAN, except input
                                port and those blocked or link down. */
    OFPP13_ALL = 0xfffffffc, /* All physical ports except input port. */
    OFPP13_CONTROLLER = 0xfffffffd, /* Send to controller. */
    OFPP13_LOCAL = 0xfffffffe, /* Local openflow "port". */
    OFPP13_ANY = 0xffffffff /* Wildcard port used only for flow mod
                             (delete) and flow stats requests. Selects
                             all flows regardless of output port
                             (including flows with no output port). */
};

/* Features of ports available in a datapath. */
enum ofp13_port_features {
    OFPPF13_10MB_HD = 1 << 0, /* 10 Mb half-duplex rate support. */
    OFPPF13_10MB_FD = 1 << 1, /* 10 Mb full-duplex rate support. */
    OFPPF13_100MB_HD = 1 << 2, /* 100 Mb half-duplex rate support. */
    OFPPF13_100MB_FD = 1 << 3, /* 100 Mb full-duplex rate support. */
    OFPPF13_1GB_HD = 1 << 4, /* 1 Gb half-duplex rate support. */
    OFPPF13_1GB_FD = 1 << 5, /* 1 Gb full-duplex rate support. */
    OFPPF13_10GB_FD = 1 << 6, /* 10 Gb full-duplex rate support. */
    OFPPF13_40GB_FD = 1 << 7, /* 40 Gb full-duplex rate support. */
    OFPPF13_100GB_FD = 1 << 8, /* 100 Gb full-duplex rate support. */
    OFPPF13_1TB_FD = 1 << 9, /* 1 Tb full-duplex rate support. */
    OFPPF13_OTHER = 1 << 10, /* Other rate, not in the list. */
    OFPPF13_COPPER = 1 << 11, /* Copper medium. */
    OFPPF13_FIBER = 1 << 12, /* Fiber medium. */
    OFPPF13_AUTONEG = 1 << 13, /* Auto-negotiation. */
    OFPPF13_PAUSE = 1 << 14, /* Pause. */
    OFPPF13_PAUSE_ASYM = 1 << 15 /* Asymmetric pause. */
};

/* Full description for a queue. */
struct ofp13_packet_queue {
    uint32_t queue_id; /* id for the specific queue. */
    uint32_t port; /* Port this queue is attached to. */
    uint16_t len; /* Length in bytes of this queue desc. */
    uint8_t pad[6]; /* 64-bit alignment. */
    struct ofp_queue_prop_header properties[0]; /* List of properties. */
};
OFP_ASSERT(sizeof(struct ofp13_packet_queue) == 16);

enum ofp13_queue_properties {
    OFPQT13_MIN_RATE = 1, /* Minimum datarate guaranteed. */
    OFPQT13_MAX_RATE = 2, /* Maximum datarate. */
    OFPQT13_EXPERIMENTER = 0xffff /* Experimenter defined property. */
};

/* Common description for a queue. */
struct ofp13_queue_prop_header {
    uint16_t property; /* One of OFPQT_. */
    uint16_t len; /* Length of property, including this header. */
    uint8_t pad[4]; /* 64-bit alignemnt. */
};
OFP_ASSERT(sizeof(struct ofp13_queue_prop_header) == 8);

/* Min-Rate queue property description. */
struct ofp13_queue_prop_min_rate {
    struct ofp13_queue_prop_header prop_header; /* prop: OFPQT_MIN, len: 16. */
    uint16_t rate; /* In 1/10 of a percent; >1000 -> disabled. */
    uint8_t pad[6]; /* 64-bit alignment */
};
OFP_ASSERT(sizeof(struct ofp13_queue_prop_min_rate));

/* Max-Rate queue property description. */
struct ofp13_queue_prop_max_rate {
    struct ofp_queue_prop_header prop_header; /* prop: OFPQT_MAX, len: 16. */
    uint16_t rate; /* In 1/10 of a percent; >1000 -> disabled. */
    uint8_t pad[6]; /* 64-bit alignment */
};
OFP_ASSERT(sizeof(struct ofp13_queue_prop_max_rate) == 16);

/* Experimenter queue property description. */
struct ofp13_queue_prop_experimenter {
    struct ofp13_queue_prop_header prop_header; /* prop: OFPQT_EXPERIMENTER, len: 16. */
    uint32_t experimenter; /* Experimenter ID which takes the same
                              form as in struct
                              ofp_experimenter_header. */
    uint8_t pad[4]; /* 64-bit alignment */
    uint8_t data[0]; /* Experimenter defined data. */
};
OFP_ASSERT(sizeof(struct ofp13_queue_prop_experimenter) == 16);

/* Header for OXM experimenter match fields. */
struct ofp13_oxm_experimenter_header {
    uint32_t oxm_header; /* oxm_class = OFPXMC_EXPERIMENTER */
    uint32_t experimenter; /* Experimenter ID which takes the same
                              form as in struct ofp_experimenter_header. */
};
OFP_ASSERT(sizeof(struct ofp13_oxm_experimenter_header) == 8);

enum ofp13_action_type {
    OFPAT13_OUTPUT = 0, /* Output to switch port. */
    OFPAT13_COPY_TTL_OUT = 11, /* Copy TTL "outwards" -- from next-to-outermost
                                    to outermost */
    OFPAT13_COPY_TTL_IN = 12, /* Copy TTL "inwards" -- from outermost to
                                    next-to-outermost */
    OFPAT13_MPLS_TTL = 15, /* MPLS TTL */
    OFPAT13_DEC_MPLS_TTL = 16, /* Decrement MPLS TTL */
    OFPAT13_PUSH_VLAN = 17, /* Push a new VLAN tag */
    OFPAT13_POP_VLAN = 18, /* Pop the outer VLAN tag */
    OFPAT13_PUSH_MPLS = 19, /* Push a new MPLS tag */
    OFPAT13_POP_MPLS = 20, /* Pop the outer MPLS tag */
    OFPAT13_SET_QUEUE = 21, /* Set queue id when outputting to a port */
    OFPAT13_GROUP = 22, /* Apply group. */
    OFPAT13_SET_NW_TTL = 23, /* IP TTL. */
    OFPAT13_DEC_NW_TTL = 24, /* Decrement IP TTL. */
    OFPAT13_SET_FIELD = 25, /* Set a header field using OXM TLV format. */
    OFPAT13_PUSH_PBB = 26, /* Push a new PBB service tag (I-TAG) */
    OFPAT13_POP_PBB = 27, /* Pop the outer PBB service tag (I-TAG) */
    OFPAT13_EXPERIMENTER = 0xffff
};

/* Action structure for OFPAT_OUTPUT, which sends packets out 'port'.
* When the 'port' is the OFPP_CONTROLLER, 'max_len' indicates the max
* number of bytes to send. A 'max_len' of zero means no bytes of the
* packet should be sent. A 'max_len' of OFPCML_NO_BUFFER means that
* the packet is not buffered and the complete packet is to be sent to
* the controller. */
struct ofp13_action_output {
    uint16_t type; /* OFPAT_OUTPUT. */
    uint16_t len; /* Length is 16. */
    uint32_t port; /* Output port. */
    uint16_t max_len; /* Max length to send to controller. */
    uint8_t pad[6]; /* Pad to 64 bits. */
};
OFP_ASSERT(sizeof(struct ofp13_action_output) == 16);

/* OFPAT_SET_QUEUE action struct: send packets to given queue on port. */
struct ofp13_action_set_queue {
    uint16_t type; /* OFPAT_SET_QUEUE. */
    uint16_t len; /* Len is 8. */
    uint32_t queue_id; /* Queue id for the packets. */
};
OFP_ASSERT(sizeof(struct ofp13_action_set_queue) == 8);

/* Switch features. */
struct ofp13_switch_features {
    struct ofp_header header;
    uint64_t datapath_id; /* Datapath unique ID. The lower 48-bits are for
                             a MAC address, while the upper 16-bits are
                             implementer-defined. */
    uint32_t n_buffers; /* Max packets buffered at once. */
    uint8_t n_tables; /* Number of tables supported by datapath. */
    uint8_t auxiliary_id; /* Identify auxiliary connections */
    uint8_t pad[2]; /* Align to 64-bits. */
    /* Features. */
    uint32_t capabilities; /* Bitmap of support "ofp_capabilities". */
    uint32_t reserved;
};
OFP_ASSERT(sizeof(struct ofp13_switch_features) == 32);

/* Capabilities supported by the datapath. */
enum ofp13_capabilities {
    OFPC13_FLOW_STATS = 1 << 0, /* Flow statistics. */
    OFPC13_TABLE_STATS = 1 << 1, /* Table statistics. */
    OFPC13_PORT_STATS = 1 << 2, /* Port statistics. */
    OFPC13_GROUP_STATS = 1 << 3, /* Group statistics. */
    OFPC13_IP_REASM = 1 << 5, /* Can reassemble IP fragments. */
    OFPC13_QUEUE_STATS = 1 << 6, /* Queue statistics. */
    OFPC13_PORT_BLOCKED = 1 << 8 /* Switch will block looping ports. */
};

/* Flow setup and teardown (controller -> datapath). */
struct ofp13_flow_mod {
    struct ofp_header header;
    uint64_t cookie; /* Opaque controller-issued identifier. */
    uint64_t cookie_mask; /* Mask used to restrict the cookie bits
                             that must match when the command is
                             OFPFC_MODIFY* or OFPFC_DELETE*. A value
                             of 0 indicates no restriction. */
    /* Flow actions. */
    uint8_t table_id; /* ID of the table to put the flow in.
                         For OFPFC_DELETE_* commands, OFPTT_ALL
                         can also be used to delete matching
                         flows from all tables. */
    uint8_t command; /* One of OFPFC_*. */
    uint16_t idle_timeout; /* Idle time before discarding (seconds). */
    uint16_t hard_timeout; /* Max time before discarding (seconds). */
    uint16_t priority; /* Priority level of flow entry. */
    uint32_t buffer_id; /* Buffered packet to apply to, or
                           OFP_NO_BUFFER.
                            Not meaningful for OFPFC_DELETE*. */
    uint32_t out_port; /* For OFPFC_DELETE* commands, require
                          matching entries to include this as an
                          output port. A value of OFPP_ANY
                          indicates no restriction. */
    uint32_t out_group; /* For OFPFC_DELETE* commands, require
                           matching e.ntries to include this as an
                           output group. A value of OFPG_ANY
                           indicates no restriction. */
    uint16_t flags; /* One of OFPFF_*. */
    uint8_t pad[2];
    struct ofpx_match match; /* Fields to match. Variable size. */
    //struct ofp_instruction instructions[0]; /* Instruction set */
};
OFP_ASSERT(sizeof(struct ofp13_flow_mod) == 56);

enum ofp13_flow_mod_flags {
    OFPFF13_SEND_FLOW_REM = 1 << 0, /* Send flow removed message when flow
                                                   * expires or is deleted. */
    OFPFF13_CHECK_OVERLAP = 1 << 1, /* Check for overlapping entries first. */
    OFPFF13_RESET_COUNTS = 1 << 2, /* Reset flow packet and byte counts. */
    OFPFF13_NO_PKT_COUNTS = 1 << 3, /* Don't keep track of packet count. */
    OFPFF13_NO_BYT_COUNTS = 1 << 4, /* Don't keep track of byte count. */
};

/* Modify behavior of the physical port */
struct ofp13_port_mod {
    struct ofp_header header;
    uint32_t port_no;
    uint8_t pad[4];
    uint8_t hw_addr[OFP_ETH_ALEN]; /* The hardware address is not
                                      configurable. This is used to
                                      sanity-check the request, so it must
                                      be the same as returned in an
                                      ofp_port struct. */
    uint8_t pad2[2]; /* Pad to 64 bits. */
    uint32_t config; /* Bitmap of OFPPC13_* flags. */
    uint32_t mask; /* Bitmap of OFPPC13_* flags to be changed. */
    uint32_t advertise; /* Bitmap of OFPPF13_*. Zero all bits to prevent
                          any action taking place. */
    uint8_t pad3[4]; /* Pad to 64 bits. */
};
OFP_ASSERT(sizeof(struct ofp13_port_mod) == 40);

/* Body for ofp_multipart_request of type OFPMP_FLOW. */
struct ofp13_flow_stats_request {
    uint8_t table_id; /* ID of table to read (from ofp_table_stats),
                         OFPTT_ALL for all tables. */
    uint8_t pad[3]; /* Align to 32 bits. */
    uint32_t out_port; /* Require matching entries to include this
                          as an output port. A value of OFPP_ANY
                          indicates no restriction. */
    uint32_t out_group; /* Require matching entries to include this
                           as an output group. A value of OFPG_ANY
                           indicates no restriction. */
    uint8_t pad2[4]; /* Align to 64 bits. */
    uint64_t cookie; /* Require matching entries to contain this
                        cookie value */
    uint64_t cookie_mask; /* Mask used to restrict the cookie bits that
                             must match. A value of 0 indicates
                             no restriction. */
    struct ofpx_match match; /* Fields to match. Variable size. */
};
OFP_ASSERT(sizeof(struct ofp13_flow_stats_request) == 40);

/* Body of reply to OFPMP_FLOW request. */
struct ofp13_flow_stats {
    uint16_t length; /* Length of this entry. */
    uint8_t table_id; /* ID of table flow came from. */
    uint8_t pad;
    uint32_t duration_sec; /* Time flow has been alive in seconds. */
    uint32_t duration_nsec; /* Time flow has been alive in nanoseconds beyond
                              duration_sec. */
    uint16_t priority; /* Priority of the entry. */
    uint16_t idle_timeout; /* Number of seconds idle before expiration. */
    uint16_t hard_timeout; /* Number of seconds before expiration. */
    uint16_t flags; /* One of OFPFF_*. */
    uint8_t pad2[4]; /* Align to 64-bits. */
    uint64_t cookie; /* Opaque controller-issued identifier. */
    uint64_t packet_count; /* Number of packets in flow. */
    uint64_t byte_count; /* Number of bytes in flow. */
    struct ofpx_match match; /* Description of fields. Variable size. */
    //struct ofp_instruction instructions[0]; /* Instruction set. */
};
OFP_ASSERT(sizeof(struct ofp13_flow_stats) == 56);


/* Body of reply to OFPMP_FLOW request. */
struct ofp13_flow_stats_gn {
    uint16_t length; /* Length of this entry. */
    uint8_t table_id; /* ID of table flow came from. */
    uint8_t pad;
    struct ofp_match match; /* Description of fields. Variable size. */
    uint32_t duration_sec; /* Time flow has been alive in seconds. */
    uint32_t duration_nsec; /* Time flow has been alive in nanoseconds beyond
                              duration_sec. */
    uint16_t priority; /* Priority of the entry. */
    uint16_t idle_timeout; /* Number of seconds idle before expiration. */
    uint16_t hard_timeout; /* Number of seconds before expiration. */
    uint16_t flags; /* One of OFPFF_*. */
    uint8_t pad2[6]; /* Align to 64-bits. */
    uint64_t cookie; /* Opaque controller-issued identifier. */
    uint64_t packet_count; /* Number of packets in flow. */
    uint64_t byte_count; /* Number of bytes in flow. */
    //struct ofp_instruction instructions[0]; /* Instruction set. */
};
OFP_ASSERT(sizeof(struct ofp13_flow_stats_gn) == 96);

/* Body for ofp_multipart_request of type OFPMP_AGGREGATE. */
struct ofp13_aggregate_stats_request {
    uint8_t table_id; /* ID of table to read (from ofp_table_stats)
                         OFPTT_ALL for all tables. */
    uint8_t pad[3]; /* Align to 32 bits. */
    uint32_t out_port; /* Require matching entries to include this
                          as an output port. A value of OFPP_ANY
                          indicates no restriction. */
    uint32_t out_group; /* Require matching entries to include this
                            as an output group. A value of OFPG_ANY
                            indicates no restriction. */
    uint8_t pad2[4]; /* Align to 64 bits. */
    uint64_t cookie; /* Require matching entries to contain this
                        cookie value */
    uint64_t cookie_mask; /* Mask used to restrict the cookie bits that
                            must match. A value of 0 indicates
                            no restriction. */
    struct ofpx_match match; /* Fields to match. Variable size. */
};
OFP_ASSERT(sizeof(struct ofp13_aggregate_stats_request) == 40);

/* Body of reply to OFPMP_TABLE request. */
struct ofp13_table_stats {
    uint8_t table_id; /* Identifier of table. Lower numbered tables
                    are consulted first. */
    uint8_t pad[3]; /* Align to 32-bits. */
    uint32_t active_count; /* Number of active entries. */
    uint64_t lookup_count; /* Number of packets looked up in table. */
    uint64_t matched_count; /* Number of packets that hit table. */
};
OFP_ASSERT(sizeof(struct ofp13_table_stats) == 24);

/* Body for ofp_multipart_request of type OFPMP_PORT. */
struct ofp13_port_stats_request {
    uint32_t port_no; /* OFPMP_PORT message must request statistics
                       * either for a single port (specified in
                       * port_no) or for all ports (if port_no ==
                       * OFPP_ANY). */
    uint8_t pad[4];
};
OFP_ASSERT(sizeof(struct ofp13_port_stats_request) == 8);

/* Body of reply to OFPMP_PORT request. If a counter is unsupported, set
* the field to all ones. */
struct ofp13_port_stats {
    uint32_t port_no;
    uint8_t pad[4]; /* Align to 64-bits. */
    uint64_t rx_packets; /* Number of received packets. */
    uint64_t tx_packets; /* Number of transmitted packets. */
    uint64_t rx_bytes; /* Number of received bytes. */
    uint64_t tx_bytes; /* Number of transmitted bytes. */
    uint64_t rx_dropped; /* Number of packets dropped by RX. */
    uint64_t tx_dropped; /* Number of packets dropped by TX. */
    uint64_t rx_errors; /* Number of receive errors. This is a super-set
                            of more specific receive errors and should be
                            greater than or equal to the sum of all
                            rx_*_err values. */
    uint64_t tx_errors; /* Number of transmit errors. This is a super-set
                            of more specific transmit errors and should be
                            greater than or equal to the sum of all
                            tx_*_err values (none currently defined.) */
    uint64_t rx_frame_err; /* Number of frame alignment errors. */
    uint64_t rx_over_err; /* Number of packets with RX overrun. */
    uint64_t rx_crc_err; /* Number of CRC errors. */
    uint64_t collisions; /* Number of collisions. */
    uint32_t duration_sec; /* Time port has been alive in seconds. */
    uint32_t duration_nsec; /* Time port has been alive in nanoseconds beyond
                                duration_sec. */
};
OFP_ASSERT(sizeof(struct ofp13_port_stats) == 112);

struct ofp13_queue_stats_request {
    uint32_t port_no; /* All ports if OFPP_ANY. */
    uint32_t queue_id; /* All queues if OFPQ_ALL. */
};
OFP_ASSERT(sizeof(struct ofp13_queue_stats_request) == 8);

struct ofp13_queue_stats {
    uint32_t port_no;
    uint32_t queue_id; /* Queue i.d */
    uint64_t tx_bytes; /* Number of transmitted bytes. */
    uint64_t tx_packets; /* Number of transmitted packets. */
    uint64_t tx_errors; /* Number of packets dropped due to overrun. */
    uint32_t duration_sec; /* Time queue has been alive in seconds. */
    uint32_t duration_nsec; /* Time queue has been alive in nanoseconds beyond
                                duration_sec. */
};
OFP_ASSERT(sizeof(struct ofp13_queue_stats) == 40);

/* Send packet (controller -> datapath). */
struct ofp13_packet_out {
    struct ofp_header header;
    uint32_t buffer_id; /* ID assigned by datapath (OFP_NO_BUFFER
                            if none). */
    uint32_t in_port; /* Packet's input port or OFPP_CONTROLLER. */
    uint16_t actions_len; /* Size of action array in bytes. */
    uint8_t pad[6];
    struct ofp_action_header actions[0]; /* Action list. */
    /* uint8_t data[0]; */ /* Packet data. The length is inferred
    from the length field in the header.
    (Only meaningful if buffer_id == -1.) */
};
OFP_ASSERT(sizeof(struct ofp13_packet_out) == 24);

/* Packet received on port (datapath -> controller). */
struct ofp13_packet_in {
    struct ofp_header header;
    uint32_t buffer_id; /* ID assigned by datapath. */
    uint16_t total_len; /* Full length of frame. */
    uint8_t reason; /* Reason packet is being sent (one of OFPR_*) */
    uint8_t table_id; /* ID of the table that was looked up */
    uint64_t cookie; /* Cookie of the flow entry that was looked up. */
    struct ofpx_match match; /* Packet metadata. Variable size. */
    /* Followed by:
    * - Exactly 2 all-zero padding bytes, then
    * - An Ethernet frame whose length is inferred from header.length.
    * The padding bytes preceding the Ethernet frame ensure that the IP
    * header (if any) following the Ethernet header is 32-bit aligned.
    */
    //uint8_t pad[2]; /* Align to 64 bit + 16 bit */
    //uint8_t data[0]; /* Ethernet frame */
};
OFP_ASSERT(sizeof(struct ofp13_packet_in) == 32);

/* Flow removed (datapath -> controller). */
struct ofp13_flow_removed {
    struct ofp_header header;
    uint64_t cookie; /* Opaque controller-issued identifier. */
    uint16_t priority; /* Priority level of flow entry. */
    uint8_t reason; /* One of OFPRR_*. */
    uint8_t table_id; /* ID of the table */
    uint32_t duration_sec; /* Time flow was alive in seconds. */
    uint32_t duration_nsec; /* Time flow was alive in nanoseconds beyond
                               duration_sec. */
    uint16_t idle_timeout; /* Idle timeout from original flow mod. */
    uint16_t hard_timeout; /* Hard timeout from original flow mod. */
    uint64_t packet_count;
    uint64_t byte_count;
    struct ofpx_match match; /* Description of fields. Variable size. */
};
OFP_ASSERT(sizeof(struct ofp13_flow_removed) == 56);

/* A physical port has changed in the datapath */
struct ofp13_port_status {
    struct ofp_header header;
    uint8_t reason; /* One of OFPPR_*. */
    uint8_t pad[7]; /* Align to 64-bits. */
    struct ofp13_port desc;
};
OFP_ASSERT(sizeof(struct ofp13_port_status) == 80);

/* Values for 'type' in ofp_error_message. These values are immutable: they
* will not change in future versions of the protocol (although new values may
* be added). */
enum ofp13_error_type {
    OFPET13_HELLO_FAILED = 0, /* Hello protocol failed. */
    OFPET13_BAD_REQUEST = 1, /* Request was not understood. */
    OFPET13_BAD_ACTION = 2, /* Error in action description. */
    OFPET13_BAD_INSTRUCTION = 3, /* Error in instruction list. */
    OFPET13_BAD_MATCH = 4, /* Error in match. */
    OFPET13_FLOW_MOD_FAILED = 5, /* Problem modifying flow entry. */
    OFPET13_GROUP_MOD_FAILED = 6, /* Problem modifying group entry. */
    OFPET13_PORT_MOD_FAILED = 7, /* Port mod request failed. */
    OFPET13_TABLE_MOD_FAILED = 8, /* Table mod request failed. */
    OFPET13_QUEUE_OP_FAILED = 9, /* Queue operation failed. */
    OFPET13_SWITCH_CONFIG_FAILED = 10, /* Switch config request failed. */
    OFPET13_ROLE_REQUEST_FAILED = 11, /* Controller Role request failed. */
    OFPET13_METER_MOD_FAILED = 12, /* Error in meter. */
    OFPET13_TABLE_FEATURES_FAILED = 13, /* Setting table features failed. */
    OFPET13_EXPERIMENTER = 0xffff /* Experimenter error messages. */
};

/* ofp_error_msg 'code' values for OFPET_BAD_REQUEST. 'data' contains at least
* the first 64 bytes of the failed request. */
enum ofp13_bad_request_code {
    OFPBRC13_BAD_VERSION = 0, /* ofp_header.version not supported. */
    OFPBRC13_BAD_TYPE = 1, /* ofp_header.type not supported. */
    OFPBRC13_BAD_MULTIPART = 2, /* ofp_multipart_request.type not supported. */
    OFPBRC13_BAD_EXPERIMENTER = 3, /* Experimenter id not supported
                                  * (in ofp_experimenter_header or
                                  * ofp_multipart_request or
                                  * ofp_multipart_reply). */
    OFPBRC13_BAD_EXP_TYPE = 4, /* Experimenter type not supported. */
    OFPBRC13_EPERM = 5, /* Permissions error. */
    OFPBRC13_BAD_LEN = 6, /* Wrong request length for type. */
    OFPBRC13_BUFFER_EMPTY = 7, /* Specified buffer has already been used. */
    OFPBRC13_BUFFER_UNKNOWN = 8, /* Specified buffer does not exist. */
    OFPBRC13_BAD_TABLE_ID = 9, /* Specified table-id invalid or does not
                              * exist. */
    OFPBRC13_IS_SLAVE = 10, /* Denied because controller is slave. */
    OFPBRC13_BAD_PORT = 11, /* Invalid port. */
    OFPBRC13_BAD_PACKET = 12, /* Invalid packet in packet-out. */
    OFPBRC13_MULTIPART_BUFFER_OVERFLOW = 13, /* ofp_multipart_request
                                              overflowed the assigned buffer. */
};

/* ofp_error_msg 'code' values for OFPET_BAD_ACTION. 'data' contains at least
* the first 64 bytes of the failed request. */
enum ofp13_bad_action_code {
    OFPBAC13_BAD_TYPE = 0, /* Unknown action type. */
    OFPBAC13_BAD_LEN = 1, /* Length problem in actions. */
    OFPBAC13_BAD_EXPERIMENTER = 2, /* Unknown experimenter id specified. */
    OFPBAC13_BAD_EXP_TYPE = 3, /* Unknown action for experimenter id. */
    OFPBAC13_BAD_OUT_PORT = 4, /* Problem validating output port. */
    OFPBAC13_BAD_ARGUMENT = 5, /* Bad action argument. */
    OFPBAC13_EPERM = 6, /* Permissions error. */
    OFPBAC13_TOO_MANY = 7, /* Can't handle this many actions. */
    OFPBAC13_BAD_QUEUE = 8, /* Problem validating output queue. */
    OFPBAC13_BAD_OUT_GROUP = 9, /* Invalid group id in forward action. */
    OFPBAC13_MATCH_INCONSISTENT = 10, /* Action can't apply for this match,
                                       or Set-Field missing prerequisite. */
    OFPBAC13_UNSUPPORTED_ORDER = 11, /* Action order is unsupported for the
                                      action list in an Apply-Actions instruction */
    OFPBAC13_BAD_TAG = 12, /* Actions uses an unsupported
                            tag/encap. */
    OFPBAC13_BAD_SET_TYPE = 13, /* Unsupported type in SET_FIELD action. */
    OFPBAC13_BAD_SET_LEN = 14, /* Length problem in SET_FIELD action. */
    OFPBAC13_BAD_SET_ARGUMENT = 15, /* Bad argument in SET_FIELD action. */
};

/* ofp_error_msg 'code' values for OFPET_BAD_INSTRUCTION. 'data' contains at least
* the first 64 bytes of the failed request. */
enum ofp13_bad_instruction_code {
    OFPBIC13_UNKNOWN_INST = 0, /* Unknown instruction. */
    OFPBIC13_UNSUP_INST = 1, /* Switch or table does not support the
                              instruction. */
    OFPBIC13_BAD_TABLE_ID = 2, /* Invalid Table-ID specified. */
    OFPBIC13_UNSUP_METADATA = 3, /* Metadata value unsupported by datapath. */
    OFPBIC13_UNSUP_METADATA_MASK = 4, /* Metadata mask value unsupported by
                                       datapath. */
    OFPBIC13_BAD_EXPERIMENTER = 5, /* Unknown experimenter id specified. */
    OFPBIC13_BAD_EXP_TYPE = 6, /* Unknown instruction for experimenter id. */
    OFPBIC13_BAD_LEN = 7, /* Length problem in instructions. */
    OFPBIC13_EPERM = 8, /* Permissions error. */
};

/* ofp_error_msg 'code' values for OFPET_BAD_MATCH. 'data' contains at least
* the first 64 bytes of the failed request. */
enum ofp13_bad_match_code {
    OFPBMC13_BAD_TYPE = 0, /* Unsupported match type specified by the
                            match */
    OFPBMC13_BAD_LEN = 1, /* Length problem in match. */
    OFPBMC13_BAD_TAG = 2, /* Match uses an unsupported tag/encap. */
    OFPBMC13_BAD_DL_ADDR_MASK = 3, /* Unsupported datalink addr mask - switch
                                    does not support arbitrary datalink
                                    address mask. */
    OFPBMC13_BAD_NW_ADDR_MASK = 4, /* Unsupported network addr mask - switch
                                    does not support arbitrary network
                                    address mask. */
    OFPBMC13_BAD_WILDCARDS = 5, /* Unsupported combination of fields masked
                                 or omitted in the match. */
    OFPBMC13_BAD_FIELD = 6, /* Unsupported field type in the match. */
    OFPBMC13_BAD_VALUE = 7, /* Unsupported value in a match field. */
    OFPBMC13_BAD_MASK = 8, /* Unsupported mask specified in the match,
                            field is not dl-address or nw-address. */
    OFPBMC13_BAD_PREREQ = 9, /* A prerequisite was not met. */
    OFPBMC13_DUP_FIELD = 10, /* A field type was duplicated. */
    OFPBMC13_EPERM = 11, /* Permissions error. */
};

/* ofp_error_msg 'code' values for OFPET_FLOW_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp13_flow_mod_failed_code {
    OFPFMFC13_UNKNOWN = 0, /* Unspecified error. */
    OFPFMFC13_TABLE_FULL = 1, /* Flow not added because table was full. */
    OFPFMFC13_BAD_TABLE_ID = 2, /* Table does not exist */
    OFPFMFC13_OVERLAP = 3, /* Attempted to add overlapping flow with
                            CHECK_OVERLAP flag set. */
    OFPFMFC13_EPERM = 4, /* Permissions error. */
    OFPFMFC13_BAD_TIMEOUT = 5, /* Flow not added because of unsupported
                                idle/hard timeout. */
    OFPFMFC13_BAD_COMMAND = 6, /* Unsupported or unknown command. */
    OFPFMFC13_BAD_FLAGS = 7, /* Unsupported or unknown flags. */
};

/* ofp_error_msg 'code' values for OFPET_GROUP_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp13_group_mod_failed_code {
    OFPGMFC13_GROUP_EXISTS = 0, /* Group not added because a group ADD
                                 attempted to replace an
                                 already-present group. */
    OFPGMFC13_INVALID_GROUP = 1, /* Group not added because Group
                                  specified is invalid. */
    OFPGMFC13_WEIGHT_UNSUPPORTED = 2, /* Switch does not support unequal load
                                        sharing with select groups. */
    OFPGMFC13_OUT_OF_GROUPS = 3, /* The group table is full. */
    OFPGMFC13_OUT_OF_BUCKETS = 4, /* The maximum number of action buckets
                                   for a group has been exceeded. */
    OFPGMFC13_CHAINING_UNSUPPORTED = 5, /* Switch does not support groups that
                                         forward to groups. */
    OFPGMFC13_WATCH_UNSUPPORTED = 6, /* This group cannot watch the watch_port
                                      or watch_group specified. */
    OFPGMFC13_LOOP = 7, /* Group entry would cause a loop. */
    OFPGMFC13_UNKNOWN_GROUP = 8, /* Group not modified because a group
                                    MODIFY attempted to modify a
                                    non-existent group. */
    OFPGMFC13_CHAINED_GROUP = 9, /* Group not deleted because another
                                  group is forwarding to it. */
    OFPGMFC13_BAD_TYPE = 10, /* Unsupported or unknown group type. */
    OFPGMFC13_BAD_COMMAND = 11, /* Unsupported or unknown command. */
    OFPGMFC13_BAD_BUCKET = 12, /* Error in bucket. */
    OFPGMFC13_BAD_WATCH = 13, /* Error in watch port/group. */
    OFPGMFC13_EPERM = 14, /* Permissions error. */
};

/* ofp_error_msg 'code' values for OFPET_PORT_MOD_FAILED. 'data' contains
* at least the first 64 bytes of the failed request. */
enum ofp13_port_mod_failed_code {
    OFPPMFC13_BAD_PORT = 0, /* Specified port number does not exist. */
    OFPPMFC13_BAD_HW_ADDR = 1, /* Specified hardware address does not
                              * match the port number. */
    OFPPMFC13_BAD_CONFIG = 2, /* Specified config is invalid. */
    OFPPMFC13_BAD_ADVERTISE = 3, /* Specified advertise is invalid. */
    OFPPMFC13_EPERM = 4, /* Permissions error. */
};

/* ofp_error msg 'code' values for OFPET_QUEUE_OP_FAILED. 'data' contains
* at least the first 64 bytes of the failed request */
enum ofp13_queue_op_failed_code {
    OFPQOFC13_BAD_PORT = 0, /* Invalid port (or port does not exist). */
    OFPQOFC13_BAD_QUEUE = 1, /* Queue does not exist. */
    OFPQOFC13_EPERM = 2, /* Permissions error. */
};

/* OFPET_EXPERIMENTER: Error message (datapath -> controller). */
struct ofp13_error_experimenter_msg {
    struct ofp_header header;
    uint16_t type; /* OFPET_EXPERIMENTER. */
    uint16_t exp_type; /* Experimenter defined. */
    uint32_t experimenter; /* Experimenter ID which takes the same form
                              as in struct ofp_experimenter_header. */
    uint8_t data[0]; /* Variable-length data. Interpreted based
                        on the type and code. No padding. */
};
OFP_ASSERT(sizeof(struct ofp13_error_experimenter_msg) == 16);

struct ofp13_error_msg
{
    struct ofp_header header;
    uint16_t type;
    uint16_t code;
    uint8_t  data[0];
};
OFP_ASSERT(sizeof(struct ofp13_error_msg) == 12);

/* Switch configuration. */
struct ofp13_switch_config {
    struct ofp_header header;
    uint16_t flags;             /* OFPC_* flags. */
    uint16_t miss_send_len;     /* Max bytes of new flow that datapath should
                                   send to the controller. */
};
OFP_ASSERT(sizeof(struct ofp13_switch_config) == 12);

struct ofp13_multipart_reply {
    struct ofp_header header;
    uint16_t type; /* One of the OFPMP_* constants. */
    uint16_t flags; /* OFPMPF_REPLY_* flags. */
    uint8_t pad[4];
    uint8_t body[0]; /* Body of the reply. */
};
OFP_ASSERT(sizeof(struct ofp13_multipart_reply) == 16);

/* Role request and reply message. */
struct ofp13_role_request {
    struct ofp_header header; /* Type OFPT_ROLE_REQUEST/OFPT_ROLE_REPLY. */
    uint32_t role; /* One of NX_ROLE_*. */
    uint8_t pad[4]; /* Align to 64 bits. */
    uint64_t generation_id; /* Master Election Generation Id */
};
OFP_ASSERT(sizeof(struct ofp13_role_request) == 24);

/* Role request and reply message. */
struct ofp13_role_reply {
    struct ofp_header header; /* Type OFPT_ROLE_REQUEST/OFPT_ROLE_REPLY. */
    uint32_t role; /* One of NX_ROLE_*. */
    uint8_t pad[4]; /* Align to 64 bits. */
    uint64_t generation_id; /* Master Election Generation Id */
};
OFP_ASSERT(sizeof(struct ofp13_role_request) == 24);

struct ofp13_meter_features {
    uint32_t max_meter;        /* Maximum number of meters. */
    uint32_t band_types;        /* Can support max 32 band types. */
    uint32_t capabilities;      /* Supported flags. */
    uint8_t  max_bands;
    uint8_t  max_color;
    uint8_t  pad[2];
};
OFP_ASSERT(sizeof(struct ofp13_meter_features) == 16);

#endif
