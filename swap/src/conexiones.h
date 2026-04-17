#include <commons/log.h>
#include <commons/config.h>

#include <stdio.h> 
#include <stdlib.h> 

#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
typedef enum{
    KERNEL_SCHEDULER = 1, 
    CPU = 2,            
    MEMORY_STICK = 3,      
    SWAP = 4
} t_modulo;

int crear_cliente(char *ip, char* puerto);
int liberar(t_log *log, t_config *config, int conexion);
void handshakear (int socket_cliente, t_log *log);