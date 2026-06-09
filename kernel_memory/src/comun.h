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
    int pid;
    char* path_instrucciones;
    t_contexto contexto;
    t_list* segmentos;
} t_proceso_control;
// estructura de control para cada proceso, con su pid, path de instrucciones, contexto y lista de segmentos

typedef struct {
    uint32_t id_segmento;
    uint32_t base;
    uint32_t tamanio;
} t_segmento;

typedef struct {
    uint32_t base;
    uint32_t tamanio;
    int mstick_fd; // fd del Memory Stick al que pertenece este hueco
} t_sticks;

typedef struct {
    uint32_t base;
    uint32_t tamanio;
} t_hueco_libre;

pthread_mutex_t mutex_lista_procesos;
pthread_mutex_t mutex_lista_sticks;
pthread_mutex_t mutex_lista_huecos;
pthread_mutex_t mutex_lista_segmentos;
extern t_list* procesos_activos;
extern t_list* sticks_conectados;
extern t_list* huecos_libres;
extern t_list* segmentos;


