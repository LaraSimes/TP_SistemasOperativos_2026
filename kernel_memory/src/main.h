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
#include "utils/general.h"

typedef struct{
    char* log_level;
    int segment_max_size;
    char* allocation_strategy;
    int instruction_delay;
    int compaction_delay;
    char* scripts_basepath;
    char* puerto_escucha; 
} t_config_valores;

typedef struct {
    uint32_t pc, eax, ebx, ecx, edx;
    uint8_t ax, bx, cx, dx;
    uint32_t si, di; // dir logica origen, dir logica destino
} t_registros;
//definimos registros y contador de programa
typedef struct {
    int pid;
    char* path_instrucciones;
    t_registros registros;
} t_proceso_control;
// definimos struct del pid y path de instrucciones. Molde de 1 procesoS

void cargar_informacion_configuracion(t_config* config, t_config_valores* valores);
void terminar_programa(int server_fd, t_log* logger, t_config* config);

int iniciar_servidor(char*puerto);
int esperar_cliente (int socket_servidor);
void* manejar_cliente(void* arg);

void recibir_creacion_proceso(int socket_cliente);
char* leer_instruccion_de_archivo(char* path, uint32_t pc);
t_proceso_control* buscar_proceso_por_pid(int pid);

bool _es_el_proceso(void* elemento);


extern t_list* procesos_activos;

#endif