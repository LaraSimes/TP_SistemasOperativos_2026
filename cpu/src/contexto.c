#include "main.h"

t_contexto pedir_contexto(int fd_km, int pid, t_log* logger) {
    int op = OP_GET_CONTEXT;
    send(fd_km, &op, sizeof(int), 0);
    send(fd_km, &pid, sizeof(int), 0);

    t_contexto ctx;
    recv(fd_km, &ctx, sizeof(t_contexto), MSG_WAITALL);


    int num_segs;
    recv(fd_km, &num_segs, sizeof(int), MSG_WAITALL);

    ctx.segmentos = list_create();
    for (int i = 0; i < num_segs; i++) {
        t_segmento* s = malloc(sizeof(t_segmento));
        recv(fd_km, s, sizeof(t_segmento), MSG_WAITALL);
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