#include "main.h"

char* fetch(int fd_km, int pid, uint32_t pc, t_log* logger) {
    //Log obligatorio 
    log_info(logger, "## PID: %d - FETCH - Program Counter: %d", pid, pc); 
    t_rsp_KM respuesta;
    ssize_t bytes;
    //Enviar pc y pid a KM para pedir instruccion
    int op = OP_GET_INSTRUCCION;
    send(fd_km, &op, sizeof(int), 0);
    send(fd_km, &pid, sizeof(int), 0);
    send(fd_km, &pc, sizeof(int), 0);
    bytes = recv(fd_km,&respuesta,sizeof(t_rsp_KM),MSG_WAITALL);
    if (bytes <= 0) {
        log_error(logger, "Error recibiendo respuesta de KM en FETCH. Bytes: %zd", bytes);
        return NULL;
    }
    log_info(logger, "Respuesta cruda de KM en FETCH: %d", respuesta);
    //Recibir instruccion de KM
    if (respuesta != OK) {
        log_error(logger, "KM no pudo devolver instruccion. PID: %d - PC: %d - Respuesta: %d", pid, pc, respuesta);
        return NULL;
    }
    int largo;
    bytes = recv(fd_km, &largo, sizeof(int), MSG_WAITALL);
    if (bytes <= 0) {
        log_error(logger, "Error recibiendo tamanio de instruccion. Bytes: %zd", bytes);
        return NULL;
    }
    log_info(logger, "Tamanio de instruccion recibido: %d", largo);
    char* instruccion = malloc(largo + 1);
    if (instruccion == NULL) {
        log_error(logger, "malloc fallo para instruccion de largo %d", largo);
        return NULL;
    }
    bytes = recv(fd_km, instruccion, largo, MSG_WAITALL);
    if (bytes <= 0) {
        log_error(logger, "Error recibiendo instruccion. Bytes: %zd", bytes);
        free(instruccion);
        return NULL;
    }
    instruccion[largo] = '\0';
    return instruccion;  // el caller hace free()
}
