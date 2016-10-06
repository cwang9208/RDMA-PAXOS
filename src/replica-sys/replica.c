/**                                                                                                      
 * DARE (Direct Access REplication)
 *
 * Test code for a server
 *
 * Copyright (c) 2016 HLRS, University of Stuttgart. All rights reserved.
 * 
 * Copyright (c) 2014-2015 ETH-Zurich. All rights reserved.
 * 
 * Author(s): Marius Poke <marius.poke@inf.ethz.ch>
 *            Nakul Vyas <mailnakul@gmail.com>
 * 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#define __STDC_FORMAT_MACROS
#include <inttypes.h>
#include "../include/dare/dare_server.h"
#include "../include/dare/dare_ibv.h"

#include "../include/replica-sys/node.h"

FILE *log_fp;
extern char* global_mgid;

int dare_main()
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
    static int srv_type = SRV_TYPE_START;

    // parser
    const char *group_size = getenv("group_size");
    input.group_size = (uint8_t)atoi(group_size);
    const char *server_idx = getenv("server_idx");
    input.server_idx = (uint8_t)atoi(server_idx);
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

node* system_initialize()
{
	dare_main();
	return NULL;
}