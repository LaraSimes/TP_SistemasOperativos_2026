#ifndef COMUN_H
#define COMUN_H

#include "utils/general.h"
#include <stddef.h>  
#include <pthread.h>

typedef struct{
    char* log_level;
    int segment_max_size;
    char* allocation_strategy;
    int instruction_delay;
    int compaction_delay;
    char* scripts_basepath;
    char* puerto_escucha; 
    char* ip_ks;
    char* puerto_ks;
} t_config_valores;

typedef struct {
    int pid;
    char* path_instrucciones;
    t_contexto contexto;
} t_proceso_control;
// estructura de control para cada proceso, con su pid, path de instrucciones, contexto y lista de segmentos

typedef struct {
    uint32_t base;
    uint32_t tamanio;
} t_hueco;

typedef struct {
    int fd_stick;
    uint32_t base;
    uint32_t tamanio;
}t_stick; //estructura para manejo de sticks

extern pthread_mutex_t mutex_lista_procesos;
extern pthread_mutex_t mutex_lista_huecos;
extern pthread_mutex_t mutex_lista_segmentos;
extern pthread_mutex_t mutex_lista_sticks;
extern t_list* procesos_activos;
extern t_list* huecos_libres;
extern t_list* segmentos;
extern t_list* sticks;
extern int fd_SWAP;
extern uint32_t max_segmento;
extern t_log* logger;
extern int fd_ks_bsod;
extern char* ip_ks;
extern char* puerto_ks;
#endif
