typedef struct {
    char* name;
    char* files_folder;
    char* ip;
    int port;
    char* ip_server;
    int port_server;
} ServerConfig;

ServerConfig readConfigFile(int fd_config);

void printInitMsg(char* serverName);

void printConfigFile(ServerConfig client_config);