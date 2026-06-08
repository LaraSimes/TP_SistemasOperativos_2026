#ifndef CLIENT_H_
#define CLIENT_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "configuraciones.h"
#include "server.h"
#include "utils/hello.h"
#include "utils/general.h"
#include "utils/cliente.h"
#include "planificador.h"
#include "dispatcher.h"

void obtener_configuracion();
void registrar_proceso_en_km(int pid, char* path);   

extern char* planification_algorithm;
extern int   rr_quantum;
extern int   suspension_timeout;
extern int   fd_km;
extern pthread_mutex_t mutex_km;

// CMN
extern char** queues_algorithms;
extern int    queues_count;
extern bool   queue_preemption;

#endif /* CLIENT_H_ */