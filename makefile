all: bowman poole discovery

bowmanCmdProcessing.o: bowmanProgram/bowmanCmdProcessing.c bowmanProgram/bowmanCmdProcessing.h globals.h
	gcc -c bowmanProgram/bowmanCmdProcessing.c -Wall -Wextra

bowmanConfig.o: bowmanProgram/bowmanConfig.c bowmanProgram/bowmanConfig.h globals.h
	gcc -c bowmanProgram/bowmanConfig.c -Wall -Wextra

globals.o: globals.c globals.h
	gcc -c globals.c -Wall -Wextra

bowman.o: bowmanProgram/bowman.c globals.h bowmanProgram/bowmanConfig.h
	gcc -c bowmanProgram/bowman.c -Wall -Wextra

bowman: bowman.o globals.o bowmanConfig.o bowmanCmdProcessing.o
	gcc bowman.o globals.o bowmanConfig.o bowmanCmdProcessing.o -o bowman -Wall -Wextra

pooleConfig.o: pooleProgram/pooleConfig.c pooleProgram/pooleConfig.h globals.h
	gcc -c pooleProgram/pooleConfig.c -Wall -Wextra

poole.o: pooleProgram/poole.c globals.h pooleProgram/pooleConfig.h
	gcc -c pooleProgram/poole.c -Wall -Wextra

poole: poole.o globals.o pooleConfig.o
	gcc poole.o globals.o pooleConfig.o -o poole -Wall -Wextra -lpthread

discoveryConfig.o: discoveryProgram/discoveryConfig.c discoveryProgram/discoveryConfig.h globals.h
	gcc -c discoveryProgram/discoveryConfig.c -Wall -Wextra

discovery.o: discoveryProgram/discovery.c globals.h discoveryProgram/discoveryConfig.h
	gcc -c discoveryProgram/discovery.c -Wall -Wextra

discovery: discovery.o globals.o discoveryConfig.o
	gcc discovery.o globals.o discoveryConfig.o -o discovery -Wall -Wextra -lpthread

.PHONY: clean
clean:
	rm *.o
	rm bowman
	rm poole
	rm discovery