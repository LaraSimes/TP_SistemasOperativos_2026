#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#include <commons/collections/queue.h>
#include <pthread.h>
#include <stdint.h>
#include "utils/general.h"

// Estado de un proceso
typedef enum {
    NEW,
    READY,
    EXEC,
    BLOCK,
    SUSP_READY,
    SUSP_BLOCK,
    EXIT
} t_estado;

// PCB - Process Control Block
typedef struct {
    int pid;
    int prioridad;
    t_estado estado;
    t_contexto contexto;
    char* path_instrucciones;
    // IO pendiente
    t_tipo_io tipo_io_pendiente;
    int param_io;          // ms para SLEEP, cantidad para STDIN/STDOUT
} t_pcb;

// CPU registrada
typedef struct {
    int fd;
    int id;
    int libre;   // 1 = libre, 0 = ocupada
} t_cpu;

// Colas globales
extern t_queue* cola_new;
extern t_queue* cola_ready;
extern t_queue* cola_block;

// Lista de CPUs e IOs conectadas
extern t_list* lista_cpus;
extern t_list* lista_ios;

// Mutex para proteger las colas
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_block;
extern pthread_mutex_t mutex_cpus;

// Funciones
t_pcb* crear_pcb(int pid, char* path, int prioridad);
void cambiar_estado(t_pcb* pcb, t_estado nuevo_estado);
void encolar_ready(t_pcb* pcb);
t_cpu* buscar_cpu_libre();

// registro IO
typedef struct {
    int fd;
    t_tipo_io tipo;
    int libre;
} t_io_registrada;

void iniciar_planificador();
int generar_pid();
t_pcb* desencolar_ready();
void encolar_block(t_pcb* pcb);
t_io_registrada* buscar_io(t_tipo_io tipo);

#endif