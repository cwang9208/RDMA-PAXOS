#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"
#include "uthash.h"

#include "../db/db-interface.h"

typedef uint8_t nid_t;

typedef struct leader_accept_pair_t{
	int key;
    view_stamp vs;
    UT_hash_handle hh;
}leader_conn_pair;

typedef struct proxy_address_t{
    struct sockaddr_in s_addr;
    size_t s_sock_len;
}proxy_address;

typedef struct proxy_node_t{
	nid_t node_id; 
	proxy_address sys_addr;
	leader_conn_pair* leader_conn_map;
	
	uint8_t group_size;
	
	char* db_name;
	db* db_ptr;
}proxy_node;

#endif