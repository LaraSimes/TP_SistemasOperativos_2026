#include "main.h"

    t_log* logger;
    t_config* config;

int main(int argc, char* argv[]) 
{
    int cpu_id;
    t_config_valores config_memoria;

    config=iniciar_config();

    cargar_informacion_configuracion(config, &config_memoria);

    logger = log_create("kernel_memory.log","Kernel Memory", 1, config_memoria.log_level); //log de kernel memory, variable global

    log_info(logger, "Estoy escuchando en el puerto de: %s", config_memoria.puerto_escucha); //ejemplo

    log_info(logger, "## Memory Stick de %d bytes Conectada", config_memoria.segment_max_size); //log obligatorio

    int server_fd=iniciar_servidor(logger, config_memoria.puerto_escucha); // donde se inicia el servidor

    if(server_fd==-1){
        log_error(logger, "Fallo al iniciar el servidor"); //analizamos si falla el sv
        return -1;
    }

    while(true){ //logica de esperar clientes
        int cliente_fd=esperar_cliente(logger, server_fd);

        if(cliente_fd!=-1){
            pthread_t hilo;

            int* fd_ptr = malloc(sizeof(int));
            *fd_ptr = cliente_fd;

            pthread_create(&hilo, NULL, manejar_cliente, fd_ptr);
            pthread_detach(hilo);
        }
    }

    terminar_programa(server_fd,logger,config);

    return 0;
}

t_config* iniciar_config(void) //funcion de iniciar config y se pasa el archivo donde estan los datos
{
	t_config* nuevo_config=config_create("kernel_memory.config");
	if(nuevo_config==NULL){
	printf("No se pudo encontrar el archivo de kernel_memory.config");
	return NULL;
	}

	return nuevo_config;
}

void cargar_informacion_configuracion(t_config*config, t_config_valores* valores){
    valores->segment_max_size=config_get_int_value(config,"SEGMENT_MAX_SIZE");
    valores->instruction_delay=config_get_int_value(config,"INSTRUCTION_DELAY");
    valores->compaction_delay=config_get_int_value(config,"COMPACTION_DELAY");
    valores->log_level=config_get_string_value(config,"LOG_LEVEL");
    valores->allocation_strategy=config_get_string_value(config,"ALLOCATION_STRATEGY");
    valores->scripts_basepath=config_get_string_value(config,"SCRIPTS_BASEPATH");
    valores->puerto_escucha=config_get_string_value(config,"PUERTO_ESCUCHA");

}

//paso b, fijarse

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
	printf("Servidor de Memoria listo para escuchar en el puerto: %s", puerto);

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

void* manejar_cliente(void* arg) {

    int cliente_fd = *(int*)arg;
    free(arg);

    t_modulo id_recibida;
    int cpu_id;

    int bytes = recv(cliente_fd, &id_recibida, sizeof(int), MSG_WAITALL);

    if (bytes <= 0) {
        log_error(logger, "Error en handshake");
        close(cliente_fd);
        return NULL;
    }

    int recibido = 0;
    send(cliente_fd, &recibido, sizeof(int), 0);

    switch(id_recibida){

        case KERNEL_SCHEDULER:
            log_info(logger, "## Kernel Scheduler Conectado - FD del socket: %d", cliente_fd);
            break;

        case CPU:
            recv(cliente_fd, &cpu_id, sizeof(int), MSG_WAITALL);
            log_info(logger, "## CPU %d Conectada", cpu_id);
            break;

        case MEMORY_STICK:
            log_info(logger, "## Memory Stick Conectado"); //falta poner tamaño 
            break;

        case SWAP:
            log_info(logger, "## SWAP Conectado");
            break;

        default:
            log_warning(logger, "Cliente desconocido");
            close(cliente_fd);
            return NULL;
    }

    close(cliente_fd);
    return NULL;
}

void terminar_programa(int server_fd, t_log* logger, t_config* config)
{
	/* Y por ultimo, hay que liberar lo que utilizamos (conexion, log y config) 
	  con las funciones de las commons y del TP mencionadas en el enunciado */
		log_destroy(logger);
		config_destroy(config);	  
		close(server_fd);
}
