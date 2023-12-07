#include "discoveryConfig.h"


// Read the configuration file and return the clientConfig struct with all fields filled.
DiscoveryConfig readConfigFile(int fd_config) {

    DiscoveryConfig discovery_config;
    char* port;

    discovery_config.ip_poole = readUntilChar(fd_config, '\n');
    port = readUntilChar(fd_config, '\n');
    discovery_config.port_poole = atoi(port);   
    free(port);

    discovery_config.ip_bowman = readUntilChar(fd_config, '\n');
    port = readUntilChar(fd_config, ' ');   //Does not matter which end character we send
    discovery_config.port_bowman = atoi(port);   
    free(port);

    return discovery_config;
}

// Print the received configuration.
void printConfigFile(DiscoveryConfig server_config) {
    char* buffer;
    int buffSize;

    printx("\nFile read correctly:\n");

    buffSize = asprintf(&buffer, "IP Poole - %s\n", server_config.ip_poole);
    printDynStr(buffer, buffSize);
    free(buffer);
    
    buffSize = asprintf(&buffer, "Port Poole - %d\n", server_config.port_poole);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "IP Bowman - %s\n", server_config.ip_bowman);
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "Port Bowman - %d\n", server_config.port_bowman);
    printDynStr(buffer, buffSize);
    free(buffer);
}