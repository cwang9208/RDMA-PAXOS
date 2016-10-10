#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"
#include "../../../utils/uthash/uthash.h"
#include "../../../utils/uthash/utlist.h"

#include "../db/db-interface.h"

typedef uint8_t nid_t;

typedef struct inner_thread {
    pthread_t tid;
    struct inner_thread *next;
}inner_thread;

typedef struct socket_pair_t{
	int clt_id;
	int p_s;
    UT_hash_handle hh;
}socket_pair;

typedef struct proxy_address_t{
    struct sockaddr_in s_addr;
    size_t s_sock_len;
}proxy_address;

typedef struct proxy_node_t{
	nid_t node_id; 
	proxy_address sys_addr;
	
	socket_pair* hash_map;
	inner_thread* inner_threads;
	
    // log option
    int req_log;

	uint8_t group_size;
	FILE* req_log_file;
	char* db_name;
	db* db_ptr;
}proxy_node;

#endif