#ifndef CONFIGURACIONES_H_
#define CONFIGURACIONES_H_

#include<stdio.h>
#include<stdlib.h>
#include<commons/log.h>
#include<commons/string.h>
#include<commons/config.h>
#include<readline/readline.h>

char* obtener_string(char* valor, t_config* config);
int obtener_int(char* valor, t_config* config);


#endif /* CONFIGURACIONES_H_ */