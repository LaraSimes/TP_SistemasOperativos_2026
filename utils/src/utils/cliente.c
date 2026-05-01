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


// handshakear ahora retorna bool
void handshakear(int socket_cliente, t_log *log, t_modulo id) {
    int mensaje;

    if (send(socket_cliente, &id, sizeof(t_modulo), 0) == -1) {
        log_error(log, "Problema enviando handshake");
        return false;
    }

    if (recv(socket_cliente, &mensaje, sizeof(int), 0) <= 0) {
        log_error(log, "Problema recibiendo handshake");
        return false;
    }

    switch (id) {
        case MOD_KERNEL_SCHEDULER:
            log_info(log, "## Conectado a Kernel Scheduler");
            break;
        case MOD_MEMORY_STICK:
            log_info(log, "## Conectado a Memory Stick");
            break;
        case MOD_KERNEL_MEMORY:
            log_info(log, "## Conectado a Kernel Memory");
            break;
        default:
            log_warning(log, "## Módulo desconocido (id: %d)", id);
            break;
    }
    return ;
}
