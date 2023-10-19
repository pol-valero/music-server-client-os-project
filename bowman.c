#include "globals.h"


typedef struct {
    char* name;
    char* files_folder;
    char* ip;
    int port;
} ClientConfig;


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
    buffSize = asprintf(&buffer, "Directory - %s\n", client_config.files_folder);
    printDynStr(buffer, buffSize);
    buffSize = asprintf(&buffer, "IP - %s\n", client_config.ip);
    printDynStr(buffer, buffSize);
    buffSize = asprintf(&buffer, "Port - %d\n", client_config.port);
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

int main (int argc, char** argv) {

    int fd_config;
    ClientConfig client_config;


    if (argc < 2) {
        printx("\nERROR: You must enter a the configuration file name as a parameter\n");
        return 0;
    }

    fd_config = open(argv[1], O_RDONLY);

    if (fd_config < 0) {
        printx("\nERROR: Cannot open the file. Filename may be incorrect\n");
        return 0;
    } else {
        client_config = readConfigFile(fd_config);
    }

    printInitMsg(client_config.name);

    printConfigFile(client_config);

    //TODO: Enter COMMAND mode

    return 0;
}


//TODO: Associate SIGINT with function to liberate memory. 