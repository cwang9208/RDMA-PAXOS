#include "../include/proxy/proxy.h"
#include "../include/config-comp/config-proxy.h"
#include <fcntl.h>
#include <netinet/tcp.h>
#define __STDC_FORMAT_MACROS
#include "../include/dare/dare_server.h"

static void stablestorage_save_request(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void*arg);
static void do_action_to_server(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void *arg);
static void do_action_connect(uint16_t clt_id,size_t data_size,void* data,void* arg);
static void do_action_send(uint16_t clt_id,size_t data_size,void* data,void* arg);
static void do_action_close(uint16_t clt_id,size_t data_size,void* data,void* arg);
static int set_blocking(int fd, int blocking);
static void process_data(proxy_node* proxy, uint8_t type, ssize_t data_size, void* buf, int clt_id);

FILE *log_fp;

int dare_main(node_id_t node_id, uint8_t group_size, proxy_node* proxy)
{
    int rc; 
    dare_server_input_t *input = (dare_server_input_t*)malloc(sizeof(dare_server_input_t));
    input->log = stdout;
    input->name = "";
    input->output = "dare_servers.out";
    input->srv_type = SRV_TYPE_START;
    input->sm_type = CLT_KVS;
    input->group_size = 3;
    input->server_idx = 0xFF;

    input->do_action = do_action_to_server;
    input->store_cmd = stablestorage_save_request;
    input->up_para = proxy;
    static int srv_type = SRV_TYPE_START;

    // parser
    input->group_size = group_size;
    input->server_idx = node_id;

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

static hk_t gen_key(nid_t node_id,nc_t node_count){
    hk_t key = 0;
    key |= ((hk_t)node_id<<8);
    key |= (hk_t)node_count;
    return key;
}

void process_data(proxy_node* proxy, uint8_t type, ssize_t data_size, void* buf, int clt_id)
{
    void *cmd = build_req_sub_msg(data_size, buf);

    socket_pair* pair = NULL;
    uint64_t req_id;
    uint16_t connection_id;
    
    pthread_spin_lock(&proxy->spinlock);
    switch(type) {
        case P_CONNECT:
            pair = (socket_pair*)malloc(sizeof(socket_pair));
            memset(pair,0,sizeof(socket_pair));
            pair->clt_id = clt_id;
            pair->req_id = 0;
            pair->connection_id = gen_key(proxy->node_id,proxy->pair_count++);
            
            req_id = ++pair->req_id;
            connection_id = pair->connection_id;
            
            HASH_ADD_INT(proxy->leader_hash_map, clt_id, pair);
            break;
        case P_SEND:
            HASH_FIND_INT(proxy->leader_hash_map, &clt_id, pair);
            
            req_id = ++pair->req_id;
            connection_id = pair->connection_id;
            
            socket_pair* replaced_pair = NULL;
            HASH_REPLACE_INT(proxy->leader_hash_map, clt_id, pair, replaced_pair);
            break;
        case P_CLOSE:
            HASH_FIND_INT(proxy->leader_hash_map, &clt_id, pair);
            
            req_id = ++pair->req_id;
            connection_id = pair->connection_id;
            
            HASH_DEL(proxy->leader_hash_map, pair);
            break;
    }
    leader_handle_submit_req(req_id, connection_id, type, cmd);
    pthread_spin_unlock(&proxy->spinlock);
}

void proxy_on_read(proxy_node* proxy, void* buf, ssize_t bytes_read, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;

	if (is_leader())
        process_data(proxy, P_SEND, bytes_read, buf, fd);

	return;
}

void proxy_on_accept(proxy_node* proxy, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;

	if (is_leader())
        process_data(proxy, P_CONNECT, 0, NULL, fd);

	return;	
}

void proxy_on_close(proxy_node* proxy, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;

	if (is_leader())
        process_data(proxy, P_CLOSE, 0, NULL, fd);

	return;
}

static void stablestorage_save_request(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void*arg)
{
    proxy_node* proxy = arg;
    switch(type){
        case P_CONNECT:
        {
            proxy_connect_msg* co_msg = (proxy_connect_msg*)malloc(PROXY_CONNECT_MSG_SIZE);
            co_msg->header.action = P_CONNECT;
            co_msg->header.connection_id = clt_id;
            store_record(proxy->db_ptr,PROXY_CONNECT_MSG_SIZE,co_msg);
            break;
        }
        case P_SEND:
        {
            proxy_send_msg* send_msg = (proxy_send_msg*)malloc(sizeof(proxy_send_msg)+data_size);
            send_msg->header.action = P_SEND;
            send_msg->header.connection_id = clt_id;
            send_msg->data_size = data_size;
            memcpy(send_msg->data,data,data_size);
            store_record(proxy->db_ptr,PROXY_SEND_MSG_SIZE(send_msg),send_msg);
            break;
        }
        case P_CLOSE:
        {
            proxy_close_msg* cl_msg = malloc(PROXY_CLOSE_MSG_SIZE);
            cl_msg->header.action = P_CLOSE;
            cl_msg->header.connection_id = clt_id;
            store_record(proxy->db_ptr,PROXY_CLOSE_MSG_SIZE,cl_msg);
            break;
        }
    }
}

static void do_action_to_server(uint16_t clt_id,uint8_t type,size_t data_size,void* data,void*arg)
{
    proxy_node* proxy = arg;
    FILE* output = NULL;
    if(proxy->req_log){
        output = proxy->req_log_file;
    }
    switch(type){
        case P_CONNECT:
        	if(output!=NULL){
        		fprintf(output,"Operation: Connects.\n");
            }
            do_action_connect(clt_id,data_size,data,arg);
            break;
        case P_SEND:
        	if(output!=NULL){
        		fprintf(output,"Operation: Sends data.\n");
            }
            do_action_send(clt_id,data_size,data,arg);
            break;
        case P_CLOSE:
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

    socket_pair* ret;
    HASH_FIND(hh, proxy->follower_hash_map, &clt_id, sizeof(uint16_t), ret);
    if (NULL == ret)
    {
        ret = malloc(sizeof(socket_pair));
        memset(ret,0,sizeof(socket_pair));

        ret->connection_id = clt_id;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0)
        {
            fprintf(stderr, "ERROR opening socket!\n");
            goto do_action_connect_exit;
        }
        ret->p_s = sockfd;
        HASH_ADD(hh, proxy->follower_hash_map, connection_id, sizeof(uint16_t), ret);

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
	socket_pair* ret;
	HASH_FIND(hh, proxy->follower_hash_map, &clt_id, sizeof(uint16_t), ret);

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
	socket_pair* ret;
	HASH_FIND(hh, proxy->follower_hash_map, &clt_id, sizeof(uint16_t), ret);
	if(NULL==ret){
		goto do_action_close_exit;
	}else{
		if (close(ret->p_s))
			fprintf(stderr, "ERROR closing socket!\n");
		HASH_DEL(proxy->follower_hash_map, ret);
	}
do_action_close_exit:
	return;
}

proxy_node* proxy_init(node_id_t node_id,const char* config_path,const char* proxy_log_path)
{
    proxy_node* proxy = (proxy_node*)malloc(sizeof(proxy_node));

    if(NULL==proxy){
        err_log("PROXY : Cannot Malloc Memory For The Proxy.\n");
        goto proxy_exit_error;
    }

    memset(proxy,0,sizeof(proxy_node));
    
    proxy->node_id = node_id;
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
                sprintf(req_log_path,"%s/node-%u-proxy-req.log",proxy_log_path,proxy->node_id);
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

    if(pthread_spin_init(&proxy->spinlock, PTHREAD_PROCESS_PRIVATE)){
        err_log("PROXY : Cannot Init The Lock.\n");
        goto proxy_exit_error;
    }

	proxy->leader_hash_map = NULL;
    proxy->follower_hash_map = NULL;
	
    proxy->inner_threads = NULL;

	dare_main(node_id, proxy->group_size, proxy);

    return proxy;

proxy_exit_error:
    if(NULL!=proxy){
        free(proxy);
    }
    return NULL;

}
