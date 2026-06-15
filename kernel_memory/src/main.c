#include "main.h"

t_log* logger;
t_config* config;
t_list* procesos_activos; //funcion donde todos los hilos lo puedan ver, el hilo va a poder guardar un proceso e hilo de cpu pueda buscar ese proceso. Lista general
int pid_a_buscar;
int max_segmento;

int main(int argc, char* argv[]) 
{
    if(argc <2){
        printf("Error a la hora de pasar la ruta del archivo de configuracion\n");
        return EXIT_FAILURE;
    }
    memoria_corrupta = false;

    t_config_valores config_memoria;

    config=iniciar_config(argv[1]);

    cargar_informacion_configuracion(config, &config_memoria);

    max_segmento = config_memoria.segment_max_size;

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

void* manejar_cliente(void* arg)  {
    int cliente_fd = *(int*)arg;
    free(arg);

    t_modulo id_recibida;
    t_operacion cod_op;

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

    while (true) { // el hilo se queda activo hasta que el cliente se desconecta
        if (memoria_corrupta) {
            t_errores_KM aviso=MEMORIA_CORRUPTA;
            int aviso = send (fd_KS,&aviso, sizeof(t_errores_KM),0);
            break;
        }
        int operacion_rec = recv(cliente_fd, &cod_op, sizeof(int), MSG_WAITALL);
        if (operacion_rec <= 0) {
            log_error(logger, "Cliente desconectado del socket %d", cliente_fd);
            if (id_recibida == MOD_MEMORY_STICK) {
                sacar_stick(cliente_fd);
            }
            close(cliente_fd);
            return NULL;
        }
        if (memoria_corrupta) {
            t_errores_KM aviso=MEMORIA_CORRUPTA;
            int aviso = send (fd_KS,&aviso, sizeof(t_errores_KM),0);
            break;
        }
        manejar_operacion(cliente_fd, cod_op);
    };
    close(cliente_fd);
    return NULL;
}
