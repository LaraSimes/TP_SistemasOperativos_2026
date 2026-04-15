#ifndef CLIENT_H_
#define CLIENT_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "configuraciones.h"
#include "conexiones.h"
#include "server.h"
#include "utils/hello.h"

t_log* iniciar_logger(t_log_level nivel);
void obtener_configuracion();


#endif /* CLIENT_H_ */