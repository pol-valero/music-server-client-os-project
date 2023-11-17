#include "globals.h"
#include "bowmanConfig.h"
#include "bowmanCmdProcessing.h"

PointersToFree pointers_list = {.numPointers = 0};

int fd_config;

// Function to manage user-input commands.
void enterCommandMode() {

    char* command;
    int command_case_num;
    int exit_flag = 0;

    do {

        printx("$ ");
        command = readUntilChar(STDIN_FILENO, '\n');
        command_case_num = commandToCmdCaseNum(command);
        free(command);

        switch (command_case_num) {
            case CONNECT_CMD:
                printx("Comanda OK\n");
                //printx("CONNECT_CMD\n");
                break;
            case LOGOUT_CMD:
                printx("Comanda OK\n");
                //printx("LOGOUT_CMD\n");
                exit_flag = 1;
                break;
            case LIST_SONGS_CMD:
                printx("Comanda OK\n");
                //printx("LIST_SONGS_CMD\n");
                break;
            case LIST_PLAYLISTS_CMD:
                printx("Comanda OK\n");
                //printx("LIST_PLAYLISTS_CMD\n");
                break;
            case DOWNLOAD_SONG_CMD:
                printx("Comanda OK\n");
                //printx("DOWNLOAD_SONG_CMD\n");
                break;
            case DOWNLOAD_PLAYLIST_CMD:
                printx("Comanda OK\n");
                //printx("DOWNLOAD_PLAYLIST_CMD\n");
                break;
            case CHECK_DOWNLOADS_CMD:
                printx("Comanda OK\n");
                //printx("CHECK_DOWNLOADS_CMD\n");
                break;
            case CLEAR_DOWNLOADS_CMD:
                printx("Comanda OK\n");
                //printx("CLEAR_DOWNLOADS_CMD\n");
                break;
            case PARTIALLY_CORRECT_CMD:
                printx("Comanda KO\n");
                //printx("PARTIALLY_CORRECT_CMD\n");
                //Unknown command
                break;
            case INVALID_CMD:
                printx("Comanda KO\n");
                //printEr("INVALID_CMD\n");
                //Not valid command
                break;
            case NO_CMD:
                //No command entered
                break;
            default:
                break;
        }

    } while (exit_flag == 0);

}

// Handle unexpected termination scenarios.
void terminateExecution () {

    char* currentInputPointer = getGlobalsCurrentInputPointer();

    for (int i = 0; i < pointers_list.numPointers; i++) {
        if (pointers_list.pointers[i] != NULL) {
            free(pointers_list.pointers[i]);
        }
    }

    if (pointers_list.pointers != NULL) {
        free(pointers_list.pointers);
    }

    if (currentInputPointer != NULL) {
        free(currentInputPointer);
    }

    close (fd_config);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);

}

//main function :P
int main (int argc, char** argv) {

    ClientConfig client_config;

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

    addPointerToList(client_config.files_folder, &pointers_list);
    addPointerToList(client_config.ip, &pointers_list);
    addPointerToList(client_config.name, &pointers_list);

    printConfigFile(client_config);

    enterCommandMode();
    
    terminateExecution();

    return 0;

}