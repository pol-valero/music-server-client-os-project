#include "../globals.h"
#include "discoveryConfig.h"

int fd_config;

DiscoveryConfig discovery_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

void* listenForPooleConnections(/*void* arg*/) {
    //Thread 1
    int fd_client;
    int fd_socket_poole;

    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket_poole = startServer(discovery_config.port_poole, discovery_config.ip_poole);

    fd_client = accept(fd_socket_poole, (void *) &c_addr, &c_len);

    while (1) {

        Frame frame = receiveFrame(fd_client);

        char buffer2[100];
        sprintf(buffer2, "%d %d %s %s", frame.type, frame.header_length, frame.header, frame.data);
        printx(buffer2);

        if (frameIsValid(frame)) {
            sendFrame(0x01, "CON_OK", "", fd_client);
        } else {
            sendFrame(0x01, "CON_KO", "", fd_client);
        }

        free(frame.header);
        free(frame.data);
    }

}

void* listenForBowmanConnections(/*void* arg*/) {
    //Thread2
    int fd_client;
    int fd_socket_bowman;

    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket_bowman = startServer(discovery_config.port_bowman, discovery_config.ip_bowman);

    fd_client = accept(fd_socket_bowman, (void *) &c_addr, &c_len);

    while (1) {

        Frame frame = receiveFrame(fd_client);

        char buffer2[100];
        sprintf(buffer2, "%d %d %s %s", frame.type, frame.header_length, frame.header, frame.data);
        printx(buffer2);

        if (frameIsValid(frame)) {
            sendFrame(0x01, "CON_OK", "", fd_client);
        } else {
            sendFrame(0x01, "CON_KO", "", fd_client);
        }

        free(frame.header);
        free(frame.data);
    }

}

void listenForConnections() {

    pthread_t thread_poole;
    pthread_t thread_bowman;

    pthread_create(&thread_poole, NULL, (void *) listenForPooleConnections, NULL);
    pthread_create(&thread_bowman, NULL, (void *) listenForBowmanConnections, NULL);

    pthread_join(thread_poole, NULL);
    pthread_join(thread_bowman, NULL);

    //TODO: Close file descriptors inside threads 
    
}

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

    listenForConnections();
    
    terminateExecution();
    
    return 0;
}