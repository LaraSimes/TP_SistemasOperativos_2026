#ifndef OPERACIONES_H
#define OPERACIONES_H

#include "comun.h"

void manejar_operacion(int cliente_fd, t_operacion cod_op);
void crear_proceso(int pid, char* path);
void terminar_programa(int server_fd, t_log* logger, t_config* config);
uint32_t obtener_espacio_libre();
void* hueco_mas_chico(void* a, void* b);
t_rsp_KM agregar_segmento(int pid, uint32_t tamanio, int id_segmento);
t_proceso_control* buscar_proceso_por_pid(int pid);
t_stick* donde_guardo(uint32_t base);
t_rsp_KM eliminar_segmento (int pid, int id_segmento);
t_rsp_KM leer_dir(uint32_t direccion, uint32_t tamanio, uint8_t* leido,uint32_t b_leidos);
t_rsp_KM escribir_dir(uint32_t direccion, uint32_t tamanio, uint8_t* escrito,uint32_t b_escritos);
t_list* filtrar_huecos_por_tamanio(t_list* huecos, uint32_t tamanio_seg);
char* leer_instruccion_de_archivo(char* path, uint32_t pc);
t_contexto obtener_contexto(t_proceso_control* proceso);
void enviar_segmentos(int cliente_fd, t_list* segmentos);
void actualizar_contexto(t_proceso_control* proceso, t_contexto* cxt);
int suspender_proceso(t_proceso_control* proceso);
t_rsp_KM desuspender_proceso(int pid);
bool menor_base(void* a, void* b);
t_rsp_KM compactar(void);
t_rsp_KM finalizar_proceso(int pid);
int cargar_bloques(t_segmento* seg, int pid);
t_bloque* elegir_bloque();
bool ordenar_bloques(void* a, void* b);
t_list* bloques_proceso(int pid);
t_list* contar_segmentos(t_list* bloques);
t_list* bloq_x_segmento(int id,t_list* total);
uint32_t tamanio_real(uint8_t* bytes,uint32_t tamanio);
t_segmento* buscar_segmento(int pid, int id_seg);
#endif