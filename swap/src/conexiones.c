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
	t_modulo modulo = MOD_SWAP;
	int mensaje;
	if (send(socket_cliente,&modulo,sizeof(t_modulo),0)==-1){
		log_debug(log,"Problema enviando handshake");
	}
	if (recv(socket_cliente,&mensaje,sizeof(int),0)<=0){
		log_debug(log,"Problema recibiendo handshake");
	}
	else{
		log_info(log,"## Conectado a Kernel Memory");
	}
}