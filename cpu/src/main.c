#include <utils/hello.h>
#include "main.h"

int main(int argc, char* argv[]) {
    if (argc < 3)
    {
        fprintf(stderr,"Uso: %s <config> <identificador>\n",argv[0]);
        return EXIT_FAILURE;
    }
    char* config_path = argv[1];
    char* identificador = argv[2];
    int cpu_id =atoi(identificador); // Convierte el string "0" en int 0, ya que el handshake envia el id como binario
    
    t_log* logger = iniciar_logger(identificador);
    t_config* config = iniciar_config(config_path);

    t_config_valores valores; //Declara struct en el stack
    cargar_informacion_configuracion(config, &valores, logger);

    signal(SIGPIPE, SIG_IGN); //Preguntar a IA

    //Conexion a KM

    int fd_km =conectar_a_servidor(valores.ip_kernel_memory, valores.puerto_kernel_memory);

    if(fd_km == -1){
        log_error(logger,"No se pudo conectar a Kernel Memory");
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    }//Si falla conexion, destruye config y log y devuelve error

    log_info(logger, "## Conectado a Kernel Memory");

    if(handshake_kernel_memory(fd_km, cpu_id, logger) == -1){
        log_error(logger,"Handshake con Kernel memory fallo");
        close(fd_km);
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    }//Si falla hanshake, cierra el fd, destruye config y log y devuelve error

    //Conexion a KS
    int fd_ks = conectar_a_servidor(valores.ip_kernel_scheduler, valores.puerto_kernel_scheduler);

    if(fd_ks == -1){
        log_error(logger, "No se pudo conectar al Kernel Scheduler");
        close(fd_km);
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    }//Si falla conexion, destruye config y log y devuelve error

    log_info(logger, "## Conectado al Kernel Scheduler");

    t_modulo modulo_cpu = CPU;
    if (send(fd_ks, &modulo_cpu, sizeof(int),0) == -1){
        log_error(logger,"Error identificandose ante Kernel Scheduler");
        close(fd_km);
        close(fd_ks);
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    }
    log_info(logger,"Identificacion enviada a Kernel Scheduler");

    int fd_ms = conectar_a_servidor(valores.ip_memory_stick, valores.puerto_memory_stick);

    if (fd_ms == -1) {
        log_error(logger, "No se pudo conectar a Memory Stick");
        close(fd_km);
        close(fd_ks);
        config_destroy(config);
        log_destroy(logger);
        return EXIT_FAILURE;
    }
log_info(logger, "## Conectado a Memory Stick");

    log_info(logger, "CPU %d lista.Esperando PID del Kernel Scheduler",cpu_id);
    pause();

    close(fd_ms);
    close(fd_km);
    close(fd_ks);
    config_destroy(config);
    log_destroy(logger);
    return EXIT_SUCCESS;
    //Cierra todo lo usado, tanto los sockets como las commons
}
