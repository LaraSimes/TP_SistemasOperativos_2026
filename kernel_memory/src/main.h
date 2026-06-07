#ifndef MAIN_H_
#define MAIN_H_

#include "utils/general.h"
#include "servidor.h"
#include "operaciones.h"

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
    int pid;
    char* path_instrucciones;
    t_contexto contexto;
    t_list* segmentos;
} t_proceso_control;
// definimos struct del pid y path de instrucciones. Molde de 1 procesoS

typedef struct {
    int num_segmento;
    uint32_t base;
    uint32_t limite;
} t_segmento;
//definimos struct del segmento, con su numero, base y limite

void cargar_informacion_configuracion(t_config* config, t_config_valores* valores);

void* manejar_cliente(void* arg);

char* leer_instruccion_de_archivo(char* path, uint32_t pc);
t_proceso_control* buscar_proceso_por_pid(int pid);

bool _es_el_proceso(void* elemento);

extern t_list* procesos_activos;

pthread_mutex_t mutex_lista_procesos;

#endif