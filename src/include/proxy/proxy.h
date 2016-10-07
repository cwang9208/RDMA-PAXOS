#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"
#include "uthash.h"

#include "../db/db-interface.h"

typedef uint8_t nid_t;

typedef struct pair_count_t{
	int clt_id;
	uint64_t req_id;
    UT_hash_handle hh;
}pair_count;

typedef struct proxy_address_t{
    struct sockaddr_in s_addr;
    size_t s_sock_len;
}proxy_address;

typedef struct proxy_node_t{
	nid_t node_id; 
	proxy_address sys_addr;
	
	pair_count* hash_map;
	
	uint8_t group_size;
	
	char* db_name;
	db* db_ptr;
}proxy_node;

#endif