
#ifndef CONEXIONES_H_
#define CONEXIONES_H_

#include <commons/log.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>


int crear_conexion(char *ip, char* puerto);
void handshakear (int socket_cliente, t_log *log);

#endif 

