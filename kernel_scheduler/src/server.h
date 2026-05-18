#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // close
#include <pthread.h>    // hilos
#include <sys/socket.h> // recv
#include <stddef.h>     // NULL

#include <commons/log.h> // t_log, log_info, log_warning
#include "utils/general.h"
#include "planificador.h"

void manejar_clientes(int server_fd, char* proceso0_path);
void* atender_cliente(void* arg);
int recibir_operacion(int socket_cliente);

 /*
   * Protocolo de comunicación Scheduler ↔ CPU ↔ IO
   *
   * CPU → Scheduler (al terminar un ciclo):                                                                    
   *   int            resultado    (t_resultado_ciclo)
   *   int            pid                                                                                       
   *   t_contexto     contexto                                                                                
   *   // solo si resultado == SYSCALL:                                                                         
   *   int            tipo_io      (t_tipo_io)                                                                
   *   int            param_io                                                                                  
   *
   * Scheduler → IO (al asignar trabajo):                                                                       
   *   int            pid                                                                                     
   *   int            param_io
   *
   * IO → Scheduler (al terminar):
   *   int            ok           (siempre 1)                                                                  
   */
       


#endif /* SERVER_H_ */