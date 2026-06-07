
#ifndef SERVIDOR_H
#define SERVIDOR_H
#include "utils/general.h"

int iniciar_servidor(char*puerto);
int esperar_cliente(int socket_servidor);
void identificar_modulo(t_modulo id_recibida, t_log* logger);

#endif