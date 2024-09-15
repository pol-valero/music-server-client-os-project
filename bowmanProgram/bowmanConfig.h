#ifndef _BOWMAN_CONFIG_H_
#define _BOWMAN_CONFIG_H_

#include "../globals.h"

typedef struct {
    char* name;
    char* files_folder;
    char* ip_discovery;
    int port_discovery;
} ClientConfig;

ClientConfig readConfigFile(int fd_config);

void printConfigFile(ClientConfig client_config);

void cleanClientConfig(ClientConfig* client_config);

#endif