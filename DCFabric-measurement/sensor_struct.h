#define SLEEP_INTERVAL 10000
#define COUNT_INTERVAL 10
#define WINDOW_SIZE 100

#define DROP_CNT_L 0x2b800
#define DROP_CNT_H 0x2c000
#define DROP_BYTE_CNT_L 0x2c800
#define DROP_BYTE_CNT_H 0x2d000

#define PKT_RX_CNT_L 0x26000
#define PKT_RX_CNT_H 0x26800
#define PKT_RX_BYTE_CNT_L 0x27000
#define PKT_RX_BYTE_CNT_H 0x27800

#define PKT_TX_CNT_L 0x28000
#define PKT_TX_CNT_H 0x28800
#define PKT_TX_BYTE_CNT_L 0x29000
#define PKT_TX_BYTE_CNT_H 0x29800


typedef struct counter_value{
	u64 c_in_byte;
	u64 c_out_byte;
	u64 c_in_pkt;
	u64 c_out_pkt;
}COUNTER_VALUE;

typedef struct data_vector{
	float byte_ps;//每秒千字节数
	float pkt_ps;//每秒千报文数
	float asy_byte;//不对称性（字节）
	float asy_pkt;//不对称性（报文）
}DATA_VECTOR;

typedef struct data_sequence{
	DATA_VECTOR now_data;
	DATA_VECTOR data[WINDOW_SIZE];
	float out_rate_byte;
	float in_rate_byte;
	float out_rate_pkt;
	float in_rate_pkt;
	int front, rear;
}DATA_SEQUENCE;

//data_vector相关操作
int compare_vector(char result[], int len, DATA_VECTOR a, DATA_VECTOR b);

void print_data_vector(DATA_VECTOR v);

void print_counter_value(COUNTER_VALUE v);

//sequence相关操作
void in_sequence(DATA_SEQUENCE *q, DATA_VECTOR v);

void out_sequence(DATA_SEQUENCE *q);

DATA_VECTOR caculate_avg(DATA_SEQUENCE *q);

DATA_VECTOR caculate_std(DATA_SEQUENCE *q);

void print_sequence(DATA_SEQUENCE q);

//计数器相关操作
u64 read_REG(u32 address_low, u32 address_high);

void read_counters(COUNTER_VALUE *v) ;

void caculate_data_vector(DATA_VECTOR *d, COUNTER_VALUE *v);


//测试用方法
void test_read_counters();

void test_read();
