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

bool _es_el_proceso(void* elemento){ // criterio de busqueda, devuelve true o false
    t_proceso_control* proceso = (t_proceso_control*) elemento;
    return proceso->pid == pid_a_buscar;
}   

t_proceso_control* buscar_proceso_por_pid(int pid){ //busco el proceso a traves de la funcion _es_el_proceso pasandole el pid_a_buscar como variable global
    pid_a_buscar = pid; 
    pthread_mutex_lock(&mutex_lista_procesos);  
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
    identificar_modulo(id_recibida, logger);

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