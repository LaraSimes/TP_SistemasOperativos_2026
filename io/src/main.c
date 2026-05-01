#include "main.h"

int main(int argc, char* argv[]) {

    if (argc < 3) {
        printf("Uso: ./bin/io [Archivo Config] [Tipo]\n");
        return EXIT_FAILURE;
    }

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

    t_tipo_io tipo = parsear_tipo(argv[2]);
    if (tipo == -1) {
        log_error(logger, "Tipo de IO desconocido: %s", argv[2]);
        return EXIT_FAILURE;
    }

    while (1) {
        int pid;
        if (recv(conexion, &pid, sizeof(int), MSG_WAITALL) <= 0) {
            log_warning(logger, "Kernel Scheduler desconectado");
            break;
        }

        log_info(logger, "## PID: %d - Inicio de IO", pid);

        switch (tipo) {
            case IO_SLEEP: {
                int ms;
                recv(conexion, &ms, sizeof(int), MSG_WAITALL);
                log_info(logger, "## PID: %d - Haciendo sleep por %d milisegundos.", pid, ms);
                usleep(ms * 1000);
                int ok = 1;
                send(conexion, &ok, sizeof(int), 0);
                break;
            }
            case IO_STDIN: {
                int cantidad;
                recv(conexion, &cantidad, sizeof(int), MSG_WAITALL);
                log_info(logger, "## PID: %d - Ingrese %d caracteres:", pid, cantidad);
                char *buffer = calloc(cantidad + 1, sizeof(char));
                char linea[1024];
                fgets(linea, sizeof(linea), stdin);
                linea[strcspn(linea, "\n")] = '\0';
                strncpy(buffer, linea, cantidad);
                send(conexion, buffer, cantidad, 0);
                free(buffer);
                break;
            }
            case IO_STDOUT: {
                int tamanio;
                recv(conexion, &tamanio, sizeof(int), MSG_WAITALL);
                char *contenido = calloc(tamanio + 1, sizeof(char));
                recv(conexion, contenido, tamanio, MSG_WAITALL);
                log_info(logger, "## PID: %d - %s", pid, contenido);
                free(contenido);
                int ok = 1;
                send(conexion, &ok, sizeof(int), 0);
                break;
            }
        }

        log_info(logger, "## PID: %d - Fin de IO", pid);
    }
    close(conexion);
    return 0;
}

