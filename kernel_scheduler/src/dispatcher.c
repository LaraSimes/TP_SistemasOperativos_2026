#include "dispatcher.h"

extern t_log* logger;

// Semaforo para despertar al despachador
sem_t sem_dispatcher;

void init_dispatcher() {
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

        if (send(cpu->fd, &pcb->pid, sizeof(int), 0) == -1) {
            log_error(logger, "Error enviando PID a CPU");
        }
    }
    return NULL;
}

void wake_dispatcher() {
    sem_post(&sem_dispatcher);
}