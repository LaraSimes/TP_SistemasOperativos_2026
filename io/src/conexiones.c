#include "conexiones.h"


int crear_cliente(char *ip, char* puerto){
	struct addrinfo hints;
	struct addrinfo *server_info;
	int err;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	err= getaddrinfo(ip, puerto, &hints, &server_info);

	int socket_cliente = socket(server_info-> ai_family,server_info->ai_socktype, server_info->ai_protocol);

	err = connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen);
    if (err==-1){
        close(socket_cliente);
        socket_cliente=-1;
    }
	freeaddrinfo(server_info);

	return socket_cliente;
}

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
		log_info(log,"Conectado a Kernel Scheduler");
	}
}
