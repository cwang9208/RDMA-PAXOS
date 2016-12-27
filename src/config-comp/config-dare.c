#include "../include/util/common-header.h"
#include "../include/dare/dare_server.h"
#include <libconfig.h>


int dare_read_config(const char* config_path){
    config_t config_file;
    config_init(&config_file);

    if(!config_read_file(&config_file,config_path)){
        goto goto_config_error;
    }

    config_setting_t *dare_global_config = NULL;
    dare_global_config = config_lookup(&config_file,"dare_global_config");

    if(NULL!=dare_global_config){
        double temp;
        if(config_setting_lookup_float(dare_global_config,"hb_period",&temp)){
            hb_period = temp;
        }
    }

    config_destroy(&config_file);
    return 0;

goto_config_error:
    err_log("%s:%d - %s\n", config_error_file(&config_file),
            config_error_line(&config_file), config_error_text(&config_file));
    config_destroy(&config_file);
    return -1;
}
