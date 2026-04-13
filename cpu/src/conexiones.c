#include main.h

t_log* iniciar_logger(char* identificador){
    char nombre_log[64];
    snprintf(nombre_log, sizeof(nombre_log),"cpu-%s.log",identificador); //arma string del log segun id que pasan x parametro
    return log_create(nombre_log, "cpu", true, LOG_LEVEL_INFO);
}

t_config* iniciar_config(char* path){
    t_config* config = config_create(path);
    if(!config){ //Se fija si se creo la config, o si dio null
        fprintf(stderr, "Error al leer config: %s\n",path);
        exit(EXIT_FAILURE);
    }
return config;
}

void cargar_informacion_configuracion(t_config* config, t_config_valores* valores, t_log* logger) {
    valores->log_level               = config_get_string_value(config, "LOG_LEVEL");
    valores->ip_kernel_scheduler     = config_get_string_value(config, "IP_KERNEL_SCHEDULER");
    valores->puerto_kernel_scheduler = config_get_string_value(config, "PUERTO_KERNEL_SCHEDULER");
    valores->ip_kernel_memory        = config_get_string_value(config, "IP_KERNEL_MEMORY");
    valores->puerto_kernel_memory    = config_get_string_value(config, "PUERTO_KERNEL_MEMORY");

log_info(logger, "Config cargada - KS: %s:%s | KM: %s:%s",
    valores->ip_kernel_scheduler, valores->puerto_kernel_scheduler,
    valores->ip_kernel_memory,    valores->puerto_kernel_memory);
}

