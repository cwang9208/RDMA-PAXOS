#ifndef PROXY_H 
#define PROXY_H

#include "../util/common-header.h"
#include "../rsm-interface.h"
#include "../../../utils/uthash/utlist.h"
#include "../../../utils/uthash/uthash.h"
#include "../db/db-interface.h"

typedef uint16_t nc_t;

typedef struct inner_thread {
    pthread_t tid;
    struct inner_thread *next;
}inner_thread;

typedef struct proxy_address_t{
    struct sockaddr_in c_addr;
    size_t c_sock_len;
    struct sockaddr_in s_addr;
    size_t s_sock_len;
}proxy_address;

typedef struct socket_pair_t{
	int clt_id;
	uint64_t req_id;

    uint16_t connection_id;
    int p_s;
    
    UT_hash_handle hh;
}socket_pair;

typedef struct proxy_node_t{
    nc_t pair_count;
	proxy_address sys_addr;

    socket_pair* leader_hash_map;
    socket_pair* follower_hash_map;
	inner_thread* inner_threads;
	int con_conn;
    pthread_spinlock_t spinlock;
	
    // log option
    int req_log;

	FILE* req_log_file;
	char* db_name;
	db* db_ptr;
}proxy_node;

typedef struct proxy_msg_header_t{
    uint16_t connection_id;
    uint8_t action;
}proxy_msg_header;
#define PROXY_MSG_HEADER_SIZE (sizeof(proxy_msg_header))

typedef struct proxy_connect_msg_t{
    proxy_msg_header header;
}proxy_connect_msg;
#define PROXY_CONNECT_MSG_SIZE (sizeof(proxy_connect_msg))

struct fake_dare_cid_t {
    uint64_t epoch;
    uint8_t size[2];
    uint8_t state;
    uint8_t pad[1];
    uint32_t bitmask;
};
typedef struct fake_dare_cid_t fake_dare_cid_t;

struct fake_sm_cmd_t {
    uint16_t    len;
    uint8_t cmd[0];
};
typedef struct fake_sm_cmd_t fake_sm_cmd_t;

typedef struct proxy_send_msg_t{
    proxy_msg_header header;
    union {
        fake_sm_cmd_t   cmd;
        fake_dare_cid_t cid;
        uint64_t head;
    } data;
}proxy_send_msg;
#define PROXY_SEND_MSG_SIZE(M) (M->data.cmd.len+sizeof(proxy_send_msg))

typedef struct proxy_close_msg_t{
    proxy_msg_header header;
}proxy_close_msg;
#define PROXY_CLOSE_MSG_SIZE (sizeof(proxy_close_msg))

#endif