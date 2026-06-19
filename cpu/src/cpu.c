#include "main.h"

char* fetch(int fd_km, int pid, uint32_t pc, t_log* logger) {
    //Log obligatorio 
    log_info(logger, "## PID: %d - FETCH - Program Counter: %d", pid, pc); 
    t_rsp_KM respuesta;
    //Enviar pc y pid a KM para pedir instruccion
    int op = OP_GET_INSTRUCCION;
    send(fd_km, &op, sizeof(int), 0);
    send(fd_km, &pid, sizeof(int), 0);
    send(fd_km, &pc, sizeof(int), 0);
    recv(fd_km,&respuesta,sizeof(t_rsp_KM),MSG_WAITALL);
    //Recibir instruccion de KM
    if(respuesta==OK){
        int largo;
        recv(fd_km, &largo, sizeof(int), MSG_WAITALL);
        char* instruccion = malloc(largo + 1);
        recv(fd_km, instruccion, largo, MSG_WAITALL);
        instruccion[largo] = '\0';
        return instruccion;  // el caller hace free()
    }
}
