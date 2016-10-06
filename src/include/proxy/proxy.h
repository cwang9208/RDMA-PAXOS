#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"
#include "uthash.h"

#include "../db/db-interface.h"

typedef struct leader_accept_pair_t{
	int key;
    view_stamp vs;
    UT_hash_handle hh;
}leader_conn_pair;

typedef struct proxy_node_t{
	leader_conn_pair* leader_conn_map;
	
	char* db_name;
	db* db_ptr;
}proxy_node;

#endif