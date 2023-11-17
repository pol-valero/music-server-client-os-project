#include "pooleConfig.h"
#include "globals.h"

// Read the configuration file and return the clientConfig struct with all fields filled.
ServerConfig readConfigFile(int fd_config) {

    ServerConfig server_config;
    char* port;

    server_config.name = readUntilCharExceptLetter(fd_config, '\n', '&');

    server_config.files_folder = readUntilChar(fd_config, '\n');

    server_config.ip_discovery = readUntilChar(fd_config, '\n');

    port = readUntilChar(fd_config, '\n');
    server_config.port_discovery = atoi(port);   
    free(port);

    server_config.ip_poole = readUntilChar(fd_config, '\n');
    port = readUntilChar(fd_config, ' ');   //Does not matter which end character we send
    server_config.port_poole = atoi(port);   
    free(port);

    return server_config;
}

// Print the received configuration.
void printConfigFile(ServerConfig server_config) {
    char* buffer;
    int buffSize;

    buffSize = asprintf (&buffer, "\n%s server initialized\n", server_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);

    printx("\nFile read correctly:\n");

    buffSize = asprintf(&buffer, "Server - %s\n", server_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "Directory - %s\n", server_config.files_folder);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "IP Discovery - %s\n", server_config.ip_discovery);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "Port Discovery - %d\n", server_config.port_discovery);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "IP Poole - %s\n", server_config.ip_poole);
    printDynStr(buffer, buffSize);
    free(buffer);
    
    buffSize = asprintf(&buffer, "Port Poole - %d\n", server_config.port_poole);
    printDynStr(buffer, buffSize);
    free(buffer);
}