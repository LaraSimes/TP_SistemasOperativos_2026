#include "general.h"
#include <commons/log.h> //Biblioteca de la catedra
#include <commons/config.h>

#include <stdio.h>  // Para usar 
#include <stdlib.h> // Exit o Exit_failure

#include <signal.h> //Para usar signal()

#include <string.h> // Headers de la API
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>

t_log *iniciar_logger(t_modulo id, t_log_level nivel){
    t_log* nuevo_logger;
    switch (id){
        case 0:
        nuevo_logger = log_create("KERNEL_SCHEDULER.log","KERNEL_SCHEDULER",1, nivel);
        case 1:
        nuevo_logger = log_create("CPU.log","CPU",1, nivel);
        case 2:
        nuevo_logger = log_create("MEMORY_STICK.log","MEMORY_STICK",1, nivel);
        case 3:
        nuevo_logger = log_create("SWAP.log","SWAP",1, nivel);
        case 4:
        nuevo_logger = log_create("IO.log","IO",1, nivel);
        case 5:
        nuevo_logger = log_create("KERNEL_MEMORY.log","KERNEL_MEMORY",1, nivel);
    }
	if(nuevo_logger != NULL){
        return nuevo_logger;
    }else{
        abort();
    }
}
t_config* iniciar_config(char* path){
    t_config* config = config_create(path);
    if(!config){ //Se fija si se creo la config, o si dio null
        fprintf(stderr, "Error al leer config: %s\n",path);
        exit(EXIT_FAILURE);
    }
    return config;
}
