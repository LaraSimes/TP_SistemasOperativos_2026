#include "conexiones.h"

int liberar(t_log *log, t_config *config, int conexion){
    log_destroy(log);
    config_destroy(config);
    if (conexion==-1){
        return 0;
    }
    close(conexion);
    return 0;
}
void handshakear (int socket_cliente, t_log *log){
	op_code modulo = IO;
	int mensaje;
	if (send(socket_cliente,&modulo,sizeof(op_code),0)==-1){
		log_debug(log,"Problema enviando handshake");
	}
	else{
		log_info(log,"## Conectado a Kernel Scheduler");
	}
}
