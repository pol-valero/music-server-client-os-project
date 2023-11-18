#include "bowmanConfig.h"

// Read the configuration file and return the clientConfig struct with all fields filled.
ClientConfig readConfigFile(int fd_config) {

    ClientConfig client_config;
    char* port;

    client_config.name = readUntilCharExceptLetter(fd_config, '\n', '&');
    client_config.files_folder = readUntilChar(fd_config, '\n');
    client_config.ip_discovery = readUntilChar(fd_config, '\n');

    port = readUntilChar(fd_config, ' ');   //Does not matter which end character we send
    client_config.port_discovery = atoi(port);   
    free(port);

    return client_config;
}

// Print the received configuration.
void printConfigFile(ClientConfig client_config) {
    char* buffer;
    int buffSize;

    buffSize = asprintf(&buffer, "\n%s user initialized\n", client_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);

    printx("\nFile read correctly:\n");
    buffSize = asprintf(&buffer, "User - %s\n", client_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);
    buffSize = asprintf(&buffer, "Directory - %s\n", client_config.files_folder);
    printDynStr(buffer, buffSize);
    free(buffer);
    buffSize = asprintf(&buffer, "IP - %s\n", client_config.ip_discovery);
    printDynStr(buffer, buffSize);
    free(buffer);
    buffSize = asprintf(&buffer, "Port - %d\n\n", client_config.port_discovery);
    printDynStr(buffer, buffSize);
    free(buffer);
}