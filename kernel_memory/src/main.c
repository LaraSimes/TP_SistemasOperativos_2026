#include "main.h"
    t_log* logger;
    t_config* config;
    t_list* procesos_activos; //funcion donde todos los hilos lo puedan ver, el hilo va a poder guardar un proceso e hilo de cpu pueda buscar ese proceso. Lista general
    int pid_a_buscar;

int main(int argc, char* argv[]) 
{
    if(argc <2){
        printf("Error a la hora de pasar la ruta del archivo de configuracion\n");
        return EXIT_FAILURE;
    }

    t_config_valores config_memoria;

    config=iniciar_config(argv[1]);

    cargar_informacion_configuracion(config, &config_memoria);

    t_log_level nivel_log = log_level_from_string(config_memoria.log_level);

    logger=iniciar_logger(MOD_KERNEL_MEMORY,nivel_log);

    int server_fd=iniciar_servidor(config_memoria.puerto_escucha); // donde se inicia el servidor

    if(server_fd==-1){
        log_error(logger, "Fallo al iniciar el servidor"); //analizamos si falla el sv
        return -1;
    }

    log_info(logger, "Estoy escuchando en el puerto: %s", config_memoria.puerto_escucha); //ejemplo

    procesos_activos=list_create(); //funcion que pide pedazo de memoria al ssoo y se van a guardar los procesos
    pthread_mutex_init(&mutex_lista_procesos, NULL);

    while(true){ //logica de esperar clientes
        int cliente_fd=esperar_cliente(server_fd);

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

void cargar_informacion_configuracion(t_config*config, t_config_valores* valores){
    valores->segment_max_size=config_get_int_value(config,"SEGMENT_MAX_SIZE");
    valores->instruction_delay=config_get_int_value(config,"INSTRUCTION_DELAY");
    valores->compaction_delay=config_get_int_value(config,"COMPACTION_DELAY");
    valores->log_level=config_get_string_value(config,"LOG_LEVEL");
    valores->allocation_strategy=config_get_string_value(config,"ALLOCATION_STRATEGY");
    valores->scripts_basepath=config_get_string_value(config,"SCRIPTS_BASEPATH");
    valores->puerto_escucha=config_get_string_value(config,"PUERTO_ESCUCHA");
}

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

bool _es_el_proceso(void* elemento){ // criterio de busqueda, devuelve true o false
        t_proceso_control* proceso = (t_proceso_control*) elemento;
        return proceso->pid == pid_a_buscar;
    }   

t_proceso_control* buscar_proceso_por_pid(int pid){ //busco el proceso a traves de la funcion _es_el_proceso pasandole el pid_a_buscar como variable global
    pthread_mutex_lock(&mutex_lista_procesos);
    pid_a_buscar = pid;   
    t_proceso_control* resultado = list_find(procesos_activos, _es_el_proceso);
    pthread_mutex_unlock(&mutex_lista_procesos);
    return resultado;
}

char* leer_instruccion_de_archivo(char* path, uint32_t pc) {
    FILE* archivo = fopen(path, "r");
    if (archivo == NULL){
        return NULL;
    }
    char* linea = malloc(128); //espacio suficiente para una instruccion
    uint32_t renglon_actual = 0; //contador

    //recorro el archivo renglon por renglon
    while(fgets(linea, 128, archivo) != NULL) { //leo una linea completa de un archivo txt
        if(renglon_actual == pc) { //encontramos el pc buscado, se saca el \n del final si existe
            linea[strcspn(linea, "\n")] = '\0';
            fclose(archivo);
            return linea; //buffer donde se va a guardar lo que leyo
        }
        renglon_actual ++;
    }
    fclose(archivo);
    free(linea);
    return NULL;
}

void* manejar_cliente(void* arg) {
    int cliente_fd = *(int*)arg;
    free(arg);

    t_modulo id_recibida;
    int cpu_id;

    // El handhsake se hace solo 1 vez al principio de la conexion
    int bytes = recv(cliente_fd, &id_recibida, sizeof(int), MSG_WAITALL);

    if (bytes <= 0) {
        log_error(logger, "Error en handshake");
        close(cliente_fd);
        return NULL;
    }

    int recibido = 0;
    send(cliente_fd, &recibido, sizeof(int), 0);

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

    while (true) { // Donde el hilo se queda activo hasta que cliente se vaya
         t_operacion cod_op; // creo variable cod op donde me va a indicar cual es la operacion a realizar gracias al switch. el op_code se declaro en el conexiones.h

        int res= recv(cliente_fd, &cod_op, sizeof(t_operacion), MSG_WAITALL); //recibo la proxima operacion

        if(res <= 0){ // si da menor que 0, es porque el cliente se desconecto, se corta el while por el break
            log_info(logger, "Cliente desconectado del socket %d", cliente_fd);
            break;
        }

        switch(id_recibida){

            case MOD_KERNEL_SCHEDULER: {
                if(cod_op==OP_GET_PROCESO){
                    recibir_creacion_proceso(cliente_fd);
                }
                else if(cod_op == OP_GET_FREE_SPACE){
                    uint32_t espacio = obtener_espacio_libre_mock();
                    send(cliente_fd, &espacio, sizeof(uint32_t), 0);
                    log_info(logger, "## Espacio libre solicitado por Kernel: %u bytes", espacio);
                }
                else if (cod_op == OP_MEM_ALLOC) {
                    int pid, tamanio;
                    uint32_t id_segmento;
                    recibir_datos_memoria_mock(cliente_fd, &pid, &id_segmento, &tamanio);
                    log_info(logger, "## PID: %d - Segmento Creado %d - Tamaño: %d", pid, id_segmento, tamanio);
        
                    uint32_t ok = 0;
                    send(cliente_fd, &ok, sizeof(uint32_t), 0);
                }
                else if(cod_op == OP_MEM_FREE){
                    uint32_t ok = 0;
                    send(cliente_fd, &ok, sizeof(uint32_t), 0);
                }
                else if(cod_op == OP_READ_MEM){
                    int pid;
                    uint32_t dir;
                    int tam;

                    recibir_datos_memoria_mock(cliente_fd, &pid, &dir, &tam);

                    log_info(logger,"## PID: %d - Lectura (KS) - Dir. Fisica: %u - Tamaño: %d", pid, dir, tam);

                    uint32_t ok = 0;
                    send(cliente_fd, &ok, sizeof(uint32_t), 0);

                    char* bytes_vacios = calloc(1, tam); // es como el malloc pero lo limpia llenadolo de ceros \0
                    send(cliente_fd, bytes_vacios, tam, 0);
                    free(bytes_vacios);
                }
                else if(cod_op == OP_WRITE_MEM){
                    int pid;
                    uint32_t dir;
                    int tam;

                    recibir_datos_memoria_mock(cliente_fd, &pid, &dir, &tam);

                    char* bytes = malloc(tam); 
                    recv(cliente_fd, bytes, tam, MSG_WAITALL);
                    free(bytes);

                    log_info(logger,"## PID: %d - Escritura (KS) - Dir. Fisica: %u - Tamaño: %d", pid, dir, tam);

                    uint32_t ok = 0;
                    send(cliente_fd, &ok, sizeof(uint32_t), 0);
                }
                break;
            }

            case MOD_CPU:{
                if (cod_op == OP_GET_INSTRUCCION){
                    int pid_pedido;
                    uint32_t pc_pedido;

                    //recibo los datos de la CPU
                    int res_pid = recv(cliente_fd, &pid_pedido, sizeof(int), MSG_WAITALL);
                    int res_pc = recv(cliente_fd, &pc_pedido, sizeof(uint32_t), MSG_WAITALL);

                    if(res_pid <= 0 || res_pc <= 0) {
                        log_error(logger, "Error al recibir pedido de instruccion");
                        break;
                    }

                    //busco el proceso con la funcion definida antes
                    t_proceso_control* proceso_encontrado = buscar_proceso_por_pid(pid_pedido);

                    if(proceso_encontrado != NULL) {
                        char* instruccion_texto = leer_instruccion_de_archivo(proceso_encontrado->path_instrucciones, pc_pedido);
                    
                        if(instruccion_texto != NULL){

                            int tam = strlen(instruccion_texto) + 1; // saco el tamaño de la instruccion
                            send(cliente_fd, &tam, sizeof(int), 0); //envio el tamaño
                            send(cliente_fd, instruccion_texto, tam, 0); //envio la instruccion

                            log_info(logger, "## PID: %d - Obtener instruccion: %d - Instruccion: %s", pid_pedido, pc_pedido, instruccion_texto);
                            free (instruccion_texto);
                        } else{
                            log_error(logger, "No se encontro la instruccion para el PC %d", pc_pedido);
                            send(cliente_fd, &(int){-1}, sizeof(int), 0); //envio -1 para indicar que no se encontro la instruccion
                        }
                    
                    } else{
                        log_error(logger, "Nose encontro el PID %d en la memoria", pid_pedido);
                        send(cliente_fd, &(int){-1}, sizeof(int), 0); //envio -1 para indicar que no se encontro el proceso
                    }   
                } 
                else if (cod_op == OP_GET_CONTEXT){ 
                    int pid_pedido;
                    if(recv(cliente_fd, &pid_pedido, sizeof(int), MSG_WAITALL) > 0 ){ 
                        t_proceso_control* proceso_a_buscar = buscar_proceso_por_pid(pid_pedido);

                        if(proceso_a_buscar != NULL){
                            send(cliente_fd, &(proceso_a_buscar->contexto), sizeof(t_contexto), 0);
                            log_info(logger, "PID: %d - Contexto enviado a CPU", pid_pedido);
                        } else{
                            log_error(logger, "PID: %d no encontrado para enviar contexto", pid_pedido);
                        }
                    }
                } 
                else if (cod_op == OP_SET_CONTEXT){
                    int pid_pedido;
                    t_contexto nuevo_contexto;

                    if(recv(cliente_fd, &pid_pedido, sizeof(int), MSG_WAITALL) > 0 && recv(cliente_fd, &nuevo_contexto, sizeof(t_contexto), MSG_WAITALL) > 0){
                        t_proceso_control* proceso_a_buscar = buscar_proceso_por_pid(pid_pedido);
                        if(proceso_a_buscar != NULL){
                            proceso_a_buscar->contexto = nuevo_contexto;

                            uint32_t ok = 0;
                            send(cliente_fd, &ok, sizeof(uint32_t), 0);
                            log_info(logger, "PID: %d Contexto actualizado (Mock OK)", pid_pedido);
                        }
                    }
                }
                else if(cod_op == OP_READ_MEM){
                    int pid;
                    uint32_t dir;
                    int tam;

                    recibir_datos_memoria_mock(cliente_fd, &pid, &dir, &tam);

                    log_info(logger,"## PID: %d - Lectura - Dir. Fisica: %u - Tamaño: %d", pid, dir, tam);

                    uint32_t ok = 0;
                    send(cliente_fd, &ok, sizeof(uint32_t), 0);
                }
                else if(cod_op == OP_WRITE_MEM){
                    int pid; 
                    uint32_t dir; 
                    int tam;

                    recibir_datos_memoria_mock(cliente_fd, &pid, &dir, &tam);

                    log_info(logger, "## PID: %d - Escritura - Dir. Fisica: %u - Tamaño: %d", pid, dir, tam);

                    uint32_t ok = 0;
                    send(cliente_fd, &ok, sizeof(uint32_t), 0);
                }


                else{
                    log_warning(logger, "Operacion desconocida de CPU");
                }
                    break;
            }
            case MOD_MEMORY_STICK:
                break;

            case MOD_SWAP:
                break;

            default:
                break;
        }   
    }
    close(cliente_fd);
    return NULL;
}

void recibir_creacion_proceso(int socket_cliente){
    int pid_recibido;
    int tam_path;

    recv(socket_cliente, &pid_recibido, sizeof(int), MSG_WAITALL); //recibo el pid
    recv(socket_cliente, &tam_path, sizeof(int), MSG_WAITALL); //recibo el tamaño del path para saber cuanto espacio pedir
    
    char* path_recibido= malloc(tam_path);
    recv(socket_cliente, path_recibido, tam_path, MSG_WAITALL);
    
    // creo el proceso en la lista
    t_proceso_control* nuevo_proceso = malloc(sizeof(t_proceso_control)); 
    nuevo_proceso->pid = pid_recibido;
    nuevo_proceso->path_instrucciones = path_recibido;
    //inicializamos todos los registros en 0
    nuevo_proceso->contexto.pc = 0;
    nuevo_proceso->contexto.ax = 0;
    nuevo_proceso->contexto.bx = 0;
    nuevo_proceso->contexto.cx = 0;
    nuevo_proceso->contexto.dx = 0;

    nuevo_proceso->contexto.eax = 0;
    nuevo_proceso->contexto.ebx = 0;
    nuevo_proceso->contexto.ecx = 0;
    nuevo_proceso->contexto.edx = 0;

    nuevo_proceso->contexto.si = 0;
    nuevo_proceso->contexto.di = 0;

    //guardar el proceso creado en una lista global ya que esta fuera del main.c
    //guarda punteros a las estructuras de t_proceso_control declarado en el main.h
    pthread_mutex_lock(&mutex_lista_procesos);
    list_add(procesos_activos, nuevo_proceso);
    pthread_mutex_unlock(&mutex_lista_procesos);
    int ok = 0;
    send(socket_cliente, &ok, sizeof(int), 0);
    log_info(logger,"## PID: %d - Proceso Creado", pid_recibido);

}

void terminar_programa(int server_fd, t_log* logger, t_config* config)
{
        list_destroy_and_destroy_elements(procesos_activos, (void*)free);
        pthread_mutex_destroy(&mutex_lista_procesos);
		log_destroy(logger);
		config_destroy(config);	  
		close(server_fd);
}

uint32_t obtener_espacio_libre_mock() {
    return 1024; // Valor fijo solicitado para el Check 2[cite: 1]
}

void recibir_datos_memoria_mock(int socket, int* pid, uint32_t* dir, int* tam) {
    recv(socket, pid, sizeof(int), MSG_WAITALL);
    recv(socket, dir, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, tam, sizeof(int), MSG_WAITALL);
}
