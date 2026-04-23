#include "main.h"


void cargar_informacion_configuracion(t_config* config, t_config_valores* valores, t_log* logger) {
    valores->log_level               = config_get_string_value(config, "LOG_LEVEL");
    valores->ip_kernel_scheduler     = config_get_string_value(config, "IP_KERNEL_SCHEDULER");
    valores->puerto_kernel_scheduler = config_get_string_value(config, "PUERTO_KERNEL_SCHEDULER");
    valores->ip_kernel_memory        = config_get_string_value(config, "IP_KERNEL_MEMORY");
    valores->puerto_kernel_memory    = config_get_string_value(config, "PUERTO_KERNEL_MEMORY");
    valores->ip_memory_stick     = config_get_string_value(config, "IP_MEMORY_STICK");
    valores->puerto_memory_stick = config_get_string_value(config, "PUERTO_MEMORY_STICK");

log_info(logger, "Config cargada - KS: %s:%s | KM: %s:%s", 
    valores->ip_kernel_scheduler, valores->puerto_kernel_scheduler,
    valores->ip_kernel_memory,    valores->puerto_kernel_memory); // informa que la config se leyo bien
}
