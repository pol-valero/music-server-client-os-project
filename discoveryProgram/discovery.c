#include "../globals.h"
#include "discoveryConfig.h"

int fd_config;
int fd_client;
int fd_socket;

DiscoveryConfig discovery_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

// Handle unexpected termination scenarios.
void terminateExecution () {
    char* currentInputPointer = getGlobalsCurrentInputPointer();

    free(discovery_config.ip_bowman);
    free(discovery_config.ip_poole);

    if (currentInputPointer != NULL) {
        free(currentInputPointer);
    }

    close (fd_config);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

//main function :p
int main (int argc, char** argv) {

    signal(SIGINT, terminateExecution);
    signal(SIGTERM, terminateExecution);

    if (argc < 2) {
        printEr("\nERROR: You must enter a the configuration file name as a parameter\n");
        return 0;
    } else if(argc > 2){
        printEr("\nERROR: More arguments than needed.\n");
        return 0;
    }

    fd_config = open(argv[1], O_RDONLY);

    if (fd_config < 0) {
        printEr("\nERROR: Cannot open the file. Filename may be incorrect\n");
        return 0;
    }

    discovery_config = readConfigFile(fd_config);

    //TODO: REMOVE THESE LINES, THEY ARE JUST FOR TESTING
    /*struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket = start_server(server_config.port_poole, server_config.ip_poole);
    fd_client = accept(fd_socket, (void *) &c_addr, &c_len);
    char* msg = readUntilChar(fd_client, '\n');
    printx(msg);
    write(fd_client, "prova2\n", strlen("prova2\n"));*/
    /////////
    
    terminateExecution();

    return 0;
}