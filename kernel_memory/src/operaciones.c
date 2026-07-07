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
            t_rsp_KM exito = OK;
            send(cliente_fd,&exito, sizeof(exito),0);
            log_info(logger,"## PID: %d - Proceso Creado \n", pid);
            break;
        }
        case OP_GET_FREE_SPACE:{
            uint32_t espacio = obtener_espacio_libre();
            send(cliente_fd, &espacio, sizeof(uint32_t), 0);
            log_info(logger, "## Espacio libre solicitado por Kernel: %u bytes \n", espacio);
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
            if(resultado<0){break;}
            log_info(logger, "## PID: %d - Segmento Creado %d - Tamaño: %u \n", pid,id_segmento,tamanio);
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
            t_rsp_KM respuesta = leer_dir(dir_fisica,tamanio,leido,0);
            send(cliente_fd,&respuesta,sizeof(respuesta),0);
            if(respuesta<0){
                free(leido);
                break;
            }
            send(cliente_fd,leido,tamanio,0);
            log_info(logger, "##PID: %d, Lectura, Dir. fisica: %u, Tamanio: %u \n", pid, dir_fisica, tamanio);
            break;
        }
        case OP_WRITE_MEM:{
            int pid;
            uint32_t tamanio, dir_fisica;
            recv (cliente_fd, &pid, sizeof(int), MSG_WAITALL);
            recv (cliente_fd, &dir_fisica, sizeof(uint32_t), MSG_WAITALL);
            recv(cliente_fd, &tamanio, sizeof(int), MSG_WAITALL);
            uint8_t* contenido = malloc(tamanio);
            recv(cliente_fd,contenido,tamanio,MSG_WAITALL);
            t_rsp_KM respuesta = escribir_dir(dir_fisica, tamanio, contenido, pid);
            send(cliente_fd,&respuesta,sizeof(respuesta),0);
            free(contenido);
            if(respuesta<0){
                break;
            }
            log_info(logger, "##PID: %d, Escritur, Dir. fisica: %u, Tamanio: %u \n", pid, dir_fisica, tamanio);
            break;
        }
        case OP_GET_INSTRUCCION:{
            int pid,pc;
            t_rsp_KM resp;
            recv(cliente_fd,&pid,sizeof(int),MSG_WAITALL);
            recv(cliente_fd,&pc,sizeof(int),MSG_WAITALL);
            log_info(logger, "KM recibio pedido de instruccion. PID: %d - PC: %d", pid, pc);
            t_proceso_control* proceso = buscar_proceso_por_pid(pid);
            log_info(logger, "Despues de buscar_proceso_por_pid. proceso ptr: %p", (void*) proceso);
            if (proceso==NULL) {
                log_error(logger, "No se encontro proceso PID: %d", pid);
                resp = PID_NOT_FOUND;
                send(cliente_fd,&resp, sizeof(resp),0);
                break;
            }
            log_info(logger, "Proceso encontrado. ptr: %p", (void*) proceso);
            log_info(logger, "PID guardado en proceso: %d", proceso->pid);
            log_info(logger, "Path ptr: %p", (void*) proceso->path_instrucciones);
            if (proceso->path_instrucciones == NULL) {
                log_error(logger, "Path del proceso es NULL");
                resp = ERROR;
                send(cliente_fd, &resp, sizeof(resp), 0);
                break;
            }
            char* instruccion = leer_instruccion_de_archivo(proceso->path_instrucciones,pc);
            if(instruccion==NULL){
                log_error(logger,"leer_instruccion_de_archivo devolvio NULL. PID: %d - PC: %d - Path: %s",pid,pc,proceso->path_instrucciones);
                resp= ERROR;
                send(cliente_fd,&resp, sizeof(resp),0);
                break;
            }
            log_info(logger, "Instruccion leida antes de OK: %s", instruccion);
            resp = OK;
            send(cliente_fd,&resp, sizeof(resp),0);
            log_info(logger, "KM mando OK");
            int tamanio=strlen(instruccion);
            log_info(logger,"Estoy mandando tamanio %d \n",tamanio);
            send(cliente_fd,&tamanio, sizeof(int),0);
            send(cliente_fd,instruccion,tamanio,0);
            log_info(logger, "## PID: %d - Obtener instrucción: %d - Instrucción: %s \n", pid, pc, instruccion);
            free(instruccion);
            break;
        }
        case OP_GET_CONTEXT:{
            int pid, tamanio;
            t_rsp_KM resp;
            recv(cliente_fd,&pid,sizeof(int),MSG_WAITALL);
            t_proceso_control* proceso = buscar_proceso_por_pid(pid);
            if (proceso==NULL) {
                resp = PID_NOT_FOUND;
                send(cliente_fd,&resp, sizeof(resp),0);
                break;
            }
            resp = OK;
            send(cliente_fd,&resp, sizeof(int),0);
            t_contexto contextoProceso = obtener_contexto(proceso);
            send(cliente_fd,&contextoProceso,sizeof(t_contexto),0);
            enviar_segmentos(cliente_fd,contextoProceso.segmentos);
            break;
        }
        case OP_SET_CONTEXT:{
            int pid;
            t_contexto contexto;
            recv(cliente_fd,&pid,sizeof(int),MSG_WAITALL);
            recv(cliente_fd,&contexto,sizeof(t_contexto),MSG_WAITALL);
            t_proceso_control* proceso = buscar_proceso_por_pid(pid);
            if (proceso==NULL) {
                t_rsp_KM resp = PID_NOT_FOUND;
                send(cliente_fd,&resp, sizeof(resp),0);
                break;
            }
            actualizar_contexto(proceso,&contexto);
            t_rsp_KM exito = OK;
            send(cliente_fd,&exito, sizeof(exito),0);
            break;
        }
        case OP_SUSPEND:{
            int pid;
            recv(cliente_fd,&pid,sizeof(int),MSG_WAITALL);
            t_proceso_control* p = buscar_proceso_por_pid(pid);
            suspender_proceso(p);
            t_rsp_KM exito = OK;
            send(cliente_fd,&exito, sizeof(exito),0);
            break;
        }
        case OP_DESUSPEND:{
            int pid;
            recv(cliente_fd,&pid,sizeof(int),MSG_WAITALL);
            t_rsp_KM resp = desuspender_proceso(pid);
            send(cliente_fd,&resp, sizeof(resp),0);
            break;
        }
        case OP_COMPACT_OK:{
            t_rsp_KM resultado = compactar();
            send(cliente_fd,&resultado, sizeof(resultado),0);
            break;
        }
        case KS_SERVER_OK:{
            fd_ks_bsod = crear_cliente(ip_ks, puerto_ks);
            handshake(fd_ks_bsod, logger, MOD_KERNEL_MEMORY, MOD_KERNEL_SCHEDULER);
            break;
        }
        default:
            log_warning(logger, "Operacion desconocida de CPU");
            break;
    }
}

void crear_proceso(int pid, char* path) {    
    // creo el proceso en la lista
    t_proceso_control* nuevo_proceso = calloc(1, sizeof(t_proceso_control)); 
    nuevo_proceso->pid = pid;
    nuevo_proceso->path_instrucciones = strdup(path);
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
    pthread_mutex_lock(&mutex_lista_huecos);
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
    t_segmento* eliminado = NULL;
    pthread_mutex_lock(&mutex_lista_procesos);
    t_list* tabla_segmentos = proceso->contexto.segmentos;
    for(int i = 0; i<list_size(tabla_segmentos);i++){
        t_segmento* s = list_get(tabla_segmentos,i);
        if(s->id_segmento == id_segmento){
            eliminado = s;
            list_remove_element(tabla_segmentos, eliminado);
            break;
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
    b_leidos += tam;
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
    b_escritos += tam;
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

t_contexto obtener_contexto(t_proceso_control* proceso){
    pthread_mutex_lock(&mutex_lista_procesos);
    t_contexto ret = proceso->contexto;
    pthread_mutex_unlock(&mutex_lista_procesos);
    return ret;
}

void enviar_segmentos(int cliente_fd, t_list* segmentos) {
    int cantidad_segmentos = 0;

    if (segmentos != NULL) {
        cantidad_segmentos = list_size(segmentos);
    }
    log_info(logger, "KM envia cantidad_segmentos: %d", cantidad_segmentos);
    send(cliente_fd, &cantidad_segmentos, sizeof(int), 0);
    for (int i = 0; i < cantidad_segmentos; i++) {
        t_segmento* segmento = list_get(segmentos, i);

        send(cliente_fd, segmento, sizeof(t_segmento), 0);
    }
}

void actualizar_contexto(t_proceso_control* proceso, t_contexto* cxt){
    pthread_mutex_lock(&mutex_lista_procesos);
    t_contexto* actual = &(proceso->contexto);
    actual->pc= cxt->pc;
    actual->ax = cxt->ax;
    actual->bx = cxt->bx;
    actual->cx = cxt->cx;
    actual->dx = cxt->dx;
    actual->eax = cxt->eax;
    actual->ebx = cxt->ebx;
    actual->ecx = cxt->ecx;
    actual->edx = cxt->edx;
    actual->si = cxt -> si;
    actual->di = cxt ->di;
    pthread_mutex_unlock(&mutex_lista_procesos);
}

void suspender_proceso(t_proceso_control* proceso){
    t_operacion op = OP_SUSPEND;
    send(fd_SWAP,&op,sizeof(op),0);
    pthread_mutex_lock(&mutex_lista_segmentos);
    t_list* segmentos = proceso->contexto.segmentos;
    pthread_mutex_unlock(&mutex_lista_segmentos);
    for(int i=0;i<list_size(segmentos);i++){
        t_segmento* seg = list_get(segmentos,i);
        int id = seg->id_segmento;
        uint32_t tamanio = seg->tamanio;
        send(fd_SWAP,&id,sizeof(int),0);
        send(fd_SWAP,&tamanio,sizeof(uint32_t),0);
        eliminar_segmento(proceso->pid,id);
    }
}

t_rsp_KM desuspender_proceso(int pid){
    t_operacion op= OP_DESUSPEND;
    t_rsp_KM ret= OK;
    int cantidad_seg;
    send(fd_SWAP,&op,sizeof(op),0);
    send(fd_SWAP,&pid,sizeof(int),0);
    recv(fd_SWAP,&cantidad_seg,sizeof(int),MSG_WAITALL);
    for(int i=0;i<cantidad_seg;i++){
        int id;
        uint32_t tamanio;
        recv(fd_SWAP,&id,sizeof(int),0);
        recv(fd_SWAP,&tamanio,sizeof(uint32_t),0);
        uint8_t* contenido = malloc(tamanio);
        recv(fd_SWAP, contenido, tamanio, MSG_WAITALL);
        t_segmento* desus=crear_segmento(id,tamanio);
        ret = agregar_segmento(pid,desus);
        if (ret < 0) {
            free(contenido);
            free(desus);
            return ret;
        }
        escribir_dir(desus->base, tamanio, contenido,0);
        free(contenido);
    }
    return ret;
}

bool menor_base(void* a, void* b) {
    t_segmento* seg_a = (t_segmento*) a;
    t_segmento* seg_b = (t_segmento*) b;

    return seg_a->base < seg_b->base;
}

t_rsp_KM compactar(void){
    uint32_t base = 0;
    t_rsp_KM ret=OK;
    pthread_mutex_lock(&mutex_lista_segmentos);
    list_sort(segmentos, menor_base);
    for(int i = 0; i<list_size(segmentos);i++){
        t_segmento* seg = list_get(segmentos,i);
        uint8_t* contenido = malloc(seg->tamanio);
        if (contenido == NULL) {
            pthread_mutex_unlock(&mutex_lista_segmentos);
            return ERROR;
        }
        ret= leer_dir(seg->base,seg->tamanio,contenido,0);
        if(ret<0){
            free(contenido);
            pthread_mutex_unlock(&mutex_lista_segmentos);
            return ret;
        }
        ret= escribir_dir(base, seg->tamanio,contenido,0);
        if(ret<0){
            free(contenido);
            pthread_mutex_unlock(&mutex_lista_segmentos);
            return ret;
        }
        seg->base = base;
        base += seg->tamanio;
        free(contenido);
    }
    pthread_mutex_unlock(&mutex_lista_segmentos);
    pthread_mutex_lock(&mutex_lista_sticks);
    uint32_t tamanio = 0;
    for(int i=0;i<list_size(sticks);i++){
        t_stick* s = list_get(sticks,i);
        tamanio += s->tamanio;
    }
    pthread_mutex_unlock(&mutex_lista_sticks);
    pthread_mutex_lock(&mutex_lista_huecos);
    list_clean_and_destroy_elements(huecos_libres, free);
    if((tamanio-base)!= 0){
        t_hueco* nuevo = malloc(sizeof(t_hueco));
        if (nuevo == NULL) {
            pthread_mutex_unlock(&mutex_lista_huecos);
            return ERROR;
        }
        nuevo->base = base;
        nuevo->tamanio = tamanio - base;
        list_add(huecos_libres,nuevo);
    }
    pthread_mutex_unlock(&mutex_lista_huecos);
    return ret;
}