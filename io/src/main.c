#include <main.h>

int main(int argc, char* argv[]) {
    t_log *logger=log_create("log_IO.log", "IO",1, LOG_LEVEL_INFO);
    t_config *config = config_create("config_IO.config");
    if (config == NULL ){
	    log_debug(logger,"No se pudo crear el config");
		abort();
	};
    char *ip = config_get_string_value(config,"IP");
	char *puerto= config_get_string_value(config,"PUERTO");
	char *valor = config_get_string_value(config,"CLAVE");
    int sckt_scheduler= crear_cliente(ip,puerto);
    if (sckt_scheduler==-1){
        log_debug(logger,"No se pudo conectar el socket");
    }
    else{
        log_info(logger,"Conectado a Kernel Scheduler");
    }
    liberar(logger,config,sckt_scheduler);
}
