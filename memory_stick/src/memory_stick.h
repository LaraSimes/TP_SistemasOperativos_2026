#ifndef MEMORY_STICK
#define MEMORY_STICK


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <commons/log.h>
#include <commons/config.h>

#include <stdint.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "utils/general.h"
#include "utils/cliente.h"

#include "conexiones.h"

extern uint8_t* memoria;
extern int memory_delay;
extern pthread_mutex_t mutex_memoria;
extern int tamanio_memoryStick;

void* manejar_km(void* arg);
void  manejar_cpu_thread(int* arg);
int iniciar_servidor_ms(char*puerto);
int esperar_cliente_ms(int socket_servidor);
void manejar_cpu(int* arg);
void obtenerConfig(char* ruta);
#endif




