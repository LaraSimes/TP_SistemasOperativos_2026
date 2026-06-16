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
#include <stdint.h>

typedef enum {
    IO_SLEEP,
    IO_STDIN,
    IO_STDOUT
} t_tipo_io;

t_tipo_io parsear_tipo(char *tipo);

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
    OP_GET_PROCESO     = 4, // necesita km 
    OP_GET_FREE_SPACE  = 5, // para que el ks pregunta cuanto espacio tiene km
    OP_MEM_ALLOC       = 6, // para que el ks pida crear un segmento (km)
    OP_MEM_FREE        = 7, // para que el ks pida liberar segmento (km)
    OP_READ_MEM        = 8, // para que cpu simle lectura de datos y contestar ok (km)
    OP_WRITE_MEM        = 9, // para que cpu smile escritura y contestar ok (km)
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
    t_list* segmentos; 
} t_contexto;

typedef struct {
    int id_segmento;
    uint32_t base;
    uint32_t tamanio;
} t_segmento;//tabla de segmentos por proceso


typedef enum {
    CONTINUAR    = 0,
    SYSCALL      = 1,
    FIN_PROCESO  = 2,
    SEG_FAULT    = 3,
} t_resultado_ciclo;

typedef enum {
    SYSCALL_IO_SLEEP     = 0,
    SYSCALL_IO_STDIN     = 1,
    SYSCALL_IO_STDOUT    = 2,
    SYSCALL_MUTEX_CREATE = 3,
    SYSCALL_MUTEX_LOCK   = 4,
    SYSCALL_MUTEX_UNLOCK = 5,
    SYSCALL_MEM_ALLOC    = 6,
    SYSCALL_MEM_FREE     = 7,
    SYSCALL_INIT_PROC    = 8,
} t_tipo_syscall;

typedef enum {
    OK = 0,
    FRAGMENTACION_EXT = -1,
    MEMORIA_CORRUPTA= -2,
    PID_NOT_FOUND= -3,
    SEGMENTO_EXCEDIDO= -4,
    SEGMENTO_NOT_FOUND= -5,
} t_rsp_KM;


t_log * iniciar_logger (t_modulo identificador, t_log_level nivel);
t_config* iniciar_config(char* path);



#endif