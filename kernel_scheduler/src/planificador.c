#include "planificador.h"
extern t_log* logger;

// Colas globales
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_block;

// Listas globales
t_list* lista_cpus;
t_list* lista_ios;

// Mutex
pthread_mutex_t mutex_ready  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_block  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cpus   = PTHREAD_MUTEX_INITIALIZER;

// PID autoincremental
static int siguiente_pid = 0;
static pthread_mutex_t mutex_pid = PTHREAD_MUTEX_INITIALIZER;

void iniciar_planificador() {
    cola_new   = queue_create();
    cola_ready = queue_create();
    cola_block = queue_create();
    lista_cpus = list_create();
    lista_ios  = list_create();
}

t_pcb* crear_pcb(int pid, char* path, int prioridad) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->prioridad = prioridad;
    pcb->estado = NEW;
    pcb->path_instrucciones = strdup(path);
    pcb->param_io = 0;
    memset(&pcb->contexto, 0, sizeof(t_contexto));
    return pcb;
}

int generar_pid() {
    pthread_mutex_lock(&mutex_pid);
    int pid = siguiente_pid++;
    pthread_mutex_unlock(&mutex_pid);
    return pid;
}

void cambiar_estado(t_pcb* pcb, t_estado nuevo_estado) {
    // Log obligatorio del enunciado
    char* estados[] = {"NEW", "READY", "EXEC", "BLOCK",
                       "SUSP_READY", "SUSP_BLOCK", "EXIT"};
    log_info(logger, "## (%d) Pasa del estado %s al estado %s",
             pcb->pid,
             estados[pcb->estado],
             estados[nuevo_estado]);
    pcb->estado = nuevo_estado;
}

void encolar_ready(t_pcb* pcb) {
    cambiar_estado(pcb, READY);
    pthread_mutex_lock(&mutex_ready);
    queue_push(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_ready);
}

t_pcb* desencolar_ready() {
    pthread_mutex_lock(&mutex_ready);
    t_pcb* pcb = queue_pop(cola_ready);
    pthread_mutex_unlock(&mutex_ready);
    return pcb;
}

void encolar_block(t_pcb* pcb) {
    cambiar_estado(pcb, BLOCK);
    pthread_mutex_lock(&mutex_block);
    queue_push(cola_block, pcb);
    pthread_mutex_unlock(&mutex_block);
}

t_cpu* buscar_cpu_libre() {
    pthread_mutex_lock(&mutex_cpus);
    t_cpu* cpu_libre = NULL;
    for (int i = 0; i < list_size(lista_cpus); i++) {
        t_cpu* cpu = list_get(lista_cpus, i);
        if (cpu->libre) {
            cpu_libre = cpu;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cpus);
    return cpu_libre;
}

t_io_registrada* buscar_io(t_tipo_io tipo) {
    for (int i = 0; i < list_size(lista_ios); i++) {
        t_io_registrada* io = list_get(lista_ios, i);
        if (io->tipo == tipo && io->libre) {
            return io;
        }
    }
    return NULL;
}