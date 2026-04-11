#ifndef MAIN_H_
#define MAIN_H_

#include <stdio.h>
#include <stdlib.h>
#include <commons/log.h>
#include <commons/config.h>

typedef struct{
    char* log_level;
    int segment_max_size;
    char* allocation_strategy;
    int instruction_delay;
    int compaction_delay;
    char* scripts_basepath;
    char* puerto_escucha; // puerto donde la memoria va a esperar a los demas. paso b
} t_config_valores;

void cargar_informacion_configuracion(t_config* config, t_config_valores* valores, t_log* logger);
t_config* iniciar_config(void);
t_log* iniciar_logger(void);

#endif