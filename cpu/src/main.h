#ifndef CPU_MAIN_H_
#define CPU_MAIN_H_

#include <commons/log.h> //Biblioteca de la catedra
#include <commons/config.h>

#include <stdio.h>  // Para usar 
#include <stdlib.h> // Exit o Exit_failure

#include <signal.h> //Para usar signal()

#include <string.h> // Headers de la API
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

typedef enum {
    KERNEL_SCHEDULER,  // 0
    CPU,               // 1
    MEMORY_STICK       // 2
} t_modulo; //POdria hacerlo en un conexiones.h


typedef struct {
    char* log_level;
    char* ip_kernel_scheduler;
    char* puerto_kernel_scheduler;
    char* ip_kernel_memory;
    char* puerto_kernel_memory;
} t_config_valores;


t_log* iniciar_logger(char* identificador); // Declara funcion que crea logger
t_config* iniciar_config(char* path); // Abre el archivo de config
void cargar_informacion_configuracion(t_config* config, t_config_valores* valores, t_log* logger); //Recibe la config, lo mete en el struct y lo loguea 
int conectar_a_servidor(char* ip, char* puerto); // devuelve el fd del socket
int handshake_kernel_memory(int fd, int cpu_id, t_log* logger);

#endif