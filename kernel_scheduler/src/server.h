#ifndef SERVER_H_
#define SERVER_H_

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>     // close
#include <pthread.h>    // hilos
#include <sys/socket.h> // recv
#include <stddef.h>     // NULL

#include <commons/log.h> // t_log, log_info, log_warning

void atender_clientes(int server_fd);
void* atender_cliente(void* arg);
int recibir_operacion(int socket_cliente);

typedef enum {
    CPU = 1,
    IO = 2
} op_code;

#endif /* SERVER_H_ */