#include "dispatcher.h"
#include "main.h"

extern t_log* logger;

typedef struct {
    t_cpu* cpu;                                                                    
} t_args_quantum;


// Semaforo para despertar al despachador
sem_t sem_dispatcher;

void iniciar_dispatcher() {
    sem_init(&sem_dispatcher, 0, 0);
    pthread_t hilo;
    pthread_create(&hilo, NULL, dispatcher_thread, NULL);
    pthread_detach(hilo);
}

void* dispatcher_thread(void* arg) {
    // Se duerme hasta que haya algo para despachar
    while (1) {
        sem_wait(&sem_dispatcher);

        t_pcb* pcb = desencolar_ready();
        if (pcb == NULL) continue;

        t_cpu* cpu = buscar_cpu_libre();
        if (cpu == NULL) {
            // No hay CPU libre, vuelvo a encolar
            encolar_ready(pcb);
            continue;
        }

        // Mando el PID a la CPU
        pthread_mutex_lock(&mutex_cpus);
        cpu->libre = 0;
        pthread_mutex_unlock(&mutex_cpus);

        cambiar_estado(pcb, EXEC);
        pcb->fd_cpu_dispatch = cpu->fd;

        if (send(cpu->fd, &pcb->pid, sizeof(int), 0) == -1) {
            log_error(logger, "Error enviando PID a CPU");
        }

        cpu->pcb_actual = pcb;

        if (strcmp(planification_algorithm, "RR") == 0) {
                cpu->interrupcion_pendiente = true;
                t_args_quantum* args = malloc(sizeof(t_args_quantum));
                args->cpu = cpu;
                pthread_t hilo;
                pthread_create(&hilo, NULL, thread_quantum, args);
                pthread_detach(hilo);
        }
    }
    return NULL;
}

void wake_dispatcher() {
    sem_post(&sem_dispatcher);
}

void* thread_quantum(void* arg) {
    t_args_quantum* args = arg;
    t_cpu* cpu = args->cpu;
    free(args);

    usleep(rr_quantum * 1000);

    pthread_mutex_lock(&mutex_cpus);
    if (cpu->interrupcion_pendiente && cpu->pcb_actual != NULL && cpu->fd_interrupt >= 0) {
        int interrupcion = 1;
        send(cpu->fd_interrupt, &interrupcion, sizeof(int), 0);
        cpu->interrupcion_pendiente = false;
    }
    pthread_mutex_unlock(&mutex_cpus);

    return NULL;
}

