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
#include "main.h"

void manejar_clientes(int server_fd, char* proceso0_path);
void* atender_cliente(void* arg);
int recibir_operacion(int socket_cliente);
void enviar_trabajo_io(t_io_registrada* io, t_pcb* pcb);

       


#endif /* SERVER_H_ */