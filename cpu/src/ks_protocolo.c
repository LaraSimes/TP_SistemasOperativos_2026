#include "main.h"

void devolver_a_ks(int fd_ks, int pid, int resultado, t_contexto* ctx, int tipo_io, int param_io, t_log* logger) {
    send(fd_ks, &resultado, sizeof(int), 0);
    send(fd_ks, &pid, sizeof(int), 0);
    send(fd_ks, ctx, sizeof(t_contexto), 0);

    if (resultado == SYSCALL) {
        send(fd_ks, &tipo_io, sizeof(int), 0);
        send(fd_ks, &param_io, sizeof(int), 0);
    }

    log_info(logger, "## PID: %d - Devuelto a KS con resultado %d", pid, resultado);
}