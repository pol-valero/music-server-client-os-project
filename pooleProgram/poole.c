#include "../globals.h"
#include "pooleConfig.h"

#include "sys/select.h"
#include <dirent.h>
#include "../semaphore_2v.h"

#define MAX_NAME_LENGTH 50
#define FILENAME "stats.txt"

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
    int id;
    long lenght;
    char* songName;
    char* md5sum;
    char* path;
} DownloadSong;



/**
 * 
 * Global variables
 * 
 */

ServerConfig server_config = { NULL, NULL, NULL, 0, NULL, 0 };

PlayLists playLists = { NULL, 0 };  

int close_monolit = 0;

int pipe_fd[2];
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

void updatePlaylist(const char *playlistName) {
    playLists.numPlayList++;
    playLists.playList = realloc(playLists.playList, sizeof(PlayList) * playLists.numPlayList);

    playLists.playList[playLists.numPlayList - 1].name = strdup(playlistName);
    asprintf(&playLists.playList[playLists.numPlayList - 1].path, "%s", playlistName); //TODO: Write this better
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
    cleanPointer(clientInfo->name);
    cleanPointer(clientInfo->threadPetitions);
    close(clientInfo->fd_client);
    free(clientInfo);
        
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
    char* buffer;
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
    char* buffer;
    ClientInfo* clientInfo = (ClientInfo*)arg;
    if (playLists.numPlayList == 0 || playLists.playList[0].numSongs == 0){
        SEM_wait(&clientInfo->sender);
        sendFrame(0x02, PLAYLISTS_RESPONSE, "NO_HAY_CANCIONES&0", clientInfo->fd_client);
        SEM_signal(&clientInfo->sender);
        return NULL;
    }
    int first = 1;
    asprintf(&buffer, "%s", playLists.playList[0].name);

    char* tempBuffer;
    for (int i = 0; i < playLists.numPlayList; i++){
        if (!first){
            if (strlen(buffer) + strlen(playLists.playList[i].name) + 1 < 256 - 6 - strlen(PLAYLISTS_RESPONSE)) {
                asprintf(&tempBuffer, "%s#%s", buffer, playLists.playList[i].name);
                cleanPointer(buffer);
                buffer = tempBuffer;
            }else{
                asprintf(&tempBuffer, "%s&2", buffer);
                cleanPointer(buffer);
                buffer = tempBuffer;
                SEM_wait(&clientInfo->sender);
                sendFrame(0x02, PLAYLISTS_RESPONSE, buffer, clientInfo->fd_client);
                SEM_signal(&clientInfo->sender);
                cleanPointer(buffer);
                
                asprintf(&buffer, "%s", playLists.playList[i].name);
            }
        }else{
            first = 0;
        }
        for (int j = 0; j < playLists.playList[i].numSongs; j++){
            if (strlen(buffer) + strlen(playLists.playList[i].songs[j]) + 1 < 256 - 6 - strlen(PLAYLISTS_RESPONSE)) {
                asprintf(&tempBuffer, "%s&%s", buffer, playLists.playList[i].songs[j]);
                cleanPointer(buffer);
                buffer = tempBuffer;
            }else{
                asprintf(&tempBuffer, "%s&1", buffer);
                cleanPointer(buffer);
                buffer = tempBuffer;
                SEM_wait(&clientInfo->sender);
                sendFrame(0x02, PLAYLISTS_RESPONSE, buffer, clientInfo->fd_client);
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
    sendFrame(0x02, PLAYLISTS_RESPONSE, buffer, clientInfo->fd_client);
    SEM_signal(&clientInfo->sender);
    cleanPointer(buffer);

    return NULL;
}

/**
 * 
 * Functions for download the song 
 * 
 */

char* checkSongMD5SUM (char* path){
    char *openssl_command[] = {"md5sum", path, NULL};
    char* buffer;
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("pipe");
        return NULL;
    }

    pid_t pid = fork();

    if (pid == -1) {
        perror("fork");
         return NULL;
    }

    if (pid == 0) {  // Proceso hijo
        close(pipefd[0]);
        dup2(pipefd[1], STDOUT_FILENO); 

        execvp("md5sum", openssl_command);

        perror("execvp");
        return NULL;
    } else {  // Proceso padre
        close(pipefd[1]);

        int status;
        waitpid(pid, &status, 0); 

        if (WIFEXITED(status)) {
            // Read the output of the md5sum command from the pipe
            buffer = malloc (sizeof(char) * 1024);
            
            ssize_t bytesRead;
            
            bytesRead = read(pipefd[0], buffer, 1024);
            buffer[bytesRead] = '\0';
            
            char *token = strdup(strtok(buffer, " "));
            
            close(pipefd[0]);
            cleanPointer(buffer);
            return token;
        } else {
            printEr("El proceso hijo terminó con error.\n");
            close(pipefd[0]);
        }

        close(pipefd[0]);
    }

    cleanPointer(path);

    return NULL;
}

long getLenghtArchive(const char *path) {
    struct stat statArchivo;
    if (stat(path, &statArchivo) == -1) {
        perror("Error al obtener información del archivo");
        return -1;
    }

    return statArchivo.st_size;
}

int getID() {
    return rand() % 1000;
}

char* getSongPath (char* name){
    char* buffer;
    for (int i = 0; i < playLists.numPlayList; i++)    {
        for (int j = 0; j < playLists.playList[i].numSongs; j++){
            if (!strcmp(name, playLists.playList[i].songs[j])){
                asprintf(&buffer, "%s/%s", playLists.playList[i].path, name);
                return buffer;
            }
        }
    }
    return NULL;
}

void clearDownloadSong(DownloadSong* downloadSong){
    if (downloadSong->songName != NULL){
        cleanPointer(downloadSong->songName);
    }
    if (downloadSong->md5sum != NULL){
        cleanPointer(downloadSong->md5sum);
    }
    if (downloadSong->path != NULL){
        cleanPointer(downloadSong->path);
    }
}

void* sendSong(void* arg){
    DownloadSong* downloadSong = (DownloadSong*)arg;

    int fd_song = open(downloadSong->path, O_RDONLY);

    if (fd_song == -1){
        printEr("Error: Cannot open the song\n");
        return NULL;
    }

    int lengthFrame = 256 - 3 /* Type and header length */- strlen(FILE_DATA) - 1 /*\0*/ - snprintf(NULL, 0, "%d", downloadSong->id) - 1 /*&*/;
    char tempBuffer[lengthFrame];
    int bytesRead;

    char* buffer;
    
    while((bytesRead = read (fd_song, tempBuffer, lengthFrame)) > 0){
        int bufferSize = snprintf(NULL, 0, "%d", downloadSong->id) + 1 + bytesRead;
        buffer = malloc(sizeof(char) * bufferSize);
        sprintf(buffer, "%d", downloadSong->id);
        buffer[snprintf(NULL, 0, "%d", downloadSong->id)] = '&';
        memcpy(buffer + snprintf(NULL, 0, "%d", downloadSong->id) + 1, tempBuffer, bytesRead);
        usleep(10000);
        SEM_wait(&downloadSong->ClientInfo->sender);
        sendFrameSong(0x04, FILE_DATA, buffer, downloadSong->ClientInfo->fd_client, bufferSize);
        SEM_signal(&downloadSong->ClientInfo->sender);
        cleanPointer(buffer);
    }

    close(fd_song);
    cleanPointer(downloadSong->songName);
    cleanPointer(downloadSong->md5sum);
    cleanPointer(downloadSong->path);
    
    return NULL;
}

void processDownloadSong(char* name, ClientInfo* clientInfo){
    DownloadSong* downloadSong;
    char* buffer;
    downloadSong = malloc(sizeof(DownloadSong));
    
    downloadSong->songName = strdup(name);

    if (strchr(name, '/') != NULL){
        strtok(name, "/");
        char* temp_name = strtok(NULL, "/");
        downloadSong->path = strdup(getSongPath(temp_name));
        write(pipe_fd[1], temp_name, strlen(temp_name));
        write(pipe_fd[1], "&", 1);
    }else{
        downloadSong->path = strdup(getSongPath(name));
        write(pipe_fd[1], name, strlen(name));
        write(pipe_fd[1], "&", 1);  
    }
    
    if(downloadSong->path == NULL){
        printEr("Error: The song doesn't exist\n");
        SEM_wait(&clientInfo->sender);
        sendFrame(0x04, RESPONSE_KO, "", clientInfo->fd_client);
        SEM_signal(&clientInfo->sender);
        return;
    }
    downloadSong->md5sum = checkSongMD5SUM(downloadSong->path);
    if(downloadSong->md5sum == NULL){
        printEr("Error: The md5sum of the song is NULL\n");
        SEM_wait(&clientInfo->sender);
        sendFrame(0x04, RESPONSE_KO, "", clientInfo->fd_client);
        SEM_signal(&clientInfo->sender);
        return;
    }
    downloadSong->lenght = getLenghtArchive(downloadSong->path);
    if(downloadSong->lenght == -1){
        printEr("Error: The lenght of the song is -1\n");
        SEM_wait(&clientInfo->sender);
        sendFrame(0x04, RESPONSE_KO, "", clientInfo->fd_client);
        SEM_signal(&clientInfo->sender);
        return;
    }
    downloadSong->id = getID();
    if(downloadSong->id == -1){
        printEr("Error: The id of the song is -1\n");
        SEM_wait(&clientInfo->sender);
        sendFrame(0x04, RESPONSE_KO, "", clientInfo->fd_client);
        SEM_signal(&clientInfo->sender);
        return;
    }
    
    downloadSong->ClientInfo = clientInfo;    
    
    asprintf(&buffer, "%s&%ld&%s&%d", downloadSong->songName, downloadSong->lenght, downloadSong->md5sum, downloadSong->id);
    SEM_wait(&clientInfo->sender);
    sendFrame(0x04, NEW_FILE, buffer, clientInfo->fd_client);
    SEM_signal(&clientInfo->sender);

    cleanPointer(buffer);
    SEM_wait(&downloadSong->ClientInfo->sender);
    SEM_signal(&downloadSong->ClientInfo->sender);
    clientInfo->num_petitions++;
    clientInfo->threadPetitions = realloc(clientInfo->threadPetitions, sizeof(pthread_t) * clientInfo->num_petitions);
    pthread_create(&clientInfo->threadPetitions[clientInfo->num_petitions - 1], NULL, sendSong, downloadSong);
}

/**
 * 
 * Functions for download a list of songs 
 * 
 */
void downloadListSongs(char* name, ClientInfo* clientInfo){
    char* buffer;
    for (int i = 0; i < playLists.numPlayList; i++){
        if (strcmp(name, playLists.playList[i].name) == 0){
            for (int j = 0; j < playLists.playList[i].numSongs; j++){
                asprintf(&buffer, "%s/%s", name,playLists.playList[i].songs[j]);
                processDownloadSong(buffer, clientInfo);
            }
            return;
        }
    }
    SEM_wait(&clientInfo->sender);
    sendFrame(0x04, RESPONSE_KO, "", clientInfo->fd_client);
    SEM_signal(&clientInfo->sender);
}

//
void disconnectPooleServerFromDiscovery() {
    int fd_socket = startServerConnection(server_config.ip_discovery, server_config.port_discovery);

    if (fd_socket < 0) {
        printEr("ERROR: Cannot connect to the discovery server\n");
    }

    char* buffer;
    asprintf(&buffer, "%d", server_config.port_poole);

    sendFrame(0x08, "DISCONNECT", buffer, fd_socket);

    free(buffer);

    close(fd_socket);

}
//

void decreasePooleConnectionNum() {

    int fd_socket = startServerConnection(server_config.ip_discovery, server_config.port_discovery);

    if (fd_socket < 0) {
        printEr("ERROR: Cannot connect to the discovery server\n");
    }

    char* buffer;
    asprintf(&buffer, "%d", server_config.port_poole);

    sendFrame(0x06, EXIT, buffer, fd_socket);

    free(buffer);

}

/**
 * 
 * Functions manage the client petitions
 * 
 */

void* runServer(void* arg){
    ClientInfo* clientInfo = (ClientInfo*)arg;
    char* buffer;
    Frame receive = {0, 0, NULL, NULL};
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

            processDownloadSong(receive.data, clientInfo);

            asprintf(&buffer,"Sending %s to %s\n\n", receive.data,clientInfo->name);
            printRes(buffer);
            cleanPointer(buffer);
        }else if(!strcmp(receive.header, DOWNLOAD_LIST)){
            asprintf(&buffer, "New request – %s wants to download the playlist %s.\n", clientInfo->name, receive.data);
            printQue(buffer);
            cleanPointer(buffer);

            downloadListSongs(receive.data, clientInfo);

            asprintf(&buffer,"Sending %s to %s.\n\n", receive.data,clientInfo->name);
            printRes(buffer);
            cleanPointer(buffer);
        }else if (!strcmp(receive.header, UNKNOWN)){
            printEr("Error: last packet sent was lost\n");
        }else if (!strcmp(receive.header, EXIT)){
            asprintf(&buffer, "New request - %s requires disconnection\n", receive.data);
            printQue(buffer);
            cleanPointer(buffer);
        }else{
            printEr("Error: Error receiving package\n");
            SEM_wait(&clientInfo->sender);
            sendFrame(0x02, UNKNOWN, "", clientInfo->fd_client);
            SEM_signal(&clientInfo->sender);
        }
    } while (strcmp(receive.header, EXIT));

    decreasePooleConnectionNum();

    return NULL;
}

/**
 * 
 * Functions for configure the client
 * 
 */

void configureClient(int fd_temp, Frame receive){
    
    char* buffer;
    //TODO: manage the threads from the client who had disconected already
    ClientInfo* clientInfo;
    clientInfo = malloc(sizeof(ClientInfo));

    clientInfo->fd_client = fd_temp;
    clientInfo->name = strdup(receive.data);

    clientInfo->num_petitions = 0;
    clientInfo->threadPetitions = NULL;

    SEM_constructor_with_name(&clientInfo->sender, ftok(receive.data, rand() % 1000));
    SEM_init(&clientInfo->sender, 1);

    pthread_create(&clientInfo->threadClient, NULL, runServer, clientInfo);
    
    SEM_wait(&clientInfo->sender);
    sendFrame(0x01, RESPONSE_OK, "", fd_temp);
    SEM_signal(&clientInfo->sender);
    
    asprintf(&buffer, "New user connected: %s\n", clientInfo->name);
    printx(buffer);
    cleanPointer(buffer);
}

void addClient(){
    struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);
    Frame receive = {0, 0, NULL, NULL };

    int fd_temp = accept(fd_socket, (void *) &c_addr, &c_len);
    if (fd_temp == -1){
        printEr("ERROR: Problemas al acceptar client\n");
        return;
    }
    cleanFrame(&receive);
    receive = receiveFrame(fd_temp);

    if (!strcmp(receive.header, BOWMAN_TO_POOLE)){
        configureClient(fd_temp, receive);
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
    Frame receive = {0, 0, NULL, NULL };
    int fd_socket = startServerConnection(server_config.ip_discovery, server_config.port_discovery);
    char* buffer;
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

//function that open a txt and search a name and update the number of petitions,if doesn't exist the name, add it. the file could be empty
void updateStats(const char *name){
    int fd_stats = open(FILENAME, O_RDWR | O_CREAT, 0666);
    if (fd_stats == -1){
        printEr("Error: Cannot open the file\n");
        return;
    }
    char* buffer = NULL;
    while ((buffer = readUntilChar(fd_stats, '\n')) != NULL && strlen(buffer) > 0){
        char* temp_name = strtok(buffer, "&");
        if (!strcmp(temp_name, name)){
            char* temp_petitions = strtok(NULL, "&");
            int petitions = atoi(temp_petitions);
            petitions++;
            lseek(fd_stats, -strlen(temp_petitions) - 1, SEEK_CUR);
            char* buffer;
            asprintf(&buffer, "%d", petitions);
            write(fd_stats, buffer, strlen(buffer));
            cleanPointer(buffer);
            return;
        }
    }
    lseek(fd_stats, 0, SEEK_END);
    asprintf(&buffer, "%s&1\n", name);
    write(fd_stats, buffer, strlen(buffer));
    cleanPointer(buffer);
    close(fd_stats);
}

void createMonolit (){
    pid_t pid;

    // Crear el pipe
    if (pipe(pipe_fd) == -1) {
        perror("Error al crear el pipe");
        exit(EXIT_FAILURE);
    }

    // Crear un nuevo proceso
    pid = fork();

    if (pid == -1) {
        perror("Error al crear el proceso");
        exit(EXIT_FAILURE);
    }

    if (pid == 0) { 
        semaphore sem;

        // Crear o abrir el semáforo
        int semResult = SEM_constructor_with_name(&sem, ftok("poole", 'S'));
        if (semResult < 0) {
            perror("Error al abrir el semáforo");
            exit(EXIT_FAILURE);
        }
        while (close_monolit == 0){
            char* buffer = readUntilChar(pipe_fd[0], '&');
            updateStats(buffer);
            cleanPointer(buffer);
        }
    }
}

// Handle unexpected termination scenarios.
void terminateExecution () {

    disconnectPooleServerFromDiscovery();

    close(pipe_fd[0]); 
    close(pipe_fd[1]); 

    close_monolit = 1;

    if(fd_socket != -1){
        cleanSockets(fd_socket);
    }
    if(fd_config != -1){
        cleanSockets(fd_config);
    }
    
    //Free the memory allocated for the server_config
    cleanServerConfig(&server_config);
    cleanPlayLists();

    
    printx("Poole terminated\n");
    
    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

//main function :p
int main (int argc, char** argv) {
    signal(SIGINT, terminateExecution);
    signal(SIGTERM, terminateExecution);

    srand((unsigned int)time(NULL));

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

    createMonolit();

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