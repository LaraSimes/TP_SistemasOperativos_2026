void recibir_creacion_proceso(int socket_cliente){
    int pid_recibido;
    int tam_path;

    recv(socket_cliente, &pid_recibido, sizeof(int), MSG_WAITALL); //recibo el pid
    recv(socket_cliente, &tam_path, sizeof(int), MSG_WAITALL); //recibo el tamaño del path para saber cuanto espacio pedir
    
    char* path_recibido= malloc(tam_path);
    recv(socket_cliente, path_recibido, tam_path, MSG_WAITALL);
    
    // creo el proceso en la lista
    t_proceso_control* nuevo_proceso = malloc(sizeof(t_proceso_control)); 
    nuevo_proceso->pid = pid_recibido;
    nuevo_proceso->path_instrucciones = path_recibido;
    //inicializamos todos los registros en 0
    nuevo_proceso->contexto.pc = 0;
    nuevo_proceso->contexto.ax = 0;
    nuevo_proceso->contexto.bx = 0;
    nuevo_proceso->contexto.cx = 0;
    nuevo_proceso->contexto.dx = 0;

    nuevo_proceso->contexto.eax = 0;
    nuevo_proceso->contexto.ebx = 0;
    nuevo_proceso->contexto.ecx = 0;
    nuevo_proceso->contexto.edx = 0;

    nuevo_proceso->contexto.si = 0;
    nuevo_proceso->contexto.di = 0;

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

uint32_t obtener_espacio_libre_mock() {
    return 1024; // Valor fijo solicitado para el Check 2[cite: 1]
}

void recibir_datos_memoria_mock(int socket, int* pid, uint32_t* dir, int* tam) {
    recv(socket, pid, sizeof(int), MSG_WAITALL);
    recv(socket, dir, sizeof(uint32_t), MSG_WAITALL);
    recv(socket, tam, sizeof(int), MSG_WAITALL);
}