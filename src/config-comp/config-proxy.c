#include "../include/util/common-header.h"
#include "../include/proxy/proxy.h"
#include <libconfig.h>


int proxy_read_config(struct proxy_node_t* cur_node,const char* config_path){
    config_t config_file;
    config_init(&config_file);

    if(!config_read_file(&config_file,config_path)){
        goto goto_config_error;
    }

    config_setting_t *proxy_global_config;
    proxy_global_config = config_lookup(&config_file,"proxy_global_config");
    const char* db_name;
    if(!config_setting_lookup_string(proxy_global_config,"db_name",&db_name)){
        goto goto_config_error;
    }
    if(!config_setting_lookup_int(proxy_global_config,"req_log",&cur_node->req_log)){
        goto goto_config_error;
    }

    size_t db_name_len = strlen(db_name);
    cur_node->db_name = (char*)malloc(sizeof(char)*(db_name_len+1));
    if(cur_node->db_name==NULL){
        goto goto_config_error;
    }
    if(NULL==strncpy(cur_node->db_name,db_name,db_name_len)){
        free(cur_node->db_name);
        goto goto_config_error;
    }
    cur_node->db_name[db_name_len] = '\0';

    const char* peer_ipaddr=NULL;
    int peer_port=-1;
// parse server address
    config_setting_t *server_global_config;
    server_global_config = config_lookup(&config_file,"server_global_config");

    if(!config_setting_lookup_string(server_global_config,"ip_address",&peer_ipaddr)){
        goto goto_config_error;
    }
    if(!config_setting_lookup_int(server_global_config,"port",&peer_port)){
        goto goto_config_error;
    }

    cur_node->sys_addr.s_addr.sin_port = htons(peer_port);
    cur_node->sys_addr.s_addr.sin_family = AF_INET;
    inet_pton(AF_INET,peer_ipaddr,&cur_node->sys_addr.s_addr.sin_addr);
    cur_node->sys_addr.s_sock_len = sizeof(cur_node->sys_addr.s_addr);


// parse consensus address
    config_setting_t *consensus_global_config;
    consensus_global_config = config_lookup(&config_file,"consensus_global_config");

    if(NULL==consensus_global_config){
        err_log("PROXY : Cannot Find Nodes Settings \n");
        goto goto_config_error;
    }    

    peer_ipaddr=NULL;
    peer_port=-1;

    if(!config_setting_lookup_string(consensus_global_config,"ip_address",&peer_ipaddr)){
        goto goto_config_error;
    }
    if(!config_setting_lookup_int(consensus_global_config,"port",&peer_port)){
        goto goto_config_error;
    }
    cur_node->sys_addr.c_addr.sin_port = htons(peer_port);
    cur_node->sys_addr.c_addr.sin_family = AF_INET;
    inet_pton(AF_INET,peer_ipaddr,&cur_node->sys_addr.c_addr.sin_addr);
    cur_node->sys_addr.c_sock_len = sizeof(cur_node->sys_addr.c_addr);

    config_destroy(&config_file);
    return 0;

goto_config_error:
    err_log("%s:%d - %s\n", config_error_file(&config_file),
            config_error_line(&config_file), config_error_text(&config_file));
    config_destroy(&config_file);
    return -1;
}
