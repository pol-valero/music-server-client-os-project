#include "bowmanConfig.h"
#include "globals.h"

ClientConfig readConfigFile(int fd_config) {

    ClientConfig client_config;
    char* port;

    client_config.name = readUntilChar(fd_config, '\n');
    client_config.files_folder = readUntilChar(fd_config, '\n');
    client_config.ip = readUntilChar(fd_config, '\n');

    port = readUntilChar(fd_config, ' ');   //Does not matter which end character we send
    client_config.port = atoi(port);   
    free(port);

    return client_config;
}


void printConfigFile(ClientConfig client_config) {
    
    char* buffer;
    int buffSize;

    printx("\nFile read correctly:\n");
    buffSize = asprintf(&buffer, "User - %s\n", client_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);
    buffSize = asprintf(&buffer, "Directory - %s\n", client_config.files_folder);
    printDynStr(buffer, buffSize);
    free(buffer);
    buffSize = asprintf(&buffer, "IP - %s\n", client_config.ip);
    printDynStr(buffer, buffSize);
    free(buffer);
    buffSize = asprintf(&buffer, "Port - %d\n\n", client_config.port);
    printDynStr(buffer, buffSize);
    free(buffer);

}

void checkUsernameFormat(char* username) {
    
    char* forbiddenChar;

    while (strchr(username, '&') != NULL) {
        forbiddenChar = strchr(username, '&');
        *forbiddenChar = ' ';
    }

}

void printInitMsg(char* username) {

    char* buffer;
    int bufferSize;

    checkUsernameFormat(username);

    bufferSize = asprintf(&buffer, "\n%s user initialized\n", username);
    printDynStr(buffer, bufferSize);
    free(buffer);

}