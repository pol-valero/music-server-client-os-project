#include "globals.h"
#include "pooleConfig.h"
#include "pooleCmdProcessing.h"

PointersToFree pointers_list = {.numPointers = 0};

int fd_config;

void enterCommandMode() {

    char* command;
    int command_case_num = NO_CMD;

    do {
        printx("$ ");
        command = readUntilChar(STDIN_FILENO, '\n');
        command_case_num = commandToCmdCaseNum(command);
        free(command);


        switch (command_case_num) {
            case LOGOUT_CMD:
                printx("Comanda OK\n");
                //printx("LOGOUT_CMD\n");
                break;
            case INVALID_CMD:
                printx("Comanda KO\n");
                //printx("INVALID_CMD\n");
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

int main (int argc, char** argv) {
    ServerConfig server_config;

    signal(SIGINT, terminateExecution);
    signal(SIGTERM, terminateExecution);

    if (argc < 2) {
        printx("\nERROR: You must enter a the configuration file name as a parameter\n");
        return 0;
    }

    fd_config = open(argv[1], O_RDONLY);

    if (fd_config < 0) {
        printx("\nERROR: Cannot open the file. Filename may be incorrect\n");
        return 0;
    }

    server_config = readConfigFile(fd_config);

    addPointerToList(server_config.name, &pointers_list);
    addPointerToList(server_config.files_folder, &pointers_list);
    addPointerToList(server_config.ip, &pointers_list);
    addPointerToList(server_config.ip_server, &pointers_list);

    printInitMsg(server_config.name);
    printf("%s\n\n",server_config.name);
    printConfigFile(server_config);

    enterCommandMode();
    
    terminateExecution();

    return 0;
}