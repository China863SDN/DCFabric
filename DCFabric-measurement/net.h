#define PORT_WITH_DEFENSEC 20185
#define IP_OF_DEFENSEC "1000:0:200::800"

int send_msg(int sock, const void *buf, int len, char* ip, int port);

int recv_msg(int sock, void *buf, int len);

int get_server_socket(int port);

int get_client_socket();
