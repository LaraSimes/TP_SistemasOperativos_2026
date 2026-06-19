
#ifndef SERVIDOR_H
#define SERVIDOR_H
#include "comun.h"

int iniciar_servidor(char*puerto);
int esperar_cliente(int socket_servidor);
void identificar_modulo(t_modulo id_recibida, t_log* logger,int cliente_fd);
void agregar_hueco_libre(int tamanio_ms, int stick_fd);
void sacar_stick(int id_stick);
void* comparar_bases_hueco(t_hueco* h1, t_hueco* h2);
void* comparar_bases_segmento(t_segmento* s1, t_segmento* s2);
#endif