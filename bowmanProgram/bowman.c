#include "../globals.h"
#include "bowmanConfig.h"
#include "bowmanCmdProcessing.h"

int fd_config;

ClientConfig client_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

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
                int fd_socket = startServerConnection(client_config.ip_discovery, client_config.port_discovery);
                write(fd_socket, "prova\n", strlen("prova\n")); //TODO: REMOVE THIS LINE
                char* msg = readUntilChar(fd_socket, '\n'); //TODO: REMOVE THIS LINE
                printx(msg); //TODO: REMOVE THIS LINE
                break;
            case LOGOUT_CMD:
                printx("Comanda OK\n");
                exit_flag = 1;
                break;
            case LIST_SONGS_CMD:
                printx("Comanda OK\n");
                break;
            case LIST_PLAYLISTS_CMD:
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

    } while (exit_flag == 0);

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

    printConfigFile(client_config);

    enterCommandMode();
    
    terminateExecution();

    return 0;

}