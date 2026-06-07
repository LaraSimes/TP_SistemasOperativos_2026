#ifndef OPERACIONES_H
#define OPERACIONES_H

#include "utils/general.h"

void recibir_creacion_proceso(int socket_cliente);

void terminar_programa(int server_fd, t_log* logger, t_config* config);

uint32_t obtener_espacio_libre_mock();

void recibir_datos_memoria_mock(int socket, int* pid, uint32_t* dir, int* tam);

#endif