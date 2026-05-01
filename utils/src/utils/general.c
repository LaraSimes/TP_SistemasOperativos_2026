#include "general.h"

t_log *iniciar_logger(t_modulo id, t_log_level nivel) {
    t_log* nuevo_logger;
    switch (id) {
        case MOD_KERNEL_SCHEDULER:
            nuevo_logger = log_create("KERNEL_SCHEDULER.log", "KERNEL_SCHEDULER", 1, nivel);
            break;
        case MOD_CPU:
            nuevo_logger = log_create("CPU.log", "CPU", 1, nivel);
            break;
        case MOD_MEMORY_STICK:
            nuevo_logger = log_create("MEMORY_STICK.log", "MEMORY_STICK", 1, nivel);
            break;
        case MOD_SWAP:
            nuevo_logger = log_create("SWAP.log", "SWAP", 1, nivel);
            break;
        case MOD_IO:
            nuevo_logger = log_create("IO.log", "IO", 1, nivel);
            break;
        case MOD_KERNEL_MEMORY:
            nuevo_logger = log_create("KERNEL_MEMORY.log", "KERNEL_MEMORY", 1, nivel);
            break;
        default:
            abort(); // id desconocido
    }

    if (nuevo_logger != NULL) {
        return nuevo_logger;
    } else {
        abort();
    }
}

t_config* iniciar_config(char* path){
    t_config* config = config_create(path);
    if(!config){ //Se fija si se creo la config, o si dio null
        fprintf(stderr, "Error al leer config: %s\n",path);
        exit(EXIT_FAILURE);
    }
    return config;
}

t_tipo_io parsear_tipo(char *tipo) {
    if (strcmp(tipo, "SLEEP") == 0)  return IO_SLEEP;
    if (strcmp(tipo, "STDIN") == 0)  return IO_STDIN;
    if (strcmp(tipo, "STDOUT") == 0) return IO_STDOUT;
    return -1;
}
