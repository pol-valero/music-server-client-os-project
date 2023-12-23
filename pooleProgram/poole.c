#include "../globals.h"
#include "pooleConfig.h"

#include "sys/select.h"
#include <dirent.h>
#include "../semaphore_2v.h"

/**
 * 
 * Structs
 * 
 */
typedef struct {
    pthread_t threadClient;
    pthread_t* threadPetitions;
    semaphore sender;
    int num_petitions;
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
typedef struct {
    ClientInfo* ClientInfo;
    char* song;
} DownloadSong;

/**
 * 
 * Global variables
 * 
 */
semaphore clearClients;

ClientsSockets clients = { NULL, 0 };

ServerConfig server_config = { NULL, NULL, NULL, 0, NULL, 0 };

PlayLists playLists = { NULL, 0 };  

Frame receive = { 0, 0, NULL, NULL };

char* buffer = NULL;

/**
 * 
 * Sockets file descriptors
 * 
 */

int fd_config = -1;

int fd_socket = -1;

/**
 * 
 * Functions for load data of songs
 * 
 */

void addPlaylist(const char *playlistName) {
    playLists.numPlayList++;
    playLists.playList = realloc(playLists.playList, sizeof(PlayList) * playLists.numPlayList);

    playLists.playList[playLists.numPlayList - 1].name = strdup(playlistName);
    asprintf(&playLists.playList[playLists.numPlayList - 1].path, "%s/%s", PATH, playlistName);
    playLists.playList[playLists.numPlayList - 1].songs = NULL;
    playLists.playList[playLists.numPlayList - 1].numSongs = 0;
}

void addSongToCurrentPlaylist(const char *SongName) {
    playLists.playList[playLists.numPlayList - 1].numSongs++;

    playLists.playList[playLists.numPlayList - 1].songs = realloc(
        playLists.playList[playLists.numPlayList - 1].songs,
        sizeof(char*) * playLists.playList[playLists.numPlayList - 1].numSongs
    );

    playLists.playList[playLists.numPlayList - 1].songs[playLists.playList[playLists.numPlayList - 1].numSongs - 1] = strdup(SongName);
}

int loadPlaylists(const int fd_dir) {
    DIR *dirp;
    struct dirent *entry;
    int fd;

    if (fchdir(fd_dir) < 0) {
        printEr("Error: Cannot change directory\n");
        return -1;
    }

    close(fd_dir);

    if ((dirp = opendir(".")) == NULL) {
        printEr("Error: Cannot open directory\n");
        return -2;
    }

    while ((entry = readdir(dirp)) != NULL) {
        switch (entry->d_type) {
            case DT_REG:
                if (strcmp(entry->d_name, ".DS_Store") != 0) {  //We ignore .DS_Store files
                    addSongToCurrentPlaylist(entry->d_name);
                }
            break;
            case DT_DIR:
                if ((fd = open(entry->d_name, O_RDONLY)) < 0) {
                    perror(entry->d_name);
                } else {
                    if ((strcmp(entry->d_name, ".") != 0) && (strcmp(entry->d_name, "..") != 0)) {
                        addPlaylist(entry->d_name);
                        
                        loadPlaylists(fd);
                        chdir("..");
                    }
                }
            break;
        }
    }
    closedir(dirp);

    return 0;
}

void freePlayList(PlayList* playlist) {
    cleanPointer(playlist->name);
    cleanPointer(playlist->path);

    for (int i = 0; i < playlist->numSongs; i++) {
        cleanPointer(playlist->songs[i]);
    }

    cleanPointer(playlist->songs);
}

void cleanPlayLists() {
    //Free the memory allocated for the playLists
    if (playLists.numPlayList > 0){
        for (int i = 0; i < playLists.numPlayList; i++) {
            freePlayList(&(playLists.playList[i]));
        }
        cleanPointer(playLists.playList);
    }
} 

/**
 * 
 * Functions for disconect client
 * 
 */

void disconect(void* arg){
    ClientInfo* clientInfo = (ClientInfo*)arg;
    //TODO: manage phtreads, semaphores and sockets
    SEM_wait(&clearClients);
    for (int i = 0; i < clients.numClients; i++){
        if (clients.clientInfo[i].fd_client == clientInfo->fd_client){
            cleanPointer(clients.clientInfo[i].name);
            for(int j = 0; j < clients.clientInfo[i].num_petitions; j++){
                pthread_join(clients.clientInfo[i].threadPetitions[j], NULL);
            }
            free(clients.clientInfo[i].threadPetitions);
            SEM_destructor(&clients.clientInfo[i].sender);

            //This is for reallocate al the list of clients.
            for (int j = i; j < clients.numClients - 1; j++) {
                clients.clientInfo[j].fd_client = clients.clientInfo[j + 1].fd_client;
            }
            (clients.numClients)--;
            //clients.clientInfo = realloc(clients.clientInfo, sizeof(ClientInfo) * (clients.numClients));  //TODO: solve this problem
            break;
        }
    }
    SEM_signal(&clearClients);
        
    sendFrame(0x06, RESPONSE_OK, "", clientInfo->fd_client);
    cleanSockets(clientInfo->fd_client);
}

/**
 * 
 * Functions for send all songs.
 * 
 */

void* sendAllSongs(void* arg) {
    ClientInfo* clientInfo = (ClientInfo*)arg;

    if (playLists.numPlayList == 0 || playLists.playList[0].numSongs == 0){
        SEM_wait(&clientInfo->sender);
        sendFrame(0x02, SONGS_RESPONSE, "NO_HAY_CANCIONES&0", clientInfo->fd_client);
        SEM_signal(&clientInfo->sender);
        return NULL;
    }
    int first = 1;

    char *tempBuffer;
    asprintf(&buffer, "test"); //TODO: memory leak
    for (int i = 0; i < playLists.numPlayList; i++){
        for (int j = 0; j < playLists.playList[i].numSongs; j++){
            if (strlen(buffer) + strlen(playLists.playList[i].songs[j]) + 1 < 256 - 6 - strlen(SONGS_RESPONSE)) {
                if (first){
                    cleanPointer(buffer);
                    asprintf(&buffer, "%s", playLists.playList[i].songs[j]);
                    first = 0;
                }else{
                    asprintf(&tempBuffer, "%s&%s", buffer, playLists.playList[i].songs[j]);
                    cleanPointer(buffer);
                    buffer = tempBuffer;
                }
            }else{
                asprintf(&tempBuffer, "%s&1", buffer);
                cleanPointer(buffer); 
                buffer = tempBuffer;
                SEM_wait(&clientInfo->sender);
                sendFrame(0x02, SONGS_RESPONSE, buffer, clientInfo->fd_client);
                SEM_signal(&clientInfo->sender);
                cleanPointer(buffer);

                asprintf(&buffer, "%s", playLists.playList[i].songs[j]);
            }
        }
    }
    asprintf(&tempBuffer, "%s&0", buffer);
    cleanPointer(buffer); 
    buffer = tempBuffer;
    SEM_wait(&clientInfo->sender);
    sendFrame(0x02, SONGS_RESPONSE, buffer, clientInfo->fd_client);
    SEM_signal(&clientInfo->sender);
    cleanPointer(buffer);

    return NULL;
}

/**
 * 
 * Functions for send all playlists.
 * 
 */

void* sendAllPlaylists(void* arg) {
    ClientInfo* clientInfo = (ClientInfo*)arg;
    if (playLists.numPlayList == 0 || playLists.playList[0].numSongs == 0){
        SEM_wait(&clientInfo->sender);
        sendFrame(0x02, SONGS_RESPONSE, "NO_HAY_CANCIONES&0", clientInfo->fd_client);
        SEM_signal(&clientInfo->sender);
        return NULL;
    }
    int first = 1;
    asprintf(&buffer, "%s", playLists.playList[0].name);

    char* tempBuffer;
    for (int i = 0; i < playLists.numPlayList; i++){
        if (!first){
            if (strlen(buffer) + strlen(playLists.playList[i].name) + 1 < 256 - 6 - strlen(SONGS_RESPONSE)) {
                asprintf(&tempBuffer, "%s#%s", buffer, playLists.playList[i].name);
                cleanPointer(buffer);
                buffer = tempBuffer;
            }else{
                asprintf(&tempBuffer, "%s&2", buffer);
                cleanPointer(buffer);
                buffer = tempBuffer;
                SEM_wait(&clientInfo->sender);
                sendFrame(0x02, SONGS_RESPONSE, buffer, clientInfo->fd_client);
                SEM_signal(&clientInfo->sender);
                cleanPointer(buffer);
                
                asprintf(&buffer, "%s", playLists.playList[i].name);
            }
        }else{
            first = 0;
        }
        for (int j = 0; j < playLists.playList[i].numSongs; j++){
            if (strlen(buffer) + strlen(playLists.playList[i].songs[j]) + 1 < 256 - 6 - strlen(SONGS_RESPONSE)) {
                asprintf(&tempBuffer, "%s&%s", buffer, playLists.playList[i].songs[j]);
                cleanPointer(buffer);
                buffer = tempBuffer;
            }else{
                asprintf(&tempBuffer, "%s&1", buffer);
                cleanPointer(buffer);
                buffer = tempBuffer;
                SEM_wait(&clientInfo->sender);
                sendFrame(0x02, SONGS_RESPONSE, buffer, clientInfo->fd_client);
                SEM_signal(&clientInfo->sender);
                cleanPointer(buffer);

                asprintf(&buffer, "%s", playLists.playList[i].songs[j]);
            }
        }
        
    }
    
    asprintf(&tempBuffer, "%s&0", buffer);
    cleanPointer(buffer);
    buffer = tempBuffer;
    SEM_wait(&clientInfo->sender);
    sendFrame(0x02, SONGS_RESPONSE, buffer, clientInfo->fd_client);
    SEM_signal(&clientInfo->sender);
    cleanPointer(buffer);

    return NULL;
}

/**
 * 
 * Functions for download the song 
 * 
 */

void* downloadSong(void* arg);

/**
 * 
 * Functions for download a list of songs 
 * 
 */
void* downloadListSongs(void* arg);

/**
 * 
 * Functions manage the client petitions
 * 
 */

void* runServer(void* arg){
    ClientInfo* clientInfo = (ClientInfo*)arg;
    pthread_cleanup_push(disconect, clientInfo);
    do{
        cleanFrame(&receive);
        receive = receiveFrame(clientInfo->fd_client);
        if(!strcmp(receive.header, LIST_SONGS)){
            asprintf(&buffer, "New request - %s requires the list of songs.\n", clientInfo->name);
            printQue(buffer);
            cleanPointer(buffer);

            clientInfo->num_petitions++;
            clientInfo->threadPetitions = realloc(clientInfo->threadPetitions, sizeof(pthread_t) * clientInfo->num_petitions);
            pthread_create(&clientInfo->threadPetitions[clientInfo->num_petitions - 1], NULL, sendAllSongs, clientInfo);

            asprintf(&buffer, "Sending song list to %s\n\n", clientInfo->name);
            printRes(buffer);
            cleanPointer(buffer);
        }else if(!strcmp(receive.header, LIST_PLAYLISTS)){
            asprintf(&buffer, "New request - %s requires the list of playlists.\n", clientInfo->name);
            printQue(buffer);
            cleanPointer(buffer);
            
            clientInfo->num_petitions++;
            clientInfo->threadPetitions = realloc(clientInfo->threadPetitions, sizeof(pthread_t) * clientInfo->num_petitions);
            pthread_create(&clientInfo->threadPetitions[clientInfo->num_petitions - 1], NULL, sendAllPlaylists, clientInfo);

            asprintf(&buffer, "Sending playlist list to %s\n\n", clientInfo->name);
            printRes(buffer);
            cleanPointer(buffer);
        }else if(!strcmp(receive.header, DOWNLOAD_SONG)){
            asprintf(&buffer, "New request – %s wants to download %s\n", clientInfo->name, receive.data);
            printQue(buffer);
            cleanPointer(buffer);

            asprintf(&buffer,"Sending %s to %s\n\n", receive.data,clientInfo->name);
            printRes(buffer);
            cleanPointer(buffer);
        }else if(!strcmp(receive.header, DOWNLOAD_LIST)){
            asprintf(&buffer, "New request – %s wants to download the playlist %s.\n", clientInfo->name, receive.data);
            printQue(buffer);
            cleanPointer(buffer);

            asprintf(&buffer,"Sending %s to %s.\n\n", receive.data,clientInfo->name);
            printRes(buffer);
            cleanPointer(buffer);
        }else if (!strcmp(receive.header, UNKNOWN)){
            printEr("Error: last packet sent was lost\n");
        }else if (!strcmp(receive.header, EXIT)){
            asprintf(&buffer, "New request - %s requires disconnection\n", receive.data);
            printQue(buffer);
            cleanPointer(buffer);
        }
        else{
            printEr("Error: Error receiving package\n");
            sendFrame(0x02, UNKNOWN, "", clientInfo->fd_client);
        }
    } while (strcmp(receive.header, EXIT));

    

    pthread_cleanup_pop(1);

    asprintf(&buffer, "User -%s- disconnected\n\n", receive.data);
    printRes(buffer);
    cleanPointer(buffer);

    return NULL;
}

/**
 * 
 * Functions for configure the client
 * 
 */

void configureClient(int fd_temp){
    SEM_wait(&clearClients);
    int index = clients.numClients;
    
    //TODO: manage the threads from the client who had disconected already
    clients.clientInfo = realloc(clients.clientInfo, sizeof(ClientInfo) * (index + 1));
    
    
    (clients.numClients)++;

    clients.clientInfo[index].fd_client = fd_temp;
    clients.clientInfo[index].name = strdup(receive.data);

    clients.clientInfo[index].num_petitions = 0;
    clients.clientInfo[index].threadPetitions = NULL;

    SEM_constructor_with_name(&clients.clientInfo[index].sender, ftok(receive.data, clients.numClients));
    SEM_init(&clients.clientInfo[index].sender, 1);

    pthread_create(&clients.clientInfo[index].threadClient, NULL, runServer, &clients.clientInfo[index]);
    
    SEM_signal(&clearClients);

    SEM_wait(&clients.clientInfo[index].sender);
    sendFrame(0x01, RESPONSE_OK, "", fd_temp);
    SEM_signal(&clients.clientInfo[index].sender);
    
    asprintf(&buffer, "New user connected: %s\n", clients.clientInfo[index].name);
    printx(buffer);
    cleanPointer(buffer);
}

void addClient(){
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    int fd_temp = accept(fd_socket, (void *) &c_addr, &c_len);
    if (fd_temp == -1){
        printEr("ERROR: Problemas al acceptar client\n");
        return;
    }
    cleanFrame(&receive);
    receive = receiveFrame(fd_temp);

    if (!strcmp(receive.header, BOWMAN_TO_POOLE)){
        configureClient(fd_temp);
    }else{
        sendFrame(0x07, UNKNOWN, "", fd_temp);
        cleanSockets(fd_temp);
        printEr("Error: Problemas al acceptar un cliente.\n");
    }
}

/**
 * 
 * Functions for connect to the discovery server
 * 
 */

int doDiscoveryHandshake() {
    int fd_socket = startServerConnection(server_config.ip_discovery, server_config.port_discovery);

    if (fd_socket < 0) {
        printEr("ERROR: Cannot connect to the discovery server\n");
        return 0;
    }

    asprintf(&buffer, "%s&%s&%d", server_config.name, server_config.ip_poole, server_config.port_poole);

    sendFrame(0x01, POOLE_TO_DISCOVERY, buffer, fd_socket);

    cleanPointer(buffer);

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

void cleanClientInfo(){
    if (clients.numClients > 0){
        for (int i = 0; i < clients.numClients; i++){
            cleanPointer(clients.clientInfo[i].name);
            cleanSockets(clients.clientInfo[i].fd_client);
            pthread_cancel(clients.clientInfo[i].threadClient);
            pthread_join(clients.clientInfo[i].threadClient,NULL);
        }
        cleanPointer(clients.clientInfo);
    }
}

// Handle unexpected termination scenarios.
void terminateExecution () {
    //Free the memory allocated for the server_config
    cleanServerConfig(&server_config);
    cleanFrame(&receive);
    cleanClientInfo();
    cleanPlayLists();

    SEM_destructor(&clearClients);

    if(fd_socket != -1){
        cleanSockets(fd_socket);
    }
    if(fd_config != -1){
        cleanSockets(fd_config);
    }

    pthread_exit(NULL);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

//main function :p
int main (int argc, char** argv) {

    signal(SIGINT, terminateExecution);
    signal(SIGTERM, terminateExecution);

    //This is for the semaphore which delete a client from the list of client
    SEM_constructor_with_name(&clearClients, ftok("clearSender", 7));
    SEM_init(&clearClients, 1);

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

    loadPlaylists(current_dir);

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