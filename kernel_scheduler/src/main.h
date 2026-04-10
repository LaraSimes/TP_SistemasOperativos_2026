#ifndef CLIENT_H_
#define CLIENT_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

#include "config.h"



t_log* iniciar_logger(void);
t_config* iniciar_config(char* path);
char* obtener(char* valor, t_config* config);

#endif /* CLIENT_H_ */