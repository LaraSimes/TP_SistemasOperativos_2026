#include "server.h"

extern t_log* logger;
extern t_list* lista_cpus;
extern t_list* lista_ios;
extern pthread_mutex_t mutex_cpus;
extern pthread_mutex_t mutex_ios;
extern pthread_mutex_t mutex_mutex;

typedef struct {                                                                   
      int fd;
      char* proceso0_path;                                                           
  } t_cliente_args;        
                                                                                   
void manejar_clientes(int server_fd, char* proceso0_path) {
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
                        int param_io;                                                                                                                                                      
                        recv(cliente_fd, &param_io, sizeof(int), MSG_WAITALL);                                                                                                             

                        char* nombre_io;                                                                   
                        switch (tipo_syscall) {
                            case SYSCALL_IO_SLEEP:  nombre_io = "SLEEP";  break;                           
                            case SYSCALL_IO_STDIN:  nombre_io = "STDIN";  break;                           
                            default:                nombre_io = "STDOUT"; break;                         
                        }                                                                                  
                        log_info(logger, "## (%d) - Solicitó syscall: %s", pcb->pid, nombre_io);


                        pcb->tipo_io_pendiente = tipo_syscall;                                                                                                                             
                        pcb->param_io = param_io;
                        encolar_block(pcb);                                                                                                                                                
                                                                                                                                                                                            
                        t_io_registrada* io = buscar_io_por_tipo(tipo_syscall);
                        if (io != NULL) {                                                                                                                                                  
                            pthread_mutex_lock(&mutex_ios);                                                                                                                              
                            if (io->libre) {                                                                                                                                               
                                io->libre = 0;
                                io->pid_actual = pcb->pid;                                                                                                                                 
                                pthread_mutex_unlock(&mutex_ios);                                                                                                                        
                                send(io->fd, &pcb->pid, sizeof(int), 0);
                                send(io->fd, &param_io, sizeof(int), 0);                                                                                                                   
                            } else {
                                queue_push(io->cola_espera, pcb);                                                                                                                          
                                pthread_mutex_unlock(&mutex_ios);                                                                                                                          
                            }
                        }                                                                                                                                                                  
                    } else {                                                                                                                                                             
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
                                  t_pcb* siguiente = queue_pop(m->cola_espera);      
                                  if (siguiente != NULL) {                           
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
            wake_dispatcher();                                                     
        }                  
        else if(id == MOD_IO){ 
            t_io_registrada* io = buscar_io_por_fd(cliente_fd);  
            int pid = io->pid_actual; 
            
            pthread_mutex_lock(&mutex_ios);                                                    
            t_pcb* siguiente = queue_pop(io->cola_espera);
            if (siguiente != NULL) {                                                         
                // Hay alguien esperando → asignarlo directamente sin liberar el IO
                io->pid_actual = siguiente->pid;                                               
                pthread_mutex_unlock(&mutex_ios);                                              
                send(io->fd, &siguiente->pid, sizeof(int), 0);                                 
                send(io->fd, &siguiente->param_io, sizeof(int), 0);                            
            } else {                                                                           
                // Nadie esperando → liberar el IO                                           
                io->libre = 1;                                                                 
                io->pid_actual = -1;
                pthread_mutex_unlock(&mutex_ios);                                              
            }
                                                                                                
            // En ambos casos, despertar al proceso que terminó IO
            t_pcb* pcb = buscar_en_block(pid);                                               
            if (pcb != NULL) {  
                log_info(logger, "## (%d) finalizó IO y pasa a READY / SUSP. READY", pcb->pid);                                                                
                encolar_ready(pcb);
            }              
        }
    }
}

int recibir_operacion(int socket_cliente) {
    int cod_op;
    int bytes = recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL);
    if (bytes <= 0) return -1;
    return cod_op;
}