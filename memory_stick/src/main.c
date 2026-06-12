#include "memory_stick.h"
#include <pthread.h>

t_log *logger;
int socket_servidor;
char* puerto_escucha; //hacer que iniciar servidor reciba como parametro el puerto de escucha
t_config* config;



char* ip_kernelMemory;
char* puerto_kernelMemory;
int socket_kernelMemory;
int tamanio_memoryStick;


uint8_t*        memoria;
int             memory_delay;
pthread_mutex_t mutex_memoria = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {

    if(argc < 3){
        printf("Uso: %s [Config] [Tamanio]\n", argv[0]);
        return EXIT_FAILURE;
    }

    int tamanio = atoi(argv[2]);
    memoria = malloc(tamanio);
    memset(memoria, 0, tamanio);

   logger = iniciar_logger(MOD_MEMORY_STICK, LOG_LEVEL_INFO);
   obtenerConfig(argv[1]);

    //ser cliente de KM
   socket_kernelMemory= crear_cliente(ip_kernelMemory, puerto_kernelMemory);

   if(socket_kernelMemory == -1){
    log_error(logger, "No se pudo conectar a Kernel Memory");
    log_destroy(logger); 
    return EXIT_FAILURE;
   }
   //handshake(socket_kernelMemory,logger,MOD_KERNEL_MEMORY);
    if (!handshake(socket_kernelMemory, logger, MOD_MEMORY_STICK, MOD_KERNEL_MEMORY)) {
        log_error(logger, "Fallo el handshake con memoria");
        close(socket_kernelMemory);
        return EXIT_FAILURE;
    } 

    log_info(logger, "## Conectado a Kernel Memory");

    send(socket_kernelMemory, &tamanio, sizeof(int), 0);

   
    pthread_t hilo_km;
    int* fd_ptr = malloc(sizeof(int));
    *fd_ptr = socket_kernelMemory;
    pthread_create(&hilo_km, NULL, manejar_km, fd_ptr);
    pthread_detach(hilo_km);

    //ser servidor de CPU

   socket_servidor = iniciar_servidor_ms(puerto_escucha);
   log_info(logger, "Memory Stick listo para recibir CPU");

   // loop de CPUs — reemplazar el while actual:
    while (1) {
        int cliente_fd = esperar_cliente_ms(socket_servidor);
        if (cliente_fd != -1) {
            pthread_t hilo_cpu;
            int* fd_ptr = malloc(sizeof(int));
            *fd_ptr = cliente_fd;
            pthread_create(&hilo_cpu, NULL, (void*(*)(void*))manejar_cpu, fd_ptr);
            pthread_detach(hilo_cpu);
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
    memory_delay = config_get_int_value(config, "MEMORY_DELAY");
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

void manejar_cpu(int* arg) {

    int cliente_fd = *(int*)arg;
    free(arg);
    t_modulo modulo;
    

    recv(cliente_fd, &modulo, sizeof(int), MSG_WAITALL);

    if(modulo != MOD_CPU){
        log_warning(logger, "Cliente no es CPU");
        close(cliente_fd);
        return;
    }

    int ok = 1;
    send(cliente_fd, &ok, sizeof(int), 0);

    int cpu_id;
    recv(cliente_fd, &cpu_id, sizeof(int), MSG_WAITALL);
    log_info(logger, "## CPU %d Conectada", cpu_id);

    char buf[1];
    while (recv(cliente_fd, buf, 1, MSG_WAITALL) > 0);
    close(cliente_fd);
}

void* manejar_km(void* arg) {
    int fd = *(int*)arg;
    free(arg);

    while (1) {
        t_operacion op;
        if (recv(fd, &op, sizeof(t_operacion), MSG_WAITALL) <= 0) {
            log_warning(logger, "KM desconectado");
            break;
        }

        if (op == OP_READ_MEM) {
            int      pid;
            uint32_t dir_fisica;
            int      size;
            recv(fd, &pid,       sizeof(int),       MSG_WAITALL);
            recv(fd, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
            recv(fd, &size,      sizeof(int),       MSG_WAITALL);

            usleep(memory_delay * 1000);

            pthread_mutex_lock(&mutex_memoria);
            send(fd, memoria + dir_fisica, size, 0);
            pthread_mutex_unlock(&mutex_memoria);

            log_info(logger, "## Lectura de %d bytes", size);

        } else if (op == OP_WRITE_MEM) {
            int      pid;
            uint32_t dir_fisica;
            int      size;
            recv(fd, &pid,        sizeof(int),      MSG_WAITALL);
            recv(fd, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
            recv(fd, &size,       sizeof(int),      MSG_WAITALL);

            uint8_t* buffer = malloc(size);
            recv(fd, buffer, size, MSG_WAITALL);

            usleep(memory_delay * 1000);

            pthread_mutex_lock(&mutex_memoria);
            memcpy(memoria + dir_fisica, buffer, size);
            pthread_mutex_unlock(&mutex_memoria);

            free(buffer);

            uint32_t ok = 1;
            send(fd, &ok, sizeof(uint32_t), 0);

            log_info(logger, "## Escritura de %d bytes", size);
        }
    }

    close(fd);
    return NULL;
}