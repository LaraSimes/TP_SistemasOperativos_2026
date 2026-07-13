#include "conexiones.h"
t_atributos valores;
t_log *logger;
uint32_t tamanio_total;
uint32_t tamanio_bloque;
char* path_SWAP;
int socket_km;

int main(int argc, char* argv[]) {
    t_config *config;
    logger = iniciar_logger(MOD_SWAP,LOG_LEVEL_INFO);
    config = iniciar_config(argv[1]);
    if (config == NULL ){
	    log_debug(logger,"No se pudo crear el config");
		abort();
	};
    cargar_informacion_configuracion(config, &valores);
    tamanio_total = valores.swap_size;
    tamanio_bloque=valores.block_size;
    path_SWAP = valores.swap_path;
    socket_km= crear_cliente(valores.ip, valores.puerto);
    if (socket_km == -1) {
        log_error(logger, "No se pudo conectar a Kernel Memory");
        liberar(logger,config,socket_km);
        return -1;
    }
    int conexion=handshakear_swap(socket_km);
    if (conexion == -1) {
        log_error(logger, "No se pudo completar el handshake con Kernel Memory");
        liberar(logger,config,socket_km);
        return -1;
    }
    int res1 = send(socket_km,&tamanio_total,sizeof(uint32_t),0);
    int res2 = send(socket_km,&tamanio_bloque,sizeof(uint32_t),0);
    if (res1<0 || res2<0){
        log_error(logger, "Error enviando datos a Kernel Memory");
        liberar(logger,config,socket_km);
        return -1;
    }
    log_info(logger, "## Conectado a Kernel Memory\n");
    while (true) {
        t_operacion op;
        ssize_t escuchar = recv(socket_km, &op, sizeof(op), MSG_WAITALL);
        if (escuchar == 0) {
            log_error(logger, "Kernel Memory cerró la conexión");
            break;
        }
        if (escuchar < 0) {
            log_error(logger, "Error recibiendo operación de Kernel Memory");
            break;
        }
        manejar_operacion(op);
    }
    return 0;
}

