/*
 * GNFlush SDN Controller GPL Source Code
 * Copyright (C) 2015, Greenet <greenet@greenet.net.cn>
 *
 * This file is part of the GNFlush SDN Controller. GNFlush SDN
 * Controller is a free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, , see <http://www.gnu.org/licenses/>.
 */

/******************************************************************************
*                                                                             *
*   File Name   : common.c           *
*   Author      : greenet Administrator           *
*   Create Date : 2015-2-12           *
*   Version     : 1.0           *
*   Function    : .           *
*                                                                             *
******************************************************************************/

#include "common.h"
#include <netinet/tcp.h>
#include <execinfo.h>
#include <stdio.h>
#include <stdlib.h>

static unsigned long poolprefixarray[32]=
{
    0x0,0x8,0xC,0xE,
    0xF,0xF8,0xFC,0xFE,
    0xFF,0x80FF,0xC0FF,0xE0FF,
    0xF0FF,0xF8FF,0xFCFF,0xFEFF,
    0xFFFF,0x80FFFF,0xC0FFFF,0xE0FFFF,
    0xF0FFFF,0xF8FFFF,0xFCFFFF,0xFEFFFF,
    0xFFFFFF,0x80FFFFFF,0xC0FFFFFF,0xE0FFFFFF,
    0xF0FFFFFF,0xF8FFFFFF,0xFCFFFFFF,0xFEFFFFFF
};


void Debug_PrintTrace(void)
{
	void	*array[20];
	size_t	size;
	char	**strings;
	size_t i;
	size	= backtrace(array, 20);
	strings = backtrace_symbols(array, size);
	if (NULL == strings)
	{
		perror( "backtrace_synbols" );
	}
	else
	{
		LOG_PROC("DEBUG", "Obtained %zd stack frames",size);
		for ( i = 0; i < size; i++ )
		{
			LOG_PROC("DEBUG", "%s",strings[i]);
		}
		free( strings );
		strings = NULL;
	}
}

void MsecSleep(UINT4 uiMillSec)
{
	struct timeval tv;

	tv.tv_sec = uiMillSec/1000;
	tv.tv_usec = (uiMillSec%1000)*1000;
	select(1, NULL, NULL, NULL, &tv);
	return;

}

//malloc and memset
void* gn_malloc (size_t size)
{
    void* mem = malloc(size);
    if(NULL == mem)
    {
        return NULL;
    }

    memset(mem, 0, size);
    return mem;
}

UINT8 gn_htonll(UINT8 n)
{
    return htonl(1) == 1 ? n : ((UINT8) htonl(n) << 32) | htonl(n >> 32);
}

UINT8 gn_ntohll(UINT8 n)
{
    return htonl(1) == 1 ? n : ((UINT8) ntohl(n) << 32) | ntohl(n >> 32);
}

void random_init(void)
{
    unsigned char  inited = FALSE;
    if (!inited) {
        struct timeval tv;
        inited = TRUE;
        if (gettimeofday(&tv, NULL) < 0) {
            return;
        }
        srand(tv.tv_sec ^ tv.tv_usec);
    }
}

void random_bytes(void *p_, size_t n)
{
    uint8_t *p = p_;
    random_init();
    while (n--) {
        *p++ = rand();
    }
}

inline uint32_t random_uint32()
{
    if (RAND_MAX >= UINT32_MAX)
    {
        random_init();
        return rand();
    }
    else if (RAND_MAX == INT32_MAX)
    {
        random_init();
        return rand() | ((rand() & 1u) << 31);
    }
    else
    {
        uint32_t x;
        random_bytes(&x, sizeof x);
        return x;
    }
}

// Cut string by a char 'delim', it's function like strtok_r, the difference is when you separate string like "1|||2|3|4|" by char '|'
char* strcut(char *org, char delim, char **next)
{
    char *p_str = NULL;
    char *p_str_bak = NULL;

    if(org)
        p_str = org;
    else if(next && *next)
        p_str = *next;
    else
    {
        *next = NULL;
        return NULL;
    }

    p_str_bak = p_str;
    do
    {
       if((*p_str) == delim)
       {
          *p_str = '\0';
          *next = p_str + 1;
          return p_str_bak;
       }
    }while('\0' != *p_str++);

    *next = NULL;
    return p_str_bak;
}

//by:yhy free and set value to null
void gn_free(void **ptr)
{
    free(*ptr);
    *ptr = NULL;
}

//unsigned long long int ---->unsigned char [8]
UINT1 *ulli64_to_uc8(UINT8 dpid, unsigned char *DpidStr)
{
    DpidStr[0]= (unsigned char)((dpid>>56) & 0x00000000000000ff);
    DpidStr[1]= (unsigned char)((dpid>>48) & 0x00000000000000ff);
    DpidStr[2]= (unsigned char)((dpid>>40) & 0x00000000000000ff);
    DpidStr[3]= (unsigned char)((dpid>>32) & 0x00000000000000ff);
    DpidStr[4]= (unsigned char)((dpid>>24) & 0x00000000000000ff);
    DpidStr[5]= (unsigned char)((dpid>>16) & 0x00000000000000ff);
    DpidStr[6]= (unsigned char)((dpid>> 8) & 0x00000000000000ff);
    DpidStr[7]= (unsigned char)((dpid>> 0) & 0x00000000000000ff);

    return DpidStr;
}

//unsigned char [8] ---->unsigned long long int
UINT8 uc8_to_ulli64(const unsigned char *dpid_str, UINT8 *dpid)
{
    unsigned long long int dpid_tmp = 0;
    UINT4 idx = 0;
    while(idx < 8)
    {
        dpid_tmp <<= 8;
        dpid_tmp |= (*(dpid_str + idx++) & 0x00000000000000ff);
    }

    *dpid = dpid_tmp;
    return dpid_tmp;
}

int mac_str_to_bin( char *str, unsigned char *mac)
{
    int i;
    char *s, *e;

    if ((NULL == mac) || (NULL == str))
    {
        return -1;
    }

    s = (char *) str;
    for (i = 0; i < 8; ++i)
    {
        mac[i] = s ? strtoul (s, &e, 16) : 0;
        if (s)
           s = (*e) ? e + 1 : e;
    }
    return 0;
}

UINT4 MacHash_local(UINT1 *mac, UINT4 size)
{
    UINT4 hash_index = 0;
    UINT1 first= mac[2];
    UINT1 second= mac[3];
    UINT1 third= mac[4];
    UINT1 four= mac[5];

    hash_index  = (first<<24)+(second<<16)+(third<<8)+four;

    return hash_index%size;
}

UINT4 mac_hash(UINT1 *mac , UINT4 size )
{
    UINT4 seed = 131; // 31 131 1313 13131 131313 etc..
    UINT4 hash = 0;

    hash = hash * seed + (*mac++);
    hash = hash * seed + (*mac++);
    hash = hash * seed + (*mac++);
    hash = hash * seed + (*mac++);
    hash = hash * seed + (*mac++);
    hash = hash * seed + (*mac++);

    return (hash & (size-1) );
}

char *inet_htoa(UINT4 ip)
{
    struct in_addr in;
    in.s_addr = htonl(ip);
    return inet_ntoa(in);
}

void masked_ip_parser(INT1 *masked_ip, net_mask_t *net_mask)
{
    UINT4 i = 0;

    memset(net_mask, 0, sizeof(net_mask_t));
    for(; masked_ip[i] != '\0'; i++)
    {
        if(masked_ip[i] == '/')
        {
            masked_ip[i] = '\0';
            net_mask->prefix = atoll(masked_ip + i +1);
            break;
        }
    }

    net_mask->ip = inet_addr(masked_ip);
    net_mask->minip = (net_mask->ip)&(poolprefixarray[net_mask->prefix]);
    net_mask->maxip = (net_mask->ip)|(~poolprefixarray[net_mask->prefix]);
}
//by:yhy MAC转string表达方式
char *mac2str(UINT1 *hex, char *result)
{
    sprintf(result, "%02x:%02x:%02x:%02x:%02x:%02x",hex[0], hex[1],\
            hex[2],hex[3],hex[4],hex[5]);

    return result;
}

UINT1 *macstr2hex(char *macstr, UINT1 *result)
{
    int i;
    char *s, *e;

    if ((macstr == NULL) || (result == NULL))
    {
        return result;
    }

    s = (char *) macstr;
    for (i = 0; i < 6; ++i)
    {
        result[i] = s ? strtoul (s, &e, 16) : 0;
        if (s)
           s = (*e) ? e + 1 : e;
    }

    return result;
}
//by:yhy 获取CPU核心数
INT4 get_total_cpu()
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

//by:yhy CPU绑定线程(提高运行效率)
INT4 set_cpu_affinity(UINT4 cpu_id)
{
    int total_cpu = get_total_cpu();
    cpu_set_t cpu_set;
    if(cpu_id > total_cpu) return -1;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_id, &cpu_set);
	//by:yhy CPU核与该线程绑定
    return pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &cpu_set);
}

UINT4 ip2number(const INT1* ip)
{
   return inet_addr(ip);
}

INT1* number2ip(INT4 ip_num, INT1* ip)
{
   strcpy(ip, inet_ntoa(*(struct in_addr*)&ip_num));
   return ip;
}

UINT1* ipv6_str_to_number(char* str, UINT1* ipv6)
{
	struct in6_addr ip;
	inet_pton(AF_INET6, str, &ip);
	memcpy(ipv6, ip.s6_addr, 16);
	return ipv6;
}

// show ip
void nat_show_ip(UINT4 ip)
{
	struct in_addr addr;
	memcpy(&addr, &ip, 4);
	LOG_PROC("INFO","IP: %s  |",inet_ntoa(addr));
	return;
}

// show mac
void nat_show_mac(UINT1* mac)
{
	char temp[16] = {0};
	mac2str(mac, temp);
	LOG_PROC("INFO","MAC: %s  |",temp);
	return;
}

void nat_show_ipv6(UINT1* ip)
{
	UINT1 temp[16] = {0};
	memcpy(temp, ip , 16);
	int i = 0;
	printf("[INFO] ipv6 is: ");
	for (;i < 16;i++) {
		if (i && !(i%2)) {
			printf(":");
		}
		printf("%02x", temp[i]);
	}
	printf("\n");
	// LOG_PROC("INFO", "IP: %s |", temp);
}


//00:00:00:00:00:00:01:91 --> 401
INT4 dpidStr2Uint8(const INT1 *dpid, UINT8 *ret)
{
    if (NULL == dpid)
    {
        return -1;
    }
    
    INT4 len = strlen(dpid);
    if (len < 23)
    {
        return -1;
    }

    INT4 i = 0;
    INT4 j = 0;
    UINT1 tmp[17] = {0};
    for (; i <23 && j < 16; i++)
    {
        if (':' != dpid[i])
        {
            if (!((dpid[i] <= '9' && dpid[i] >= '0') 
                || (dpid[i] <= 'f' && dpid[i] >= 'a') 
                || (dpid[i] <= 'F' && dpid[i] >= 'A')))
            {
                return -1;
            }
            tmp[j] = dpid[i];
            j++;
        }
    }

    *ret = strtoul((const char*)tmp, NULL, 16);

    return 0;
}


//is digit, just support base=10/16
BOOL is_digit(const INT1 *str, INT4 base)
{
    if (NULL == str || (10 != base && 16 != base))
    {
        return -1;
    }
    
    UINT8 len = strlen(str);
    int i = 0;
    switch (base)
    {
        case 10:
        {
            for (i = 0; i < len; i++)
            {
                if (str[i] < '0' || str[i] > '9') 
                {                        
                    return FALSE;                
                }
            }
            break;
        }
        case 16:
        {
            if(0 != strncmp("0x", str, 2))
            {
                return FALSE;
            }
            
            for (i = 2; i < len; i++)
            {
                if (!((str[i] <= '9' && str[i] >= '0') 
                        || (str[i] <= 'f' && str[i] >= 'a') 
                        || (str[i] <= 'F' && str[i] >= 'A')))
                {                        
                    return FALSE;                
                }
            }
            break;
        }
        default:
        {
            return FALSE;
        }
    }
    
    return TRUE; 
}


// calculate checksum
UINT2 calc_ip_checksum(UINT2 *buffer, UINT4 size)
{
	UINT4 cksum = 0;
	while(size >1)
	{
		cksum += *buffer++;
		size -= sizeof(UINT2);
	}
	if(size )
	{
		cksum += *(UINT1*)buffer;
	}

	cksum = (cksum >> 16) + (cksum & 0xffff);
	cksum += (cksum >>16);
	return (UINT2)(~cksum);
}


UINT2 calc_tcp_checksum(ip_t* p_ip, tcp_t* p_tcp)
{
    char buf[IP_MAXPACKET];
    char *ptr;
    int chksumlen = 0;
    UINT2 svalue;
    UINT1 svalue2;
    bzero(buf, IP_MAXPACKET);

    ptr = &buf[0];

    memcpy(ptr, &p_ip->src, sizeof(p_ip->src));
    ptr += sizeof(p_ip->src);
    chksumlen += sizeof(p_ip->src);

    memcpy(ptr, &p_ip->dest, sizeof(p_ip->dest));
    ptr += sizeof(p_ip->dest);
    chksumlen += sizeof(p_ip->dest);

    *ptr = 0;
    ptr ++;
    chksumlen ++;

    memcpy(ptr, &p_ip->proto, sizeof(p_ip->proto));
    ptr += sizeof(p_ip->proto);
    chksumlen += sizeof(p_ip->proto);

    // tcp lengh
    svalue =  ((p_tcp->offset << 4) * 4);
    memcpy (ptr, &svalue, sizeof (svalue));
    ptr += sizeof (svalue);
    chksumlen += sizeof (svalue);

    memcpy(ptr, &p_tcp->sport, sizeof(p_tcp->sport));
    ptr += sizeof(p_tcp->sport);
    chksumlen += sizeof(p_tcp->sport);

    memcpy(ptr, &p_tcp->dport, sizeof(p_tcp->dport));
    ptr += sizeof(p_tcp->dport);
    chksumlen += sizeof(p_tcp->dport);

    memcpy(ptr, &p_tcp->seq, sizeof(p_tcp->seq));
    ptr += sizeof(p_tcp->seq);
    chksumlen += sizeof(p_tcp->seq);

    memcpy(ptr, &p_tcp->ack, sizeof(p_tcp->ack));
    ptr += sizeof(p_tcp->ack);
    chksumlen += sizeof(p_tcp->ack);

    svalue2 = (p_tcp->offset) ;
    memcpy(ptr, &svalue2, sizeof(svalue2));
    ptr += sizeof(svalue2);
    chksumlen += sizeof(svalue2);

    memcpy(ptr, &p_tcp->code, sizeof(p_tcp->code));
    ptr += sizeof(p_tcp->code);
    chksumlen += sizeof(p_tcp->code);

    memcpy(ptr, &p_tcp->window, sizeof(p_tcp->window));
    ptr += sizeof(p_tcp->window);
    chksumlen += sizeof(p_tcp->window);

    *ptr = 0; ptr++;
    *ptr = 0; ptr++;
    chksumlen += 2;

    memcpy(ptr, &p_tcp->urg, sizeof(p_tcp->urg));
    ptr += sizeof(p_tcp->urg);
    chksumlen += sizeof(p_tcp->urg);

    UINT2 calc = calc_ip_checksum((UINT2 *) buf, chksumlen);

    // printf("calc: %x\n", htons(calc));

    return calc;

}
//by:yhy dpid转string
INT1* dpidUint8ToStr(UINT8 ori_dpid, INT1* str)
{
	UINT1 dpid[8];
	ulli64_to_uc8(ori_dpid, dpid);
	sprintf(str, "%02x:%02x:%02x:%02x:%02x:%02x:%02x:%02x", dpid[0], dpid[1], dpid[2], dpid[3], dpid[4], dpid[5], dpid[6], dpid[7]);
    return str;
}

INT4 compare_str(char* str1, char* str2) 
{
	if ((0 == strlen(str1)) && (0 == strlen(str2))) {
		return 1;
	}
	else if ((strlen(str1)) && (strlen(str2)) && (0 == strcmp(str1, str2))) {
		return 1;
	}
	else {
		return 0;
	}
}

INT4 compare_array(void* str1, void* str2, INT4 num) 
{
	if ((NULL == str1) && (NULL == str2)) {
		return 1;
	}
	else if ((str1) && (str2) && (0 == memcmp(str1, str2, num))) {
		return 1;
	}
	else {
		return 0;
	}
}

INT4 compare_pointer(void* ptr1, void* ptr2)
{
	if ((NULL == ptr1) && (NULL == ptr2)) {
		return 1;
	}
	else if ((ptr1) && (ptr2) && (ptr1 == ptr2)) {
		return 1;
	}
	else {
		return 0;
	}
}
//by:yhy 查找 CIDR中的IP字段和Mask字段
INT4 cidr_str_to_ip_prefix(char* ori_cidr, UINT4* ip, UINT4* mask)
{
	char cidr[48] = {0};
	memcpy(cidr, ori_cidr, 48);
	
	char* token = NULL;
	char* buf = cidr; 
	INT4 count = 0;

	while((token = strsep(&buf , "/")) != NULL)
	{
		if (0 == count) 
		{
			// printf("cidr ip: %s\n", token);
			*ip = ip2number(token);
			// printf("%u\n", cidr_ip);
		}
		else 
		{
			// printf("mask is: %s\n", token);
			*mask = (0 == strcmp("0", token)) ? 0:atoll(token);
			// printf("%u\n", cidr_mask);
		}
		count++;
	}

	return *ip;
}

// convert cidr to subnet mask
UINT4 cidr_to_subnet_mask(UINT4 num) 
{ 
	UINT4 mask = 0;

	if (32 >= num) {
	    int bits = sizeof(UINT4) * 8;
 
	    mask = ~0;
	    bits -= num;
	    mask <<= bits;
	}
	
    return htonl(mask);
}

/***********************************************************************/
//Circle Queue Related start
loop_buffer_t *init_loop_buffer(INT4 len)
{	
	loop_buffer_t *p_loop_buffer = NULL;
	p_loop_buffer = (loop_buffer_t *)gn_malloc(sizeof(loop_buffer_t));
	if(NULL== p_loop_buffer)
	{
		LOG_PROC("ERROR", "init_loop_buffer -- p_loop_buffer gn_malloc  Finall return GN_ERR");
		return NULL;
	}

	p_loop_buffer->buffer = (char *)gn_malloc(len * sizeof(char));
	if(NULL== p_loop_buffer->buffer)
	{
		LOG_PROC("ERROR", "init_loop_buffer -- p_loop_buffer.buffer gn_malloc  Finall return GN_ERR");
		return NULL;
	}
	p_loop_buffer->head = p_loop_buffer->buffer;
	p_loop_buffer->total_len = len;
	pthread_mutex_init(&(p_loop_buffer->buffer_mutex) , NULL);
	
	return p_loop_buffer;
}

void reset_loop_buffer(loop_buffer_t *p_loop_buffer)
{
	pthread_mutex_lock(&(p_loop_buffer->buffer_mutex));
	p_loop_buffer->cur_len = 0;
	p_loop_buffer->head = p_loop_buffer->buffer;
	pthread_mutex_unlock(&(p_loop_buffer->buffer_mutex));
}

BOOL buffer_write(loop_buffer_t *p_loop_buffer,char* p_recv,UINT4 len)
{

	UINT4 n_pos = 0;
	UINT4 first_len = 0;
	UINT4 second_len = 0;
	
	pthread_mutex_lock(&(p_loop_buffer->buffer_mutex));
	if(p_loop_buffer->cur_len + len > p_loop_buffer->total_len)
	{//
		UINT4 need_space = 0;
	    char *p_temp = NULL;
		p_temp = p_loop_buffer->buffer;
        need_space = p_loop_buffer->cur_len + len - p_loop_buffer->total_len;
		need_space = need_space > 1024? need_space : 1024;

	    p_loop_buffer->buffer = (char *)gn_malloc(p_loop_buffer->total_len + need_space);

		if(NULL == p_loop_buffer->buffer)
		{//
			//LOG_PROC("ERROR", "%s -- p_loop_buffer.buffer gn_malloc  Finall return GN_ERR",FN);
			pthread_mutex_unlock(&(p_loop_buffer->buffer_mutex));
			return FALSE;
		}

		if(p_loop_buffer->head + p_loop_buffer->cur_len >= p_temp + p_loop_buffer->total_len)
		{//
		    second_len = p_loop_buffer->head + p_loop_buffer->cur_len - (p_temp + p_loop_buffer->total_len);
			memcpy(p_loop_buffer->buffer,p_loop_buffer->head,(p_loop_buffer->cur_len - second_len));
		    memcpy(p_loop_buffer->buffer + p_loop_buffer->cur_len - second_len,p_temp,second_len);
	    }
		else
		{//
		    memcpy(p_loop_buffer->buffer,p_loop_buffer->head,p_loop_buffer->cur_len);	
		}
		p_loop_buffer->total_len = p_loop_buffer->total_len + need_space;
        p_loop_buffer->head = p_loop_buffer->buffer;

		free(p_temp);
	}
	if(p_loop_buffer->head + p_loop_buffer->cur_len + len <= p_loop_buffer->buffer + p_loop_buffer->total_len)
    {//
        memcpy(p_loop_buffer->head + p_loop_buffer->cur_len,p_recv,len);
    }
    else
	{//
		if(p_loop_buffer->head + p_loop_buffer->cur_len >= p_loop_buffer->buffer + p_loop_buffer->total_len)
		{//
			n_pos = (p_loop_buffer->head + p_loop_buffer->cur_len) - (p_loop_buffer->buffer + p_loop_buffer->total_len);
			memcpy(p_loop_buffer->buffer + n_pos,p_recv,len);
		}
		else
		{//
			first_len = p_loop_buffer->buffer + p_loop_buffer->total_len - (p_loop_buffer->head + p_loop_buffer->cur_len);
			if(first_len >= len)
            {//
                memcpy(p_loop_buffer->head + p_loop_buffer->cur_len,p_recv,len);
            }
            else
            {//
                memcpy(p_loop_buffer->head + p_loop_buffer->cur_len,p_recv,first_len);
                memcpy(p_loop_buffer->buffer,p_recv + first_len,len - first_len);
            }
		}
	}
	p_loop_buffer->cur_len += len;
	
	pthread_mutex_unlock(&(p_loop_buffer->buffer_mutex));
	
	return TRUE;
}

BOOL buffer_read(loop_buffer_t *p_loop_buffer,char* p_dest,UINT4 len,BOOL peek)
{
	UINT4 first_len = 0;
	UINT4 second_len = 0;
	if(NULL == p_loop_buffer || NULL == p_dest)
	{
		//LOG_PROC("ERROR", "%s -- NULL == p_loop_buffer",FN);
		return FALSE;
	}
	if(p_loop_buffer->cur_len < len)
	{
		//LOG_PROC("ERROR", "%s -- p_loop_buffer->cur_len < len READ memory out of range",FN);
		return FALSE;
	}
	pthread_mutex_lock(&(p_loop_buffer->buffer_mutex));
	if(p_loop_buffer->head + p_loop_buffer->cur_len > p_loop_buffer->buffer + p_loop_buffer->total_len)
	{//回头
		//LOG_PROC("INFO", "-----------------buffer circle-----------------");
		second_len = p_loop_buffer->head + p_loop_buffer->cur_len - (p_loop_buffer->buffer + p_loop_buffer->total_len);
	    first_len = p_loop_buffer->cur_len - second_len;
		
		if(len >= first_len)
		{//回头读取
			memcpy(p_dest,p_loop_buffer->head,first_len);
			memcpy(p_dest + first_len,p_loop_buffer->buffer,len - first_len);
			if(FALSE == peek)
			{
				p_loop_buffer->head = p_loop_buffer->buffer +len - first_len;
				p_loop_buffer->cur_len = p_loop_buffer->cur_len - len;
			}	
		}
		else
		{//尾部读取
			memcpy(p_dest,p_loop_buffer->head,len);
			if(FALSE == peek)
			{
				p_loop_buffer->cur_len = p_loop_buffer->cur_len - len;
				p_loop_buffer->head = p_loop_buffer->head + len ;
			}	
		}
	}
	else
	{
		memcpy(p_dest,p_loop_buffer->head,len);
		if(FALSE == peek)
		{
			p_loop_buffer->head += len;
			
			if(p_loop_buffer->head == (p_loop_buffer->buffer + p_loop_buffer->total_len))
			{
				p_loop_buffer->head = p_loop_buffer->buffer;	
			}
			p_loop_buffer->cur_len = p_loop_buffer->cur_len - len;
		}	
	}
	
	pthread_mutex_unlock(&(p_loop_buffer->buffer_mutex));
	
	return TRUE;
}
//Circle Queue Related stop
/***********************************************************************/


/***********************************************************************/
//Queue Related start
//队列初始化
void init_queue(p_MultiPurpose_queue para_queue)
{
	para_queue->QueueHead =NULL;
	para_queue->QueueRear =NULL;
	para_queue->QueueLength =0;
	pthread_mutex_init(&(para_queue->QueueThreadMutex),NULL);
	return;
}
//入队
void push_node_into_queue( p_MultiPurpose_queue para_queue,p_Queue_node node)
{
	pthread_mutex_lock(&(para_queue->QueueThreadMutex));
	if ( node != NULL )
	{
		if ( para_queue->QueueLength == 0 )
		{
			para_queue->QueueHead = node;
		}
		else
		{
			para_queue->QueueRear->next = node;
		}
		para_queue->QueueRear = node;
		para_queue->QueueLength++;
	}
	pthread_mutex_unlock(&(para_queue->QueueThreadMutex));
	return;
}
//p出队
p_Queue_node pop_node_from_queue(p_MultiPurpose_queue para_queue)
{
	p_Queue_node ret = NULL;
	pthread_mutex_lock(&(para_queue->QueueThreadMutex));
	if ( para_queue->QueueLength > 0 )
	{
		ret	= para_queue->QueueHead;
		para_queue->QueueHead = ret->next;
		para_queue->QueueLength--;
		if ( para_queue->QueueLength == 0 )
		{
			para_queue->QueueRear = NULL;
		}
	}
	pthread_mutex_unlock(&(para_queue->QueueThreadMutex));
	return(ret);
}
//获取queue头
p_Queue_node get_head_node_from_queue(p_MultiPurpose_queue para_queue)
{
	p_Queue_node ret = NULL;
	pthread_mutex_lock(&(para_queue->QueueThreadMutex));
	ret =	para_queue->QueueHead;
	pthread_mutex_unlock(&(para_queue->QueueThreadMutex));
	return(ret);
}
//判空
UINT4 is_queue_empty(p_MultiPurpose_queue para_queue)
{
	UINT4 ret =0;
	pthread_mutex_lock(&(para_queue->QueueThreadMutex));
	ret =	para_queue->QueueLength;
	pthread_mutex_unlock(&(para_queue->QueueThreadMutex));
	return(ret);
}
//销毁
void destory_queue(p_MultiPurpose_queue para_queue)
{
	p_Queue_node ret = NULL;
	while((ret = pop_node_from_queue(para_queue)))
	{
	    free(ret->nodeData);
		free(ret);
		ret =NULL;
	}
	return;
}
//Queue Related stop
/***********************************************************************/
#define SW_TIMER_TASK_HEARTBEAT 1
#define SW_TIMER_TASK_LLDP      2

#define GET_BIT(X,Y)	((*X)&(1<<Y))
#define SET_BIT(X,Y)	((*X)|=(1<<Y))
#define CLR_BIT(X,Y)	((*X)&=(~(1<<Y)))

INT1 GetBit_SW_TIMER_TASK_HEARTBEAT(INT1 * X)
{
	return (GET_BIT(X,SW_TIMER_TASK_HEARTBEAT));
}
void SetBit_SW_TIMER_TASK_HEARTBEAT(INT1 * X)
{
	SET_BIT(X,SW_TIMER_TASK_HEARTBEAT);
}
void ClrBit_SW_TIMER_TASK_HEARTBEAT(INT1 * X)
{
	CLR_BIT(X,SW_TIMER_TASK_HEARTBEAT);
}

INT1 GetBit_SW_TIMER_TASK_LLDP(INT1 * X)
{
	return (GET_BIT(X,SW_TIMER_TASK_LLDP));
}
void SetBit_SW_TIMER_TASK_LLDP(INT1 * X)
{
	SET_BIT(X,SW_TIMER_TASK_LLDP);
}
void ClrBit_SW_TIMER_TASK_LLDP(INT1 * X)
{
	CLR_BIT(X,SW_TIMER_TASK_LLDP);
}

