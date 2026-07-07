#include "main.h"

t_contexto pedir_contexto(int fd_km, int pid, t_log* logger) {
    t_contexto ctx;
    int op = OP_GET_CONTEXT;
    int resp;
    send(fd_km, &op, sizeof(int), 0);
    send(fd_km, &pid, sizeof(int), 0);
    ssize_t bytes = recv(fd_km, &resp, sizeof(int), MSG_WAITALL);
    if (bytes <= 0) {
        log_error(logger, "Error recibiendo respuesta de KM al pedir contexto. Bytes: %zd", bytes);
        return ctx;
    }
    log_info(logger, "Respuesta de KM a OP_GET_CONTEXT: %d", resp);

    if (resp != OK) {
        log_error(logger, "KM no pudo devolver contexto. PID: %d - Respuesta: %d", pid, resp);
        return ctx;
    }
    bytes = recv(fd_km, &ctx, sizeof(t_contexto), MSG_WAITALL);
    if (bytes <= 0) {
        log_error(logger, "Error recibiendo contexto. Bytes: %zd", bytes);
        return ctx;
    }

    int num_segs;
    bytes= recv(fd_km, &num_segs, sizeof(int), MSG_WAITALL);
    if (bytes <= 0) {
        log_error(logger, "Error recibiendo cant segmentos. Bytes: %zd", bytes);
        return ctx;
    }
    log_info(logger, "Cantidad de segmentos recibida desde KM: %d", num_segs);
    ctx.segmentos = list_create();
    for (int i = 0; i < num_segs; i++) {
        t_segmento* s = malloc(sizeof(t_segmento));
        bytes= recv(fd_km, s, sizeof(t_segmento), MSG_WAITALL);
        if (bytes <= 0) {
            log_error(logger, "Error recibiendo segmento %d", i);
            free(s);
            return ctx;
        }
        list_add(ctx.segmentos, s);
    }

    log_info(logger, "## PID: %d - Contexto recibido", pid);
    return ctx;
}

void guardar_contexto(int fd_km, int pid, t_contexto* ctx, t_log* logger) {
    int op = OP_SET_CONTEXT;
    send(fd_km, &op, sizeof(int), 0);
    send(fd_km, &pid, sizeof(int), 0);
    send(fd_km, ctx, sizeof(t_contexto), 0);

    uint32_t ok;
    recv(fd_km, &ok, sizeof(uint32_t), MSG_WAITALL);


    log_info(logger, "## PID: %d - Contexto guardado", pid);
}