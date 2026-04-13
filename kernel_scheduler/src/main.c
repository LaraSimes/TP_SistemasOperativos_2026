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
    static char* log_level_str;
    static t_log_level log_level;
    static char* ip_memoria;
    static char* puerto_memoria;
    static char* puerto_escucha;

int main(int argc, char* argv[]) {  // argc = argument count (cantidad) ; argv = argument vector (los valores)

    char* config_path =  argv[1];
    char* proceso0_path =  argv[2];

    t_log* logger;
    t_config* config;
    int conexion;

    config = iniciar_config(config_path);
    obtener_configuracion(config);
   
    logger = iniciar_logger(log_level);

    log_info(logger, "IP Memoria: %s", ip_memoria); 
    log_info(logger, "Puerto Memoria: %s", puerto_memoria); 
    log_info(logger, "Puerto Escucha: %s", puerto_escucha); 
    log_info(logger, "log level: %s", log_level_str); 


    conexion = crear_conexion(ip_memoria, puerto_memoria);


    return 0;
}

t_log *iniciar_logger(t_log_level nivel)
{
	t_log* nuevo_logger = log_create("kernel_scheduler.log","KERNEL_SCHEDULER",true, nivel);
	if(nuevo_logger != NULL){
        return nuevo_logger;
    }else{
        abort();
    }
}

void obtener_configuracion(t_config* config){
    log_level_str = obtener_string("LOG_LEVEL", config); 
    log_level = log_level_from_string(log_level_str);
	ip_memoria = obtener_string("IP_MEMORIA", config);
    puerto_memoria = obtener_string("PUERTO_MEMORIA", config);
    puerto_escucha = obtener_string("PUERTO_ESCUCHA", config);

    return;
}
