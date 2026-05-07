#include "main.h"

t_contexto pedir_contexto(int fd_km, int pid, t_log* logger) {
    int op = OP_GET_CONTEXT;
    send(fd_km, &op, sizeof(int), 0);
    send(fd_km, &pid, sizeof(int), 0);

    t_contexto ctx;
    recv(fd_km, &ctx, sizeof(t_contexto), MSG_WAITALL);

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