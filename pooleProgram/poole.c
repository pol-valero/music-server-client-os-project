#include "../globals.h"
#include "pooleConfig.h"
#include "sys/select.h"
#include <dirent.h>

#define PATH "pooleProgram/data"

int fd_config;
int fd_client;

ServerConfig server_config; //This variable has to be global in order to be freed if the program is interrupted by a SIGNAL

int fd_socket;

void doDiscoveryHandshake() {

    char* buffer;

    int fd_socket = startServerConnection(server_config.ip_discovery, server_config.port_discovery);

    if (fd_socket < 0) {
        printEr("ERROR: Cannot connect to the discovery server\n");
        return;
    }

    asprintf(&buffer, "%s&%s&%d", server_config.name, server_config.ip_poole, server_config.port_poole);

    sendFrame(0x01, "NEW_POOLE", buffer, fd_socket);

    Frame responseFrame = receiveFrame(fd_socket);

    char buffer2[100];
    sprintf(buffer2, "%d %d %s %s", responseFrame.type, responseFrame.header_length, responseFrame.header, responseFrame.data);
    printx(buffer2);
        
}

// Handle unexpected termination scenarios.
void terminateExecution () {

    free(server_config.name);
    free(server_config.files_folder);
    free(server_config.ip_discovery);
    free(server_config.ip_poole);

    close (fd_config);

    signal(SIGINT, SIG_DFL);
    raise(SIGINT);
}

char** getFilesName(int *numFiles){
    // Abre el directorio
    DIR* dir = opendir(PATH);

    if (dir == NULL) {
        return NULL;
    }

    struct dirent *input;
    char** filesList = NULL;
    *numFiles = 0;
    while ((input = readdir(dir)) != NULL) {
        if (input->d_type == DT_REG) {
            filesList = realloc(filesList,sizeof(char*) * (*numFiles + 1));
            if (filesList == NULL) {
                printEr("Error:Problemas con realloc");
                return NULL;
            }
            if (asprintf(&filesList[*numFiles], "%s", input->d_name) == -1) {
                printEr("Error:Problemas con asprintf");
                return NULL;
            }
            (*numFiles)++;
        }
    }

    closedir(dir);

    return filesList;
}

char** getFoldersName(int *numFiles){
    // Abre el directorio
    DIR* dir = opendir(PATH);

    if (dir == NULL) {
        return NULL;
    }

    struct dirent *input;
    char** filesList = NULL;
    *numFiles = 0;

    while ((input = readdir(dir)) != NULL) {
        if (input->d_type == DT_DIR && strcmp(input->d_name, ".") != 0 && strcmp(input->d_name, "..") != 0) {
            filesList = realloc(filesList,sizeof(char*) * (*numFiles + 1));
            if (filesList == NULL) {
                printEr("Error:Problemas con realloc");
                return NULL;
            }
            if (asprintf(&filesList[*numFiles], "%s", input->d_name) == -1) {
                printEr("Error:Problemas con asprintf");
                return NULL;
            }
            (*numFiles)++;
        }
    }

    closedir(dir);

    return filesList;
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

    //TODO:  REMOVE THESE LINES, THEY ARE JUST FOR TESTING

    int numFiles;
    char** files;

    files = getFoldersName(&numFiles);
    if (files != NULL){
        for (int i = 0; i < numFiles; i++){
            write(STDOUT_FILENO, files[i], strlen(files[i]));
            free(files[i]);
        }
        free(files);
    }else {
        printx("Error en la lectura de archivos o carpeta vacia");
    }

    
    files = getFilesName(&numFiles);
    if (files != NULL){
        for (int i = 0; i < numFiles; i++){
            write(STDOUT_FILENO, files[i], strlen(files[i]));
            free(files[i]);
        }
        free(files);
    }else {
        printx("Error en la lectura de archivos o carpeta vacia");
    }

    ///////////////////

    doDiscoveryHandshake();

    //TODO: REMOVE THESE LINES, THEY ARE JUST FOR TESTING

    /*struct sockaddr_in c_addr;
    socklen_t c_len = sizeof(c_addr);

    fd_socket = startServer(server_config.port_poole, server_config.ip_poole);
    fd_client = accept(fd_socket, (void *) &c_addr, &c_len);*/
    /////////
    
    terminateExecution();

    return 0;
}