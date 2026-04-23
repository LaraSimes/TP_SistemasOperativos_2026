#ifndef MEMORY_STICK
#define MEMORY_STICK


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "utils/general.h"

#include "conexiones.h"


int crear_conexion(char *ip, char* puerto);
int iniciar_servidor_ms(char*puerto);
int esperar_cliente_ms(int socket_servidor);
void manejar_cpu(int cliente_fd);
void obtenerConfig(char* ruta);
#endif
