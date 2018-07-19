#include "sensor_struct.h"
#include <math.h>

u64 read_REG(u32 address_low, u32 address_high) {	
    u64 low = fast_reg_rd(address_low);
    u64 high = fast_reg_rd(address_high);
	return (high<<32)+low;
}


void read_counters(COUNTER_VALUE *v) {
	COUNTER_VALUE v1, v2;

    v1.c_in_pkt = read_REG(PKT_RX_CNT_L, PKT_RX_CNT_H);
	v1.c_out_pkt = read_REG(PKT_TX_CNT_L, PKT_TX_CNT_H);
	v1.c_in_byte = read_REG(PKT_RX_BYTE_CNT_L, PKT_RX_BYTE_CNT_H);
	v1.c_out_byte = read_REG(PKT_TX_BYTE_CNT_L, PKT_TX_BYTE_CNT_H);
		
	usleep(SLEEP_INTERVAL);

	v2.c_in_pkt = read_REG(PKT_RX_CNT_L, PKT_RX_CNT_H);
	v2.c_out_pkt = read_REG(PKT_TX_CNT_L, PKT_TX_CNT_H);
	v2.c_in_byte = read_REG(PKT_RX_BYTE_CNT_L, PKT_RX_BYTE_CNT_H);
	v2.c_out_byte = read_REG(PKT_TX_BYTE_CNT_L, PKT_TX_BYTE_CNT_H);

	if(v2.c_in_pkt < v1.c_in_pkt){
		v->c_in_pkt = v2.c_in_pkt + (0xffffffffffffffff - v1.c_in_pkt);
	}else{
		v->c_in_pkt = v2.c_in_pkt - v1.c_in_pkt;
	}

	if(v2.c_out_pkt < v1.c_out_pkt){
		v->c_out_pkt = v2.c_out_pkt + (0xffffffffffffffff - v1.c_out_pkt);
	}else{
		v->c_out_pkt = v2.c_out_pkt - v1.c_out_pkt;
	}

	if(v2.c_in_byte < v1.c_in_byte){
		v->c_in_byte = v2.c_in_byte + (0xffffffffffffffff - v1.c_in_byte);
	}else{
		v->c_in_byte = v2.c_in_byte - v1.c_in_byte;
	}

	if(v2.c_out_pkt < v1.c_out_pkt){
		v->c_out_byte = v2.c_out_byte + (0xffffffffffffffff - v1.c_out_byte);
	}else{
		v->c_out_byte = v2.c_out_byte - v1.c_out_byte;
	}

}

void caculate_data_vector(DATA_VECTOR *d, COUNTER_VALUE *v) {

    d->byte_ps = (v->c_in_byte + v->c_out_byte) / (COUNT_INTERVAL);
    d->pkt_ps = (v->c_in_pkt + v->c_out_pkt) / (COUNT_INTERVAL);
	if(v->c_out_byte == 0) {
		v->c_out_byte = 1;
	} 
	d->asy_byte = (float)v->c_in_byte / (float)v->c_out_byte;
	
	if(v->c_out_pkt == 0) {
		v->c_out_pkt = 1;
	} 
	d->asy_pkt = (float)v->c_in_pkt / (float)v->c_out_pkt;
}

void in_sequence(DATA_SEQUENCE *q, DATA_VECTOR v) {
	if(q->rear == WINDOW_SIZE-1) {
		printf("sequence is full\n");
		return;
	}
	
	if(q->rear < q->front) {
		q->data[q->front] = v;
		q->rear = 0;
		return;
	}
	
	int i;
	for(i=q->rear; i>=q->front; i--) {
		q->data[i+1] = q->data[i];
	}
	q->data[q->front] = v;
	q->rear++;
}

void out_sequence(DATA_SEQUENCE *q) {
	if(q->rear < q->front) {
		printf("sequence is empty\n");
		return;
	}
	
	q->rear--;
}

//计算平均值
DATA_VECTOR caculate_avg(DATA_SEQUENCE *q) {
	DATA_VECTOR avg;
	avg.byte_ps = 0;
	avg.pkt_ps = 0;
	avg.asy_byte = 0;
	avg.asy_pkt = 0;
	
	int i;
	for(i=q->front; i<=q->rear; i++) {
		avg.byte_ps += q->data[i].byte_ps;
		avg.pkt_ps += q->data[i].pkt_ps;
		avg.asy_byte += q->data[i].asy_byte;
		avg.asy_pkt += q->data[i].asy_pkt;
	}
	
	avg.byte_ps = avg.byte_ps / (q->rear - q->front + 1);
	avg.pkt_ps = avg.pkt_ps / (q->rear - q->front + 1);
	avg.asy_byte = avg.asy_byte / (q->rear - q->front + 1);
	avg.asy_pkt = avg.asy_pkt / (q->rear - q->front + 1);
	
	return avg;
}

//计算方差
DATA_VECTOR caculate_std(DATA_SEQUENCE *q) {
	DATA_VECTOR avg, std;
	avg = caculate_avg(q);
	std.byte_ps = 0;
	std.pkt_ps = 0;
	std.asy_byte = 0;
	std.asy_pkt = 0;
	
	int i;
	for(i=q->front; i<=q->rear; i++) {
		std.byte_ps += pow(q->data[i].byte_ps - avg.byte_ps, 2);
		std.pkt_ps += pow(q->data[i].pkt_ps - avg.pkt_ps, 2);
		std.asy_byte += pow(q->data[i].asy_byte - avg.asy_byte, 2);
		std.asy_pkt += pow(q->data[i].asy_pkt - avg.asy_pkt, 2);
	}
	
	std.byte_ps = sqrt(std.byte_ps / (q->rear - q->front + 1));
	std.pkt_ps = sqrt(std.pkt_ps / (q->rear - q->front + 1));
	std.asy_byte = sqrt(std.asy_byte / (q->rear - q->front + 1));
	std.asy_pkt = sqrt(std.asy_pkt / (q->rear - q->front + 1));

	return std;
}