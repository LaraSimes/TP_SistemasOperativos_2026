#include "server.h"
/*
server.c
    Iniciar servidor
    Escuchar conexiones en loop infinito
    Crear un hilo por cada cliente
    Identificar tipo de cliente (CPU / IO)
    Loguear conexión de CPU
    Mantener conexiones activas
    Manejar desconexiones sin romper el sistema
*/

extern t_log* logger;

void atender_clientes(int server_fd){

    while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        pthread_t hilo;
        pthread_create(&hilo, NULL, atender_cliente, (void*) cliente_fd);
        pthread_detach(hilo);
    }

    return;
}

void* atender_cliente(void* arg) {
    int cliente_fd = (int) arg;

    int tipo = recibir_operacion(cliente_fd);

    switch (tipo) {
        case CPU:
            log_info(logger, "Se conectó una CPU");
            break;

        case IO:
            log_info(logger, "Se conectó un IO");
            break;

        default:
            log_warning(logger, "Cliente desconocido");
            close(cliente_fd);
            return NULL;
    }

    // mantener conexión viva
    while (1) {
        int cod_op = recibir_operacion(cliente_fd);

        if (cod_op == -1) {
            log_warning(logger, "Cliente desconectado");
            close(cliente_fd);
            return NULL;
        }

        // por ahora no se hace nada mas.
    }
}

int recibir_operacion(int socket_cliente) {
    int cod_op;

    int bytes = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);

    if (bytes <= 0) {
        return -1;
    }

    return cod_op;
}