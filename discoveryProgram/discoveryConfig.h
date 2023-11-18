#ifndef _DISCOVERY_CONFIG_H_
#define _DISCOVERY_CONFIG_H_

#include "../globals.h"

typedef struct {
    char* ip_poole;
    int port_poole;
    char* ip_bowman;
    int port_bowman;
} DiscoveryConfig;

DiscoveryConfig readConfigFile(int fd_config);

void printConfigFile(DiscoveryConfig server_config);

#endif