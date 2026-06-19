#include "main.h"

t_config* config;
t_list* procesos_activos; //funcion donde todos los hilos lo puedan ver, el hilo va a poder guardar un proceso e hilo de cpu pueda buscar ese proceso. Lista general
int pid_a_buscar;
uint32_t max_segmento;
int fd_ks_bsod;

int main(int argc, char* argv[]) {
    if(argc <2){
        printf("Error a la hora de pasar la ruta del archivo de configuracion\n");
        return EXIT_FAILURE;
    }

    t_config_valores config_memoria;

    config=iniciar_config(argv[1]);

    cargar_informacion_configuracion(config, &config_memoria);

    max_segmento =(uint32_t*) config_memoria.segment_max_size;

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

    char* ip_ks = config_memoria.ip_ks;
    char* puerto_ks = config_memoria.puerto_ks;

    fd_ks_bsod = crear_cliente(ip_ks, puerto_ks);
    handshake(fd_ks_bsod, logger, MOD_KERNEL_MEMORY, MOD_KERNEL_SCHEDULER);


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
    valores->puerto_ks=config_get_string_value(config,"PUERTO_KS");
    valores->ip_ks=config_get_string_value(config,"IP_KS");
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
    identificar_modulo(id_recibida, logger,cliente_fd);

    while (true) { // el hilo se queda activo hasta que el cliente se desconecta
        int operacion_rec = recv(cliente_fd, &cod_op, sizeof(int), MSG_WAITALL);
        if (operacion_rec <= 0) {
            log_error(logger, "Cliente desconectado del socket %d", cliente_fd);
            if (id_recibida == MOD_MEMORY_STICK) {
                sacar_stick(cliente_fd);
                int op= OP_BSOD;
                send(fd_ks_bsod,&op,sizeof(int),0);
            }
            close(cliente_fd);
            return NULL;
        }
        manejar_operacion(cliente_fd, cod_op);
    };
    close(cliente_fd);
    return NULL;
}
bool _es_el_proceso(void* elemento){ // criterio de busqueda, devuelve true o false
    t_proceso_control* proceso = (t_proceso_control*) elemento;
    return proceso->pid == pid_a_buscar;
}