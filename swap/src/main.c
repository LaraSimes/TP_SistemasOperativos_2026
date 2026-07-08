#include "conexiones.h"
t_config_valores valores;
t_log *logger;

int main(int argc, char* argv[]) {
    t_config *config;
    
    logger = iniciar_logger(MOD_SWAP,LOG_LEVEL_INFO);
    config = iniciar_config(argv[1]);
    if (config == NULL ){
	    log_debug(logger,"No se pudo crear el config");
		abort();
	};
    cargar_informacion_configuracion(config, &valores);
    int sckt_km= crear_cliente(valores.ip, valores.puerto);
    if (sckt_km == -1) {
        log_error(logger, "No se pudo conectar a Kernel Memory");
        liberar(logger,config,sckt_km);
        return -1;
    }
    int conexion=handshakear(sckt_km);
    if (conexion == -1) {
        log_error(logger, "No se pudo completar el handshake con Kernel Memory");
        liberar(logger,config,sckt_km);
        return -1;
    }
    log_info(logger, "## Conectado a Kernel Memory\n");
    send(sckt_km,&valores.swap_size,sizeof(int),0);
    send(sckt_km,&valores.block_size,sizeof(int),0);
    return 0;
}

