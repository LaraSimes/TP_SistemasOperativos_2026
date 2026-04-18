
#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <commons/log.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

typedef enum{
    MOD_KERNEL_SCHEDULER,    //0
    MOD_CPU,                //1
    MOD_MEMORY_STICK,      //2
    MOD_SWAP              //3
} t_modulo;

int crear_conexion(char *ip, char* puerto);
void handshakear (int socket_cliente, t_log *log);

#endif 

