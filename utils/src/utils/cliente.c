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


bool handshake(int socket, t_log *log, t_modulo quien_soy, t_modulo a_quien_me_conecto) {
    
    // Enviás quién sos vos
    if (send(socket, &quien_soy, sizeof(t_modulo), 0) == -1) {
        log_error(log, "Problema enviando handshake");
        return false;
    }

    int mensaje;
    if (recv(socket, &mensaje, sizeof(int), 0) <= 0) {
        log_error(log, "Problema recibiendo handshake");
        return false;
    }

    // Logueás a quién te conectaste
    switch (a_quien_me_conecto) {
        case MOD_KERNEL_SCHEDULER:
            log_info(log, "## Conectado a Kernel Scheduler");
            break;
        case MOD_KERNEL_MEMORY:
            log_info(log, "## Conectado a Kernel Memory");
            break;
        case MOD_MEMORY_STICK:
            log_info(log, "## Conectado a Memory Stick");
            break;
        default:
            log_warning(log, "## Módulo desconocido (id: %d)", a_quien_me_conecto);
            break;
    }
    return true;
}