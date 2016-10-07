#include "../include/proxy/proxy.h"
#include "../include/config-comp/config-proxy.h"
#define __STDC_FORMAT_MACROS
#include "../include/dare/dare_server.h"

static void do_action_to_server(size_t data_size,void* data,uint8_t type);
static void do_action_connect(size_t data_size,void* data);
static void do_action_send(size_t data_size,void* data);
static void do_action_close(size_t data_size,void* data);

FILE *log_fp;
extern char* global_mgid;

int dare_main(node_id_t node_id, uint8_t group_size)
{
    int rc; 
    char *log_file="";
    dare_server_input_t input = {
        .log = stdout,
        .name = "",
        .output = "dare_servers.out",
        .srv_type = SRV_TYPE_START,
        .group_size = 3,
        .server_idx = 0xFF
    };
    input.ucb = do_action_to_server;
    //void(*user_cb)(size_t data_size,void* data,void* arg),
    static int srv_type = SRV_TYPE_START;

    // parser
    input.group_size = group_size;
    input.server_idx = node_id;
    global_mgid = getenv("mgid");
    const char *server_type = getenv("server_type");
    if (strcmp(server_type, "join") == 0)
    	srv_type = SRV_TYPE_JOIN;
    
    input.srv_type = srv_type;

    if (strcmp(log_file, "") != 0) {
        input.log = fopen(log_file, "w+");
        if (input.log==NULL) {
            printf("Cannot open log file\n");
            exit(1);
        }
    }
    if (SRV_TYPE_START == input.srv_type) {
        if (0xFF == input.server_idx) {
            printf("A server cannot start without an index\n");
            exit(1);
        }
    }
    pthread_t dare_thread;
    rc = pthread_create(&dare_thread, NULL, &dare_server_init, &input);
    if (0 != rc) {
        fprintf(log_fp, "Cannot init dare_thread\n");
        return 1;
    }
    
    //fclose(log_fp);
    
    return 0;
}

void proxy_on_read(proxy_node* proxy, void* buf, ssize_t bytes_read, int fd)
{
	if (is_leader())
	{
		pair_count* pair = NULL;
		HASH_FIND_INT(proxy->hash_map, &fd, pair);
		pair->req_id = ++pair->req_id;
		pair_count* replaced_pair = NULL;
		HASH_REPLACE_INT(proxy->hash_map, clt_id, pair, replaced_pair);
		leader_handle_submit_req(P_SEND, bytes_read, buf, fd, pair->req_id);
	}

	return;
}

void proxy_on_accept(proxy_node* proxy, int fd)
{
    if (is_leader())
    {
        pair_count* pair = malloc(sizeof(pair_count));
        memset(pair,0,sizeof(pair_count));

        pair->clt_id = fd;
        pair->req_id = 1;
        HASH_ADD_INT(proxy->hash_map, clt_id, pair);
        leader_handle_submit_req(P_CONNECT, 0, NULL, fd, pair->req_id);
    } else {
    }

    return;	
}

void proxy_on_close(proxy_node* proxy, int fd)
{
    if (is_leader())
    {
        pair_count* pair = NULL;
        HASH_FIND_INT(proxy->hash_map, &fd, pair);
        pair->req_id = ++pair->req_id;
        uint64_t req_id = pair->req_id;
        pair_count* replaced_pair = NULL;
        HASH_REPLACE_INT(proxy->hash_map, clt_id, pair, replaced_pair);
        HASH_DEL(proxy->hash_map, pair);
        leader_handle_submit_req(P_CLOSE, 0, NULL, fd, req_id);
    }
    return;
}

static void do_action_to_server(size_t data_size,void* data,uint8_t type)
{
    switch(type){
        case P_CONNECT:
            do_action_connect(data_size,data);
            break;
        case P_SEND:
            do_action_send(data_size,data);
            break;
        case P_CLOSE:
            do_action_close(data_size,data);
            break;
        default:
            break;
    }
    return;
}

static void do_action_connect(size_t data_size,void* data)
{

}

static void do_action_send(size_t data_size,void* data)
{

}

static void do_action_close(size_t data_size,void* data)
{

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

	proxy->hash_map = NULL;

    dare_main(node_id, proxy->group_size);

    return proxy;

proxy_exit_error:
    if(NULL!=proxy){
        free(proxy);
    }
    return NULL;

}