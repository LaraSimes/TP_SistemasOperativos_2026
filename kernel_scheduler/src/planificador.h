#ifndef PLANIFICADOR_H
#define PLANIFICADOR_H

#include <commons/collections/queue.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>
#include "utils/general.h"
#include "dispatcher.h"

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
    int param_io;           // ms para SLEEP, cantidad para STDIN/STDOUT
    uint32_t dir_fisica;    // dirección física para STDIN/STDOUT
    int fd_cpu_dispatch;    // fd de la CPU que despachó este proceso (para syscalls de memoria)
    int prioridad_original;
    int seg_id_pendiente;
    int tamanio_pendiente;
    time_t tiempo_suspension; // timestamp al momento de pasar a SUSP_BLOCK, para desempate en intentar_desuspender
    int numero_bloqueo; // se incrementa en cada encolar_block; invalida timers de suspensión de bloqueos anteriores
} t_pcb;

// CPU registrada
typedef struct {
    int fd;
    int fd_interrupt; 
    int id;
    int libre;   // 1 = libre, 0 = ocupada
    t_pcb* pcb_actual;
    bool interrupcion_pendiente;
    bool interrupcion_compactacion;
    bool interrupcion_prioridad;
    int quantum_id; //para respetar quantum 
} t_cpu;

// MUTEX
typedef struct {
    char* nombre;
    int owner_pid;
    t_queue* cola_espera;
} t_mutex;

// Colas globales
extern t_queue*  cola_new;
extern t_queue** colas_ready;
extern int       num_colas;
extern t_queue*  cola_block;

// Listas Globales
extern t_list* lista_cpus;
extern t_list* lista_ios;
extern t_list* lista_mutex;
extern t_queue* cola_susp_block;
extern t_queue* cola_susp_ready;
extern int suspension_timeout;
extern t_list* lista_desalojados;

// Compactacion
extern bool compactando;
extern pthread_mutex_t mutex_compactacion;
extern pthread_cond_t  cond_compactacion;
extern int cpus_pendientes_compactacion;


// Mutex para proteger las colas
extern pthread_mutex_t mutex_ready;
extern pthread_mutex_t mutex_block;
extern pthread_mutex_t mutex_cpus;
extern pthread_mutex_t mutex_ios;
extern pthread_mutex_t mutex_mutex;
extern pthread_mutex_t mutex_susp;


// Funciones
t_pcb* crear_pcb(int pid, char* path, int prioridad);
void cambiar_estado(t_pcb* pcb, t_estado nuevo_estado);
void encolar_ready(t_pcb* pcb);
void reencolar_ready_sin_wake(t_pcb* pcb);
t_cpu* buscar_cpu_libre();
t_pcb* buscar_en_block(int pid);

// registro IO
typedef struct {
    int fd;
    t_tipo_io tipo;
    int libre;
    int pid_actual;
    t_queue* cola_espera;
} t_io_registrada;

void iniciar_planificador();
int generar_pid();
t_pcb* desencolar_ready();
void encolar_block(t_pcb* pcb);
t_io_registrada* buscar_io(t_tipo_io tipo);
t_cpu* buscar_cpu_por_fd(int fd);
t_io_registrada* buscar_io_por_fd(int fd);
void crear_proceso_inicial(char* path);
t_io_registrada* buscar_io_por_tipo(t_tipo_io tipo);
t_mutex* buscar_mutex(char* nombre);
t_mutex* crear_mutex(char* nombre);
t_cpu* buscar_cpu_por_id(int id);
void aplicar_herencia_prioridad(int owner_pid, int nueva_prioridad);
void encolar_susp_ready(t_pcb* pcb);
void intentar_desuspender();
t_pcb* buscar_en_susp_block(int pid);
void reencolar_ready(t_pcb* pcb);
void iniciar_compactacion(t_pcb* pcb_original);
void bsod();
void notificar_fin_proceso_km(int pid);


#endif