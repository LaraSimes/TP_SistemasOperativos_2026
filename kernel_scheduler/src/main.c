#include "main.h"

/*
1ra entrega main.c 
    Leer argumentos (config + path proceso inicial)
    Inicializar logger
    Inicializar config
    Conectarse a Kernel Memory
    Crear proceso inicial (PID 0)
    Levantar el servidor
*/

int main(int argc, char* argv[]) {  // argc = argument count (cantidad) ; argv = argument vector (los valores)

    char* config_path =  argv[1];
    char* proceso0_path =  argv[2];

    t_log* logger;
    t_config* config;
    
    int conexion;
    char* ip_memoria;
    char* puerto_memoria;
    char* puerto_escucha;

    logger = iniciar_logger();
    config = iniciar_config(config_path);

	ip_memoria = obtener("IP_MEMORIA", config);
    puerto_memoria = obtener("PUERTO_MEMORIA", config);
    puerto_escucha = obtener("PUERTO_ESCUCHA", config);

    log_info(logger, "IP Memoria: %s", ip_memoria); 
    log_info(logger, "Puerto Memoria: %s", puerto_memoria); 
    log_info(logger, "Puerto Escucha: %s", puerto_escucha); 

    return 0;
}

t_log *iniciar_logger()
{
	t_log* nuevo_logger = log_create("kernel_scheduler.log","KERNEL_SCHEDULER",true, LOG_LEVEL_INFO);
	if(nuevo_logger != NULL){
        return nuevo_logger;
    }else{
        abort();
    }
}

t_config* iniciar_config(char* path){
	t_config* nuevo_config = config_create(path);
    if(nuevo_config != NULL){
        return nuevo_config;
    }else{
        printf("Error al cargar config\n");
        abort();    
        }
}

char* obtener(char* clave, t_config* config){
    return config_get_string_value(config, clave);
}
