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

typedef struct {
    uint32_t pc;  
    uint8_t  ax;   
    uint8_t  bx;
    uint8_t  cx;
    uint8_t  dx;
    uint32_t eax;  
    uint32_t ebx;
    uint32_t ecx;
    uint32_t edx;
    uint32_t si;   
    uint32_t di;   
} t_contexto;

void cargar_informacion_configuracion(t_config* config, t_config_valores* valores, t_log* logger); //Recibe la config, lo mete en el struct y lo loguea 

char* fetch(int fd_km, int pid, uint32_t pc, t_log* logger)

#endif