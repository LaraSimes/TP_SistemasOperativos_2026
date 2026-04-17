#ifndef MEMORY_STICK
#define MEMORY_STICK


#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>


#include "conexiones.h"


int crear_conexion(char *ip, char* puerto);
int iniciar_servidor(char*puerto);
int esperar_cliente(int socket_servidor);
void obtenerConfig();
#endif
