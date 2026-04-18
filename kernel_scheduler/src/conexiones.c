#include "conexiones.h"

/*
conexiones.c
    Crear conexión a Kernel Memory // HECHO en utils
    Funciones de envío/recepción básicas
    Manejo de operaciones recibidas
    log obligatorio -> HECHO 
*/

void handshakear (int socket_cliente, t_log *log){
	t_modulo modulo = MOD_KERNEL_SCHEDULER;
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

