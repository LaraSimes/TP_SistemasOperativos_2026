#include <utils/hello.h>
#include "main.h"

int main(int argc, char *argv[])
{
    t_log *logger;
    if (argc < 3)
    {
        fprintf(stderr, "Uso: %s <config> <identificador>\n", argv[0]);
        return EXIT_FAILURE;
    }
    char *config_path = argv[1];
    char *identificador = argv[2];
    int cpu_id = atoi(identificador); // Convierte el string "0" en int 0, ya que el handshake envia el id como binario

    logger = iniciar_logger(MOD_CPU, LOG_LEVEL_INFO);
    t_config *config = iniciar_config(config_path);

    t_config_valores valores; // Declara struct en el stack
    cargar_informacion_configuracion(config, &valores, logger);

    signal(SIGPIPE, SIG_IGN); // Preguntar a IA

    // Conexion a KM

    int fd_km = crear_cliente(valores.ip_kernel_memory, valores.puerto_kernel_memory);

    if (fd_km == -1)
    {
        log_error(logger, "No se pudo conectar a Kernel Memory");
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    } // Si falla conexion, destruye config y log y devuelve error

    //handshakear(fd_km, logger, MOD_KERNEL_MEMORY);
     if (!handshake(fd_km, logger, MOD_CPU, MOD_KERNEL_MEMORY)) {
        log_error(logger, "Fallo el handshake con Memoria");
        close(fd_km);
        return EXIT_FAILURE;
    }   

    if (send(fd_km, &cpu_id, sizeof(int), 0) == -1)
    {
        log_error(logger, "Error enviando cpu_id a Memory Stick");
        return EXIT_FAILURE;
    }

    // Conexion a KS
    int fd_ks = crear_cliente(valores.ip_kernel_scheduler, valores.puerto_kernel_scheduler);

    if (fd_ks == -1)
    {
        log_error(logger, "No se pudo conectar al Kernel Scheduler");
        close(fd_km);
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    } // Si falla conexion, destruye config y log y devuelve error

    // handshakear(fd_ks, logger, MOD_KERNEL_SCHEDULER);
    if (!handshake(fd_ks, logger, MOD_CPU, MOD_KERNEL_SCHEDULER)) {
        log_error(logger, "Fallo el handshake con Kernel");
        close(fd_ks);
        return EXIT_FAILURE;
    } 

    if (send(fd_ks, &cpu_id, sizeof(int), 0) == -1)
    {
        log_error(logger, "Error enviando cpu_id a KM");
        return -1;
    }
    log_info(logger, "Identificacion enviada a Kernel Scheduler");

    int fd_ms = crear_cliente(valores.ip_memory_stick, valores.puerto_memory_stick);

    if (fd_ms == -1)
    {
        log_error(logger, "No se pudo conectar a Memory Stick");
        close(fd_km);
        close(fd_ks);
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    }
    // handshakear(fd_ms, logger, MOD_MEMORY_STICK);
    if (!handshake(fd_ms, logger, MOD_CPU, MOD_MEMORY_STICK)) {
        log_error(logger, "Fallo el handshake con Memory Stick");
        close(fd_ms);
        return EXIT_FAILURE;
    } 
    if (send(fd_ms, &cpu_id, sizeof(int), 0) == -1)
    {
        log_error(logger, "Error enviando cpu_id a Memory Stick");
        return EXIT_FAILURE;
    }

    log_info(logger, "CPU %d lista.Esperando PID del Kernel Scheduler", cpu_id);
    pause();

    close(fd_ms);
    close(fd_km);
    close(fd_ks);
    config_destroy(config);
    log_destroy(logger);
    return EXIT_SUCCESS;
    // Cierra todo lo usado, tanto los sockets como las commons
}
