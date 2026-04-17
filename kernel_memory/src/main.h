#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <pthread.h> 
#include "conexiones.h"

typedef struct{
    char* log_level;
    int segment_max_size;
    char* allocation_strategy;
    int instruction_delay;
    int compaction_delay;
    char* scripts_basepath;
    char* puerto_escucha; 
} t_config_valores;

void cargar_informacion_configuracion(t_config* config, t_config_valores* valores);
t_config* iniciar_config(void);
t_log* iniciar_logger(void);

int iniciar_servidor(char*puerto);
int esperar_cliente (int socket_servidor);
void terminar_programa(int server_fd, t_log* logger, t_config* config);
void* manejar_cliente(void* arg);

#endif