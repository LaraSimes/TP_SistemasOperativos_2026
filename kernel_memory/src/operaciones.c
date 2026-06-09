include "operaciones.h"

void manejar_operacion(int cliente_fd, t_operacion cod_op){
    switch(cod_op){
        case OP_GET_PROCESO:
            int pid;
            int tam_path;
            recv(cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &tam_path, sizeof(int), MSG_WAITALL);
            char* path = malloc(tam_path);
            recv(cliente_fd, path, tam_path, MSG_WAITALL);
            crear_proceso(pid, path);
            break;
        case OP_GET_FREE_SPACE:
            uint32_t espacio = obtener_espacio_libre();
            send(cliente_fd, &espacio, sizeof(uint32_t), 0);
            log_info(logger, "## Espacio libre solicitado por Kernel: %u bytes", espacio);
            break;
        case OP_MEM_ALLOC:
            int pid, tamanio;
            uint32_t id_segmento;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &tamanio, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &id_segmento, sizeof(uint32_t), MSG_WAITALL);
            t_proceso_control* proceso = buscar_proceso_por_pid(pid);
            if (proceso == NULL) {
                log_error(logger, "PID: %d no encontrado para crear segmento", pid);
                send(cliente_fd, (uint32_t*)(-2), sizeof(uint32_t), 0);
                break;
            }
            int resultado = crear_segmento(proceso, id_segmento, tamanio);
            send(cliente_fd, &resultado, sizeof(int), 0);
            switch (resultado){
                case -1:
                    log_error(logger, "## PID: %d - No hay espacio suficiente para crear el segmento %d de tamaño %d", pid, id_segmento, tamanio);
                    break;
                case 1:
                    log_error(logger, "## PID: %d - No se encontró un hueco libre adecuado para el segmento %d de tamaño %d", pid, id_segmento, tamanio);
                    break;
                case 0:
                    log_info(logger, "## PID: %d - Segmento Creado %d - Tamaño: %d", pid, id_segmento, tamanio);
                    break;
            }
            break;
        case OP_MEM_FREE:
            uint32_t ok = 0;
            send(cliente_fd, &ok, sizeof(uint32_t), 0);
            break;
        case OP_READ_MEM:
            int pid_lec;
            uint32_t dir_lec;
            int tam_lec;

            recibir_datos_memoria_mock(cliente_fd, &pid_lec, &dir_lec, &tam_lec);

            log_info(logger,"## PID: %d - Lectura - Dir. Fisica: %u - Tamaño: %d", pid_lec, dir_lec, tam_lec);

            uint32_t ok_lectura = 0;
            send(cliente_fd, &ok_lectura, sizeof(uint32_t), 0);
            break;
        case OP_WRITE_MEM:
            int pid_esc; 
            uint32_t dir_esc; 
            int tam_esc;

            recibir_datos_memoria_mock(cliente_fd, &pid_esc, &dir_esc, &tam_esc);

            log_info(logger, "## PID: %d - Escritura - Dir. Fisica: %u - Tamaño: %d", pid_esc, dir_esc, tam_esc);

            uint32_t ok_escritura = 0;
            send(cliente_fd, &ok_escritura, sizeof(uint32_t), 0);
            break;

        default:
            log_warning(logger, "Operacion desconocida de CPU");
    }
}

void crear_proceso(int pid, char* path) {    
    // creo el proceso en la lista
    t_proceso_control* nuevo_proceso = calloc(1, sizeof(t_proceso_control)); 
    nuevo_proceso->pid = pid_recibido;
    nuevo_proceso->path_instrucciones = path_recibido;
    //guardar el proceso creado en una lista global ya que esta fuera del main.c
    //guarda punteros a las estructuras de t_proceso_control declarado en el main.h
    pthread_mutex_lock(&mutex_lista_procesos);
    list_add(procesos_activos, nuevo_proceso);
    pthread_mutex_unlock(&mutex_lista_procesos);
    int ok = 0;
    send(socket_cliente, &ok, sizeof(int), 0);
    log_info(logger,"## PID: %d - Proceso Creado", pid_recibido);
}

void terminar_programa(int server_fd, t_log* logger, t_config* config)
{
        list_destroy_and_destroy_elements(procesos_activos, (void*)free);
        pthread_mutex_destroy(&mutex_lista_procesos);
		log_destroy(logger);
		config_destroy(config);	  
		close(server_fd);
}

uint32_t obtener_espacio_libre() {
    uint32_t total = 0;
    pthread_mutex_lock(&mutex_lista_huecos);
    for (int i = 0; i < list_size(huecos_libres); i++) {
        t_hueco* hueco = list_get(huecos_libres, i);
        total += hueco->tamaño;
    }
    pthread_mutex_unlock(&mutex_lista_huecos);
    return total;
}

int crear_segmento(t_proceso_control* proceso, uint32_t id_segmento, int tamanio) {
    t_segmento* nuevo_segmento = malloc(sizeof(t_segmento));
    nuevo_segmento->id_segmento = id_segmento;
    nuevo_segmento->tamaño = tamanio;
    uint32_t espacio_libre = obtener_espacio_libre();
    if (espacio_libre < tamanio) {
        free(nuevo_segmento);
        return -1;
    }
    phtread_mutex_lock(&mutex_lista_huecos);
    pthread_mutex_lock(&mutex_lista_segmentos);
    t*list posibles =list_filter(huecos_libres, (void*)hueco->tamanio >= tamanio);
    if (list_is_empty(posibles)) {
        free(nuevo_segmento);
        pthread_mutex_unlock(&mutex_lista_segmentos);
        pthread_mutex_unlock(&mutex_lista_huecos);
        return 1;
    }
    t_hueco* hueco_elegido = list_get_minimum(posibles, hueco_mas_chico);
    nuevo_segmento->base = hueco_elegido->base;
    hueco_elegido->base += tamanio;
    hueco_elegido->tamanio -= tamanio;
    if (hueco_elegido->tamanio == 0) {
        list_remove_and_destroy_element(huecos_libres, list_index_of(huecos_libres, hueco_elegido), (void*)free);
    }
    list_destroy(posibles);
    list_add(segmentos, nuevo_segmento);
    pthread_mutex_unlock(&mutex_lista_segmentos);
    pthread_mutex_unlock(&mutex_lista_huecos);
    pthread_mutex_lock(&mutex_lista_procesos);
    list_add(proceso->segmentos, nuevo_segmento);
    pthread_mutex_unlock(&mutex_lista_procesos);
    return 0;
}
void* hueco_mas_chico(void* a, void* b) {
    t_hueco* hueco_a = (t_hueco*) a;
    t_hueco* hueco_b = (t_hueco*) b;

    return hueco_a->tamanio <= hueco_b->tamanio ? hueco_a : hueco_b;
}

t_proceso_control* buscar_proceso_por_pid(int pid) {
    t_proceso_control* resultado = NULL;
    pthread_mutex_lock(&mutex_lista_procesos);
    for (int i = 0; i < list_size(procesos_activos); i++) {
        t_proceso_control* proceso = list_get(procesos_activos, i);

        if (proceso->pid == pid) {
            resultado = proceso;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_lista_procesos);
    return resultado;
}
