#ifndef GENERAL_H
#define GENERAL_H

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

t_log * iniciar_logger (t_modulo identificador, t_log_level nivel);
t_config* iniciar_config(char* path);



#endif