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

ClientsSockets Clients;

ServerConfig server_config;  //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

int fd_config;
int fd_socket;

char* buffer;

int doDiscoveryHandshake() {
    int fd_socket = startServerConnection(server_config.ip_discovery, server_config.port_discovery);

    if (fd_socket < 0) {
        printEr("ERROR: Cannot connect to the discovery server\n");
        return 0;
    }

    asprintf(&buffer, "%s&%s&%d", server_config.name, server_config.ip_poole, server_config.port_poole);

    sendFrame(0x01, "NEW_POOLE", buffer, fd_socket);

    free(buffer);

    Frame responseFrame = receiveFrame(fd_socket);

    if(!strcmp(responseFrame.header, RESPONSE_OK)){
        printx("Connected to HAL 9000 System, ready to listen to Bowmans petitions\n");
        printx("\nWaiting for connections...\n\n");

        return 1;
    }else if(!strcmp(responseFrame.header, RESPONSE_KO)){
        printEr("Discovery refused the connection\n");
    }else if (!strcmp(responseFrame.header, UNKNOWN)){
        printEr("Error: last packet sent was lost\n");
    }else{
        printEr("Error: Error receiving package\n");
        sendFrame(0x02, UNKNOWN, "", fd_socket);
    }

    return 0;
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

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}


char** getFilesName(int *numFiles, char* pathToOpen){
    DIR* dir = opendir(pathToOpen);

    if (dir == NULL) {
        return NULL;
    }

    struct dirent *input;
    char** filesList = NULL;
    *numFiles = 0;

    while ((input = readdir(dir)) != NULL) {
        if (strcmp(input->d_name, ".") != 0 && strcmp(input->d_name, "..") != 0) {
            filesList = realloc(filesList,sizeof(char*) * (*numFiles + 1));
            asprintf(&filesList[*numFiles], "%s", input->d_name);
            (*numFiles)++;
        }
    }

    closedir(dir);

    return filesList;
   
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

char** getAllPlaylists(int* n_playlists) {
    char** playlists;
    playlists = getFilesName(n_playlists, PATH);
    return playlists;
}

char** getAllSongs(int* n_songs) {

    int n_playlists;
    char** playlists = getAllPlaylists(&n_playlists);

    int n_songs_playlist = 0;
    char** songs_playlist;

    char** songs = NULL;
    int n_total_songs = 0;

    char* playlistName;
    char* pathToPlaylist;

    for (int i = 0; i < n_playlists; i++) {
        
        playlistName = playlists[i];
        asprintf(&pathToPlaylist, "%s/%s", PATH, playlistName);
        songs_playlist = getFilesName(&n_songs_playlist, pathToPlaylist);
        
        for (int i = 0; i < n_songs_playlist; i++){
            songs = realloc(songs, sizeof(char*) * (n_total_songs + 1));
            songs[n_total_songs] = songs_playlist[i];
            n_total_songs++;
        }
    }

    *n_songs = n_total_songs;
    return songs;
}

//TODO: Remove this function, it is just for testing
void printListString() {
    int numFiles;
    char** files;

    //files = getAllPlaylists(&numFiles);
    files = getAllSongs(&numFiles);
    if (files != NULL){
        for (int i = 0; i < numFiles; i++){
            write(STDOUT_FILENO, files[i], strlen(files[i]));
            free(files[i]);
        }
        free(files);
    }else {
        printx("Error en la lectura de archivos o carpeta vacia\n");
    }
}

void sendAllSongs(char** songs, int n_songs, int fd_client) {

    char* header;

    asprintf(&header, "SONGS_RESPONSE&%d", n_songs);

    char* newFrameIncomingMsg = "NEW_FRAME_INCOMING";
    char* newFrameNotIncomingMsg = "NEW_FRAME_NOT_INCOMING";


    for (int i = 0; i < n_songs; i++){

        if (i == 0) {

            asprintf(&buffer, "%s", songs[i]);

        } else {

            asprintf(&buffer, "%s&%s", buffer, songs[i]);
            if (i + 1 < n_songs && ( strlen(buffer) + strlen(songs[i + 1]) ) >= (256 -3 -(strlen(header) + 1) -strlen(newFrameIncomingMsg)) ) {
                //The current frame is full, we need to attach a "newFrameIncomingMsg" at the end of the frame, send it, and create a new frame that will
                asprintf(&buffer, "%s&%s", buffer, newFrameIncomingMsg);
                sendFrame(0x02, header, buffer, fd_client);
                free(buffer);


                asprintf(&buffer, "%s", songs[i + 1]);
                i++;
            }

        }
    }
    
    asprintf(&buffer, "%s&%s", buffer, newFrameNotIncomingMsg);
    sendFrame(0x02, header, buffer, fd_client);
    free(buffer);

}

void sendAllPlaylists(/*char** playlists, int n_playlists, int fd_client*/) {
    //Rewrite all 
    
    /*char* buffer;

    char* header;

    asprintf(&header, "PLAYLISTS_RESPONSE&%d", n_playlists);

    char* newFrameIncomingMsg = "NEW_FRAME_INCOMING";

    for (int i = 0; i < n_playlists; i++){

        if (i == 0) {

            asprintf(&buffer, "%s", playlists[i]);

        } else {

            asprintf(&buffer, "%s&%s", buffer, playlists[i]);
            if (i + 1 < n_playlists && ( strlen(buffer) + strlen(playlists[i + 1]) ) >= (256 -3 -(strlen(header) + 1) -strlen(newFrameIncomingMsg)) ) {
                //The current frame is full, we need to attach a "newFrameIncomingMsg" at the end of the frame, send it, and create a new frame that will
                asprintf(&buffer, "%s&%s", buffer, newFrameIncomingMsg);
                sendFrame(0x02, header, buffer, fd_client);
                free(buffer);
            }
        }
    }*/
}


void* runServer(void* arg){
    ClientInfo clientInfo = *((ClientInfo*)arg);
    Frame receive;
    do{
        receive = receiveFrame(clientInfo.fd_client);
        if(!strcmp(receive.header, LIST_SONGS)){
            asprintf(&buffer, "New request - %s requires the list of songs.\n", clientInfo.name);
            printQue(buffer);
            free(buffer);
            
            //TODO: ParseList()
            int n_songs = 0;
            char** songs = getAllSongs(&n_songs);
            sendAllSongs(songs, n_songs, clientInfo.fd_client);

            asprintf(&buffer, "Sending song list to %s\n\n", clientInfo.name);
            printRes(buffer);
            free(buffer);
        } else if(!strcmp(receive.header, LIST_PLAYLISTS)){
            asprintf(&buffer, "New request - %s requires the list of playlists.\n", clientInfo.name);
            printQue(buffer);
            free(buffer);
            
            //TODO: ParseList()

            asprintf(&buffer, "Sending playlist list to %s\n\n", clientInfo.name);
            printRes(buffer);
            free(buffer);
            sendFrame(0x02, PLAYLISTS_RESPONSE, "", clientInfo.fd_client);
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

void configureClient(int fd_temp, Frame receive){
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

    Frame receive;
    receive = receiveFrame(fd_temp);

    if (!strcmp(receive.header, BOWMAN_TO_POOLE)){
        configureClient(fd_temp, receive);
    }else{
        sendFrame(0x07, UNKNOWN, "", fd_temp);
        close(fd_temp);
        printEr("Error: Problemas al acceptar un cliente.");
    }
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

    if (fd_socket != -1 && result){
        while (1){
            addClient();
        }
    }
    
    terminateExecution();

    return 0;
}