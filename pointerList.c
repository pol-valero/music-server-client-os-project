#include <stdio.h>
#include <stdlib.h>
#include "pointerList.h"

typedef struct {
    void** pointers;
    int numPointers;
} PointersToFree;

PointersToFree* listOfPointerLists;
int num_lists = 0;

int initPointerList() {

    if (num_lists == 0) {
        listOfPointerLists = malloc(sizeof(PointersToFree));
        listOfPointerLists[0].numPointers = 0;
    } else {
        listOfPointerLists = realloc(listOfPointerLists, sizeof(PointersToFree) * (num_lists + 1));
    }

    return num_lists++; //return the list id, which will be used as handler

}

void addPointerToList(void* pointer, int listId) {

    int numPointers = listOfPointerLists[listId].numPointers;
    
    if (listOfPointerLists[listId].numPointers == 0) {
        listOfPointerLists[listId].pointers = malloc(sizeof(void**));
    } else {
        listOfPointerLists[listId].pointers = realloc(listOfPointerLists[listId].pointers, sizeof(void**) * (numPointers + 1));
    }

    listOfPointerLists[listId].pointers[numPointers] = pointer;
    listOfPointerLists[listId].numPointers++;

}

void** getPointersFromList(int listId) {
    return listOfPointerLists[listId].pointers;
}

void freePointerList(int listId) {
    int i;
    for (i = 0; i < listOfPointerLists[listId].numPointers; i++) {
        if (listOfPointerLists[listId].pointers[i] != NULL) {
            free(listOfPointerLists[listId].pointers[i]);
        }
    }
    //free(listOfPointerLists[listId].pointers);
    listOfPointerLists[listId].numPointers = 0;
}

void freeListOfPointerLists() {

    int i;
    int pointersLeftToFree = 0;
    for (i = 0; i < num_lists; i++) {
        if (listOfPointerLists[i].pointers != NULL) {
            pointersLeftToFree++;
        }
    }

    if (pointersLeftToFree == 0) {
        free(listOfPointerLists);
        num_lists = 0;
    }
    
}