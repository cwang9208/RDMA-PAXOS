#ifndef MESSAGE_H
#define MESSAGE_H

#define CONNECT 4
#define SEND 5
#define CLOSE 6

typedef struct sys_msg_header_t{
    uint64_t req_id;
    uint16_t connection_id;
    uint8_t type;
    size_t data_size;
}sys_msg_header;
#define SYS_MSG_HEADER_SIZE (sizeof(sys_msg_header))

typedef struct request_submit_msg_t{
    sys_msg_header header; 
    char data[0];
}__attribute__((packed))req_sub_msg;
#define REQ_SUB_SIZE(M) (((M)->header.data_size)+sizeof(sys_msg_header))

#endif