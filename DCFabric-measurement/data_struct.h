#define ALERT_CANCEL_PACKET_SIZE 2
#define ALERT_CANCEL_TYPE 0x11

//解除告警信息
typedef struct AlertCancelPacket {
	unsigned int timeStamp;
	unsigned char protocol_type;//0x11
	unsigned char EGP_ID;
	unsigned int dropPackets;
}ALERT_CANCEL_PACKET;

int encode_alert_cancel_packet(ALERT_CANCEL_PACKET pkt, unsigned char* msg);

void decode_alert_cancel_packet(ALERT_CANCEL_PACKET* pkt, unsigned char* msg);

void print_alert_cancel_packet(ALERT_CANCEL_PACKET pkt);

