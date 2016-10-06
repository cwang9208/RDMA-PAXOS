#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"

#include "../db/db-interface.h"

typedef struct proxy_node_t{
	char* db_name;
	db* db_ptr;
}proxy_node;

#endif