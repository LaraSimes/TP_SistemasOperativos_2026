#include "configuraciones.h"

/*   1ER CHECKPOINT
config.c 
    Cargar el archivo de configuración
    Obtener valores (IP, puertos, algoritmo, etc.)
*/

char* obtener_string(char* clave, t_config* config){
    return config_get_string_value(config, clave);
}

int obtener_int(char* clave, t_config* config){
    return config_get_int_value(config, clave);
}


