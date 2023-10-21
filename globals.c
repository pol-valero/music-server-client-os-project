#include "globals.h"


PointersToFree globals_pointers_list = {.numPointers = 0};

char* currentInputPointer;  //We will free this pointer if the program is interrupted by a SIGNAL

void addPointerToList(void* pointer, PointersToFree* pointers_list2) {

    int numPointers = (*pointers_list2).numPointers;
    
    if ((*pointers_list2).numPointers == 0) {
        (*pointers_list2).pointers = malloc(sizeof(void**));
    } else {
        (*pointers_list2).pointers = realloc((*pointers_list2).pointers, sizeof(void**) * (numPointers + 1));
    }

    (*pointers_list2).pointers[numPointers] = pointer;
    (*pointers_list2).numPointers += 1;

}

//Returns the current input pointer, in order to be freed if the program is interrupted by a SIGNAL
char* getGlobalsCurrentInputPointer() {
    return currentInputPointer;
}


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

    return string;
}

//Prints dynamic string, where we cannot use strlen
void printDynStr(char* buffer, int bufferSize) {
     write(1, buffer, bufferSize);
}