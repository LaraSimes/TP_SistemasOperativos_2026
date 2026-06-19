#ifndef OPERACIONES_H
#define OPERACIONES_H

#include "comun.h"

void manejar_operacion(int cliente_fd, t_operacion cod_op);
void crear_proceso(int pid, char* path);
void terminar_programa(int server_fd, t_log* logger, t_config* config);
uint32_t obtener_espacio_libre();
t_segmento* crear_segmento(int id_segmento, uint32_t tamanio);
void* hueco_mas_chico(void* a, void* b);
t_rsp_KM agregar_segmento(int pid, t_segmento* seg);
t_proceso_control* buscar_proceso_por_pid(int pid);
t_stick* donde_guardo(uint32_t base);
t_rsp_KM eliminar_segmento (int pid, int id_segmento);
t_rsp_KM leer_dir(uint32_t direccion, uint32_t tamanio, uint8_t* leido,uint32_t b_leidos);
t_rsp_KM escribir_dir(uint32_t direccion, uint32_t tamanio, uint8_t* escrito,uint32_t b_escritos);
t_list* filtrar_huecos_por_tamanio(t_list* huecos, uint32_t tamanio_seg);
#endif