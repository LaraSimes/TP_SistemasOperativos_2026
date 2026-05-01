#ifndef DISPATCHER_H
#define DISPATCHER_H

#include <semaphore.h>
#include <pthread.h>
#include "planificador.h"

extern sem_t sem_dispatcher;

void init_dispatcher();
void* dispatcher_thread(void* arg);
void wake_dispatcher();

#endif