#include "configuraciones.h"

/*   1ER CHECKPOINT
config.c 
    Cargar el archivo de configuración
    Obtener valores (IP, puertos, algoritmo, etc.)
*/

t_config* iniciar_config(char* path){
	t_config* nuevo_config = config_create(path);
    if(nuevo_config != NULL){
        return nuevo_config;
    }else{
        printf("Error al cargar config\n");
        abort();    
        }
}

char* obtener_string(char* clave, t_config* config){
    return config_get_string_value(config, clave);
}

int obtener_int(char* clave, t_config* config){
    return config_get_int_value(config, clave);
}
