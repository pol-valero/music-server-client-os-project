#include "globals.h"

char* currentInputPointer;  //We will free this pointer if the program is interrupted by a SIGNAL

//Returns the current input pointer, in order to be freed if the program is interrupted by a SIGNAL
char* getGlobalsCurrentInputPointer() {
    return currentInputPointer;
}

int start_server(int port, char *ip) {
    int fd_socket;

    struct sockaddr_in socket_addr;

    fd_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (fd_socket < 0) {

        printx("ERROR creating socket\n");

        return -1;
    }

    bzero(&socket_addr, sizeof (socket_addr));

    socket_addr.sin_port = htons (port);
    socket_addr.sin_family = AF_INET;
    socket_addr.sin_addr.s_addr = inet_addr(ip); //We convert IP string to a proper address for the in_addr structure

    if (bind (fd_socket, (void *) &socket_addr, sizeof (socket_addr)) < 0) {
        printx("ERROR binding port\n");

        close(fd_socket);

        return -1;
    }

    listen(fd_socket, 20);

    return fd_socket;
}


int startServerConnection(char* ip, int port) {

    char buffer[100]; //TODO: Make dynamic memory buffer
    int buffer_bytes;

    struct sockaddr_in socket_addr;
    int socket_conn = -1;

    socket_conn = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    if (socket_conn < 0) {
        printx("Error while creating socket\n");

    } else {

        memset(&socket_addr, 0, sizeof(socket_addr));
        socket_addr.sin_family = AF_INET;
        socket_addr.sin_port = htons(port);
        socket_addr.sin_addr.s_addr = inet_addr(ip); //We convert IP string to a proper address for the in_addr structure

        if (connect(socket_conn, (void *) &socket_addr, sizeof(socket_addr)) < 0) {
            
            buffer_bytes = sprintf(buffer, "Connection error with the server: %s", strerror(errno));
            write(1, buffer, buffer_bytes);
            close(socket_conn);

            return -1;
        }

    }

    return socket_conn;

}



// Read characters until reaching either endChar or endChar2. If endChar2 is found, set endChar2Found to 1.
char* readUntilEitherChar(int fd, char endChar, char endChar2, int* endChar2Found) {
    int size;
    int i = 0;
    char c = '\0';

    char* string = malloc(sizeof(char));

    while (1) {
        size = read(fd, &c, sizeof(char));
        
        if (size > 0 && c != endChar && c != endChar2) {
            string = realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;

        } else {

            if (c == endChar2) {
                *endChar2Found = 1;
            }
            break;
        }
    }

    string[i] = '\0';

    return string;
}

//Read until reach the char endChar and return the string.
char* readUntilChar(int fd, char endChar) {
    int size;
    int i = 0;
    char c = '\0';

    char* string = malloc(sizeof(char));

    currentInputPointer = string;
    while (1) {
        size = read(fd, &c, sizeof(char));
        
        if (size > 0 && c != endChar) {
            string = realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;

            currentInputPointer = string;
        } else {
            break;
        }
    }
    string[i] = '\0';

    currentInputPointer = NULL; //If we can return the string because no SIGNALs have interrupted the program, we set the pointer to NULL so it is not freed (we will free the string)
    return string;
}

// Read characters until reaching the char endChar, ignoring any exception letters, and return the string.
char* readUntilCharExceptLetter(int fd, char endChar, char exception) {
    int size;
    int i = 0;
    char c = '\0';

    char* string = malloc(sizeof(char));

    currentInputPointer = string;
    while (1) {
        size = read(fd, &c, sizeof(char));
        
        if (size > 0 && c != endChar) {
            if (exception != c){
                string = realloc(string, sizeof(char) * (i + 2));
                string[i++] = c;

                currentInputPointer = string;
            }
        } else {
            break;
        }
    }
    string[i] = '\0';

    currentInputPointer = NULL; //If we can return the string because no SIGNALs have interrupted the program, we set the pointer to NULL so it is not freed (we will free the string)
    return string;
}

//Prints dynamic string, where we cannot use strlen
void printDynStr(char* buffer, int bufferSize) {
     write(1, buffer, bufferSize);
}