#include "../globals.h"
#include "discoveryConfig.h"

int fd_config;
int fd_client;
int fd_socket;
//int fd_bowman;
//int fd_poole;

DiscoveryConfig discovery_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

// Handle unexpected termination scenarios.
void terminateExecution () {

    free(discovery_config.ip_bowman);
    free(discovery_config.ip_poole);

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

    printConfigFile(discovery_config);

    //TODO: REMOVE THESE LINES, THEY ARE JUST FOR TESTING
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket = startServer(discovery_config.port_bowman, discovery_config.ip_bowman);
    fd_client = accept(fd_socket, (void *) &c_addr, &c_len);
 
    Frame frame = receiveFrame(fd_client);

    char buffer2[100];
    sprintf(buffer2, "%d %d %s %s", frame.type, frame.header_length, frame.header, frame.data);
    printx(buffer2);

    free(frame.header);
    free(frame.data);

    /////////
    
    terminateExecution();

    return 0;
}