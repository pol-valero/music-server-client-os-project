#ifndef _POOLE_CONFIG_H_
#define _POOLE_CONFIG_H_

#include "../globals.h"

typedef struct {
    char* name;
    char* files_folder;
    char* ip_discovery;
    int port_discovery;
    char* ip_poole;
    int port_poole;
} ServerConfig;

ServerConfig readConfigFile(int fd_config);

void printConfigFile(ServerConfig client_config);

#endif