#include "main.h"

// varibles para obtener valores del config
    static char* log_level_str;
    static t_log_level log_level;
    static char* ip_memoria;
    static char* puerto_memoria;
    static char* puerto_escucha;
    static char* puerto_interrupt;
    t_log* logger;

    char* planification_algorithm;
    int   rr_quantum;
    int   suspension_timeout;

    char** queues_algorithms = NULL;
    int    queues_count      = 0;
    bool   queue_preemption  = false;

    int fd_km;
    pthread_mutex_t mutex_km = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char* argv[]) {  

    if (argc < 3) {
        printf("Uso correcto: %s <config_path> <proceso0_path>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char* config_path =  argv[1];
    char* proceso0_path =  argv[2];

    t_config* config;

    int server_fd;

    config = iniciar_config(config_path);
    obtener_configuracion(config);
   
    logger = iniciar_logger(MOD_KERNEL_SCHEDULER, log_level); 

    log_info(logger, "IP Memoria: %s", ip_memoria); 
    log_info(logger, "Puerto Memoria: %s", puerto_memoria); 
    log_info(logger, "Puerto Escucha: %s", puerto_escucha); 
    log_info(logger, "log level: %s", log_level_str); 

    // Conexion a Memoria
    fd_km = crear_cliente(ip_memoria, puerto_memoria);
    if (fd_km == -1) {
        log_error(logger, "No se pudo conectar a memoria");
        return EXIT_FAILURE;
    }
    // Handshake con Memoria
    if (!handshake(fd_km, logger, MOD_KERNEL_SCHEDULER, MOD_KERNEL_MEMORY)) {
        log_error(logger, "Fallo el handshake con memoria");
        close(fd_km);
        return EXIT_FAILURE;
    }
    // Levanta servidor multihilos
    server_fd = iniciar_servidor(puerto_escucha);
    if (server_fd == -1) {
    log_error(logger, "No se pudo insiciar el servidor");
    return EXIT_FAILURE;
    }
	log_info(logger, "Servidor listo para recibir clientes");
    int conectar = KS_SERVER_OK;
    send(fd_km, &conectar, sizeof(int), 0);

    int server_interrupt_fd = iniciar_servidor(puerto_interrupt);  
    if (server_interrupt_fd == -1) {                               
        log_error(logger, "No se pudo iniciar el servidor de interrupciones");
        return EXIT_FAILURE;
    }
    log_info(logger, "Servidor de interrupciones listo en puerto %s", puerto_interrupt);

    pthread_t hilo_interrupt;                                      
    pthread_create(&hilo_interrupt, NULL, manejar_interrupciones, &server_interrupt_fd);
    pthread_detach(hilo_interrupt);


    iniciar_planificador();
    iniciar_dispatcher();

    manejar_clientes(server_fd, proceso0_path);

    return 0;
}


void obtener_configuracion(t_config* config){
    log_level_str = obtener_string("LOG_LEVEL", config); 
    log_level = log_level_from_string(log_level_str);
	ip_memoria = obtener_string("IP_MEMORIA", config);
    puerto_memoria = obtener_string("PUERTO_MEMORIA", config);
    puerto_escucha = obtener_string("PUERTO_ESCUCHA", config);
    planification_algorithm = config_get_string_value(config, "PLANIFICATION_ALGORITHM");
    rr_quantum = config_get_int_value(config, "RR_QUANTUM");
    suspension_timeout = config_get_int_value(config, "SUSPENSION_TIMEOUT");
    puerto_interrupt = obtener_string("PUERTO_INTERRUPT", config);

    if (strcmp(planification_algorithm, "CMN") == 0) {
        queues_algorithms = config_get_array_value(config, "QUEUES_ALGORITHMS");
        for (queues_count = 0; queues_algorithms[queues_count] != NULL; queues_count++) {
        }
        char* preemption_str = config_get_string_value(config, "QUEUE_PREEMPTION");
        queue_preemption = strcmp(preemption_str, "TRUE") == 0;
    }
    return;
}
void registrar_proceso_en_km(int pid, char* path) {                                
    int op = OP_GET_PROCESO;                                                       
    int tam_path = strlen(path) + 1;                                               
    pthread_mutex_lock(&mutex_km);                                                 
    send(fd_km, &op,       sizeof(int), 0);                                        
    send(fd_km, &pid,      sizeof(int), 0);                                        
    send(fd_km, &tam_path, sizeof(int), 0);                                        
    send(fd_km, path,      tam_path,    0);    
    int ok;                                                                            
    recv(fd_km, &ok, sizeof(int), MSG_WAITALL);                                   
    pthread_mutex_unlock(&mutex_km);                                               
}