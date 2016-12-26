#include "../include/proxy/proxy.h"
#include "../include/config-comp/config-proxy.h"
#include <fcntl.h>
#include <netinet/tcp.h>
#define __STDC_FORMAT_MACROS

static void stablestorage_save_request(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void*arg);
static uint32_t stablestorage_dump(void*buf,void*arg);
static int stablestorage_load(void*buf,uint32_t size,void*arg);
static void do_action_to_server(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void *arg);
static void do_action_connect(uint16_t clt_id,size_t data_size,void* data,void* arg);
static void do_action_send(uint16_t clt_id,size_t data_size,void* data,void* arg);
static void do_action_close(uint16_t clt_id,size_t data_size,void* data,void* arg);
static int set_blocking(int fd, int blocking);

FILE *log_fp;

int dare_main(proxy_node* proxy)
{
    int rc; 
    dare_server_input_t *input = (dare_server_input_t*)malloc(sizeof(dare_server_input_t));
    input->log = stdout;
    input->name = "";
    input->output = "dare_servers.out";
    input->srv_type = SRV_TYPE_START;
    input->sm_type = CLT_KVS;
    input->server_idx = 0xFF;
    char *server_idx = getenv("server_idx");
    if (server_idx != NULL)
        input->server_idx = (uint8_t)atoi(server_idx);
    input->group_size = 3;
    char *group_size = getenv("group_size");
    if (group_size != NULL)
        input->group_size = (uint8_t)atoi(group_size);

    input->do_action = do_action_to_server;
    input->store_cmd = stablestorage_save_request;
    input->create_db_snapshot = stablestorage_dump;
    input->apply_db_snapshot = stablestorage_load;
    input->up_para = proxy;
    static int srv_type = SRV_TYPE_START;

    const char *server_type = getenv("server_type");
    if (strcmp(server_type, "join") == 0)
    	srv_type = SRV_TYPE_JOIN;
    char *dare_log_file = getenv("dare_log_file");
    if (dare_log_file == NULL)
        dare_log_file = "";

    input->srv_type = srv_type;

    if (strcmp(dare_log_file, "") != 0) {
        input->log = fopen(dare_log_file, "w+");
        if (input->log==NULL) {
            printf("Cannot open log file\n");
            exit(1);
        }
    }
    if (SRV_TYPE_START == input->srv_type) {
        if (0xFF == input->server_idx) {
            printf("A server cannot start without an index\n");
            exit(1);
        }
    }
    pthread_t dare_thread;
    rc = pthread_create(&dare_thread, NULL, &dare_server_init, input);
    if (0 != rc) {
        fprintf(log_fp, "Cannot init dare_thread\n");
        return 1;
    }

    inner_thread *elt = (inner_thread*)malloc(sizeof(inner_thread));
    elt->tid = dare_thread;
    LL_APPEND(proxy->inner_threads,elt);
    
    //fclose(log_fp);
    
    return 0;
}

static int tidcmp(inner_thread *a, inner_thread *b) {
    return (a->tid == b->tid) ? 0 : 1;
}

static int is_inner(inner_thread* head, pthread_t tid)
{
	inner_thread *elt, etmp;
	etmp.tid = tid;
	LL_SEARCH(head,elt,&etmp,tidcmp);

	if (elt)
		return 1;
	else
		return 0;
}

static int set_blocking(int fd, int blocking) {
    int flags;

    if ((flags = fcntl(fd, F_GETFL)) == -1) {
        fprintf(stderr, "fcntl(F_GETFL): %s", strerror(errno));
    }

    if (blocking)
        flags &= ~O_NONBLOCK;
    else
        flags |= O_NONBLOCK;

    if (fcntl(fd, F_SETFL, flags) == -1) {
        fprintf(stderr, "fcntl(F_SETFL,O_NONBLOCK): %s", strerror(errno));
    }
    return 0;
}

void proxy_on_read(proxy_node* proxy, void* buf, ssize_t bytes_read, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;

	if (is_leader())
        leader_handle_submit_req(SEND, bytes_read, buf, fd);

	return;
}

void proxy_on_accept(proxy_node* proxy, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;

	if (is_leader())
        leader_handle_submit_req(CONNECT, 0, NULL, fd);

	return;	
}

void proxy_on_close(proxy_node* proxy, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;

	if (is_leader())
        leader_handle_submit_req(CLOSE, 0, NULL, fd);

	return;
}

static void stablestorage_save_request(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void*arg)
{
    proxy_node* proxy = arg;
    switch(type){
        case CONNECT:
        {
            proxy_connect_msg* co_msg = (proxy_connect_msg*)malloc(PROXY_CONNECT_MSG_SIZE);
            co_msg->header.action = CONNECT;
            co_msg->header.connection_id = clt_id;
            store_record(proxy->db_ptr,PROXY_CONNECT_MSG_SIZE,co_msg);
            break;
        }
        case SEND:
        {
            proxy_send_msg* send_msg = (proxy_send_msg*)malloc(sizeof(proxy_send_msg)+data_size);
            send_msg->header.action = SEND;
            send_msg->header.connection_id = clt_id;
            send_msg->data_size = data_size;
            memcpy(send_msg->data,data,data_size);
            store_record(proxy->db_ptr,PROXY_SEND_MSG_SIZE(send_msg),send_msg);
            break;
        }
        case CLOSE:
        {
            proxy_close_msg* cl_msg = malloc(PROXY_CLOSE_MSG_SIZE);
            cl_msg->header.action = CLOSE;
            cl_msg->header.connection_id = clt_id;
            store_record(proxy->db_ptr,PROXY_CLOSE_MSG_SIZE,cl_msg);
            break;
        }
    }
}

static uint32_t stablestorage_dump(void*buf,void*arg)
{
    proxy_node* proxy = arg;
    uint32_t size = dump_records(proxy->db_ptr,buf);
    return size;
}

static int stablestorage_load(void*buf,uint32_t size,void*arg)
{
    proxy_msg_header* header = (proxy_msg_header*)buf;
    size_t data_size = 0;
    uint32_t len = 0;
    void* data = NULL;
    while(len < size) {
        header = (proxy_msg_header*)((char*)buf + len);
        switch(header->action){
            case SEND:
            {
                proxy_send_msg* send_msg = (proxy_send_msg*)buf;
                data_size = send_msg->data_size;;
                data = send_msg->data;
                len += PROXY_SEND_MSG_SIZE(send_msg);
            }
            case CONNECT:
            {
                len += PROXY_CONNECT_MSG_SIZE;
            }
            case CLOSE:
            {
                len += PROXY_CLOSE_MSG_SIZE;
            }
        }
        do_action_to_server(header->connection_id, header->action, data_size, data ,arg);
    }
    return 0;
}

static void do_action_to_server(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void*arg)
{
    proxy_node* proxy = arg;
    FILE* output = NULL;
    if(proxy->req_log){
        output = proxy->req_log_file;
    }
    switch(type){
        case CONNECT:
        	if(output!=NULL){
        		fprintf(output,"Operation: Connects.\n");
            }
            do_action_connect(clt_id,data_size,data,arg);
            break;
        case SEND:
        	if(output!=NULL){
        		fprintf(output,"Operation: Sends data.\n");
            }
            do_action_send(clt_id,data_size,data,arg);
            break;
        case CLOSE:
        	if(output!=NULL){
        		fprintf(output,"Operation: Closes.\n");
            }
            do_action_close(clt_id,data_size,data,arg);
            break;
        default:
            break;
    }
    return;
}

static void do_action_connect(uint16_t clt_id,size_t data_size,void* data,void* arg)
{
    proxy_node* proxy = arg;

    socket_pair_t* ret;
    HASH_FIND(hh, proxy->hash_map, &clt_id, sizeof(uint16_t), ret);
    if (NULL == ret)
    {
        ret = malloc(sizeof(socket_pair_t));
        memset(ret,0,sizeof(socket_pair_t));

        ret->connection_id = clt_id;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            fprintf(stderr, "ERROR opening socket!\n");
            goto do_action_connect_exit;
        }
        ret->p_s = sockfd;
        HASH_ADD(hh, proxy->hash_map, connection_id, sizeof(uint16_t), ret);

        if (connect(ret->p_s, (struct sockaddr*)&proxy->sys_addr.s_addr, proxy->sys_addr.s_sock_len) < 0)
            fprintf(stderr, "ERROR connecting!\n");

        set_blocking(ret->p_s, 0);

        int enable = 1;
        if(setsockopt(ret->p_s, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable)) < 0)
            fprintf(stderr, "TCP_NODELAY SETTING ERROR!\n");
    }

do_action_connect_exit:
	return;
}

static void do_action_send(uint16_t clt_id,size_t data_size,void* data,void* arg)
{
	proxy_node* proxy = arg;
	socket_pair_t* ret;
	HASH_FIND(hh, proxy->hash_map, &clt_id, sizeof(uint16_t), ret);

	if(NULL==ret){
		goto do_action_send_exit;
	}else{
		int n = write(ret->p_s, data, data_size);
		if (n < 0)
			fprintf(stderr, "ERROR writing to socket!\n");
	}
do_action_send_exit:
	return;
}

static void do_action_close(uint16_t clt_id,size_t data_size,void* data,void* arg)
{
	proxy_node* proxy = arg;
	socket_pair_t* ret;
	HASH_FIND(hh, proxy->hash_map, &clt_id, sizeof(uint16_t), ret);
	if(NULL==ret){
		goto do_action_close_exit;
	}else{
		if (close(ret->p_s))
			fprintf(stderr, "ERROR closing socket!\n");
		HASH_DEL(proxy->hash_map, ret);
	}
do_action_close_exit:
	return;
}

proxy_node* proxy_init(const char* config_path,const char* proxy_log_path)
{
    proxy_node* proxy = (proxy_node*)malloc(sizeof(proxy_node));

    if(NULL==proxy){
        err_log("PROXY : Cannot Malloc Memory For The Proxy.\n");
        goto proxy_exit_error;
    }

    memset(proxy,0,sizeof(proxy_node));
    
    if(proxy_read_config(proxy,config_path)){
        err_log("PROXY : Configuration File Reading Error.\n");
        goto proxy_exit_error;
    }

    int build_log_ret = 0;
    if(proxy_log_path==NULL){
        proxy_log_path = ".";
    }else{
        if((build_log_ret=mkdir(proxy_log_path,S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH))!=0){
            if(errno!=EEXIST){
                err_log("PROXY : Log Directory Creation Failed,No Log Will Be Recorded.\n");
            }else{
                build_log_ret = 0;
            }
        }
    }

    if(!build_log_ret){
        //if(proxy->req_log){
            char* req_log_path = (char*)malloc(sizeof(char)*strlen(proxy_log_path)+50);
            memset(req_log_path,0,sizeof(char)*strlen(proxy_log_path)+50);
            if(NULL!=req_log_path){
                sprintf(req_log_path,"%s/node-proxy-req.log",proxy_log_path);
                //err_log("%s.\n",req_log_path);
                proxy->req_log_file = fopen(req_log_path,"w");
                free(req_log_path);
            }
            if(NULL==proxy->req_log_file && proxy->req_log){
                err_log("PROXY : Client Request Log File Cannot Be Created.\n");
            }
        //}
    }

	proxy->db_ptr = initialize_db(proxy->db_name,0);

	proxy->hash_map = NULL;
	
    proxy->inner_threads = NULL;

	dare_main(proxy);

    return proxy;

proxy_exit_error:
    if(NULL!=proxy){
        free(proxy);
    }
    return NULL;

}
