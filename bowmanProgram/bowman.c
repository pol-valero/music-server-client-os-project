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

/**
 * 
 * Global variables
 * 
 */
PlayLists playLists = { NULL, 0 };

Songs songs = { NULL, 0};

PooleInfo poole_info = { NULL, NULL, 0 };

ClientConfig client_config = { NULL, NULL, NULL, 0};

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

    return -1;
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
    do{
        cleanFrame(&receive);
        receive = receiveFrame(fd_socket);

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
    }while (result != 0);
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

void parseReceivedPlayList(int fd){
    char* token;
    int status = 2;
    
    while (status != 0){
        cleanFrame(&receive);
        receive = receiveFrame(fd);
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
 * Functions for command mode
 * 
 */

// Function to manage user-input commands.
void enterCommandMode() {
    char* command;
    int command_case_num;

    //int resultSongs = -1;
    //int resultPlaylists = -1;
    do {

        printx("\n$ ");
        command = readUntilChar(STDIN_FILENO, '\n');
        command_case_num = commandToCmdCaseNum(command);
        cleanPointer(command);
        
        switch (command_case_num) {
            case CONNECT_CMD:
                fd_socket = connectToPoole();
                break;
            case LOGOUT_CMD:
                disconnectFromPoole();
                break;
            case LIST_SONGS_CMD:
                sendFrame(0x02, LIST_SONGS, "", fd_socket);
                
                cleanSongs();
                parseReceivedSongs();
                
                asprintf(&buffer, "There are %d songs available for download:\n", songs.numSongs);
                printRes(buffer);
                cleanPointer(buffer);

                for (int i = 0; i < songs.numSongs; i++){
                    asprintf(&buffer, "%d. %s\n",i + 1 ,songs.songs[i]);
                    printRes(buffer);
                    cleanPointer(buffer);
                }

                break;
            case LIST_PLAYLISTS_CMD:
                sendFrame(0x02, LIST_PLAYLISTS, "", fd_socket);

                cleanPlayLists();
                parseReceivedPlayList(fd_socket);
                
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
                
                break;
            case DOWNLOAD_SONG_CMD:
                printRes("Comanda OK\n");
                break;
            case DOWNLOAD_PLAYLIST_CMD:
                printRes("Comanda OK\n");
                break;
            case CHECK_DOWNLOADS_CMD:
                printRes("Comanda OK\n");
                break;
            case CLEAR_DOWNLOADS_CMD:
                printRes("Comanda OK\n");
                break;
            case PARTIALLY_CORRECT_CMD:
                printEr("Comanda KO\n");
                //Unknown command
                break;
            case INVALID_CMD:
                printEr("Comanda KO\n");
                //Not valid command
                break;
            case NO_CMD:
                printEr("ERROR: No command entered\n");
                //No command entered
                break;
            default:
                break;
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