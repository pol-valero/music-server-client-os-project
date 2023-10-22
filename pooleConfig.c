#include "pooleConfig.h"
#include "globals.h"

ServerConfig readConfigFile(int fd_config) {

    ServerConfig server_config;
    char* port;

    server_config.name = readUntilChar(fd_config, '\n');

    server_config.files_folder = readUntilChar(fd_config, '\n');

    server_config.ip = readUntilChar(fd_config, '\n');

    port = readUntilChar(fd_config, '\n');
    server_config.port = atoi(port);   
    free(port);

    server_config.ip_server = readUntilChar(fd_config, '\n');
    port = readUntilChar(fd_config, ' ');   //Does not matter which end character we send
    server_config.port_server = atoi(port);   
    free(port);

    return server_config;
}

void printInitMsg(char* username) {
    char* buffer;
    int bufferSize;

    bufferSize = asprintf(&buffer, "\n%s server initialized\n", username);
    printDynStr(buffer, bufferSize);
    free(buffer);
}

void printConfigFile(ServerConfig server_config) {
    char* buffer;
    int buffSize;

    printx("\nFile read correctly:\n");

    buffSize = asprintf(&buffer, "Server - %s\n", server_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "Directory - %s\n", server_config.files_folder);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "IP - %s\n", server_config.ip);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "Port - %d\n", server_config.port);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "IP Server - %s\n", server_config.ip_server);
    printDynStr(buffer, buffSize);
    free(buffer);
    
    buffSize = asprintf(&buffer, "Port Server - %d\n\n", server_config.port_server);
    printDynStr(buffer, buffSize);
    free(buffer);
}