#include "../globals.h"
#include "pooleConfig.h"

int fd_config;
int fd_client;

ServerConfig server_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

int fd_socket;


// Handle unexpected termination scenarios.
void terminateExecution () {

    free(server_config.name);
    free(server_config.files_folder);
    free(server_config.ip_discovery);
    free(server_config.ip_poole);

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

    server_config = readConfigFile(fd_config);

    printConfigFile(server_config);

    //TODO: REMOVE THESE LINES, THEY ARE JUST FOR TESTING
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket = startServer(server_config.port_poole, server_config.ip_poole);
    fd_client = accept(fd_socket, (void *) &c_addr, &c_len);

    //TODO: Do a while loop that continuosuly listents for new connections and handles them depending on the port?
    //We can distinguish ports if we look at c_addr.sin_port
    //Maybe we can do it without a thread? Just a while loop that listens for new connections and handles them (the handling is really fast)
    /////////
    
    terminateExecution();

    return 0;
}