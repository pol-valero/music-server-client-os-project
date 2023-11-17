#include "discoveryConfig.h"


// Read the configuration file and return the clientConfig struct with all fields filled.
DiscoveryConfig readConfigFile(int fd_config) {

    DiscoveryConfig discovery_config;
    char* port;

    discovery_config.ip_poole = readUntilChar(fd_config, '\n');

    port = readUntilChar(fd_config, '\n');
    discovery_config.port_poole = atoi(port);   
    free(port);

    discovery_config.ip_bowlman = readUntilChar(fd_config, '\n');
    port = readUntilChar(fd_config, ' ');   //Does not matter which end character we send
    discovery_config.port_bowlman = atoi(port);   
    free(port);

    return discovery_config;
}