#ifndef _DISCOVERY_CONFIG_H_
#define _DISCOVERY_CONFIG_H_

#include "../globals.h"

typedef struct {
    char* name;
    char* files_folder;
    char* ip_poole;
    int port_poole;
    char* ip_bowlman;
    int port_bowlman;
} DiscoveryConfig;

ServerConfig readConfigFile(int fd_config);

#endif