#include "main.h"

// varibles para obtener valores del config
    static char* log_level_str;
    static t_log_level log_level;
    static char* ip_memoria;
    static char* puerto_memoria;
    static char* puerto_escucha;
    t_log* logger;


int main(int argc, char* argv[]) {  

    if (argc < 3) {
        printf("Uso correcto: %s <config_path> <proceso0_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* config_path =  argv[1];
    char* proceso0_path =  argv[2];

    t_config* config;

    int conexion;
    int server_fd;

    config = iniciar_config(config_path);
    obtener_configuracion(config);
   
    logger = iniciar_logger(MOD_KERNEL_SCHEDULER, log_level); 

    log_info(logger, "IP Memoria: %s", ip_memoria); 
    log_info(logger, "Puerto Memoria: %s", puerto_memoria); 
    log_info(logger, "Puerto Escucha: %s", puerto_escucha); 
    log_info(logger, "log level: %s", log_level_str); 


    conexion = crear_cliente(ip_memoria, puerto_memoria);
    if (conexion == -1) {
        log_error(logger, "No se pudo conectar a memoria");
        return EXIT_FAILURE;
    }

    if (!handshake(conexion, logger, MOD_KERNEL_SCHEDULER, MOD_KERNEL_MEMORY)) {
        log_error(logger, "Fallo el handshake con memoria");
        close(conexion);
        return EXIT_FAILURE;
    }


    server_fd = iniciar_servidor(puerto_escucha);
    if (server_fd == -1) {
    log_error(logger, "No se pudo insiciar el servidor");
    return EXIT_FAILURE;
    }
	log_info(logger, "Servidor listo para recibir clientes");
    manejar_clientes(server_fd);

    return 0;
}


void obtener_configuracion(t_config* config){
    log_level_str = obtener_string("LOG_LEVEL", config); 
    log_level = log_level_from_string(log_level_str);
	ip_memoria = obtener_string("IP_MEMORIA", config);
    puerto_memoria = obtener_string("PUERTO_MEMORIA", config);
    puerto_escucha = obtener_string("PUERTO_ESCUCHA", config);
    return;
}
