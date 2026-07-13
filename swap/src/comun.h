#ifndef COMUN_SWAP
#define COMUN_SWAP

#include <commons/log.h>
#include <commons/config.h>

#include <stdio.h> 
#include <stdlib.h> 
#include "utils/general.h"
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include "utils/cliente.h"

typedef struct{
    char* ip;
    char* puerto;
    char* log_level;
    char* swap_path;
    int swap_size;
    int block_size;
} t_atributos;

extern uint32_t tamanio_bloque;
extern uint32_t tamanio_total;
extern t_log* logger;
extern int socket_km;
extern char* path_SWAP;
#endif