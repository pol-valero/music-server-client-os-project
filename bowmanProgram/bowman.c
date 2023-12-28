#include "../globals.h"
#include "bowmanConfig.h"
#include "bowmanCmdProcessing.h"

/**
 * 
 * Structs
 * 
 */
typedef struct {
    char* name;
    char* ip;
    int port;
} PooleInfo;

typedef struct {
    char* name;
    char** songs;
    int numSongs;
} PlayList;

typedef struct {
    PlayList* playList;
    int numPlayList;
} PlayLists;

typedef struct {
    char** songs;
    int numSongs;
}Songs;

typedef struct {
    int id;
    long lenght;
    long actualLenght;
    char* md5sum;
    char* path;
    char* name;
} DownloadSong;
typedef struct {
    DownloadSong* downloadSong;
    int numDownloadSong;
} DownloadList;
/**
 * 
 * Global variables
 * 
 */
DownloadList downloadList = { NULL, 0 };
PlayLists playLists = { NULL, 0 };

Songs songs = { NULL, 0};

PooleInfo poole_info = { NULL, NULL, 0 };

ClientConfig client_config = { NULL, NULL, NULL, 0};

Frame receive = {0, 0, NULL, NULL};

fd_set read_fds;

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
 * Print functions
 * 
 */

void printConnectionInitMsg() {
    asprintf(&buffer, "%s connected to HAL 9000 system, welcome music lover!\n", client_config.name);
    printx(buffer);
    cleanPointer(buffer);
}

/**
 * 
 * Functions configuration file
 * 
 */

void cleanPooleInfo() {
    if (poole_info.name != NULL) {
        cleanPointer(poole_info.name);
    }
    if (poole_info.ip != NULL) {
        cleanPointer(poole_info.ip);
    }
}

PooleInfo frameToPooleInfo () {

    int endCharPos;
    
    poole_info.name = readStringUntilChar(0, receive.data, '&', &endCharPos);
    poole_info.ip = readStringUntilChar(endCharPos + 1, receive.data, '&', &endCharPos);

    buffer = readStringUntilChar(endCharPos + 1, receive.data, ' ', &endCharPos);
    poole_info.port = atoi(buffer); //Does not matter the endChar we put here

    cleanPointer (buffer);
    return poole_info;
}

int connectToPoole () {
    int fd_socket = startServerConnection(client_config.ip_discovery, client_config.port_discovery);
    sendFrame(0x01, BOWMAN_TO_DISCOVERY, client_config.name, fd_socket);
    
    
    cleanFrame(&receive);
    receive = receiveFrame(fd_socket);

    if (frameIsValid(receive)) {

        if (strcmp(receive.header, RESPONSE_OK) == 0) {
            cleanSockets (fd_socket);
            fd_socket = -1;

            poole_info = frameToPooleInfo(receive);

            fd_socket = startServerConnection(poole_info.ip, poole_info.port);
            sendFrame(0x01, BOWMAN_TO_POOLE, client_config.name, fd_socket);

            cleanFrame(&receive);
            receive = receiveFrame(fd_socket);

            if (frameIsValid(receive)) {

                if (strcmp(receive.header, RESPONSE_OK) == 0) {
                    printConnectionInitMsg();
                    return fd_socket;
                } else {
                    printEr("ERROR: Connection to poole server failed, CON_KO returned\n");
                }

            } else {
                //TODO: Maybe do a while loop to keep trying to receive a valid frame
                sendFrame(0x07, UNKNOWN, "", fd_socket); 
            }
        } else {
            printEr("ERROR: Connection to discovery server failed, CON_KO returned\n");
        }

    } else {
        sendFrame(0x07, UNKNOWN, "", fd_socket);
        //TODO: Maybe do a while loop to keep trying to receive a valid frame
    }
    return fd_socket;
}

/**
 * 
 * Functions for songs
 * 
 */

void cleanSongs() {
    if(songs.songs == 0) {
        return;
    }
    for (int i = 0; i < songs.numSongs; i++) {
        cleanPointer(songs.songs[i]);
    }
    cleanPointer(songs.songs);
    songs.songs = NULL;
    songs.numSongs = 0;
}

void updateSongList(const char *SongName) {
    songs.numSongs++;

    songs.songs = realloc(songs.songs,sizeof(char*) * songs.numSongs);

    songs.songs[songs.numSongs - 1] = strdup(SongName);
}

void parseReceivedSongs(){
    int result = -1;

    char* token;
    token = strtok(receive.data, "&");
    result = -1;
    do{
        if(!strcmp(token,"0") || !strcmp (token,"1")){
            result = atoi(token);
        }else{
            updateSongList(token);
        }
        
        token = strtok(NULL, "&");
    }while(result != 0 && result != 1);

    if (!result){
        asprintf(&buffer, "There are %d songs available for download:\n", songs.numSongs);
        printRes(buffer);
        cleanPointer(buffer);

        for (int i = 0; i < songs.numSongs; i++){
            asprintf(&buffer, "%d. %s\n",i + 1 ,songs.songs[i]);
            printRes(buffer);
            cleanPointer(buffer);
        }
        printx("\n$ ");
    }
}

/**
 * 
 * Functions for playlists
 * 
 */

void cleanPlayLists() {
    if(playLists.playList == 0) {
        return;
    }
    for (int i = 0; i < playLists.numPlayList; i++) {
        cleanPointer(playLists.playList[i].name);
        for (int j = 0; j < playLists.playList[i].numSongs; j++) {
            cleanPointer(playLists.playList[i].songs[j]);
        }
        cleanPointer(playLists.playList[i].songs);
    }
    cleanPointer(playLists.playList);
    playLists.playList = NULL;
    playLists.numPlayList = 0;
}

void updatePlaylist(const char *playlistName) {
    playLists.numPlayList++;
    playLists.playList = realloc(playLists.playList, sizeof(PlayList) * playLists.numPlayList);

    playLists.playList[playLists.numPlayList - 1].name = strdup(playlistName);
    playLists.playList[playLists.numPlayList - 1].songs = NULL;
    playLists.playList[playLists.numPlayList - 1].numSongs = 0;
}

void updateSongOfPlayList(const char *SongName) {
    playLists.playList[playLists.numPlayList - 1].numSongs++;

    playLists.playList[playLists.numPlayList - 1].songs = realloc(
        playLists.playList[playLists.numPlayList - 1].songs,
        sizeof(char*) * playLists.playList[playLists.numPlayList - 1].numSongs
    );

    playLists.playList[playLists.numPlayList - 1].songs[playLists.playList[playLists.numPlayList - 1].numSongs - 1] = strdup(SongName);
}

void parseReceivedPlayList(){
    char* token;
    int status = 2;
    
    
    token = strtok(receive.data, "&");

    while (strcmp(token,"0") && strcmp (token,"1") && strcmp (token,"2")) {
        if (strchr(token, '#')){
            char *saveptr;
            char *songToken = strtok_r(token, "#", &saveptr);
            updateSongOfPlayList(songToken);
            char *playlistToken = strtok_r(NULL, "#", &saveptr);
            if (playlistToken != NULL) {
                updatePlaylist(playlistToken);
            }
        }else if(status == 2){
            updatePlaylist(token);
            status = -1;
        }else{
            updateSongOfPlayList(token);
        }

        token = strtok(NULL, "&");
        if (!strcmp(token,"0") || !strcmp (token,"1") || !strcmp (token,"2")){
            status = atoi(token);
        }
    }

    if (!status){
        asprintf(&buffer, "There are %d lists available for download:\n",playLists.numPlayList);
        printRes(buffer);
        cleanPointer(buffer);

        for (int i = 0; i < playLists.numPlayList ; i++){
            asprintf(&buffer, "%d. %s\n",i + 1 ,playLists.playList[i].name);
            printRes(buffer);
            cleanPointer(buffer);
            for (int j = 0; j < playLists.playList[i].numSongs; j++){
                asprintf(&buffer, "\t%c. %s\n",j + 'a' ,playLists.playList[i].songs[j]);
                printRes(buffer);
                cleanPointer(buffer);
            }
        }
        printx("\n$ ");
    }
    
}

/**
 * 
 * Functions for disconect
 * 
 */

void disconnectFromPoole () {
    sendFrame(0x06, EXIT, client_config.name, fd_socket);
    cleanSockets (fd_socket);
}

/**
 * 
 * Functions for download Song
 * 
 */

char* checkSongMD5SUM (char* path){
    char *openssl_command[] = {"md5sum", path, NULL};

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
            
            char *token = strtok(buffer, " ");
            
            close(pipefd[0]);

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

void saveDataSong(DownloadSong* downloadSong, char* data) {
    int fd = open(downloadSong->path, O_WRONLY | O_CREAT | O_APPEND, 0666);
    
    if (fd > 0){
        write(fd, data, strlen(data));
        downloadSong->actualLenght += strlen(data);
        close(fd);
        if (downloadSong->actualLenght == downloadSong->lenght){
            char* md5sum = checkSongMD5SUM(downloadSong->path);
            if (md5sum != NULL){
                if (strcmp(md5sum, downloadSong->md5sum)){
                    printEr("ERROR: Song downloaded with errors\n");
                }
                cleanPointer(md5sum);
            }else{
                printEr("ERROR: Song downloaded with errors\n");
            }
        }
    }else{
        printEr("ERROR: File already exists\n");
    }
}


/**
 * 
 * Functions for command mode
 * 
 */

// Function to manage user-input commands.
void enterCommandMode() {
    char* command;
    char* SecondCommandWord = NULL;
    int command_case_num = -1;
    int *delete = NULL;
    int numDelete = 0;
    
    FD_ZERO(&read_fds);
    FD_SET(STDIN_FILENO, &read_fds);

    printx("\n$ ");
    do {
        fd_set temp = read_fds;
        if (select(1024, &temp, NULL, NULL, NULL) == -1) {
            perror("Error en select");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(STDIN_FILENO, &temp)) {
            command = readUntilChar(STDIN_FILENO, '\n');
            command_case_num = commandToCmdCaseNum(command, &SecondCommandWord);
            cleanPointer(command);
            
            switch (command_case_num) {
                case CONNECT_CMD:
                    fd_socket = connectToPoole();
                    FD_SET(fd_socket, &read_fds);
                    printx("\n$ ");
                    break;
                case LOGOUT_CMD:
                    disconnectFromPoole();
                    printx("\n$ ");
                    break;
                case LIST_SONGS_CMD:
                    sendFrame(0x02, LIST_SONGS, "", fd_socket);
                    cleanSongs();
                    break;
                case LIST_PLAYLISTS_CMD:
                    sendFrame(0x02, LIST_PLAYLISTS, "", fd_socket);
                    cleanPlayLists();
                    break;
                case DOWNLOAD_SONG_CMD:
                    sendFrame(0x02, DOWNLOAD_SONG, SecondCommandWord, fd_socket);
                    cleanPointer(SecondCommandWord);
                    printx("\n$ ");
                    break;
                case DOWNLOAD_PLAYLIST_CMD:
                    printRes("Comanda OK\n");
                    break;
                case CHECK_DOWNLOADS_CMD:

                    for(int i = 0; i < downloadList.numDownloadSong; i++){
                        float percentage = (float)downloadList.downloadSong[i].actualLenght / (float)downloadList.downloadSong[i].lenght;
                        
                        asprintf(&buffer, "%ld/%ld\n ", downloadList.downloadSong[i].actualLenght, downloadList.downloadSong[i].lenght);
                        printRes(buffer);
                        cleanPointer(buffer);

                        printRes(downloadList.downloadSong[i].name);
                        asprintf(&buffer, "\t%.2f%% |", percentage * 100);
                        printRes(buffer);
                        cleanPointer(buffer);
                        for(int j = 0; j < 20; j++){
                            if (j < percentage * 20){
                                printRes("=");
                            }else{
                                printRes(" ");
                            }
                        }
                        printRes("|\n");
                    }
                    printx("\n$ ");
                    break;
                case CLEAR_DOWNLOADS_CMD:
                    delete = NULL;
                    numDelete = 0;
                    for(int i = 0; i < downloadList.numDownloadSong; i++){
                        if (downloadList.downloadSong[i].actualLenght == downloadList.downloadSong[i].lenght){
                            cleanPointer(downloadList.downloadSong[i].path);
                            cleanPointer(downloadList.downloadSong[i].name);
                            cleanPointer(downloadList.downloadSong[i].md5sum);
                            downloadList.downloadSong[i].actualLenght = 0;
                            downloadList.downloadSong[i].id = 0;
                            downloadList.downloadSong[i].lenght = 0;
                            delete = realloc(delete, sizeof(int) * (numDelete + 1));
                            delete[numDelete] = i;
                            numDelete++;
                        }
                    }
                    for(int i = 0; i < numDelete; i++){
                        for(int j = delete[i]; j < downloadList.numDownloadSong - 1; j++){
                            downloadList.downloadSong[j] = downloadList.downloadSong[j + 1];
                        }
                        downloadList.numDownloadSong--;
                        downloadList.downloadSong = realloc(downloadList.downloadSong, sizeof(DownloadSong) * downloadList.numDownloadSong);
                    }
                    printx("\n$ ");
                    break;
                case PARTIALLY_CORRECT_CMD:
                    printEr("Comanda KO\n");
                    //Unknown command
                    break;
                case INVALID_CMD:
                    printEr("Comanda KO\n");
                    printx("\n$ ");
                    //Not valid command
                    break;
                case NO_CMD:
                    printEr("ERROR: No command entered\n");
                    //No command entered
                    break;
                default:
                    break;
            }
        }else if (FD_ISSET(fd_socket, &temp)) {
            cleanFrame(&receive);
            receive = receiveFrame(fd_socket);
            if(!strcmp(receive.header, RESPONSE_OK)){
                if(receive.type == 0x06){
                    //Response of disconnection from poole
                }
            }else if(!strcmp(receive.header, RESPONSE_KO)){
                if(receive.type == 0x06){
                    //Response of disconnection from poole
                }
            }else if(!strcmp(receive.header, SONGS_RESPONSE)){
                if (receive.type == 0x02){
                    //Response of song list.
                    parseReceivedSongs();
                }
            }else if(!strcmp(receive.header, PLAYLISTS_RESPONSE)){
                if (receive.type == 0x02){
                    //Response of playlist list.
                    parseReceivedPlayList();
                }
            }else if (!strcmp(receive.header, NEW_FILE)){
                if (receive.type == 0x04){
                    //Response of petition for download a song.
                    DownloadSong downloadSong;
                    downloadSong.name = strdup(strtok(receive.data, "&"));
                    downloadSong.lenght = atol(strtok(NULL, "&"));
                    downloadSong.md5sum = strdup(strtok(NULL, "&"));
                    downloadSong.id = atoi(strtok(NULL, "&"));
                    asprintf(&downloadSong.path, "bowmanProgram/data/Songs/%s", downloadSong.name);
                    downloadSong.actualLenght = 0;

                    downloadList.numDownloadSong++;
                    downloadList.downloadSong = realloc(downloadList.downloadSong, sizeof(DownloadSong) * downloadList.numDownloadSong);
                    downloadList.downloadSong[downloadList.numDownloadSong - 1] = downloadSong;

                    if (access(downloadSong.path, F_OK) != -1) {
                        if (remove(downloadSong.path) != 0) {
                            perror("Error al eliminar el archivo");
                        }
                    }
                }
            }else if (!strcmp(receive.header, FILE_DATA)){
                if (receive.type == 0x04){
                    //Data from download song.
                    int id = atoi(strtok(receive.data, "&"));
                    char *data = strdup(strtok(NULL, "&"));
                    if (data != NULL){
                        for (int i = 0; i < downloadList.numDownloadSong; i++){
                            if (downloadList.downloadSong[i].id == id){
                                saveDataSong(&downloadList.downloadSong[i], data);
                                cleanPointer(data);
                                break;
                            }
                        }
                    }else{
                        printEr("Error: yep we know the error :C\n");
                    }
                }
            }else if (!strcmp(receive.header, UNKNOWN)){
                if (receive.type == 0x07){
                    //Error sending package
                    printEr("Error: last packet sent was lost\n");
                }
            }else{
                printEr("Error: Error receiving package\n");
                sendFrame(0x02, UNKNOWN, "", fd_socket);
            }
        }
    } while (command_case_num != LOGOUT_CMD);
}

/**
 * 
 * Functions for unexpected termination
 * 
 */

// Handle unexpected termination scenarios.
void terminateExecution () {
    if (fd_config != -1){
        close (fd_config);
    }

    if (fd_socket != -1) {
        disconnectFromPoole();
    }

    if (buffer != NULL){
        cleanPointer(buffer);
    }

    cleanClientConfig(&client_config);
    cleanFrame(&receive);
    cleanPooleInfo();
    cleanPlayLists();
    cleanSongs();

    FD_ZERO(&read_fds);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

//main function :P
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
    } else {
        client_config = readConfigFile(fd_config);
    }

    asprintf(&buffer, "\n%s user initialized\n", client_config.name);
    printx(buffer);
    cleanPointer(buffer);

    enterCommandMode();
    
    terminateExecution();

    return 0;

}