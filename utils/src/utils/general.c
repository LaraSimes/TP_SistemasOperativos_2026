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
        t_log* nuevo_logger = log_create("KERNEL_SCHEDULER.log","KERNEL_SCHEDULER",1, nivel);
        case 1:
        t_log* nuevo_logger = log_create("CPU.log","CPU",1, nivel);
        case 2:
        t_log* nuevo_logger = log_create("MEMORY_STICK.log","MEMORY_STICK",1, nivel);
        case 3:
        t_log* nuevo_logger = log_create("SWAP.log","SWAP",1, nivel);
        case 4:
        t_log* nuevo_logger = log_create("IO.log","IO",1, nivel);
        case 5:
        t_log* nuevo_logger = log_create("KERNEL_MEMORY.log","KERNEL_MEMORY",1, nivel);
    }
	if(nuevo_logger != NULL){
        return nuevo_logger;
    }else{
        abort();
    }
}
