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

#define printx(x) write(1, x, strlen(x))

char* readUntilEitherChar(int fd, char endChar, char endChar2, int* endChar2Found);

char* readUntilChar(int fd, char endChar);

void printDynStr(char* buffer, int bufferSize);