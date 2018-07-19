#include "sensor_struct.h"
#include "data_struct.h"
#include "net.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/ioctl.h>  
#include <netinet/if_ether.h>  
#include <net/if.h>  
#include <linux/sockios.h> 
#include <time.h>
#include <signal.h>
#include <unistd.h>

int sockToDenfenseC;
u64 start_drop_pkt;
u64 end_drop_pkt;

void update_sequence(DATA_SEQUENCE *q, int doUpdate) {
	COUNTER_VALUE v;
	DATA_VECTOR d;
	int i, j;
	read_counters(&v);
	caculate_data_vector(&d, &v);

	if(doUpdate) {
		if(q->rear == WINDOW_SIZE-1) {
			out_sequence(q);
		}
		in_sequence(q, d);
		q->out_rate_byte = v.c_out_byte / (COUNT_INTERVAL);
		q->in_rate_byte = v.c_in_byte / (COUNT_INTERVAL);
		q->out_rate_pkt = v.c_out_pkt / (COUNT_INTERVAL);
		q->in_rate_pkt = v.c_in_pkt / (COUNT_INTERVAL);
	}
	q->now_data = d;

}

//初始化队列
void initial_sequence(DATA_SEQUENCE *q) {
	int doUpdate;
	q->front = 0;
	q->rear = -1;
	doUpdate = 1;
	
	update_sequence(q, doUpdate);
}

//计算预测值
DATA_VECTOR predict_value(DATA_SEQUENCE q) {
	DATA_VECTOR predicted_ans;
	predicted_ans.byte_ps = 0;
	predicted_ans.pkt_ps = 0;
	predicted_ans.asy_byte = 0;
	predicted_ans.asy_pkt = 0;
	
	int i, total = 0;
	int coefficient[q.rear - q.front + 1];
	for(i=0; i<q.rear - q.front + 1; i++) {
		coefficient[i] = q.rear - q.front + 1 - i;
		total += coefficient[i];
	}
	
	for(i=q.front; i<=q.rear; i++) {
		predicted_ans.byte_ps += coefficient[i] * q.data[i].byte_ps;
		predicted_ans.pkt_ps += coefficient[i] * q.data[i].pkt_ps;
		predicted_ans.asy_byte += coefficient[i] * q.data[i].asy_byte;
		predicted_ans.asy_pkt += coefficient[i] * q.data[i].asy_pkt;
	}
	
	predicted_ans.byte_ps = predicted_ans.byte_ps / total;
	predicted_ans.pkt_ps = predicted_ans.pkt_ps / total;
	predicted_ans.asy_byte = predicted_ans.asy_byte / total;
	predicted_ans.asy_pkt = predicted_ans.asy_pkt / total;
	
	return predicted_ans;
}

//判断是否存在DDoS攻击, 返回1表示存在攻击
char attack_check(DATA_VECTOR actual_data, DATA_VECTOR predict_data, DATA_VECTOR std) {	
	char doAttack = '1';
	DATA_VECTOR upper_data, lower_data;
	
	upper_data.byte_ps = predict_data.byte_ps + 3*std.byte_ps;
	upper_data.pkt_ps = predict_data.pkt_ps + 3*std.pkt_ps;
	upper_data.asy_byte = predict_data.asy_byte + 3*std.asy_byte;
	upper_data.asy_pkt = predict_data.asy_pkt + 3*std.asy_pkt;
	
	if(actual_data.byte_ps > upper_data.byte_ps
	|| actual_data.pkt_ps > upper_data.pkt_ps 
	) {
		doAttack = '1';
	} else if(actual_data.byte_ps <= upper_data.byte_ps 
	&& actual_data.pkt_ps <= upper_data.pkt_ps
	) {
		doAttack = '0';
	} 
	return doAttack;
}

void open_copy_reg()
{
	fast_reg_wr(0x23000, 0x2);
}

void close_copy_reg()
{
	fast_reg_wr(0x23000, 0x0);
}

unsigned int calculate_drop_packets(u64 start, u64 end)
{
	if(end >= start){
		return end - start;
	}else{
		return end + (0xffffffffffffffff - start);
	}
}


//发送解除告警信息
void send_alert_cancel_packet(char* ip, int port, int sock, unsigned char EGP_ID, unsigned int dropPackets) {
	int data_len;
	unsigned char buf[1024];
	ALERT_CANCEL_PACKET alert_cancel_pkt;
	alert_cancel_pkt.timeStamp = time(NULL);
	alert_cancel_pkt.protocol_type = ALERT_CANCEL_TYPE;
	alert_cancel_pkt.EGP_ID = EGP_ID;
	alert_cancel_pkt.dropPackets = dropPackets;
	data_len = encode_alert_cancel_packet(alert_cancel_pkt, buf);
	send_msg(sock, &data_len, sizeof(int), ip, port);
	send_msg(sock, buf, data_len, ip, port);
}

unsigned char getEGPID() {
	unsigned char macaddr[6];
	char *device = "eth0";
	int s;
	s = socket(AF_INET, SOCK_DGRAM, 0);
	struct ifreq req;
	strcpy(req.ifr_name, device);
	ioctl(s, SIOCGIFHWADDR, &req);
	close(s);
	memcpy(macaddr, req.ifr_hwaddr.sa_data, 6);
	return macaddr[5];
}

void stop(int signo) {
	close(sockToDenfenseC);
	printf("socket all closed!\n");
	exit(0);
}

int main()
{

	int doUpdate;
	unsigned char portAlertStatus = 0;
	unsigned char EGP_ID = 1;

	sockToDenfenseC = get_client_socket();
	signal(SIGINT, stop);

	fast_init_hw(0,0);	
	DATA_SEQUENCE monitor_sequences;
	initial_sequence(&monitor_sequences);
	
	DATA_VECTOR actual_data, predict_data, std;
	while(1) {
		if(portAlertStatus == 1) {
			doUpdate = 0;
		}else{
			doUpdate = 1;
		}
		predict_data = predict_value(monitor_sequences);
		std = caculate_std(&monitor_sequences);

		update_sequence(&monitor_sequences, doUpdate);

		actual_data = monitor_sequences.now_data;
		char doAttack = attack_check(actual_data, predict_data, std);
		if(doAttack == '1') {
			if(portAlertStatus != 1) {
				portAlertStatus = 1;

			    open_copy_reg();
		  
				start_drop_pkt = read_REG(DROP_CNT_L, DROP_CNT_H);
			}
			printf("1\n");
		} else if(doAttack == '0') {
			if(portAlertStatus == 1)
			{
				portAlertStatus = 0;
				close_copy_reg();

				//计算丢弃
				end_drop_pkt = read_REG(DROP_CNT_L, DROP_CNT_H);

				sleep(5);
				send_alert_cancel_packet(IP_OF_DEFENSEC, PORT_WITH_DEFENSEC,
				sockToDenfenseC, EGP_ID, calculate_drop_packets(start_drop_pkt, end_drop_pkt));
			}
			printf("0\n");
		} 		
		
	}
	
	return 0;
}
