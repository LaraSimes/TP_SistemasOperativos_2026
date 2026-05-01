#include "main.h"

int main(int argc, char* argv[]) {

    t_log *logger;
    t_config *config;
    t_log_level log_level;
    int conexion;

    config = config_create(argv[1]);
    if (config == NULL) {
        printf("No se pudo crear el config\n");
        abort();
    }

    char *ip = config_get_string_value(config,"IP");
	char *puerto_kernel = config_get_string_value(config,"PUERTO_KERNEL");
    log_level = log_level_from_string(config_get_string_value(config, "LOG_LEVEL"));

    logger = iniciar_logger(MOD_IO, log_level);

    // Conexion a Kernel
    conexion = crear_cliente(ip, puerto_kernel);
    if (conexion == -1) {
        log_error(logger, "No se pudo conectar a Kernel Scheduler");
        close(conexion);
        return EXIT_FAILURE;
    }

    // Handshake con Kernel
    if (!handshake(conexion, logger, MOD_IO, MOD_KERNEL_SCHEDULER)) {
        log_error(logger, "Fallo el handshake con KERNEL");
        close(conexion);
        return EXIT_FAILURE;
    }

    return 0;
}

