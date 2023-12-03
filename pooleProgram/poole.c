#include "../globals.h"
#include "pooleConfig.h"

#include "sys/select.h"
#include <dirent.h>

#define PATH "pooleProgram/data"

typedef struct {
    pthread_t threadClient;
    int fd_client;
    char* name;
} ClientInfo;

typedef struct {
    ClientInfo* clientInfo;
    int numClients;
} ClientsSockets;

typedef struct {
    char* name;
    char* path;
    char** songs;
    int numSongs;
} PlayList;

typedef struct {
    PlayList* playList;
    int numPlayList;
} PlayLists;

ClientsSockets Clients;

ServerConfig server_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

PlayLists playLists = { NULL, 0 };  

Frame receive = { 0, 0, NULL, NULL };

int fd_config;
int fd_socket;

char* buffer;

void updatePlaylist(const char *playlistName) {
    playLists.numPlayList++;
    playLists.playList = realloc(playLists.playList, sizeof(PlayList) * playLists.numPlayList);

    playLists.playList[playLists.numPlayList - 1].name = strdup(playlistName);
    asprintf(&playLists.playList[playLists.numPlayList - 1].path, "%s/%s", PATH, playlistName);
    playLists.playList[playLists.numPlayList - 1].songs = NULL;
    playLists.playList[playLists.numPlayList - 1].numSongs = 0;
}

void updateSong(const char *SongName) {
    playLists.playList[playLists.numPlayList - 1].numSongs++;

    playLists.playList[playLists.numPlayList - 1].songs = realloc(
        playLists.playList[playLists.numPlayList - 1].songs,
        sizeof(char*) * playLists.playList[playLists.numPlayList - 1].numSongs
    );

    playLists.playList[playLists.numPlayList - 1].songs[playLists.playList[playLists.numPlayList - 1].numSongs - 1] = strdup(SongName);
}

int loadSongs(const int fd_dir) {
    DIR *dirp;
    struct dirent *entry;
    int fd;

    if (fchdir(fd_dir) < 0) {
        perror("fchdir");
        return -1;
    }
    close(fd_dir);

    if ((dirp = opendir(".")) == NULL) {
        perror("opendir");
        return -2;
    }

    while ((entry = readdir(dirp)) != NULL) {
        switch (entry->d_type) {
            case DT_REG:
                updateSong(entry->d_name);
            break;
            case DT_DIR:
                if ((fd = open(entry->d_name, O_RDONLY)) < 0) {
                    perror(entry->d_name);
                } else {
                    if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
                        updatePlaylist(entry->d_name);
                        
                        loadSongs(fd);
                        chdir("..");
                    }
                }
            break;
        }
    }
    closedir(dirp);

    return 0;
}

//TODO: Remove this function
void printTest(){
    for (int i = 0; i < playLists.numPlayList ; i++){
        for (int j = 0; j < playLists.playList[i].numSongs; j++){
            printx(playLists.playList[i].songs[j]);
        }
    }
}

void freePlayList(PlayList* playlist) {
    free(playlist->name);
    free(playlist->path);

    for (int i = 0; i < playlist->numSongs; i++) {
        free(playlist->songs[i]);
    }

    free(playlist->songs);
}

// Handle unexpected termination scenarios.
void terminateExecution () {

    free(server_config.name);
    free(server_config.files_folder);
    free(server_config.ip_discovery);
    free(server_config.ip_poole);

    if (Clients.numClients > 0){
        for (int i = 0; i < Clients.numClients; i++){
            free(Clients.clientInfo[i].name);
            close(Clients.clientInfo[i].fd_client);
        }
        free(Clients.clientInfo);
    }

    close (fd_config);

    if (playLists.numPlayList > 0){
        for (int i = 0; i < playLists.numPlayList; i++) {
            freePlayList(&(playLists.playList[i]));
        }
        free(playLists.playList);
    }
    
    cleanFrame(&receive);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

void disconect(int fd_client){
    if (Clients.numClients > 0){
        for (int i = 0; i < Clients.numClients; i++){
            if (Clients.clientInfo[i].fd_client == fd_client){
                free(Clients.clientInfo[i].name);
                for (int j = i; j < Clients.numClients - 1; j++) {
                    Clients.clientInfo[j].fd_client = Clients.clientInfo[j + 1].fd_client;
                }
                (Clients.numClients)--;
                Clients.clientInfo = realloc(Clients.clientInfo, sizeof(ClientInfo) * (Clients.numClients));
                break;
            }
        }
    }else {
        (Clients.numClients)--;
        free(Clients.clientInfo);
    }
    
    sendFrame(0x06, "CONOK", "", fd_client);
    close(fd_client);
}

void sendAllSongs(int fd_client) {
    if (playLists.numPlayList == 0 || playLists.playList[0].numSongs == 0){
        sendFrame(0x02, SONGS_RESPONSE, "NO_HAY_CANCIONES&0", fd_client);
        return;
    }
    asprintf(&buffer, "%s", playLists.playList[0].songs[0]);

    for (int i = 0; i < playLists.numPlayList ; i++){
        for (int j = 0; j < playLists.playList[i].numSongs; j++){
            if (strlen(buffer) + strlen(playLists.playList[i].songs[j]) + 1 < 256 - 6 - strlen(SONGS_RESPONSE)) {
                asprintf(&buffer, "%s&%s", buffer, playLists.playList[i].songs[j]);
            }else{
                //The current frame is full, we need to attach a "newFrameIncomingMsg" at the end of the frame, send it, and create a new frame that will

                asprintf(&buffer, "%s&1", buffer);
                sendFrame(0x02, SONGS_RESPONSE, buffer, fd_client);
                free(buffer);

                asprintf(&buffer, "%s", playLists.playList[i].songs[j]);
            }
        }
    }
    
    asprintf(&buffer, "%s&0", buffer);
    sendFrame(0x02, SONGS_RESPONSE, buffer, fd_client);
    free(buffer);
}

void sendAllPlaylists(int fd_client) {
    if (playLists.numPlayList == 0 || playLists.playList[0].numSongs == 0){
        sendFrame(0x02, SONGS_RESPONSE, "NO_HAY_CANCIONES&0", fd_client);
        return;
    }
    asprintf(&buffer, "%s", playLists.playList[0].name);

    for (int i = 1; i < playLists.numPlayList ; i++){
        for (int j = 0; j < playLists.playList[i].numSongs; j++){
            if (strlen(buffer) + strlen(playLists.playList[i].songs[j]) + 1 < 256 - 6 - strlen(SONGS_RESPONSE)) {
                asprintf(&buffer, "%s&%s", buffer, playLists.playList[i].songs[j]);
            }else{
                //The current frame is full, we need to attach a "newFrameIncomingMsg" at the end of the frame, send it, and create a new frame that will
                asprintf(&buffer, "%s&1", buffer);
                sendFrame(0x02, SONGS_RESPONSE, buffer, fd_client);
                free(buffer);

                asprintf(&buffer, "%s", playLists.playList[i].songs[j]);
            }
        }
        if (strlen(buffer) + strlen(playLists.playList[i].name) + 1 < 256 - 6 - strlen(SONGS_RESPONSE)) {
            asprintf(&buffer, "%s#%s", buffer, playLists.playList[i].name);
        }else{
            //The current frame is full, we need to attach a "newFrameIncomingMsg" at the end of the frame, send it, and create a new frame that will
            asprintf(&buffer, "%s&2", buffer);
            sendFrame(0x02, SONGS_RESPONSE, buffer, fd_client);
            free(buffer);
            
            asprintf(&buffer, "%s#%s", buffer, playLists.playList[i].name);
        }
    }
    
    asprintf(&buffer, "%s&0", buffer);
    sendFrame(0x02, SONGS_RESPONSE, buffer, fd_client);
    free(buffer);
}


void* runServer(void* arg){
    ClientInfo clientInfo = *((ClientInfo*)arg);
    do{
        cleanFrame(&receive);
        receive = receiveFrame(clientInfo.fd_client);
        if(!strcmp(receive.header, LIST_SONGS)){
            asprintf(&buffer, "New request - %s requires the list of songs.\n", clientInfo.name);
            printQue(buffer);
            free(buffer);
            
            sendAllSongs(clientInfo.fd_client);

            asprintf(&buffer, "Sending song list to %s\n\n", clientInfo.name);
            printRes(buffer);
            free(buffer);
        } else if(!strcmp(receive.header, LIST_PLAYLISTS)){
            asprintf(&buffer, "New request - %s requires the list of playlists.\n", clientInfo.name);
            printQue(buffer);
            free(buffer);
            
            sendAllPlaylists(clientInfo.fd_client);

            asprintf(&buffer, "Sending playlist list to %s\n\n", clientInfo.name);
            printRes(buffer);
            free(buffer);
        }else if (!strcmp(receive.header, EXIT)){
            asprintf(&buffer, "New request - %s requires disconnection\n", receive.data);
            printQue(buffer);
            free(buffer);

            disconect(clientInfo.fd_client);

            asprintf(&buffer, "User -%s- disconnected\n\n", receive.data);
            printRes(buffer);
            free(buffer);
        }else if (!strcmp(receive.header, UNKNOWN)){
            printEr("Error: last packet sent was lost\n");
        }else{
            printEr("Error: Error receiving package\n");
            sendFrame(0x02, UNKNOWN, "", clientInfo.fd_client);
        }
    } while (strcmp(receive.header, EXIT));

    return NULL;
}

void configureClient(int fd_temp){
    int index = Clients.numClients;
        
    if (index == 0){
        Clients.clientInfo = malloc(sizeof(ClientInfo));
    }else{
        Clients.clientInfo = realloc(Clients.clientInfo, sizeof(ClientInfo) * (index + 1));
    }
    
    (Clients.numClients)++;

    Clients.clientInfo[index].fd_client = fd_temp;
    Clients.clientInfo[index].name = strdup(receive.data);

    pthread_create(&Clients.clientInfo[index].threadClient, NULL, runServer, &Clients.clientInfo[index]);

    sendFrame(0x01, RESPONSE_OK, "", fd_temp);
    
    asprintf(&buffer, "New user connected: %s\n", receive.data);
    printx(buffer);
    free(buffer);
}

void addClient(){
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    int fd_temp = accept(fd_socket, (void *) &c_addr, &c_len);
    if (fd_temp == -1){
        printEr("ERROR: Problemas al acceptar client");
        return;
    }
    cleanFrame(&receive);
    receive = receiveFrame(fd_temp);

    if (!strcmp(receive.header, BOWMAN_TO_POOLE)){
        configureClient(fd_temp);
    }else{
        sendFrame(0x07, UNKNOWN, "", fd_temp);
        close(fd_temp);
        printEr("Error: Problemas al acceptar un cliente.");
    }
}


int doDiscoveryHandshake() {
    int fd_socket = startServerConnection(server_config.ip_discovery, server_config.port_discovery);

    if (fd_socket < 0) {
        printEr("ERROR: Cannot connect to the discovery server\n");
        return 0;
    }

    asprintf(&buffer, "%s&%s&%d", server_config.name, server_config.ip_poole, server_config.port_poole);

    sendFrame(0x01, "NEW_POOLE", buffer, fd_socket);

    free(buffer);

    cleanFrame(&receive);
    receive = receiveFrame(fd_socket);

    if(!strcmp(receive.header, RESPONSE_OK)){
        return 1;
    }else if(!strcmp(receive.header, RESPONSE_KO)){
        printEr("Discovery refused the connection\n");
    }else if (!strcmp(receive.header, UNKNOWN)){
        printEr("Error: last packet sent was lost\n");
    }else{
        printEr("Error: Error receiving package\n");
        sendFrame(0x02, UNKNOWN, "", fd_socket);
    }

    return 0;
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
    
    printx("Reading configuration file\n");
    server_config = readConfigFile(fd_config);

    printx("Connecting Smyslov Server to the system..\n");

    int result = doDiscoveryHandshake();
    fd_socket = startServer(server_config.port_poole, server_config.ip_poole);

    int current_dir;

    current_dir = open (PATH, O_RDONLY);
    if (current_dir == -1){
        printEr("Error: Cannot open the directory\n");
        return -1;
    }

    loadSongs(current_dir);

    if (fd_socket != -1 && result){
        printx("Connected to HAL 9000 System, ready to listen to Bowmans petitions\n");
        printx("\nWaiting for connections...\n\n");
        while (1){
            addClient();
        }
    }
    
    terminateExecution();

    return 0;
}