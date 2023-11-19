#include "../globals.h"
#include "discoveryConfig.h"

typedef struct {
    char* name;
    char* ip;
    int port;
    int n_connections;
    int available; //If the poole server disconnects, we will set this to 0
} PooleStats;

typedef struct {
    PooleStats* poole_stats;
    int n_poole_stats;
} PooleStatsList;

int fd_config;

DiscoveryConfig discovery_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

PooleStats createPooleStats (Frame frame) {

    PooleStats poole_stats;

    //TODO: TODO: Create new readStringUntilChar that returns the endCharPos + 1 (we will interpret as startingPosition)
    //We will pass the string by reference to be filled. 
    
    //Alternative: do tests with the current method
    int endCharPos;
    
    poole_stats.name = readStringUntilChar(0, frame.data, '&', &endCharPos);
    poole_stats.ip = readStringUntilChar(endCharPos + 1, frame.data, '&', &endCharPos);
    poole_stats.port = atoi(readStringUntilChar(endCharPos + 1, frame.data, ' ', &endCharPos)); //Does not matter the endChar we put here
    poole_stats.n_connections = 0;
    poole_stats.available = 1;

    return poole_stats;
}

//TODO: Remove?
void printPooleStats (PooleStats poole_stats) {

    char* buffer;
    int buffSize;

    buffSize = asprintf(&buffer, "\nPoole server:\n");
    printDynStr(buffer, buffSize);
    free(buffer);

    buffSize = asprintf(&buffer, "%s %s %d %d %d\n", poole_stats.name, poole_stats.ip, poole_stats.port, poole_stats.n_connections, poole_stats.available);
    printDynStr(buffer, buffSize);
    free(buffer);

}

//TODO: Remove?
void printPooleStatsList (PooleStatsList list) {

    char* buffer;
    int buffSize;

    buffSize = asprintf(&buffer, "\nPoole servers in list:\n");
    printDynStr(buffer, buffSize);
    free(buffer);

    for (int i = 0; i < list.n_poole_stats; i++) {
        buffSize = asprintf(&buffer, "\nPoole\n");
        printDynStr(buffer, buffSize);
        free(buffer);

        buffSize = asprintf(&buffer, "%s %s %d %d %d\n", list.poole_stats[i].name, list.poole_stats[i].ip, list.poole_stats[i].port, list.poole_stats[i].n_connections, list.poole_stats[i].available);
        printDynStr(buffer, buffSize);
        free(buffer);
    }

}

void addPooleStatsToList (PooleStatsList *list, PooleStats poole_stats) {

    int n_poole_stats = (*list).n_poole_stats;    
    
    (*list).poole_stats = realloc((*list).poole_stats, sizeof(PooleStats) * (n_poole_stats + 1));

    (*list).poole_stats[n_poole_stats] = poole_stats;

    (*list).n_poole_stats++;

}

void* listenForPooleConnections(void* arg) {
    //Thread

    //PooleStatsList list = *((PooleStatsList*) arg);
    PooleStatsList* list = (PooleStatsList*) arg;

    int fd_client;
    int fd_socket_poole;

    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket_poole = startServer(discovery_config.port_poole, discovery_config.ip_poole);


    while (1) {

        fd_client = accept(fd_socket_poole, (void *) &c_addr, &c_len);

        Frame frame = receiveFrame(fd_client);

        if (frameIsValid(frame)) {
            sendFrame(0x01, "CON_OK", "", fd_client);
             
            PooleStats poole_stats = createPooleStats(frame);

            //addPooleStatsToList(&list, poole_stats);
            addPooleStatsToList(list, poole_stats);

        } else {
            sendFrame(0x01, "CON_KO", "", fd_client);
        }

        free(frame.header);
        free(frame.data);
    }

}

void* listenForBowmanConnections(void* arg) {
    //Thread

    int fd_client;
    int fd_socket_bowman;

    //PooleStatsList list = *((PooleStatsList*) arg);
    PooleStatsList* list = (PooleStatsList*) arg;

    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket_bowman = startServer(discovery_config.port_bowman, discovery_config.ip_bowman);


    while (1) {

        fd_client = accept(fd_socket_bowman, (void *) &c_addr, &c_len);

        Frame frame = receiveFrame(fd_client);
        
        printPooleStatsList(*list); //TODO: Remove
        //TODO: If the frame is valid, we will send to Bowman the info of the Poole that has the least connections

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

    PooleStatsList list = {.n_poole_stats = 0, .poole_stats = NULL};

    pthread_t thread_poole;
    pthread_t thread_bowman;

    pthread_create(&thread_poole, NULL, (void *) listenForPooleConnections,(void*) &list);
    pthread_create(&thread_bowman, NULL, (void *) listenForBowmanConnections, (void*) &list);

    pthread_join(thread_poole, NULL);
    pthread_join(thread_bowman, NULL);

    //TODO: Close file descriptors inside threads (We will have to make them global?)
    //TODO: Free memory of list (We will have to make the list global?)
    
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