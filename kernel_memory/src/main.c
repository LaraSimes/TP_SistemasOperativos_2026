#include <utils/hello.h>
#include "main.h"

//declaracion de punteros y funciones

int main(int argc, char* argv[]) {

t_log* logger;
t_config* config;
t_config* inciar_config(void);

    logger = log_create("kernel_memory.log","Kernel Memory", 1, LOG_LEVEL_INFO); //log de kernel memory, variable global

    config=iniciar_config();

    char* path = config_get_string_value(config, "SCRIPTS_BASEPATH");
    int delay = config_get_int_value(config, "INSTRUCTION_DELAY");

    log_info(logger, "Path leido: %s", path);
    log_info(logger, "Delay leido: %d", delay);

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