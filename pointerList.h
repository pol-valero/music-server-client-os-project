int initPointerList();

void addPointerToList(void* pointer, int listId);

void** getPointersFromList(int listId);

void freePointerList(int listId);

void freeListOfPointerLists();