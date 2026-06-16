include "operaciones.h"

void manejar_operacion(int cliente_fd, t_operacion cod_op){
    switch(cod_op){
        case OP_GET_PROCESO:
            int pid, tam_path;
            recv(cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &tam_path, sizeof(int), MSG_WAITALL);
            char* path = malloc(tam_path);
            recv(cliente_fd, path, tam_path, MSG_WAITALL);
            crear_proceso(pid, path);
            t_respuestas_KM response = revisar_memoria();
            send(socket_cliente, &response, sizeof(response), 0);
            log_info(logger,"## PID: %d - Proceso Creado", pid);
            break;

        case OP_GET_FREE_SPACE:
            uint32_t espacio = obtener_espacio_libre();
            t_respuestas_KM memoria = revisar_memoria();
            if (memoria==MEMORIA_CORRUPTA){
                send(cliente_fd, &memoria, sizeof(memoria), 0);
                break;
            }
            send(cliente_fd, &espacio, sizeof(uint32_t), 0);
            log_info(logger, "## Espacio libre solicitado por Kernel: %u bytes", espacio);
            break;

        case OP_MEM_ALLOC:
            int pid, tamanio, id_segmento;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &tamanio, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &id_segmento, sizeof(int), MSG_WAITALL);
            t_proceso_control* proceso = buscar_proceso_por_pid(pid);
            if (proceso == NULL) {
                log_error(logger, "PID: %d no encontrado para crear segmento", pid);
                t_respuestas_KM res = PID_NOT_FOUND;
                send(cliente_fd, &res, sizeof(res), 0);
                break;
            }
            t_segmento* seg = crear_segmento(id_segmento, tamanio);
            t_respuestas_KM memoria = revisar_memoria();
            if (memoria==MEMORIA_CORRUPTA){
                send(cliente_fd, &memoria, sizeof(memoria), 0);
                free(seg);
                break;
            }
            t_respuestas_KM resultado = agregar(proceso, seg);
            send(cliente_fd,&resultado, sizeof(resultado),0);
            break;

        case OP_MEM_FREE:
            int pid, id_segmento;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &id_segmento, sizeof(int), MSG_WAITALL);
            t_proceso_control* proceso = buscar_proceso_por_pid(pid);
            if (proceso == NULL) {
                log_error(logger, "PID: %d no encontrado para crear segmento", pid);
                t_respuestas_KM res = PID_NOT_FOUND;
                send(cliente_fd, &res, sizeof(res), 0);
                break;
            }
            t_respuestas_KM memoria = revisar_memoria();
            if (memoria==MEMORIA_CORRUPTA){
                send(cliente_fd, &memoria, sizeof(memoria), 0);
                free(seg);
                break;
            }
            t_respuesta_KM respuesta = eliminar_segmento(proceso,id_segmento);
            send (cliente_fd, &respuesta,sizeof(respuesta),0);
            
        case OP_READ_MEM:
            int pid_lec;
            uint32_t dir_lec;
            int tam_lec;

            recibir_datos_memoria_mock(cliente_fd, &pid_lec, &dir_lec, &tam_lec);
            if (memoria_corrupta) {
                log_error(logger, "Memoria Corrupta");
                char* error_msg = "Memoria Corrupta";
                send(cliente_fd, error_msg, strlen(error_msg) + 1, 0);
                break;
            }
            else {

                log_info(logger,"## PID: %d - Lectura - Dir. Fisica: %u - Tamaño: %d", pid_lec, dir_lec, tam_lec);
                uint32_t ok_lectura = 0;
                send(cliente_fd, &ok_lectura, sizeof(uint32_t), 0);
                break;
            }
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
    nuevo_proceso->pid = pid;
    nuevo_proceso->path_instrucciones = path;
    //guardar el proceso creado en una lista global ya que esta fuera del main.c
    //guarda punteros a las estructuras de t_proceso_control declarado en el main.h
    pthread_mutex_lock(&mutex_lista_procesos);
    list_add(procesos_activos, nuevo_proceso);
    pthread_mutex_unlock(&mutex_lista_procesos);
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

t_segmento* crear_segmento(int id_segmento, int tamanio) {
    t_segmento* nuevo_segmento = malloc(sizeof(t_segmento));
    nuevo_segmento->id_segmento = id_segmento;
    nuevo_segmento->tamaño = tamanio;
    return nuevo_segmento;
}

t_respuestas_KM agregar(t_proceso_control* proceso, t_segmento* seg){
    uint32_t tamanio_seg = seg->tamanio;
    uint32_t limite_seg = seg->tamanio + seg->base;
    if (tamanio_seg> max_segmento){
        return SEGMENTO_EXCEDIDO;
    }
    phtread_mutex_lock(&mutex_lista_huecos);
    pthread_mutex_lock(&mutex_lista_segmentos);
    t_list posibles =list_filter(huecos_libres, (void*)hueco->tamanio >= tamanio_seg);
    if (list_is_empty(posibles)) {
        free(seg);
        pthread_mutex_unlock(&mutex_lista_segmentos);
        pthread_mutex_unlock(&mutex_lista_huecos);
        log_error(logger, "## PID: %d - No se encontró un hueco libre adecuado para el segmento %d de tamaño %d", proceso->pid, seg->id_segmento, tamanio_seg);
        return FRAGMENTACION_EXT;
    }
    t_hueco* hueco_elegido = list_get_minimum(posibles, hueco_mas_chico);
    if(hueco_elegido-> tamanio == tamanio_seg){
        list_remove_and_destroy_element (huecos_libres, hueco_elegido);
    }
    else{
        seg->base = hueco_elegido->base;
        hueco_elegido->base = limite_seg;
        hueco_elegido->tamanio = hueco_elegido->tamanio - tamanio_seg;
        list_add(segmentos,seg);
    }
    pthread_mutex_unlock(&mutex_lista_segmentos);
    pthread_mutex_unlock(&mutex_lista_huecos);
    pthread_mutex_lock(&mutex_lista_procesos);
    list_add(proceso->contexto->segmentos,seg);
    pthread_mutex_unlock(&mutex_lista_procesos);
    return OK;
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

t_respuestas_KM revisar_memoria(void){
    t_respuestas_KM aviso;
    if (memoria_corrupta) {
        aviso=MEMORIA_CORRUPTA;
        return aviso;
    }
    aviso = OK;
    return aviso;
}

t_stick* donde_guardo(uint32_t base){
    pthread_mutex_lock(&mutex_lista_sticks);
    t_stick* resultado;
    for (int i=0; i<list_size(sticks);i++){
        t_stick pruebo = list_get (sticks, i);
        if (base >= pruebo -> base && < (pruebo->base + pruebo->tamanio)){
            resultado = pruebo;
        }
    }
    pthread_mutex_unlock(&mutex_lista_sticks);
    return resultado;
}

t_respuestas_KM eliminar_segmento (t_proceso_control *proceso, int id_segmento){
    t_segmento* eliminado;
    pthread_mutex_lock(&mutex_lista_procesos);
    t_list* tabla_segmentos = proceso->contexto->segmentos;
    for(int i = 0; i<list_size(tabla_segmentos),i++){
        t_segmento* s = list_get(tabla_segmentos,i);
        if(s->id_segmento == id_segmento){
            list_remove_element(tabla_segmentos, eliminado);
            eliminado = s;
        }
    }
    pthread_mutex_unlock(&mutex_lista_procesos);
    if(eliminado == NULL){
        return SEGMENTO_NOT_FOUND;
    }
    pthread_mutex_lock(&mutex_lista_segmentos);
    list_remove_and_destroy_element(segmentos,eliminado);
    pthread_mutex_unlock(&mutex_lista_segmentos);
    return OK;
}