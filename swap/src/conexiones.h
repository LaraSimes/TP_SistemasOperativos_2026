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

extern t_log *logger;

typedef struct{
    char* ip;
    char* puerto;
    char* log_level;
    char* swap_path;
    int swap_size;
    int block_size;
} t_config_valores;
void cargar_informacion_configuracion(t_config*config, t_config_valores* valores);
int liberar(t_log *log, t_config *config, int conexion);