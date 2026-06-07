include "servidor.h"
int iniciar_servidor(char*puerto)
{
	int err;
	int socket_servidor;
	
	struct addrinfo hints;
    struct addrinfo *servinfo;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;

	err= getaddrinfo(NULL, puerto, &hints, &servinfo);
    if(err!=0){
        printf("Error en Getaddrinfo");
        return -1;
    }
	// Creamos el socket de escucha del servidor
	socket_servidor = socket(servinfo->ai_family,servinfo->ai_socktype,servinfo->ai_protocol);
    if(socket_servidor==-1){
        printf("No se pudo crear el socket");
        freeaddrinfo(servinfo);
        return -1;
    }
	// Asociamos el socket a un puerto
	err = setsockopt(socket_servidor, SOL_SOCKET, SO_REUSEPORT, &(int){1}, sizeof(int));
	err = bind(socket_servidor, servinfo->ai_addr, servinfo->ai_addrlen);
    if(err==-1){
        printf("Error en el bind");
        freeaddrinfo(servinfo);
        return -1;
    }
	// Escuchamos las conexiones entrantes
	err = listen(socket_servidor, SOMAXCONN);
    if(err==-1){
        printf("Error en el listen");
        freeaddrinfo(servinfo);
        return -1;
    }
	freeaddrinfo(servinfo);
	printf("Servidor de Memoria listo para escuchar en el puerto: %s\n", puerto);
	return socket_servidor;
}

int esperar_cliente(int socket_servidor)
{
	// Aceptamos un nuevo cliente
	int socket_cliente = accept(socket_servidor,NULL,NULL);
    if(socket_cliente==-1){
        printf("Error al aceptar el cliente");
        return -1;
    } else{
        printf("Se conecto un cliente! FD: %d", socket_cliente);
    }
	return socket_cliente;
}
void identificar_modulo(t_modulo id_recibida, t_log* logger){
    switch(id_recibida){ 
        case MOD_KERNEL_SCHEDULER:
            log_info(logger, "## Kernel Scheduler Conectado - FD del Socket: %d", cliente_fd);
            break;
        
        case MOD_CPU:
            recv(cliente_fd, &cpu_id, sizeof(int), MSG_WAITALL);
            if (bytes <= 0) {
                log_error(logger, "Error recibiendo ID de CPU");
                close(cliente_fd);
                return NULL;
            }
            log_info(logger, "## CPU %d Conectada", cpu_id);
            break;
        
        case MOD_MEMORY_STICK:{
            int tamanio_ms_recibido;
            recv(cliente_fd, &tamanio_ms_recibido, sizeof(int), MSG_WAITALL);
            log_info(logger, "## Memory Stick de %d bytes Conectada", tamanio_ms_recibido);
            break;
        }
        
        case MOD_SWAP:
            log_info(logger, "## SWAP Conectado");
            break;

        default:
            log_warning(logger, "Módulo desconocido intentando conectarse");
            break;
    }
}