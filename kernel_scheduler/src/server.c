#include "server.h"
extern t_log* logger;

void manejar_clientes(int server_fd) {
    while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        if (cliente_fd == -1) continue; // chequeo de error

        pthread_t hilo;
        int* fd_ptr = malloc(sizeof(int));
        *fd_ptr = cliente_fd;
        pthread_create(&hilo, NULL, atender_cliente, fd_ptr);
        pthread_detach(hilo);
    }
}

void* atender_cliente(void* arg) {
    int cliente_fd = *(int*)arg;
    free(arg);

    // Handshake 
    t_modulo id;
    if (recv(cliente_fd, &id, sizeof(t_modulo), MSG_WAITALL) <= 0) {
        log_error(logger, "Error en handshake");
        close(cliente_fd);
        return NULL;
    }
    int confirmacion = 1;
    send(cliente_fd, &confirmacion, sizeof(int), 0);

    // Identificación según módulo
    int cpu_id;
    switch (id) {
        case MOD_CPU:
            recv(cliente_fd, &cpu_id, sizeof(int), MSG_WAITALL);
            log_info(logger, "## CPU %d Conectada", cpu_id);
            break;
        case MOD_IO:
            log_info(logger, "## IO Conectado");
            break;
        default:
            log_warning(logger, "Cliente desconocido");
            close(cliente_fd);
            return NULL;
    }

    // Loop de operaciones
    while (1) {
        int cod_op = recibir_operacion(cliente_fd);
        if (cod_op == -1) {
            log_warning(logger, "Cliente desconectado");
            close(cliente_fd);
            return NULL;
        }
        // manejar operaciones
    }
}

int recibir_operacion(int socket_cliente) {
    int cod_op;
    int bytes = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);
    if (bytes <= 0) return -1;
    return cod_op;
}