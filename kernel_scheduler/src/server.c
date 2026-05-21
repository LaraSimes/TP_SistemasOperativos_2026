#include "server.h"

extern t_log* logger;
extern t_list* lista_cpus;
extern t_list* lista_ios;
extern pthread_mutex_t mutex_cpus;
extern pthread_mutex_t mutex_ios;
extern pthread_mutex_t mutex_mutex;
extern int fd_km;                                                                  
extern pthread_mutex_t mutex_km;


typedef struct {     // struct para la 1er cpu                                                              
      int fd;
      char* proceso0_path;                                                           
  } t_cliente_args;        
                                                                                   
void manejar_clientes(int server_fd, char* proceso0_path) { // recibe el path para asignarselo a la 1er cpu.
    while (1) {                                                                    
        int cliente_fd = esperar_cliente(server_fd);
        if (cliente_fd == -1) continue;                                            
                                         
        t_cliente_args* args = malloc(sizeof(t_cliente_args));                     
        args->fd = cliente_fd;                                
        args->proceso0_path = proceso0_path;  
                                              
        pthread_t hilo;                                                            
        pthread_create(&hilo, NULL, atender_cliente, args);
        pthread_detach(hilo);                                                      
    }                        
} 

void enviar_trabajo_io(t_io_registrada* io, t_pcb* pcb) {                          
    send(io->fd, &pcb->pid, sizeof(int), 0);
                                                                                     
    if (io->tipo == IO_SLEEP) {                                                    
        send(io->fd, &pcb->param_io, sizeof(int), 0);                              
                                                                                     
    } else if (io->tipo == IO_STDIN) {                                           
        send(io->fd, &pcb->param_io, sizeof(int), 0);                              
                  
    } else if (io->tipo == IO_STDOUT) {                                            
        int tamanio = pcb->param_io;

        pthread_mutex_lock(&mutex_km);                                           
        t_operacion op = OP_READ_MEM;                                              
        send(fd_km, &op, sizeof(t_operacion), 0);                                  
        send(fd_km, &pcb->pid, sizeof(int), 0);                                    
        send(fd_km, &pcb->dir_logica, sizeof(uint32_t), 0);                        
        send(fd_km, &tamanio, sizeof(int), 0);                                     
        uint32_t ok_km;                                                          
        recv(fd_km, &ok_km, sizeof(uint32_t), MSG_WAITALL);              
        char* buffer = calloc(tamanio + 1, 1);
        recv(fd_km, buffer, tamanio, MSG_WAITALL);          
        pthread_mutex_unlock(&mutex_km);
                                                                                     
        send(io->fd, &tamanio, sizeof(int), 0);
        send(io->fd, buffer, tamanio, 0);                                          
        free(buffer);                                                              
    }                                                                            
}

void* atender_cliente(void* arg) {
    t_cliente_args* args = (t_cliente_args*)arg;                                 
    int cliente_fd = args->fd;                                                     
    char* proceso0_path = args->proceso0_path;
    free(args); 

    // Handshake 
    t_modulo id;
    if (recv(cliente_fd, &id, sizeof(t_modulo), MSG_WAITALL) <= 0) {
        log_error(logger, "Error en handshake");
        close(cliente_fd);
        return NULL;
    }
    int confirmacion = 1;
    send(cliente_fd, &confirmacion, sizeof(int), 0);

    // Identificación según módulo
    int cpu_id;
    t_tipo_io tipo_io;

    switch (id) {
        case MOD_CPU:
                recv(cliente_fd, &cpu_id, sizeof(int), MSG_WAITALL);
                log_info(logger, "## CPU %d Conectada", cpu_id);
                // cpu es una variable local del stack, cuando salga del case esa memoria puede pisarse. hay que alocarla en el heap. 
                t_cpu* cpu = malloc(sizeof(t_cpu));                                     
                cpu->fd = cliente_fd;                                                                
                cpu->id = cpu_id;
                cpu->libre = 1; 
                cpu->pcb_actual = NULL;      
                cpu->interrupcion_pendiente = false;                                                        
                pthread_mutex_lock(&mutex_cpus);
                list_add(lista_cpus, cpu);                                                         
                pthread_mutex_unlock(&mutex_cpus);
                crear_proceso_inicial(proceso0_path); // sucede 1 vez.
            break;
        case MOD_IO:
                recv(cliente_fd, &tipo_io, sizeof(t_tipo_io), MSG_WAITALL);
                t_io_registrada* io = malloc(sizeof(t_io_registrada));
                io -> fd = cliente_fd;
                io -> libre = 1;
                io -> tipo = tipo_io;
                io -> pid_actual = -1;
                io -> cola_espera = queue_create();
                list_add(lista_ios, io);         
                log_info(logger, "## IO Conectado");
            break;
        default:
                log_warning(logger, "Cliente desconocido");
                close(cliente_fd);
            return NULL;
    }

    // Loop de operaciones
    while (1) {
        int resultado; 
        // CPU → es t_resultado_ciclo (CONTINUAR, SYSCALL, FIN_PROCESO, SEG_FAULT)                                     
        // IO → es el ok = 1 que manda cuando termina                                                                                                
        if (recv(cliente_fd, &resultado, sizeof(int), MSG_WAITALL) <= 0) {                                            
            log_warning(logger, "Cliente desconectado");                                                              
            close(cliente_fd);                                                                                        
            return NULL;                                                                                            
        } 

        // MANEJAR OPERACIONES
        if (id == MOD_CPU) { 

            // Paso 1 - Recibir informacion del proceso:                                                                                 
            int pid;                                                                                           
            t_contexto contexto;                                                                                          
            recv(cliente_fd, &pid, sizeof(int), MSG_WAITALL);                                                             
            recv(cliente_fd, &contexto, sizeof(t_contexto), MSG_WAITALL);   

            // Paso 2 — buscar la CPU y obtener el PCB:
            t_cpu* cpu = buscar_cpu_por_fd(cliente_fd);                                                                   
            t_pcb* pcb = cpu->pcb_actual;

            if (pcb == NULL) { // Defensivo: CPU puede mandar resultados extra tras EXIT mientras no esté implementado el corte correcto de su lado 
                log_error(logger, "## CPU %d: resultado recibido sin PCB asignado, ignorando", cpu->id);       
                wake_dispatcher();                                                           
                continue;
            }    

            // Paso 3 — actualizar el contexto en el PCB:                                                                  
            pcb->contexto = contexto;                                                                                     
                                    
            // Paso 4 — marcar CPU libre:                                                                                    
            pthread_mutex_lock(&mutex_cpus);  
            cpu->interrupcion_pendiente = false;                                                                                          
            cpu->libre = 1;                 
            cpu->pcb_actual = NULL;    
            pthread_mutex_unlock(&mutex_cpus);         

                                                
            // Paso 5 — switch según resultado
            switch(resultado) {                                                                                           
                case FIN_PROCESO:
                    log_info(logger, "## (%d) finalizó su ejecución con motivo de EXIT", pcb->pid);
                    cambiar_estado(pcb, EXIT);                                                                            
                    free(pcb->path_instrucciones);                                                                      
                    free(pcb);                    
                    break;                                                                                                
                case CONTINUAR:                                                                  
                    log_info(logger, "## (%d) - Desalojado por fin de quantum", pcb->pid);         
                    encolar_ready(pcb);                                                            
                    break;
                case SYSCALL: {                                                                                               
                    int tipo_syscall;                                                                                                                                                      
                    recv(cliente_fd, &tipo_syscall, sizeof(int), MSG_WAITALL);                                                                                                             

                    if (tipo_syscall <= SYSCALL_IO_STDOUT) {                                           
                        char* nombre_io;                                                               
                        switch (tipo_syscall) {             
                            case SYSCALL_IO_SLEEP:  nombre_io = "SLEEP";  break;                       
                            case SYSCALL_IO_STDIN:  nombre_io = "STDIN";  break;                       
                            default:                nombre_io = "STDOUT"; break;                     
                        }                                                                              
                        log_info(logger, "## (%d) - Solicitó syscall: %s", pcb->pid, nombre_io);       
                                                                                                        
                        if (tipo_syscall == SYSCALL_IO_SLEEP) {                                        
                            int ms;                                                                    
                            recv(cliente_fd, &ms, sizeof(int), MSG_WAITALL);                           
                            pcb->param_io = ms;                                                        
                            pcb->dir_logica = 0;                                                       
                        } else {                                                                       
                            uint32_t dir_logica;                                                       
                            int tamanio;                                                             
                            recv(cliente_fd, &dir_logica, sizeof(uint32_t), MSG_WAITALL);              
                            recv(cliente_fd, &tamanio, sizeof(int), MSG_WAITALL);
                            pcb->dir_logica = dir_logica;                                              
                            pcb->param_io = tamanio;        
                        }                                                                              
                                                                                                        
                        pcb->tipo_io_pendiente = tipo_syscall;                                       
                        encolar_block(pcb);                                                            
                                                                
                        t_io_registrada* io = buscar_io_por_tipo(tipo_syscall);                        
                        if (io != NULL) {
                            pthread_mutex_lock(&mutex_ios);                                            
                            if (io->libre) {                    
                                io->libre = 0;                                                         
                                io->pid_actual = pcb->pid;
                                pthread_mutex_unlock(&mutex_ios);                                      
                                enviar_trabajo_io(io, pcb); 
                            } else {                                                                   
                                queue_push(io->cola_espera, pcb);                                      
                                pthread_mutex_unlock(&mutex_ios);                                    
                            }                                                                          
                        }                                                                              
                    }                                                                                                                                                                       
                     else {                                                                                                                                                             
                        char nombre[32];
                        recv(cliente_fd, nombre, 32, MSG_WAITALL);                                                                                                                         
                
                        char* nombre_syscall;                                                                                                                                              
                        switch (tipo_syscall) {                                                                                                                                          
                            case SYSCALL_MUTEX_CREATE: nombre_syscall = "MUTEX_CREATE"; break;                                                                                             
                            case SYSCALL_MUTEX_LOCK:   nombre_syscall = "MUTEX_LOCK";   break;                                                                                           
                            default:                   nombre_syscall = "MUTEX_UNLOCK"; break;                                                                                             
                        }
                        log_info(logger, "## (%d) - Solicitó syscall: %s %s", pcb->pid, nombre_syscall, nombre);                                                                           
                                                                                                                                                                                            
                        t_mutex* m = buscar_mutex(nombre);                                                                                                                                 
                                                                                                                                                                                            
                        switch (tipo_syscall) {                                                                                                                                            
                            case SYSCALL_MUTEX_CREATE:                                                                                                                                   
                                if (m == NULL) crear_mutex(nombre);                                                                                                                        
                                encolar_ready(pcb);
                                break;                                                                                                                                                     
                                                                                                                                                                                        
                            case SYSCALL_MUTEX_LOCK:
                                if (m == NULL) m = crear_mutex(nombre);
                                pthread_mutex_lock(&mutex_mutex);                                                                                                                          
                                if (m->owner_pid == -1) {
                                    m->owner_pid = pcb->pid;                                                                                                                               
                                    pthread_mutex_unlock(&mutex_mutex);                                                                                                                  
                                    log_info(logger, "## (%d) Toma el Mutex %s", pcb->pid, nombre);                                                                                        
                                    encolar_ready(pcb);                                                                                                                                  
                                } else {                                                                                                                                                   
                                    queue_push(m->cola_espera, pcb);                                                                                                                     
                                    pthread_mutex_unlock(&mutex_mutex);                                                                                                                    
                                    encolar_block(pcb);                                                                                                                                  
                                }                                                                                                                                                          
                                break;
                                                                                                                                                                                            
                            case SYSCALL_MUTEX_UNLOCK:
                                if (m != NULL) {
                                    pthread_mutex_lock(&mutex_mutex);
                                    log_info(logger, "## (%d) Libera el Mutex %s", pcb->pid, nombre);
                                    if (!queue_is_empty(m->cola_espera)) {
                                        t_pcb* siguiente = queue_pop(m->cola_espera);
                                        m->owner_pid = siguiente->pid;
                                        pthread_mutex_unlock(&mutex_mutex);
                                        log_info(logger, "## (%d) Toma el Mutex %s", siguiente->pid, nombre);
                                        buscar_en_block(siguiente->pid);
                                        encolar_ready(siguiente);
                                    } else {
                                        m->owner_pid = -1;
                                        pthread_mutex_unlock(&mutex_mutex);
                                    }
                                }
                                encolar_ready(pcb);
                                break;
                          }                            
                      }      
                      break;                                  
                  }                                       
              }                                         
        }                  
        else if (id == MOD_IO) { // Cuando un IO termino su trabajo
            t_io_registrada* io = buscar_io_por_fd(cliente_fd);
            int pid = io->pid_actual;

            if (io->tipo == IO_STDIN) {
                int tamanio = resultado;
                char* buffer = malloc(tamanio + 1);
                buffer[tamanio] = '\0';
                recv(cliente_fd, buffer, tamanio, MSG_WAITALL);
                int ok;
                recv(cliente_fd, &ok, sizeof(int), MSG_WAITALL);

                t_pcb* pcb_stdin = buscar_en_block(pid);

                if (pcb_stdin != NULL) {
                    pthread_mutex_lock(&mutex_km);
                    t_operacion op = OP_WRITE_MEM;
                    send(fd_km, &op, sizeof(t_operacion), 0);
                    send(fd_km, &pid, sizeof(int), 0);
                    send(fd_km, &pcb_stdin->dir_logica, sizeof(uint32_t), 0);
                    send(fd_km, &tamanio, sizeof(int), 0);
                    send(fd_km, buffer, tamanio, 0);
                    uint32_t ok_km;
                    recv(fd_km, &ok_km, sizeof(uint32_t), MSG_WAITALL);
                    pthread_mutex_unlock(&mutex_km);
                }
                free(buffer);

                pthread_mutex_lock(&mutex_ios);
                t_pcb* siguiente = queue_pop(io->cola_espera);
                if (siguiente != NULL) {
                    io->pid_actual = siguiente->pid;
                    pthread_mutex_unlock(&mutex_ios);
                    enviar_trabajo_io(io, siguiente);
                } else {
                    io->libre = 1;
                    io->pid_actual = -1;
                    pthread_mutex_unlock(&mutex_ios);
                }

                if (pcb_stdin != NULL) {
                    log_info(logger, "## (%d) finalizó IO y pasa a READY / SUSP. READY", pcb_stdin->pid);
                    encolar_ready(pcb_stdin);
                }

            } else { // STDOUT y SLEEP
                pthread_mutex_lock(&mutex_ios);
                t_pcb* siguiente = queue_pop(io->cola_espera);
                if (siguiente != NULL) {
                    io->pid_actual = siguiente->pid;
                    pthread_mutex_unlock(&mutex_ios);
                    enviar_trabajo_io(io, siguiente);
                } else {
                    io->libre = 1;
                    io->pid_actual = -1;
                    pthread_mutex_unlock(&mutex_ios);
                }

                t_pcb* pcb = buscar_en_block(pid);
                if (pcb != NULL) {
                    log_info(logger, "## (%d) finalizó IO y pasa a READY / SUSP. READY", pcb->pid);
                    encolar_ready(pcb);
                }
            }
            wake_dispatcher();
        }
    }
}

int recibir_operacion(int socket_cliente) {
    int cod_op;
    int bytes = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);
    if (bytes <= 0) return -1;
    return cod_op;
}