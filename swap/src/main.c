#include "conexiones.h"

int main(int argc, char* argv[]) {
    t_log *logger;
    t_config *config;
    
    logger = iniciar_logger(MOD_SWAP,LOG_LEVEL_INFO);
    config = config_create(argv[1]);
    if (config == NULL ){
	    log_debug(logger,"No se pudo crear el config");
		abort();
	};
    char *ip = config_get_string_value(config,"IP");
	char *puerto= config_get_string_value(config,"PUERTO");
    int sckt_Kmemory= crear_cliente(ip,puerto);
    if (sckt_Kmemory == -1) {
        log_error(logger, "No se pudo conectar a Kernel Memory");
        liberar(logger,config,sckt_Kmemory);
        return -1;
    }
    handshakear(sckt_Kmemory,logger);
    liberar(logger,config,sckt_Kmemory);
    return 0;
}

