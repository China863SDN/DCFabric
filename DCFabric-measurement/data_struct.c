#include "data_struct.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>

int encode_alert_cancel_packet(ALERT_CANCEL_PACKET pkt, unsigned char* msg) {
	int len = 0;
	unsigned char* p = msg;
	memcpy(p, &pkt.timeStamp, 4);
	p += 4;
	len += 4;
	
	memcpy(p, &pkt.protocol_type, 1);
	p++;
	len++;
	
	memcpy(p, &pkt.EGP_ID, 1);
	p++;
	len++;

	memcpy(p, &pkt.dropPackets, 4);
	p += 4;
	len += 4;

	return len;
}

void decode_alert_cancel_packet(ALERT_CANCEL_PACKET* pkt, unsigned char* msg) {
	unsigned char* p = msg;
	memcpy(&pkt->timeStamp, p, 4);
	p += 4;
	
	memcpy(&pkt->protocol_type, p, 1);
	p++;

	memcpy(&pkt->EGP_ID, p, 1);
	p++;
}

void print_alert_cancel_packet(ALERT_CANCEL_PACKET pkt) {
	printf("protocol_type = %02x | EGP_ID = %d\n", pkt.protocol_type, pkt.EGP_ID);
}
