#ifndef MAIN_H_
#define MAIN_H_


#include "servidor.h"
#include "operaciones.h"

void cargar_informacion_configuracion(t_config* config, t_config_valores* valores);
void* manejar_cliente(void* arg);
char* leer_instruccion_de_archivo(char* path, uint32_t pc);
bool _es_el_proceso(void* elemento);


#endif