#ifndef _POOLE_CONFIG_H_
#define _POOLE_CONFIG_H_

typedef struct {
    char* name;
    char* files_folder;
    char* ip;
    int port;
    char* ip_server;
    int port_server;
} ServerConfig;

ServerConfig readConfigFile(int fd_config);

void printConfigFile(ServerConfig client_config);

#endif