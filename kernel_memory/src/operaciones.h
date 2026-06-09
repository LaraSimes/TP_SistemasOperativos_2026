#ifndef OPERACIONES_H
#define OPERACIONES_H

#include "comun.h"

void manejar_operacion(int cliente_fd, t_operacion cod_op);
void crear_proceso(int pid, char* path);
void terminar_programa(int server_fd, t_log* logger, t_config* config);
uint32_t obtener_espacio_libre();
int crear_segmento(t_proceso_control* proceso, uint32_t id_segmento, int tamanio);
void* hueco_mas_chico(void* a, void* b);

#endif