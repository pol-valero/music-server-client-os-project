#define _GNU_SOURCE
#include <stdio.h>
#include <time.h>
#include <pthread.h>
#include <sys/types.h>
#include <fcntl.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/stat.h>
#include <unistd.h>
#include <signal.h>
#include <poll.h>
#include <arpa/inet.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <sys/ioctl.h>
#include <math.h>

#define Write(x) write(STDOUT_FILENO, x, strlen(x))

#define ERR_ARG     "Error en el número de argumentos\n"
#define ERR_OPEN    "Ha habido un error al abrir el archivo key.txt\n"
#define ERR_IP      "No se ha podido configurar la IP\n"
#define ERR_SOCK    "Error al crear el socket. Por favor, intentalo de nuevo\n"
#define ERR_CONN    "Error al conectarse al servidor\n"
#define ERR_KEY     "El servidor no reconoce la clave\n"
#define ERR_UBI     "\nAun no se te ha localizado\n"

#define KEY_RECOG   "Clave reconocida\n"
#define KEY_NVAL    "KEY:Not validated"
#define MENU        "\nOpciones:\n\n\t1. Enviar ubicación\n\t2. Llamada de emergencia\n\t3. Hospital más cercano\n\t4. Salir\n\n: "
#define MENU2       "\t1. Introducir ubicación manualmente\n\t2. Usar la ubicación enviada\n\n: "
#define CONTACTS    "\nSOSmart reconoce los siguientes contactos como emergencia:\n"
#define ASK_CONT    "\nA que contacto quieres llamar? "   
#define NO_CONT     "No existe este contacto en la lista recibida\n"  
#define NO_OP       "\nNo has escogido una opción valida.\n\n"
#define W_UBI       "\nEscribe la ubicación: "
#define NO_HOSP     "No se ha encontrado ningún hospital cercano en la ubicación seleccionada\n"


int main (int argc, char *argv[]) {
    struct sockaddr_in server;

    bzero(&server, sizeof(server));
    server.sin_family = AF_INET;
    server.sin_port = htons();

    if(inet_pton(AF_INET, argv[1], &server.sin_addr) < 0) {
        Write(ERR_IP);
        free(key);
        return -1;
    }

    if((socketfd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        Write(ERR_SOCK);
        free(key);
        return -1;
    }

    if(connect(socketfd, (struct sockaddr*) &server, sizeof(server)) < 0) {
        Write(ERR_CONN);
        free(key);
        close(socketfd);
        return -1;
    }
}