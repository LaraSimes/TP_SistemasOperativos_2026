
#ifndef SERVIDOR_H
#define SERVIDOR_H
#include "comun.h"

int iniciar_servidor(char*puerto);
int esperar_cliente(int socket_servidor);
void identificar_modulo(t_modulo id_recibida, t_log* logger);
void agregar_stick(int tamanio_ms, int cliente_fd);
void agregar_hueco_libre(int tamanio_ms);
void sacar_stick(int id_stick);

#endif