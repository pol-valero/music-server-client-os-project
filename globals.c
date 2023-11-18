#include "globals.h"

void sendFrame (uint8_t type, char* header, char* data, int fd_socket) {

    Frame frame = createFrame(type, header, data);
    char* buffer = serializeFrame(frame);
    write(fd_socket, buffer, 256);
    free(buffer);
    free(frame.header);
    free(frame.data);
}

Frame receiveFrame (int fd_socket) {

    char* buffer = malloc(sizeof(char) * 256);
    read(fd_socket, buffer, 256);
    Frame frame = deserializeFrame(buffer);
    free(buffer);

    return frame;
}

Frame createFrame(uint8_t type, char* header, char* data) {
    
    Frame frame;

    uint16_t header_length = strlen(header) + 1;    //We add 1 to the length to include the '\0' character
    uint16_t data_length = strlen(data) + 1;
    uint16_t data_field_length = 256 - 3 - header_length;

    frame.type = type;
    frame.header_length = header_length;

    frame.header = malloc(sizeof(char) * header_length);
    strcpy(frame.header, header);

    if (data_length == data_field_length) {
        frame.data = malloc(sizeof(char) * data_field_length);
        strcpy(frame.data, data);  //If the data has exactly the size of the data field, we can just point to it.
    } else {
        
        char* allocatedData;

        allocatedData = malloc(sizeof(char) * data_field_length); //If the data is smaller than the data field, we need request more memory and add padding.

        strcpy(allocatedData, data); //We copy the data to the allocated memory
        memset(allocatedData + data_length, '\0', data_field_length - data_length); //We add '\0' as a padding to the data field.
        //allocatedData + data_length = starting position of the padding
        //data_field_length - data_length = number of '\0' padding to add

        frame.data = allocatedData;
    }

    return frame;
}

char* serializeFrame(Frame frame) {

    char* buffer = malloc(sizeof(char) * 256);      //256 bytes is the fixed size of a frame

    int offset = 0;

    memcpy(buffer, &frame.type, sizeof(frame.type));
    offset += sizeof(frame.type);

    memcpy(buffer + offset, &frame.header_length, sizeof(frame.header_length));
    offset += sizeof(frame.header_length);

    memcpy(buffer + offset, frame.header, frame.header_length);
    offset += frame.header_length;

    memcpy(buffer + offset, frame.data, 256 - 3 - frame.header_length);

    return buffer;
}

Frame deserializeFrame(char* buffer) {
    Frame frame;

    int offset = 0;

    memcpy(&frame.type, buffer, sizeof(frame.type));
    offset += sizeof(frame.type);

    memcpy(&frame.header_length, buffer + offset, sizeof(frame.header_length));
    offset += sizeof(frame.header_length);

    frame.header = malloc(sizeof(char) * frame.header_length);
    memcpy(frame.header, buffer + offset, frame.header_length);
    offset += frame.header_length;

    frame.data = malloc(sizeof(char) * (256 - 3 - frame.header_length));
    memcpy(frame.data, buffer + offset, 256 - 3 - frame.header_length);

    return frame;
}

int frameIsValid(Frame frame) {
    if (frame.type < 0x01 || frame.type > 0x07) {
        return 0;
    }

    if (frame.header_length != sizeof(frame.header)) {
        return 0;
    }

    return 1;
}

int startServer(int port, char *ip) {
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

    if (bind (fd_socket, (void *)&socket_addr, sizeof(socket_addr)) < 0) {
        printx("ERROR binding port\n");

        close(fd_socket);

        return -1;
    }

    listen(fd_socket, 20);

    return fd_socket;
}


int startServerConnection(char* ip, int port) {

    char* buffer; 

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
            
            asprintf(&buffer, "Connection error with the server: %s", strerror(errno));
            printx(buffer);
            close(socket_conn);
            free(buffer);

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

    while (1) {
        size = read(fd, &c, sizeof(char));
        
        if (size > 0 && c != endChar) {
            string = realloc(string, sizeof(char) * (i + 2));
            string[i++] = c;

        } else {
            break;
        }
    }
    string[i] = '\0';

    return string;
}

// Read characters until reaching the char endChar, ignoring any exception letters, and return the string.
char* readUntilCharExceptLetter(int fd, char endChar, char exception) {
    int size;
    int i = 0;
    char c = '\0';

    char* string = malloc(sizeof(char));

    while (1) {
        size = read(fd, &c, sizeof(char));
        
        if (size > 0 && c != endChar) {
            if (exception != c){
                string = realloc(string, sizeof(char) * (i + 2));
                string[i++] = c;

            }
        } else {
            break;
        }
    }
    string[i] = '\0';

    return string;
}

//Prints dynamic string, where we cannot use strlen
void printDynStr(char* buffer, int bufferSize) {
     write(1, buffer, bufferSize);
}