#include "operaciones.h"

void manejar_operacion(int cliente_fd, t_operacion cod_op){
    switch(cod_op){
        case OP_GET_PROCESO:{
            int pid, tam_path;
            recv(cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &tam_path, sizeof(int), MSG_WAITALL);
            char* path = malloc(tam_path);
            recv(cliente_fd, path, tam_path, MSG_WAITALL);
            crear_proceso(pid, path);
            t_rsp_KM resultado = OK;
            send(cliente_fd, &resultado, sizeof(resultado), 0);
            log_info(logger,"## PID: %d - Proceso Creado", pid);
            break;
        }
        case OP_GET_FREE_SPACE:{
            uint32_t espacio = obtener_espacio_libre();
            send(cliente_fd, &espacio, sizeof(uint32_t), 0);
            log_info(logger, "## Espacio libre solicitado por Kernel: %u bytes", espacio);
            break;
        }
        case OP_MEM_ALLOC:{
            int pid, id_segmento;
            uint32_t tamanio;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &tamanio, sizeof(uint32_t), MSG_WAITALL);
            recv(cliente_fd, &id_segmento, sizeof(int), MSG_WAITALL);
            t_segmento* seg = crear_segmento(id_segmento, tamanio);
            t_rsp_KM resultado = agregar_segmento(pid, seg);
            send(cliente_fd,&resultado, sizeof(resultado),0);
            break;
        }
        case OP_MEM_FREE:{
            int pid, id_segmento;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv(cliente_fd, &id_segmento, sizeof(int), MSG_WAITALL);
            t_rsp_KM respuesta = eliminar_segmento(pid,id_segmento);
            send (cliente_fd, &respuesta,sizeof(respuesta),0);
            break;
        }
        case OP_READ_MEM:{
            int pid;
            uint32_t tamanio, dir_fisica;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv (cliente_fd, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
            recv(cliente_fd, &tamanio, sizeof(int), MSG_WAITALL);
            uint8_t *leido=malloc(tamanio);
            t_rsp_KM respuesta = leer_dir(dir_fisica,leido,tamanio,0);
            send(cliente_fd,&respuesta,sizeof(respuesta),0);
            send(cliente_fd,leido,tamanio,0);
            log_info(logger, "##PID: %d, Escritura, Dir. fisica: %u, Tamanio: %u \n", pid, dir_fisica, tamanio);
            break;
        }
        case OP_WRITE_MEM:{
            int pid;
            uint32_t tamanio, dir_fisica;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv (cliente_fd, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
            recv(cliente_fd, &tamanio, sizeof(int), MSG_WAITALL);
            uint8_t contenido = malloc (tamanio);
            recv(cliente_fd,&contenido,tamanio,MSG_WAITALL);
            t_rsp_KM respuesta = escribir_dir(dir_fisica, tamanio, &contenido, pid);
            send(cliente_fd,&respuesta,sizeof(respuesta),0);
            break;
        }
        case OP_GET_INSTRUCCION:{
            int pid,pc;
            recv(cliente_fd,&pid,sizeof(int),MSG_WAITALL);
            recv(cliente_fd,&pc,sizeof(int),MSG_WAITALL);
            t_proceso_control* proceso = buscar_proceso_por_pid(pid);
            if (proceso==NULL) {
                t_rsp_KM resp = PID_NOT_FOUND;
                send(cliente_fd,&resp, sizeof(resp),0);
                break;
            }
            char* instruccion = leer_instruccion_de_archivo(proceso->path_instrucciones,pc);
            if(instruccion==NULL){
                t_rsp_KM resp2= ERROR;
                send(cliente_fd,&resp2, sizeof(resp2),0);
                break;
            }
            int tamanio=strlen(instruccion);
            send(cliente_fd,&tamanio, sizeof(int),0);
            send(cliente_fd,instruccion,tamanio,0);
            free(instruccion);
        }
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
        total += hueco->tamanio;
    }
    pthread_mutex_unlock(&mutex_lista_huecos);
    return total;
}

t_segmento* crear_segmento(int id_segmento, uint32_t tamanio) {
    t_segmento* nuevo_segmento = malloc(sizeof(t_segmento));
    nuevo_segmento->id_segmento = id_segmento;
    nuevo_segmento->tamanio = tamanio;
    return nuevo_segmento;
}

t_rsp_KM agregar_segmento(int pid, t_segmento* seg){
    uint32_t tamanio_seg = seg->tamanio;
    uint32_t limite_seg = seg->tamanio + seg->base;
    if (tamanio_seg> max_segmento){
        return SEGMENTO_EXCEDIDO;
    }
    t_proceso_control* proceso = buscar_proceso_por_pid(pid);
    if (proceso == NULL) {
        return PID_NOT_FOUND;
    }
    phtread_mutex_lock(&mutex_lista_huecos);
    pthread_mutex_lock(&mutex_lista_segmentos);
    t_list* posibles =filtrar_huecos_por_tamanio(huecos_libres,tamanio_seg);
    if (list_is_empty(posibles)) {
        free(seg);
        pthread_mutex_unlock(&mutex_lista_segmentos);
        pthread_mutex_unlock(&mutex_lista_huecos);
        log_error(logger, "## PID: %d - No se encontró un hueco libre adecuado para el segmento %d de tamaño %d", proceso->pid, seg->id_segmento, tamanio_seg);
        return FRAGMENTACION_EXT;
    }
    t_hueco* hueco_elegido = list_get_minimum(posibles, hueco_mas_chico);
    list_destroy(posibles);
    if(hueco_elegido-> tamanio == tamanio_seg){
        list_remove_element (huecos_libres, hueco_elegido);
        free (hueco_elegido);
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
    list_add(proceso->contexto.segmentos,seg);
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

t_stick* donde_guardo(uint32_t base){
    t_stick* resultado;
    pthread_mutex_lock(&mutex_lista_sticks);
    for (int i=0; i<list_size(sticks);i++){
        t_stick* pruebo = list_get (sticks, i);
        if (base >= pruebo -> base && base<(pruebo->base + pruebo->tamanio)){
            resultado = pruebo;
        }
    }
    pthread_mutex_unlock(&mutex_lista_sticks);
    return resultado;
}

t_rsp_KM eliminar_segmento (int pid, int id_segmento){
    t_proceso_control* proceso = buscar_proceso_por_pid(pid);
    if (proceso == NULL) {
        return PID_NOT_FOUND;
    }
    t_segmento* eliminado;
    pthread_mutex_lock(&mutex_lista_procesos);
    t_list* tabla_segmentos = proceso->contexto.segmentos;
    for(int i = 0; i<list_size(tabla_segmentos);i++){
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
    list_remove_element(segmentos,eliminado);
    free(eliminado);
    pthread_mutex_unlock(&mutex_lista_segmentos);
    return OK;
}

t_rsp_KM leer_dir(uint32_t direccion, uint32_t tamanio, uint8_t* leido,uint32_t b_leidos){
    t_stick* stick = donde_guardo(direccion);
    if(stick==NULL){
        return ERROR;
    }
    t_operacion op = OP_READ_MEM;
    pthread_mutex_lock(&mutex_lista_sticks);
    send(stick -> fd_stick,&op, sizeof(op),0);
    uint32_t limite_stick = stick -> base + stick -> tamanio;
    uint32_t direccion_relativa = direccion - stick->base;
    send(stick->fd_stick,&direccion_relativa,sizeof(uint32_t),0);
    if (direccion+tamanio <= limite_stick){
        send(stick->fd_stick,&tamanio,sizeof(uint32_t),0);
        recv(stick->fd_stick,leido+b_leidos,tamanio,MSG_WAITALL);
        pthread_mutex_unlock(&mutex_lista_sticks);
        return OK;
    }
    uint32_t tam = limite_stick - direccion;
    send(stick->fd_stick,&tam,sizeof(tam),0);
    recv(stick->fd_stick,leido+b_leidos,tam,MSG_WAITALL);
    pthread_mutex_unlock(&mutex_lista_sticks);
    b_leidos =+tam;
    uint32_t siguiente_base = direccion + tam;
    uint32_t restante = tamanio - tam;
    return leer_dir(siguiente_base,restante,leido,b_leidos);
}

t_rsp_KM escribir_dir(uint32_t direccion, uint32_t tamanio, uint8_t* escrito,uint32_t b_escritos){
    t_stick* stick = donde_guardo(direccion);
    if(stick==NULL){
        return ERROR;
    }
    t_operacion op = OP_WRITE_MEM;
    pthread_mutex_lock(&mutex_lista_sticks);
    send(stick -> fd_stick,&op, sizeof(op),0);
    uint32_t limite_stick = stick -> base + stick -> tamanio;
    uint32_t direccion_relativa = direccion - stick->base;
    send(stick->fd_stick,&direccion_relativa,sizeof(uint32_t),0);
    if (direccion+tamanio <= limite_stick){
        send(stick->fd_stick,&tamanio,sizeof(uint32_t),0);
        send(stick->fd_stick,escrito+b_escritos,tamanio,0);
        pthread_mutex_unlock(&mutex_lista_sticks);
        return OK;
    }
    uint32_t tam = limite_stick - direccion;
    send(stick->fd_stick,&tam,sizeof(tam),0);
    send(stick->fd_stick,escrito+b_escritos,tam,0);
    pthread_mutex_unlock(&mutex_lista_sticks);
    b_escritos =+tam;
    uint32_t siguiente_base = direccion + tam;
    uint32_t restante = tamanio - tam;
    return escribir_dir(siguiente_base,restante,escrito,b_escritos);
}

t_list* filtrar_huecos_por_tamanio(t_list* huecos, uint32_t tamanio_seg) {
    t_list* filtrados = list_create();
    for (int i = 0; i < list_size(huecos); i++) {
        t_hueco* hueco = list_get(huecos, i);

        if (hueco->tamanio >= tamanio_seg) {
            list_add(filtrados, hueco);
        }
    }

    return filtrados;
}

char* leer_instruccion_de_archivo(char* path, uint32_t pc) {
    FILE* archivo = fopen(path, "r");
    if (archivo == NULL){
        return NULL;
    }
    char* linea = malloc(128); //espacio suficiente para una instruccion
    uint32_t renglon_actual = 0; //contador

    //recorro el archivo renglon por renglon
    while(fgets(linea, 128, archivo) != NULL) { //leo una linea completa de un archivo txt
        if(renglon_actual == pc) { //encontramos el pc buscado, se saca el \n del final si existe
            linea[strcspn(linea, "\n")] = '\0';
            fclose(archivo);
            return linea; //buffer donde se va a guardar lo que leyo
        }
        renglon_actual ++;
    }
    fclose(archivo);
    free(linea);
    return NULL;
}
