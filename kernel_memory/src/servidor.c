#include "servidor.h"
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
void identificar_modulo(t_modulo id_recibida, t_log* logger,int cliente_fd){
    switch(id_recibida){ 
        case MOD_KERNEL_SCHEDULER:{
            log_info(logger, "## Kernel Scheduler Conectado - FD del Socket: %d \n", cliente_fd);
            break;
        }
        case MOD_CPU:{
            int cpu_id;
            recv(cliente_fd, &cpu_id, sizeof(int), MSG_WAITALL);
            if (cpu_id <= 0) {
                log_error(logger, "Error recibiendo ID de CPU");
                close(cliente_fd);
                return NULL;
            }
            log_info(logger, "## CPU %d Conectada \n", cpu_id);
            break;
        }
        case MOD_MEMORY_STICK:{
            int tamanio_ms_recibido;
            t_operacion aviso= OP_NUEVO_STICK; 
            recv(cliente_fd, &tamanio_ms_recibido, sizeof(int), MSG_WAITALL);
            if (tamanio_ms_recibido <= 0) {
                log_error(logger, "Error recibiendo tamaño de Memory Stick");
                close(cliente_fd);
                return NULL;
            }
            agregar_hueco_libre(tamanio_ms_recibido, cliente_fd);
            send (fd_ks_bsod,&aviso, sizeof(t_operacion), 0);
            log_info(logger, "## Memory Stick de %d bytes Conectada \n", tamanio_ms_recibido);
            break;
        }
        case MOD_SWAP:{
            fd_SWAP = cliente_fd;
            recv(cliente_fd, &tamanio_swap, sizeof(int), MSG_WAITALL);
            if (tamanio_swap <= 0) {
                log_error(logger, "Error recibiendo tamaño de SWAP");
                close(cliente_fd);
                break;
            }
            recv(cliente_fd, &tamanio_bloque, sizeof(int), MSG_WAITALL);
            if (tamanio_bloque <= 0) {
                log_error(logger, "Error recibiendo tamaño de bloque");
                close(cliente_fd);
                break;
            }
            log_info(logger, "## SWAP Conectado\n");
            agregar_swap(tamanio_swap, tamanio_bloque);
            break;
        }
        default:
            log_warning(logger, "Módulo desconocido intentando conectarse");
            break;
    }
}

void agregar_hueco_libre(int tamanio_ms, int stick_fd) {
    t_stick* nuevo_stick = malloc (sizeof(t_stick));
    pthread_mutex_lock(&mutex_lista_huecos);
    pthread_mutex_lock(&mutex_lista_segmentos);
    if (list_is_empty(huecos_libres)) {
        if (list_is_empty(segmentos)){
        t_hueco* nuevo_hueco = malloc(sizeof(t_hueco));
        nuevo_hueco->base = 0;
        nuevo_hueco->tamanio = tamanio_ms;
        list_add(huecos_libres, nuevo_hueco);
        pthread_mutex_unlock(&mutex_lista_segmentos);
        pthread_mutex_unlock(&mutex_lista_huecos);
        return;
        }
        t_segmento* ultimo_segmento = list_get_maximum(segmentos, comparar_bases_segmento);
        t_hueco* nuevo_hueco = malloc(sizeof(t_hueco));
        nuevo_hueco->base = ultimo_segmento->base + ultimo_segmento->tamanio;
        nuevo_hueco->tamanio = tamanio_ms;
        list_add(huecos_libres, nuevo_hueco);
        pthread_mutex_unlock(&mutex_lista_segmentos);
        pthread_mutex_unlock(&mutex_lista_huecos);
        return;
    }
    if (list_is_empty(segmentos)) {
        t_hueco* ultimo_hueco = list_get_maximum(huecos_libres, comparar_bases_hueco);
        ultimo_hueco->tamanio = ultimo_hueco->tamanio + tamanio_ms;
        pthread_mutex_unlock(&mutex_lista_segmentos);
        pthread_mutex_unlock(&mutex_lista_huecos);
        return;
    }
    t_hueco* ultimo_hueco = list_get_maximum(huecos_libres, comparar_bases_hueco);
    t_segmento* ultimo_segmento = list_get_maximum(segmentos, comparar_bases_segmento);
    if (ultimo_hueco->base > ultimo_segmento->base){
        ultimo_hueco->tamanio = ultimo_hueco->tamanio + tamanio_ms;
    }
    else{
        t_hueco* nuevo_hueco = malloc(sizeof(t_hueco));
        nuevo_hueco->base = ultimo_segmento->base + ultimo_segmento->tamanio;
        nuevo_hueco->tamanio = tamanio_ms;
        list_add(huecos_libres, nuevo_hueco);
    }
    pthread_mutex_unlock(&mutex_lista_segmentos);
    pthread_mutex_unlock(&mutex_lista_huecos);
    pthread_mutex_lock(&mutex_lista_sticks);
    t_stick* ultimo_stick = list_get (sticks, list_size(sticks)-1);
    if (ultimo_stick == NULL){
        nuevo_stick -> fd_stick = stick_fd;
        nuevo_stick -> base = 0;
        nuevo_stick -> tamanio = tamanio_ms;
        list_add(sticks, nuevo_stick);
        pthread_mutex_unlock(&mutex_lista_sticks);
        return;
    }
    nuevo_stick -> fd_stick = stick_fd;
    nuevo_stick -> base = ultimo_stick -> base + ultimo_stick-> tamanio;
    nuevo_stick -> tamanio = tamanio_ms;
    list_add(sticks, nuevo_stick);
    pthread_mutex_unlock(&mutex_lista_sticks);
};

void sacar_stick(int cliente_fd) {
    pthread_mutex_lock(&mutex_lista_sticks);
    for (int i = 0; i < list_size(sticks); i++) {
        t_stick* stick = list_get(sticks, i);
        if (stick->fd_stick == cliente_fd) {
            list_remove_and_destroy_element(sticks, i, (void*)free);
            pthread_mutex_unlock(&mutex_lista_sticks);
            return;
        }
    }
    pthread_mutex_unlock(&mutex_lista_sticks);
}
void agregar_swap(int tamanio_swap, int tamanio_bloque) {
    int cantidad_bloques = tamanio_swap / tamanio_bloque;
    for (int i = 0; i < cantidad_bloques; i++) {
        t_bloque* nuevo_bloque = malloc(sizeof(t_bloque));
        nuevo_bloque->id_bloque = i;
        nuevo_bloque->proceso = -1; // -1 indica que el bloque está libre
        list_add(bloques_swap, nuevo_bloque);
    }
}
void* comparar_bases_hueco(t_hueco* h1, t_hueco* h2) {
    return h1->base >= h2->base ? h1 : h2;
}
void* comparar_bases_segmento(t_segmento* s1, t_segmento* s2) {
    return s1->base >= s2->base ? s1 : s2;
}
