#include "memory_stick.h"


t_log *logger;
int socket_servidor;
char* puerto_escucha; //hacer que iniciar servidor reciba como parametro el puerto de escucha
t_config* config;



char* ip_kernelMemory;
char* puerto_kernelMemory;
int socket_kernelMemory;
int tamanio_memoryStick;




int main(int argc, char* argv[]) {

    if(argc <2){
        printf("Error a la hora de pasar la ruta del archivo de configuracion\n");
        return EXIT_FAILURE;
    }

   logger = iniciar_logger(MOD_MEMORY_STICK, LOG_LEVEL_INFO);
   obtenerConfig(argv[1]);

    //ser cliente de KM
   socket_kernelMemory= crear_cliente(ip_kernelMemory, puerto_kernelMemory);

   if(socket_kernelMemory == -1){
    log_error(logger, "No se pudo conectar a Kernel Memory");
    log_destroy(logger); 
    return EXIT_FAILURE;
   }
   handshakear(socket_kernelMemory,logger,MOD_KERNEL_MEMORY);


   send(socket_kernelMemory, &tamanio_memoryStick, sizeof(int), 0);


    //ser servidor de CPU

   socket_servidor = iniciar_servidor_ms(puerto_escucha);
   log_info(logger, "Memory Stick listo para recibir CPU");

   while(1){
    int cliente_fd = esperar_cliente_ms(socket_servidor);

    if(cliente_fd != -1){
        manejar_cpu(cliente_fd);
    }
}



   log_destroy(logger);
   config_destroy(config);


   return 0;
}


void obtenerConfig(char* ruta){
    t_config* config = iniciar_config(ruta);
    if (config == NULL) {
       // Usamos printf porque si el logger falla también, no verías nada
       printf("ERROR: No se pudo encontrar el archivo memory_stick.config en la ruta actual.\n");
       printf("Ruta actual de ejecución: ");
       system("pwd"); // Esto te dirá dónde está parado el programa realmente
       exit(EXIT_FAILURE);
    }

    puerto_escucha = config_get_string_value(config, "PUERTO_ESCUCHA");

    ip_kernelMemory = config_get_string_value(config, "IP_KERNELMEMORY");
    puerto_kernelMemory = config_get_string_value(config, "PUERTO_KERNELMEMORY");
    tamanio_memoryStick = config_get_int_value(config, "TAMANIO_MEMORY_STICK");
}

int iniciar_servidor_ms(char*puerto)
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
	printf("Servidor de MemoryStick listo para escuchar en el puerto: %s\n", puerto);

	return socket_servidor;
}

int esperar_cliente_ms(int socket_servidor)
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

void manejar_cpu(int cliente_fd) {

    t_modulo modulo;
    int cpu_id;

    recv(cliente_fd, &modulo, sizeof(int), MSG_WAITALL);

    if(modulo != MOD_CPU){
        log_warning(logger, "Cliente no es CPU");
        close(cliente_fd);
        return;
    }

    recv(cliente_fd, &cpu_id, sizeof(int), MSG_WAITALL);

    log_info(logger, "## CPU %d Conectada", cpu_id);

    int ok = 0;
    send(cliente_fd, &ok, sizeof(int), 0);

    close(cliente_fd);
}