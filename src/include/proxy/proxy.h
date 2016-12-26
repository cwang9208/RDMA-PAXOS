#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"

#include "../../../utils/uthash/utlist.h"

#include "../db/db-interface.h"
#include "../dare/dare_server.h"
typedef struct inner_thread {
    pthread_t tid;
    struct inner_thread *next;
}inner_thread;

typedef struct proxy_address_t{
    struct sockaddr_in s_addr;
    size_t s_sock_len;
}proxy_address;

typedef struct proxy_node_t{
	proxy_address sys_addr;

    socket_pair_t* hash_map;
	inner_thread* inner_threads;
	
    // log option
    int req_log;

	FILE* req_log_file;
	char* db_name;
	db* db_ptr;
}proxy_node;

typedef struct proxy_msg_header_t{
    action_t action;
    uint16_t connection_id;
}proxy_msg_header;
#define PROXY_MSG_HEADER_SIZE (sizeof(proxy_msg_header))

typedef struct proxy_connect_msg_t{
    proxy_msg_header header;
}proxy_connect_msg;
#define PROXY_CONNECT_MSG_SIZE (sizeof(proxy_connect_msg))

typedef struct proxy_send_msg_t{
    proxy_msg_header header;
    size_t data_size;
    char data[0];
}proxy_send_msg;
#define PROXY_SEND_MSG_SIZE(M) (M->data_size+sizeof(proxy_send_msg))

typedef struct proxy_close_msg_t{
    proxy_msg_header header;
}proxy_close_msg;
#define PROXY_CLOSE_MSG_SIZE (sizeof(proxy_close_msg))

#endif