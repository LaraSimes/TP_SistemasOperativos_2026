#include "main.h"


void cargar_informacion_configuracion(t_config* config, t_config_valores* valores, t_log* logger) {
    valores->log_level               = config_get_string_value(config, "LOG_LEVEL");
    valores->ip_kernel_scheduler     = config_get_string_value(config, "IP_KERNEL_SCHEDULER");
    valores->puerto_kernel_scheduler = config_get_string_value(config, "PUERTO_KERNEL_SCHEDULER");
    valores->ip_kernel_memory        = config_get_string_value(config, "IP_KERNEL_MEMORY");
    valores->puerto_kernel_memory    = config_get_string_value(config, "PUERTO_KERNEL_MEMORY");
    valores->ip_memory_stick     = config_get_string_value(config, "IP_MEMORY_STICK");
    valores->puerto_memory_stick = config_get_string_value(config, "PUERTO_MEMORY_STICK");

log_info(logger, "Config cargada - KS: %s:%s | KM: %s:%s", 
    valores->ip_kernel_scheduler, valores->puerto_kernel_scheduler,
    valores->ip_kernel_memory,    valores->puerto_kernel_memory); // informa que la config se leyo bien
}

int conectar_a_servidor(char* ip, char* puerto) {
    struct addrinfo hints; //le digo a getaddrinfo que tipo de conexion quiero
    struct addrinfo* result;

    memset(&hints, 0, sizeof(hints)); //inicializo en 0 el socket
    hints.ai_family   = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if (getaddrinfo(ip, puerto, &hints, &result) != 0) return -1; //devuelve en result lo necesario para crear socket, sino retorna -1

    int fd = socket(result->ai_family, result->ai_socktype, result->ai_protocol); //crea el socket y retorna fd
    if (fd == -1) { // si falla el fd libera el result
        freeaddrinfo(result);
        return -1;
    }

    if (connect(fd, result->ai_addr, result->ai_addrlen) == -1) { //intenta conectarse al servidor
        close(fd);
        freeaddrinfo(result);
        return -1; // si falla libera result y devuelve -1
    }

    freeaddrinfo(result);
    return fd; //libera el result y devuelve el fd si todo sale bien
}

int handshake_kernel_memory(int fd, int cpu_id, t_log* logger) {
    t_modulo modulo = MOD_CPU; //le dice a Kernel memory que es la cpu
    if (send(fd, &modulo, sizeof(t_modulo), 0) == -1) {
        log_error(logger, "Error enviando identificacion a KM");
        return -1;
    }

    
    int resultado; //espera confirmacion de Kernel memory
    if (recv(fd, &resultado, sizeof(int), MSG_WAITALL) <= 0) {
        log_error(logger, "Error recibiendo confirmacion de KM");
        return -1;
    }


    if (send(fd, &cpu_id, sizeof(int), 0) == -1) {//si esta todo ok, le envia el id de la cpu
        log_error(logger, "Error enviando cpu_id a KM");
        return -1;
    }

    log_info(logger, "Handshake con Kernel Memory completado"); //completa el handshake
    return 0;
}