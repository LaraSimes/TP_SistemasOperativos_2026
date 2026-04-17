#include "conexiones.h"

int main(int argc, char* argv[]) {
    t_log *logger;
    t_config *config;

    logger= log_create("IO.log", "IO",1, LOG_LEVEL_INFO);
    config = config_create(argv[1]);
    if (config == NULL ){
	    log_debug(logger,"No se pudo crear el config");
		abort();
	};
    char *ip = config_get_string_value(config,"IP");
	char *puerto= config_get_string_value(config,"PUERTO");
    int sckt_scheduler= crear_cliente(ip,puerto);
    if (sckt_scheduler == -1) {
        log_error(logger, "No se pudo conectar a Kernel Scheduler");
        liberar(logger,config,sckt_scheduler);
        return -1;
    }
    handshakear(sckt_scheduler,logger);
    liberar(logger,config,sckt_scheduler);
    return 0;
}
