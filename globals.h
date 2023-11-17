#ifndef _GLOBALS_H_
#define _GLOBALS_H_

#define _XOPEN_SOURCE 500
#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>

// ANSI escape codes for text formatting
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_RESET   "\x1b[0m"

#define printx(x) write(1, x, strlen(x))
#define printEr(x) write(1, ANSI_COLOR_RED x ANSI_COLOR_RESET, strlen(ANSI_COLOR_RED x ANSI_COLOR_RESET))

int start_server(int port, char *ip);

int startServerConnection(char* ip, int port);

char* readUntilEitherChar(int fd, char endChar, char endChar2, int* endChar2Found);

char* readUntilChar(int fd, char endChar);

char* readUntilCharExceptLetter(int fd, char endChar, char exception);

void printDynStr(char* buffer, int bufferSize);

char* getGlobalsCurrentInputPointer();


#endif