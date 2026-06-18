#include "utils/general.h"

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
} t_hueco_libre;

typedef struct {
    int fd_stick;
    uint32_t base;
    uint32_t tamanio;
}t_stick; //estructura para manejo de sticks

pthread_mutex_t mutex_lista_procesos;
pthread_mutex_t mutex_lista_huecos;
pthread_mutex_t mutex_lista_segmentos;
pthread_mutex_t mutex_lista_sticks;
extern t_list* procesos_activos;
extern t_list* huecos_libres;
extern t_list* segmentos;
extern t_list* sticks;
extern int fd_KS;
extern uint32_t max_segmento;


