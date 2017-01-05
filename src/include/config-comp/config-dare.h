#ifndef CONFIG_DARE_H
#define CONFIG_DARE_H
#include "../util/debug.h"
#include "../dare/dare_server.h"
#include <libconfig.h>

int dare_read_config(dare_server_data_t* data, const char* config_path);

#endif
