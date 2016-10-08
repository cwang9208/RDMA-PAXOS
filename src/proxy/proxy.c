#include "../include/proxy/proxy.h"
#include "../include/config-comp/config-proxy.h"
#include <fcntl.h>
#include <netinet/tcp.h>
#define __STDC_FORMAT_MACROS
#include "../include/dare/dare_server.h"

static void do_action_to_server(int clt_id,uint8_t type,size_t data_size,void* data,void *arg);
static void do_action_connect(int clt_id,size_t data_size,void* data,void* arg);
static void do_action_send(int clt_id,size_t data_size,void* data,void* arg);
static void do_action_close(int clt_id,size_t data_size,void* data,void* arg);
static int set_blocking(int fd, int blocking);

FILE *log_fp;

int dare_main(node_id_t node_id, uint8_t group_size, proxy_node* proxy)
{
    int rc; 
    char *log_file="";
    dare_server_input_t *input = (dare_server_input_t*)malloc(sizeof(dare_server_input_t));
    input->log = stdout;
    input->name = "";
    input->output = "dare_servers.out";
    input->srv_type = SRV_TYPE_START;
    input->group_size = 3;
    input->server_idx = 0xFF;

    input->ucb = do_action_to_server;
    input->up_para = proxy;
    static int srv_type = SRV_TYPE_START;

    // parser
    input->group_size = group_size;
    input->server_idx = node_id;

    const char *server_type = getenv("server_type");
    if (strcmp(server_type, "join") == 0)
    	srv_type = SRV_TYPE_JOIN;
    
    input->srv_type = srv_type;

    if (strcmp(log_file, "") != 0) {
        input->log = fopen(log_file, "w+");
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
	{
		count_pair* pair = NULL;
		HASH_FIND_INT(proxy->leader_hash_map, &fd, pair);
		pair->req_id = ++pair->req_id;
		count_pair* replaced_pair = NULL;
		HASH_REPLACE_INT(proxy->leader_hash_map, clt_id, pair, replaced_pair);
		leader_handle_submit_req(P_SEND, bytes_read, buf, fd, pair->req_id);
	}

	return;
}

void proxy_on_accept(proxy_node* proxy, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;
	if (is_leader())
	{
		count_pair* pair = malloc(sizeof(count_pair));
		memset(pair,0,sizeof(count_pair));

		pair->clt_id = fd;
		pair->req_id = 1;
		HASH_ADD_INT(proxy->leader_hash_map, clt_id, pair);
		leader_handle_submit_req(P_CONNECT, 0, NULL, fd, pair->req_id);
	} else {
	}

	return;	
}

void proxy_on_close(proxy_node* proxy, int fd)
{
	if (is_inner(proxy->inner_threads, pthread_self()))
		return;

	if (is_leader())
	{
		count_pair* pair = NULL;
		HASH_FIND_INT(proxy->leader_hash_map, &fd, pair);
		pair->req_id = ++pair->req_id;
		uint64_t req_id = pair->req_id;
		HASH_DEL(proxy->leader_hash_map, pair);
		leader_handle_submit_req(P_CLOSE, 0, NULL, fd, req_id);
	}
	return;
}

static void do_action_to_server(int clt_id,uint8_t type,size_t data_size,void* data,void*arg)
{
    switch(type){
        case P_CONNECT:
            do_action_connect(clt_id,data_size,data,arg);
            break;
        case P_SEND:
            do_action_send(clt_id,data_size,data,arg);
            break;
        case P_CLOSE:
            do_action_close(clt_id,data_size,data,arg);
            break;
        default:
            break;
    }
    return;
}

static void do_action_connect(int clt_id,size_t data_size,void* data,void* arg)
{
	proxy_node* proxy = arg;

	socket_pair* ret;
	HASH_FIND_INT(proxy->follower_hash_map, &clt_id, ret);
	if (NULL == ret)
	{
		ret = malloc(sizeof(socket_pair));
		memset(ret,0,sizeof(socket_pair));

		ret->clt_id = clt_id;
		int sockfd = socket(AF_INET, SOCK_STREAM, 0);
		if (sockfd < 0)
		{
			fprintf(stderr, "ERROR opening socket!\n");
			goto do_action_connect_exit;
		}
		ret->p_s = sockfd;
		HASH_ADD_INT(proxy->follower_hash_map, clt_id, ret);
	}

	if (connect(ret->p_s, (struct sockaddr*)&proxy->sys_addr.s_addr, proxy->sys_addr.s_sock_len) < 0)
		fprintf(stderr, "ERROR connecting!\n");

	set_blocking(ret->p_s, 0);

	int enable = 1;
	if(setsockopt(ret->p_s, IPPROTO_TCP, TCP_NODELAY, (void*)&enable, sizeof(enable)) < 0)
		fprintf(stderr, "TCP_NODELAY SETTING ERROR!\n");

do_action_connect_exit:
	return;
}

static void do_action_send(int clt_id,size_t data_size,void* data,void* arg)
{
	proxy_node* proxy = arg;
	socket_pair* ret;
	HASH_FIND_INT(proxy->follower_hash_map, &clt_id, ret);

	if(NULL==ret){
		goto do_action_send_exit;
	}else{
		int n = write(ret->p_s, data, data_size);
		if (n < 0)
			fprintf(stderr, "ERROR writing to socket");
	}
do_action_send_exit:
	return;
}

static void do_action_close(int clt_id,size_t data_size,void* data,void* arg)
{
	proxy_node* proxy = arg;
	socket_pair* ret;
	HASH_FIND_INT(proxy->follower_hash_map, &clt_id, ret);
	if(NULL==ret){
		goto do_action_close_exit;
	}else{
		if (close(ret->p_s))
			fprintf(stderr, "ERROR closing socket\n");
		HASH_DEL(proxy->follower_hash_map, ret);
	}
do_action_close_exit:
	return;
}

proxy_node* proxy_init(node_id_t node_id,const char* config_path)
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

	proxy->db_ptr = initialize_db(proxy->db_name,0);

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
