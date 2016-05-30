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

//free and set value to null
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

INT4 get_total_cpu()
{
    return sysconf(_SC_NPROCESSORS_CONF);
}

INT4 set_cpu_affinity(UINT4 cpu_id)
{
    int total_cpu = get_total_cpu();
    cpu_set_t cpu_set;
    if(cpu_id > total_cpu) return -1;
    CPU_ZERO(&cpu_set);
    CPU_SET(cpu_id, &cpu_set);
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

INT4 cidr_str_to_ip_prefix(char* ori_cidr, UINT4* ip, UINT4* mask)
{
	char cidr[48] = {0};
	memcpy(cidr, ori_cidr, 48);
	
	char* token = NULL;
	char* buf = cidr; 
	INT4 count = 0;

	while((token = strsep(&buf , "/")) != NULL)
	{
		if (0 == count) {
			// printf("cidr ip: %s\n", token);
			*ip = ip2number(token);
			// printf("%u\n", cidr_ip);
		}
		else {
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


