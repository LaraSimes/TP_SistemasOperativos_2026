#include "server.h"

extern t_log* logger;
extern t_list* lista_cpus;
extern t_list* lista_ios;
extern pthread_mutex_t mutex_cpus;
extern pthread_mutex_t mutex_ios;

void manejar_clientes(int server_fd) {
    while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        if (cliente_fd == -1) continue; // chequeo de error

        pthread_t hilo;
        int* fd_ptr = malloc(sizeof(int));
        *fd_ptr = cliente_fd;
        pthread_create(&hilo, NULL, atender_cliente, fd_ptr);
        pthread_detach(hilo);
    }
}

void* atender_cliente(void* arg) {
    int cliente_fd = *(int*)arg;
    free(arg);

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
                pthread_mutex_lock(&mutex_cpus);
                list_add(lista_cpus, cpu);                                                         
                pthread_mutex_unlock(&mutex_cpus);
            break;
        case MOD_IO:
                recv(cliente_fd, &tipo_io, sizeof(t_tipo_io), MSG_WAITALL);
                t_io_registrada* io = malloc(sizeof(t_io_registrada));
                io -> fd = cliente_fd;
                io -> libre = 1;
                io -> tipo = tipo_io;
                io -> pid_actual = -1;
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
            cpu->libre = 1;                 
            cpu->pcb_actual = NULL;                                                                                       
            pthread_mutex_unlock(&mutex_cpus);                                                                          
                                                
            // Paso 5 — switch según resultado, por ahora solo estos dos:
            switch(resultado) {                                                                                           
                case FIN_PROCESO:
                    cambiar_estado(pcb, EXIT);                                                                            
                    free(pcb->path_instrucciones);                                                                      
                    free(pcb);                    
                    break;                                                                                                
                case CONTINUAR:
                    encolar_ready(pcb);                                                                                   
                    break;
                case SYSCALL: {                                                                                               
                    int tipo_io, param_io;                                                                                    
                    recv(cliente_fd, &tipo_io, sizeof(int), MSG_WAITALL);                                                   
                    recv(cliente_fd, &param_io, sizeof(int), MSG_WAITALL);                                                    
                
                    pcb->tipo_io_pendiente = tipo_io;                                                                         
                    pcb->param_io = param_io;                                                                               
                                                                                                                                
                    encolar_block(pcb);                                                                                     

                    t_io_registrada* io = buscar_io(tipo_io);                                                                 
                    if (io != NULL) {
                        pthread_mutex_lock(&mutex_ios); //Porque lista_ios es accedida desde múltiples hilos — cada cliente conectado corre en su propio hilo.
                        io->libre = 0;
                        io->pid_actual = pcb->pid; 
                        pthread_mutex_unlock(&mutex_ios);                                                                                      
                        send(io->fd, &pcb->pid, sizeof(int), 0);                                                            
                        send(io->fd, &param_io, sizeof(int), 0);                                                              
                    }
                    break;                                                                                                    
                }                                                                                                        
            }             
            wake_dispatcher(); 
        }
        else if(id == MOD_IO){ // el IO terminó su trabajo, liberamos el IO y desbloqueamos el proceso para que siga ejecutando
            t_io_registrada* io = buscar_io_por_fd(cliente_fd);                                                       
            int pid = io->pid_actual;                                                                                 
                                                                                                                        
            pthread_mutex_lock(&mutex_ios);                                                                           
            io->libre = 1;
            io->pid_actual = -1;                                                                                      
            pthread_mutex_unlock(&mutex_ios);                                                                       

            t_pcb* pcb = buscar_en_block(pid);                                                                        
            if (pcb != NULL) {
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