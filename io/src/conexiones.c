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

