
#include "main.h"

    t_log* logger;
    t_config* config;

int main(int argc, char* argv[]) 
{
    t_config_valores config_memoria;

    logger = log_create("kernel_memory.log","Kernel Memory", 1, LOG_LEVEL_INFO); //log de kernel memory, variable global

    config=iniciar_config();

    cargar_informacion_configuracion(config, &config_memoria,logger);

    log_info(logger, "Estoy escuchando en el puerto de: %s", config_memoria.puerto_escucha);

    return 0;
}

t_config* iniciar_config(void) //funcion de iniciar config y se pasa el archivo donde estan los datos
{
	t_config* nuevo_config=config_create("kernel_memory.config");
	if(nuevo_config==NULL){
	printf("No se pudo encontrar el archivo de kernel_memory.config");
	abort();
	}

	return nuevo_config;
}

void cargar_informacion_configuracion(t_config*config, t_config_valores* valores, t_log* logger){
    valores->segment_max_size=config_get_int_value(config,"SEGMENT_MAX_SIZE");
    valores->instruction_delay=config_get_int_value(config,"INSTRUCTION_DELAY");
    valores->compaction_delay=config_get_int_value(config,"COMPACTION_DELAY");
    valores->log_level=config_get_string_value(config,"LOG_LEVEL");
    valores->allocation_strategy=config_get_string_value(config,"ALLOCATION_STRATEGY");
    valores->scripts_basepath=config_get_string_value(config,"SCRIPTS_BASEPATH");
    valores->puerto_escucha=config_get_string_value(config,"PUERTO_ESCUCHA");

    log_info(logger, "Configuracion cargada de forma exitosa");
}

