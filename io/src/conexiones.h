#include "utils/general.h"
#include <commons/log.h>
#include <commons/config.h>

#include <stdio.h> 
#include <stdlib.h> 

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
typedef enum {
    CPU = 1,
    IO = 2
} op_code;

int crear_cliente(char *ip, char* puerto);
int liberar(t_log *log, t_config *config, int conexion);
void handshakear (int socket_cliente, t_log *log);