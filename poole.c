#include "globals.h"
#include "pooleConfig.h"

int fd_config;

ServerConfig server_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

// Handle unexpected termination scenarios.
void terminateExecution () {
    char* currentInputPointer = getGlobalsCurrentInputPointer();

    free(server_config.name);
    free(server_config.files_folder);
    free(server_config.ip_discovery);
    free(server_config.ip_poole);

    if (currentInputPointer != NULL) {
        free(currentInputPointer);
    }

    close (fd_config);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
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

    server_config = readConfigFile(fd_config);

    printConfigFile(server_config);
    
    terminateExecution();

    return 0;
}