#include "cliente.h"

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


void handshakear (int socket_cliente, t_log *log, t_modulo id){
	t_modulo modulo = id;
	int mensaje;
	if (send(socket_cliente,&modulo,sizeof(t_modulo),0)==-1){
		log_debug(log,"Problema enviando handshake");
	}
	if (recv(socket_cliente,&mensaje,sizeof(int),0)<=0){
		log_debug(log,"Problema recibiendo handshake");
	}
	else{
		switch (id)
		{
		case 0:
		log_info(log,"## Conectado a Kernel Scheduler");
		case 2:
		log_info(log,"## Conectado a Memory Stick");
		case 5:
		log_info(log,"## Conectado a Kernel Memory");
		};
	}
}
