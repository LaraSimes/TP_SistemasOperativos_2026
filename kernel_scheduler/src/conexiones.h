
#ifndef CONEXIONES_H_
#define CONEXIONES_H_


#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <unistd.h>

int crear_conexion(char *ip, char* puerto);


#endif 

