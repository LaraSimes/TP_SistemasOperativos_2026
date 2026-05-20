#ifndef CPU_MAIN_H_
#define CPU_MAIN_H_

#include "utils/general.h"
#include "utils/cliente.h"
#include <commons/log.h> //Biblioteca de la catedra
#include <commons/config.h>

#include <stdio.h>  // Para usar 
#include <stdlib.h> // Exit o Exit_failure

#include <signal.h> //Para usar signal()

#include <string.h> // Headers de la API
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>



typedef struct {
    char* log_level;
    char* ip_kernel_scheduler;
    char* puerto_kernel_scheduler;
    char* ip_kernel_memory;
    char* puerto_kernel_memory;
    char* ip_memory_stick;
    char* puerto_memory_stick;
} t_config_valores;


void cargar_informacion_configuracion(t_config* config, t_config_valores* valores, t_log* logger); //Recibe la config, lo mete en el struct y lo loguea 

t_contexto pedir_contexto(int fd_km, int pid, t_log* logger);
void guardar_contexto(int fd_km, int pid, t_contexto* ctx, t_log* logger);

void devolver_a_ks(int fd_ks, int pid, int resultado, t_contexto* ctx, int tipo_io, int param_io, char* nombre_mutex, t_log* logger);

int ejecutar(char* instruccion, t_contexto* ctx, t_log* logger, int pid, int* tipo_io_out, int* param_io_out, char nombre_mutex_out[32]);

char* fetch(int fd_km, int pid, uint32_t pc, t_log* logger);


uint32_t get_registro(t_contexto* ctx, char* nombre);
void set_registro(t_contexto* ctx, char* nombre, uint32_t valor);


#endif