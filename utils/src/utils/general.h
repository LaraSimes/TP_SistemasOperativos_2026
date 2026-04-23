#include <commons/log.h> //Biblioteca de la catedra
#include <commons/config.h>

#include <stdio.h>  // Para usar 
#include <stdlib.h> // Exit o Exit_failure

#include <signal.h> //Para usar signal()

#include <string.h> // Headers de la API
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>


typedef enum{
    MOD_KERNEL_SCHEDULER,    //0
    MOD_CPU,                //1
    MOD_MEMORY_STICK,      //2
    MOD_SWAP,              //3
    MOD_IO,                 //4
    MOD_KERNEL_MEMORY       //5
} t_modulo;

typedef enum {
	LOG_LEVEL_TRACE, //!< Loguea todos los mensajes
	LOG_LEVEL_DEBUG, //!< Loguea solo mensajes de log_debug en adelante
	LOG_LEVEL_INFO, //!< Loguea solo mensajes de log_info en adelante
	LOG_LEVEL_WARNING, //!< Loguea solo mensajes de log_warning en adelante
	LOG_LEVEL_ERROR //!< Loguea solo mensajes de log_error
} t_log_level;

t_log* iniciar_logger(t_modulo identificador, t_log_level nivel);
t_config* iniciar_config(char* path);