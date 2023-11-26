#include "../globals.h"
#include "bowmanConfig.h"
#include "bowmanCmdProcessing.h"

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

PlayLists playLists = { NULL, 0 };

PooleInfo poole_info;

ClientConfig client_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

int fd_config;

char* buffer;

void printInitMsg() {
    int buffSize;

    buffSize = asprintf(&buffer, "\n%s user initialized\n", client_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);
}

void printConnectionInitMsg() {
    int buffSize;

    buffSize = asprintf(&buffer, "\n%s connected to HAL 9000 system, welcome music lover!\n", client_config.name);
    printDynStr(buffer, buffSize);
    free(buffer);
}

PooleInfo frameToPooleInfo (Frame frame) {

    PooleInfo poole_info;

    int endCharPos;
    
    poole_info.name = readStringUntilChar(0, frame.data, '&', &endCharPos);
    poole_info.ip = readStringUntilChar(endCharPos + 1, frame.data, '&', &endCharPos);
    poole_info.port = atoi(readStringUntilChar(endCharPos + 1, frame.data, ' ', &endCharPos)); //Does not matter the endChar we put here

    return poole_info;
}

int connectToPoole () {
    int fd_socket = startServerConnection(client_config.ip_discovery, client_config.port_discovery);
    sendFrame(0x01, "NEW_BOWMAN", client_config.name, fd_socket);
    
    Frame responseFrame = receiveFrame(fd_socket);

    if (frameIsValid(responseFrame)) {

        if (strcmp(responseFrame.header, "CON_OK") == 0) {
            close (fd_socket);
            poole_info = frameToPooleInfo(responseFrame);

            fd_socket = startServerConnection(poole_info.ip, poole_info.port);
            sendFrame(0x01, "NEW_BOWMAN", client_config.name, fd_socket);

            responseFrame = receiveFrame(fd_socket);

            if (frameIsValid(responseFrame)) {

                if (strcmp(responseFrame.header, "CON_OK") == 0) {
                    printConnectionInitMsg();
                    return fd_socket;
                } else {
                    printEr("ERROR: Connection to poole server failed, CON_KO returned\n");
                }

            } else {
                //TODO: Maybe do a while loop to keep trying to receive a valid frame
                sendFrame(0x07, "UNKNOWN", "", fd_socket); 
            }
        } else {
            printEr("ERROR: Connection to discovery server failed, CON_KO returned\n");
        }

    } else {
        sendFrame(0x07, "UNKNOWN", "", fd_socket);
        //TODO: Maybe do a while loop to keep trying to receive a valid frame
    }

    return -1;

    //TODO: Remove below lines
    char buffer2[100];
    sprintf(buffer2, "%d %d %s %s", responseFrame.type, responseFrame.header_length, responseFrame.header, responseFrame.data);
    printx(buffer2);
}

char** parseReceivedSongs(char* data, int* hasNextFrame, int* num_songs, char** songs){
    int endChar;
    char* token = readStringUntilChar(0, data, '&', &endChar);
    while (1) {
        if (strcmp(token,"1") == 0){
            *hasNextFrame = 1;
            break;
        }else if(strcmp(token,"0") == 0){
            *hasNextFrame = 0;
            break;
        }
        (*num_songs)++;
        songs = realloc(songs, sizeof(char*) * (*num_songs));
        songs[(*num_songs) - 1] = strdup(token);

        token = readStringUntilChar(endChar + 1, data, '&', &endChar);
    }
    return songs;
}
void updatePlaylist(const char *playlistName) {
    playLists.numPlayList++;
    playLists.playList = realloc(playLists.playList, sizeof(PlayList) * playLists.numPlayList);

    playLists.playList[playLists.numPlayList - 1].name = strdup(playlistName);
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

void parseReceivedPlayList(int fd){
    Frame frame;
    char* token;
    int first = 1;

    do{
        frame = receiveFrame(fd);
        printx(frame.data);
        printx("\n");
        token = strtok(frame.data, "&");

        while (strcmp(token,"0") || strcmp(token,"1")) {
            printx(token);
            printx("\n");
            if (token[0] == '#' || first){
                updatePlaylist(token);
                first = 0;
            }else{
                updateSong(token);
            }

            token = strtok(NULL, "&");
        }
    }while (strcmp(token,"0"));
}

// Function to manage user-input commands.
void enterCommandMode() {
    char* command;
    int command_case_num;
    //TODO: Change this
        int fd_socket; 
        Frame frame;

        int hasNextFrame = 1;
        int num_songs = 0;
        char** songs = NULL;
        /////////////////////////
    do {

        printx("$ ");
        command = readUntilChar(STDIN_FILENO, '\n');
        command_case_num = commandToCmdCaseNum(command);
        free(command);
        
        switch (command_case_num) {
            case CONNECT_CMD:
                //printx("Comanda OK\n");
                fd_socket = connectToPoole();
                break;
            case LOGOUT_CMD:
                printx("Comanda OK\n");
                sendFrame(0x06, EXIT, client_config.name, fd_socket);
                //TODO: disconnectFromPoole();
                break;
            case LIST_SONGS_CMD:
                sendFrame(0x02, LIST_SONGS, "", fd_socket);
                
                //TODO: CONTROL THE HEADER and free the variables
                while (hasNextFrame) {
                    frame = receiveFrame(fd_socket);
                    songs = parseReceivedSongs(frame.data, &hasNextFrame, &num_songs, songs);
                }
                //TODO: SOLVE THE PRINT
                for (int i = 0; i < num_songs; i++) {
                    printx(songs[i]);
                    printx("\n");
                }

                break;
            case LIST_PLAYLISTS_CMD:
                //TODO: Change this and free the variables
                
                sendFrame(0x02, LIST_PLAYLISTS, "", fd_socket);

                parseReceivedPlayList(fd_socket);

                //TODO: SOLVE THE PRINT
                
                asprintf(&buffer, "There are %d lists available for download:\n",playLists.numPlayList);
                printx(buffer);
                for (int i = 0; i < playLists.numPlayList ; i++){
                    asprintf(&buffer, "%d. %s\n",i + 1 ,playLists.playList[i].name);
                    printx(buffer);
                    for (int j = 0; j < playLists.playList[i].numSongs; j++){
                        asprintf(&buffer, "\t%c. %s\n",j + 'a' ,playLists.playList[i].songs[j]);
                        printx(buffer);
                    }
                }
                free (buffer);
                
                
                ///////////////////
                printx("Comanda OK\n");
                break;
            case DOWNLOAD_SONG_CMD:
                printx("Comanda OK\n");
                break;
            case DOWNLOAD_PLAYLIST_CMD:
                printx("Comanda OK\n");
                break;
            case CHECK_DOWNLOADS_CMD:
                printx("Comanda OK\n");
                break;
            case CLEAR_DOWNLOADS_CMD:
                printx("Comanda OK\n");
                break;
            case PARTIALLY_CORRECT_CMD:
                printx("Comanda KO\n");
                //Unknown command
                break;
            case INVALID_CMD:
                printx("Comanda KO\n");
                //Not valid command
                break;
            case NO_CMD:
                //No command entered
                break;
            default:
                break;
        }

    } while (command_case_num != LOGOUT_CMD);

}

// Handle unexpected termination scenarios.
void terminateExecution () {
    free(client_config.name);
    free(client_config.files_folder);
    free(client_config.ip_discovery);

    close (fd_config);    

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

    //printConfigFile(client_config);

    printInitMsg();

    enterCommandMode();
    
    terminateExecution();

    return 0;

}