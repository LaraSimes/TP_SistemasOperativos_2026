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

typedef enum {
    OP_GET_INSTRUCCION = 1,
    OP_GET_CONTEXT     = 2,
    OP_SET_CONTEXT     = 3,
} t_operacion;

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

typedef enum {
    CONTINUAR    = 0,
    SYSCALL      = 1,
    FIN_PROCESO  = 2,
    SEG_FAULT    = 3,
} t_resultado_ciclo;

t_log * iniciar_logger (t_modulo identificador, t_log_level nivel);
t_config* iniciar_config(char* path);



#endif