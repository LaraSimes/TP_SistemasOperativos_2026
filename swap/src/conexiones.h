#include <commons/log.h>
#include <commons/config.h>

#include <stdio.h> 
#include <stdlib.h> 
#include "utils/general.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
typedef enum{
    MOD_KERNEL_SCHEDULER,    //0
    MOD_CPU,                //1
    MOD_MEMORY_STICK,      //2
    MOD_SWAP              //3
} t_modulo;

int crear_cliente(char *ip, char* puerto);
int liberar(t_log *log, t_config *config, int conexion);
void handshakear (int socket_cliente, t_log *log);