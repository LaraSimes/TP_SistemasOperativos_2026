#include "conexiones.h"

int liberar(t_log *log, t_config *config, int conexion){
    log_destroy(log);
    config_destroy(config);
    if (conexion==-1){
        return 0;
    }
    close(conexion);
    return 0;
}
void cargar_informacion_configuracion(t_config*config, t_atributos* valores){
    valores->ip=config_get_string_value(config,"IP");
    valores->puerto=config_get_string_value(config,"PUERTO");
    valores->log_level=config_get_string_value(config,"LOG_LEVEL");
    valores->swap_path=config_get_string_value(config,"SWAP_FILE_PATH");
    valores->swap_size=config_get_int_value(config,"SWAP_FILE_SIZE");
    valores->block_size=config_get_int_value(config,"BLOCK_SIZE");
}

int handshakear_swap(int km_fd){
    t_modulo id=MOD_SWAP;
    int recibido;
    int a = send(km_fd,&id,sizeof(int),0);
    if (a==-1){
        log_error(logger,"Error al enviar el id del modulo");
        close(km_fd);
        liberar(logger,NULL,km_fd);
        return -1;
    }
    int b = recv(km_fd,&recibido,sizeof(int),MSG_WAITALL);
    if (b==-1){
        log_error(logger,"Error al recibir la confirmacion del handshake");
        close(km_fd);
        liberar(logger,NULL,km_fd);
        return -1;
    }
    return 0;
}