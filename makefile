all: bowman poole

pointerList.o: pointerList.c pointerList.h
	gcc -c pointerList.c -Wall -Wextra

globals.o: globals.c globals.h
	gcc -c globals.c -Wall -Wextra

bowman.o: bowman.c globals.h
	gcc -c bowman.c -Wall -Wextra

bowman: bowman.o globals.o pointerList.o
	gcc bowman.o globals.o pointerList.o -o bowman -Wall -Wextra

poole.o: poole.c globals.h
	gcc -c poole.c -Wall -Wextra

poole: poole.o globals.o
	gcc poole.o globals.o -o poole -Wall -Wextra


.PHONY: clean
clean:
	rm *.o
	rm bowman
	rm poole