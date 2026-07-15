#include "planificador.h"

// Variables Globales
extern t_log* logger;
extern void registrar_proceso_en_km(int pid, char* path);
extern char* planification_algorithm;
extern char** queues_algorithms;
extern int    queues_count;
extern bool   queue_preemption;
typedef struct {
    int pid;
    int numero_bloqueo;
} t_args_suspension;

static void* hilo_suspension(void* arg);
static void encolar_block_sin_timer(t_pcb* pcb);
extern int suspension_timeout;
extern int fd_km;
extern pthread_mutex_t mutex_km;
int cpus_pendientes_compactacion = 0;
bool compactando = false;

// Colas globales
t_queue*  cola_new;
t_queue** colas_ready;
int       num_colas;
t_queue*  cola_block;
t_queue* cola_susp_block;
t_queue* cola_susp_ready;

// Listas globales
t_list* lista_cpus;
t_list* lista_ios;
t_list* lista_mutex;
t_list* lista_desalojados;

// Mutex
pthread_mutex_t mutex_ready  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_block  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cpus   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ios    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mutex  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_susp = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_compactacion = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t  cond_compactacion  = PTHREAD_COND_INITIALIZER;


// PID autoincremental
static int siguiente_pid = 0;
static pthread_mutex_t mutex_pid = PTHREAD_MUTEX_INITIALIZER;

void iniciar_planificador() {
    cola_new   = queue_create();
    cola_block = queue_create();
    lista_cpus  = list_create();
    lista_ios   = list_create();
    lista_mutex = list_create();
    cola_susp_block = queue_create();
    cola_susp_ready = queue_create();
    lista_desalojados = list_create();

    if (strcmp(planification_algorithm, "CMN") == 0) {
        num_colas = queues_count;
    } else {
        num_colas = 1;
    }
    colas_ready = malloc(num_colas * sizeof(t_queue*));
    for (int i = 0; i < num_colas; i++)
        colas_ready[i] = queue_create();
}

t_pcb* crear_pcb(int pid, char* path, int prioridad) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->prioridad = prioridad;
    pcb->prioridad_original = prioridad;
    pcb->estado = NEW;
    pcb->path_instrucciones = strdup(path);
    pcb->param_io = 0;
    pcb->fd_cpu_dispatch = -1;
    pcb->numero_bloqueo = 0;
    memset(&pcb->contexto, 0, sizeof(t_contexto));
    return pcb;
}

void crear_proceso_inicial(char* path) {                                           
      static int creado = 0;                                                         
      static pthread_mutex_t mutex_init = PTHREAD_MUTEX_INITIALIZER;                 
                                                                                   
      pthread_mutex_lock(&mutex_init);                                               
      if (!creado) {                  
          creado = 1;                                                                
          pthread_mutex_unlock(&mutex_init);
          int pid = generar_pid();                                                 
          t_pcb* pcb = crear_pcb(pid, path, 0);   
          log_info(logger, "## (%d) Se crea el proceso - Estado: NEW", pid);
          registrar_proceso_en_km(pid, path);                                   
          encolar_ready(pcb);                  
      } else {                                                                       
          pthread_mutex_unlock(&mutex_init);
      }                                                                              
}

int generar_pid() { // PID: Procces ID
    pthread_mutex_lock(&mutex_pid);
    int pid = siguiente_pid++;
    pthread_mutex_unlock(&mutex_pid);
    return pid;
}

void cambiar_estado(t_pcb* pcb, t_estado nuevo_estado) {
    // Log obligatorio del enunciado
    char* estados[] = {"NEW", "READY", "EXEC", "BLOCK",
                       "SUSP_READY", "SUSP_BLOCK", "EXIT"};
    log_info(logger, "## (%d) Pasa del estado %s al estado %s",
             pcb->pid,
             estados[pcb->estado],
             estados[nuevo_estado]);
    pcb->estado = nuevo_estado;
}

void encolar_ready(t_pcb* pcb) {
    cambiar_estado(pcb, READY);

    int index;
    if (strcmp(planification_algorithm, "CMN") == 0) {
        index = pcb->prioridad;
    } else {
        index = 0;
    }
    if (index < 0 || index >= num_colas) {
        log_warning(logger, "## (%d) Prioridad %d fuera de rango, proceso no planificado", pcb->pid, pcb->prioridad);
        return;
    }

    pthread_mutex_lock(&mutex_ready);
    queue_push(colas_ready[index], pcb);
    log_info(logger, "DEBUG: encolo PID %d, cola[%d] queda con %d elementos", pcb->pid, index, queue_size(colas_ready[index]));
    pthread_mutex_unlock(&mutex_ready);

    if (strcmp(planification_algorithm, "CMN") == 0 && queue_preemption) {
        pthread_mutex_lock(&mutex_cpus);
        for (int i = 0; i < list_size(lista_cpus); i++) {
            t_cpu* cpu = list_get(lista_cpus, i);
            if (!cpu->libre && cpu->pcb_actual != NULL &&
                cpu->pcb_actual->prioridad > pcb->prioridad &&
                !cpu->interrupcion_pendiente && cpu->fd_interrupt >= 0) {
                int interrupcion = 1;
                send(cpu->fd_interrupt, &interrupcion, sizeof(int), 0);
                cpu->interrupcion_pendiente = true;
                cpu->interrupcion_prioridad = true;   // <- agregar esto
                log_info(logger, "## (%d) Prioridad: %d - Desalojado por cola más prioritaria por el proceso %d con prioridad %d",
                        cpu->pcb_actual->pid, cpu->pcb_actual->prioridad, pcb->pid, pcb->prioridad);
            }
        }
        pthread_mutex_unlock(&mutex_cpus);
    }

    wake_dispatcher();
}

void reencolar_ready_sin_wake(t_pcb* pcb) {
    // Usado por el dispatcher cuando no hay CPU libre: el pcb sigue lógicamente en READY
    // (no hace falta volver a loguear el cambio de estado) y no hay que despertar al
    // dispatcher de nuevo, porque nada cambió (ninguna CPU se liberó). Despertar acá
    // causaba un loop apretado sin fin: encolar -> wake -> desencolar -> sin CPU -> encolar...
    int index;
    if (strcmp(planification_algorithm, "CMN") == 0) {
        index = pcb->prioridad;
    } else {
        index = 0;
    }
    if (index < 0 || index >= num_colas) {
        log_warning(logger, "## (%d) Prioridad %d fuera de rango, proceso no planificado", pcb->pid, pcb->prioridad);
        return;
    }

    pthread_mutex_lock(&mutex_ready);
    queue_push(colas_ready[index], pcb);
    pthread_mutex_unlock(&mutex_ready);
}

t_pcb* desencolar_ready() {
    pthread_mutex_lock(&mutex_ready);
    t_pcb* pcb = NULL;
    for (int i = 0; i < num_colas; i++) {
        if (!queue_is_empty(colas_ready[i])) {
            pcb = queue_pop(colas_ready[i]);
            log_info(logger, "DEBUG: desencolo PID %d de cola[%d]", pcb->pid, i);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_ready);
    return pcb;
}

void encolar_block(t_pcb* pcb) {
    cambiar_estado(pcb, BLOCK);
    pcb->numero_bloqueo++;
    pthread_mutex_lock(&mutex_block);
    queue_push(cola_block, pcb);
    pthread_mutex_unlock(&mutex_block);

    // Timer de suspensión
    t_args_suspension* args = malloc(sizeof(t_args_suspension));
    args->pid = pcb->pid;
    args->numero_bloqueo = pcb->numero_bloqueo;
    pthread_t th;
    pthread_create(&th, NULL, hilo_suspension, args);
    pthread_detach(th);
}

static void* hilo_suspension(void* arg) {
    t_args_suspension* args = arg;
    int pid = args->pid;
    int numero_bloqueo = args->numero_bloqueo;
    free(args);

    usleep((long)suspension_timeout * 1000);

    // Buscar el proceso en BLOCK (si ya salió, o si se volvió a bloquear después
    // de este timer, no hacemos nada: este timer quedó obsoleto)
    t_pcb* pcb = buscar_en_block(pid);
    if (pcb == NULL) return NULL;
    if (pcb->numero_bloqueo != numero_bloqueo) {
        encolar_block_sin_timer(pcb);
        return NULL;
    }

    suspender_proceso(pcb);
    return NULL;
}

// Repone en cola_block un pcb que ya tiene su propio timer de suspensión vigente
// (armado por el bloqueo más reciente); usado cuando un timer viejo y obsoleto
// lo desencola solo para descartarlo.
static void encolar_block_sin_timer(t_pcb* pcb) {
    pthread_mutex_lock(&mutex_block);
    queue_push(cola_block, pcb);
    pthread_mutex_unlock(&mutex_block);
}

void suspender_proceso(t_pcb* pcb) {
    // Notificar a KM que lo mande a SWAP
    pthread_mutex_lock(&mutex_km);
    t_operacion op = OP_SUSPEND;
    send(fd_km, &op, sizeof(t_operacion), 0);
    send(fd_km, &pcb->pid, sizeof(int), 0);
    int ok;
    recv(fd_km, &ok, sizeof(int), MSG_WAITALL);
    pthread_mutex_unlock(&mutex_km);

    cambiar_estado(pcb, SUSP_BLOCK);
    pcb->tiempo_suspension = time(NULL);
    log_info(logger, "## (%d) Se suspende el proceso - Estado: SUSP_BLOCK", pcb->pid);
    pthread_mutex_lock(&mutex_susp);
    queue_push(cola_susp_block, pcb);
    pthread_mutex_unlock(&mutex_susp);
  }



t_cpu* buscar_cpu_libre() {
    pthread_mutex_lock(&mutex_cpus);
    t_cpu* cpu_libre = NULL;
    for (int i = 0; i < list_size(lista_cpus); i++) {
        t_cpu* cpu = list_get(lista_cpus, i);
        if (cpu->libre) {
            cpu_libre = cpu;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_cpus);
    return cpu_libre;
}

t_io_registrada* buscar_io(t_tipo_io tipo) {
      pthread_mutex_lock(&mutex_ios);
      t_io_registrada* encontrada = NULL;
      for (int i = 0; i < list_size(lista_ios); i++) {
          t_io_registrada* io = list_get(lista_ios, i);
          if (io->tipo == tipo && io->libre) {
              encontrada = io;
              break;
          }
      }
      pthread_mutex_unlock(&mutex_ios);
      return encontrada;
  }

t_pcb* buscar_en_block(int pid){
    pthread_mutex_lock(&mutex_block);                                                                                                                                                                  
    t_pcb* encontrado = NULL;
    for (int i = 0; i < list_size(cola_block->elements); i++) {     // cola_block->elements es el t_list*                                          
        t_pcb* pcb = list_get(cola_block->elements, i);                                                       
          if (pcb->pid == pid) {                                                                                
                encontrado = pcb;                                                                                 
                list_remove(cola_block->elements, i);                                                             
            break;                                                                                          
          }
      }
      pthread_mutex_unlock(&mutex_block);                                                                       
      return encontrado;
}

t_cpu* buscar_cpu_por_fd(int fd) {                                                                            
      pthread_mutex_lock(&mutex_cpus);                                                                        
      t_cpu* encontrada = NULL;                                                                                 
      for (int i = 0; i < list_size(lista_cpus); i++) {                                                         
          t_cpu* cpu = list_get(lista_cpus, i);                                                                 
          if (cpu->fd == fd) {                                                                                  
              encontrada = cpu;                                                                                 
              break;
          }                                                                                                     
      }                                                                                                       
      pthread_mutex_unlock(&mutex_cpus);
      return encontrada;
}

t_cpu* buscar_cpu_por_id(int id) {
      pthread_mutex_lock(&mutex_cpus);
      t_cpu* encontrada = NULL;
      for (int i = 0; i < list_size(lista_cpus); i++) {
          t_cpu* cpu = list_get(lista_cpus, i);
          if (cpu->id == id) {
              encontrada = cpu;
              break;
          }
      }
      pthread_mutex_unlock(&mutex_cpus);
      return encontrada;
}


t_io_registrada* buscar_io_por_fd(int fd){
    pthread_mutex_lock(&mutex_ios);                                                                           
        t_io_registrada* encontrada = NULL;                                                                       
        for (int i = 0; i < list_size(lista_ios); i++) {                                                        
            t_io_registrada* io = list_get(lista_ios, i);                                                         
            if (io->fd == fd) {                                                                                 
                encontrada = io;                                                                                  
                break;                                                                                          
            }
        }
        pthread_mutex_unlock(&mutex_ios);
        return encontrada;
}

t_io_registrada* buscar_io_por_tipo(t_tipo_io tipo) {
    pthread_mutex_lock(&mutex_ios);
    t_io_registrada* libre = NULL;
    t_io_registrada* primera_del_tipo = NULL;
    for (int i = 0; i < list_size(lista_ios); i++) {
        t_io_registrada* io = list_get(lista_ios, i);
        if (io->tipo == tipo) {
            if (primera_del_tipo == NULL) primera_del_tipo = io;
            if (io->libre) {
                libre = io;
                break;
            }
        }
    }
    pthread_mutex_unlock(&mutex_ios);
    return (libre != NULL) ? libre : primera_del_tipo;
}

t_mutex* buscar_mutex(char* nombre) {                                              
    pthread_mutex_lock(&mutex_mutex);
    t_mutex* encontrado = NULL;                                                    
    for (int i = 0; i < list_size(lista_mutex); i++) {                             
        t_mutex* m = list_get(lista_mutex, i);                                   
        if (strcmp(m->nombre, nombre) == 0) {                                      
            encontrado = m;                                                        
            break;                                                               
        }                                                                          
    }           
    pthread_mutex_unlock(&mutex_mutex);                                          
    return encontrado;                 
  }                                                                                  
   
t_mutex* crear_mutex(char* nombre) {                                               
    t_mutex* m = malloc(sizeof(t_mutex));
    m->nombre = strdup(nombre);                                                  
    m->owner_pid = -1;         
    m->cola_espera = queue_create();                                               
    pthread_mutex_lock(&mutex_mutex);
    list_add(lista_mutex, m);                                                      
    pthread_mutex_unlock(&mutex_mutex);
    return m;                                                                      
} 

void aplicar_herencia_prioridad(int owner_pid, int nueva_prioridad) {

    // Caso 1: owner está EXEC en una CPU
    pthread_mutex_lock(&mutex_cpus);
    for (int i = 0; i < list_size(lista_cpus); i++) {
        t_cpu* cpu = list_get(lista_cpus, i);
        if (!cpu->libre && cpu->pcb_actual != NULL && cpu->pcb_actual->pid == owner_pid) {
            if (nueva_prioridad < cpu->pcb_actual->prioridad) {
                int prioridad_anterior = cpu->pcb_actual->prioridad;
                cpu->pcb_actual->prioridad = nueva_prioridad;
                log_info(logger, "## %d Cambio de prioridad: %d - %d", owner_pid, prioridad_anterior, nueva_prioridad);
            }
            pthread_mutex_unlock(&mutex_cpus);
            return;
        }
    }
    pthread_mutex_unlock(&mutex_cpus);

    // Caso 2: owner está en READY — moverlo a la cola correcta
    pthread_mutex_lock(&mutex_ready);
    for (int i = 0; i < num_colas; i++) {
        t_list* elementos = colas_ready[i]->elements;
        for (int j = 0; j < list_size(elementos); j++) {
            t_pcb* pcb = list_get(elementos, j);
            if (pcb->pid == owner_pid) {
                if (nueva_prioridad < pcb->prioridad && nueva_prioridad < num_colas) {
                    int prioridad_anterior = pcb->prioridad;
                    list_remove(elementos, j);
                    pcb->prioridad = nueva_prioridad;
                    queue_push(colas_ready[nueva_prioridad], pcb);
                    log_info(logger, "## %d Cambio de prioridad: %d - %d", owner_pid, prioridad_anterior, nueva_prioridad);
                }
                pthread_mutex_unlock(&mutex_ready);
                return;
            }
        }
    }
    pthread_mutex_unlock(&mutex_ready);

    // Caso 3: owner está en BLOCK (tiene el mutex pero espera IO)
    pthread_mutex_lock(&mutex_block);
    for (int i = 0; i < list_size(cola_block->elements); i++) {
        t_pcb* pcb = list_get(cola_block->elements, i);
        if (pcb->pid == owner_pid) {
            if (nueva_prioridad < pcb->prioridad) {
                int prioridad_anterior = pcb->prioridad;
                pcb->prioridad = nueva_prioridad;
                log_info(logger, "## %d Cambio de prioridad: %d - %d", owner_pid, prioridad_anterior, nueva_prioridad);
            }
            pthread_mutex_unlock(&mutex_block);
            return;
        }
    }
    pthread_mutex_unlock(&mutex_block);
    // Caso 4: owner está en SUSP_BLOCK (tiene el mutex, fue suspendido mientras esperaba IO)
    pthread_mutex_lock(&mutex_susp);
    for (int i = 0; i < list_size(cola_susp_block->elements); i++) {
        t_pcb* pcb = list_get(cola_susp_block->elements, i);
        if (pcb->pid == owner_pid) {
            if (nueva_prioridad < pcb->prioridad) {
                int prioridad_anterior = pcb->prioridad;
                pcb->prioridad = nueva_prioridad;
                log_info(logger, "## %d Cambio de prioridad: %d - %d", owner_pid, prioridad_anterior, nueva_prioridad);
            }
            pthread_mutex_unlock(&mutex_susp);
            return;
        }
    }
    pthread_mutex_unlock(&mutex_susp);
}


t_pcb* buscar_en_susp_block(int pid) {
    pthread_mutex_lock(&mutex_susp);
    t_pcb* encontrado = NULL;
    for (int i = 0; i < list_size(cola_susp_block->elements); i++) {
        t_pcb* pcb = list_get(cola_susp_block->elements, i);
        if (pcb->pid == pid) {
            encontrado = pcb;
            list_remove(cola_susp_block->elements, i);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_susp);
    return encontrado;
}

void encolar_susp_ready(t_pcb* pcb) {
    cambiar_estado(pcb, SUSP_READY);
    pthread_mutex_lock(&mutex_susp);
    queue_push(cola_susp_ready, pcb);
    pthread_mutex_unlock(&mutex_susp);
}

void intentar_desuspender() {
    pthread_mutex_lock(&mutex_susp);
    if (queue_is_empty(cola_susp_ready)) {
        pthread_mutex_unlock(&mutex_susp);
        return;
      }
    t_pcb* candidato = NULL;
    int mejor = -1;
    for (int i = 0; i < list_size(cola_susp_ready->elements); i++) {
        t_pcb* pcb = list_get(cola_susp_ready->elements, i);
        if (mejor == -1 || pcb->prioridad < mejor ||
            (pcb->prioridad == mejor && pcb->tiempo_suspension < candidato->tiempo_suspension)) {
            mejor = pcb->prioridad;
            candidato = pcb;
        }
    }
    for (int i = 0; i < list_size(cola_susp_ready->elements); i++) {
        if (list_get(cola_susp_ready->elements, i) == candidato) {
            list_remove(cola_susp_ready->elements, i);
            break;
        }
    }
    pthread_mutex_unlock(&mutex_susp);

    pthread_mutex_lock(&mutex_km);
    t_operacion op = OP_DESUSPEND;
    send(fd_km, &op, sizeof(t_operacion), 0);
    send(fd_km, &candidato->pid, sizeof(int), 0);
    int ok;
    recv(fd_km, &ok, sizeof(int), MSG_WAITALL);
    pthread_mutex_unlock(&mutex_km);

    if(ok == 0) {
        log_info(logger, "## (%d) Se des-suspende el proceso", candidato->pid);
        encolar_ready(candidato);
    } else {
        log_info(logger, "## (%d) Sin espacio para des-suspender, vuelve a SUSP_READY", candidato->pid);
        pthread_mutex_lock(&mutex_susp);
        queue_push(cola_susp_ready, candidato);
        pthread_mutex_unlock(&mutex_susp);
    }
}

void reencolar_ready(t_pcb* pcb) { // Reencola al frente no al final
    cambiar_estado(pcb, READY);

    int index;
    if (strcmp(planification_algorithm, "CMN") == 0) {
        index = pcb->prioridad;
      } else {
        index = 0;
    }
    if (index < 0 || index >= num_colas) return;
    
    pthread_mutex_lock(&mutex_ready);
    list_add_in_index(colas_ready[index]->elements, 0, pcb);
    pthread_mutex_unlock(&mutex_ready);
    wake_dispatcher();
}

void iniciar_compactacion(t_pcb* pcb_original) {
    log_info(logger, "## Inicio de compactación");
    pthread_mutex_lock(&mutex_compactacion);
    compactando = true;

    // Paso 1: interrumpir todas las CPUs ocupadas
      pthread_mutex_lock(&mutex_cpus);
      cpus_pendientes_compactacion = 0;
      for (int i = 0; i < list_size(lista_cpus); i++) {
          t_cpu* cpu = list_get(lista_cpus, i);
          if (!cpu->libre && cpu->fd_interrupt >= 0) {
              cpu->interrupcion_compactacion = true;
              cpus_pendientes_compactacion++;
              int interrupcion = 1;
              send(cpu->fd_interrupt, &interrupcion, sizeof(int), 0);
          }
      }
      pthread_mutex_unlock(&mutex_cpus);

    // Paso 2: esperar que todas las CPUs devuelvan su proceso
      while (cpus_pendientes_compactacion > 0)
          pthread_cond_wait(&cond_compactacion, &mutex_compactacion);

    // Pasos 3 y 4: avisar a KM y esperar fin de compactación
      pthread_mutex_lock(&mutex_km);
      t_operacion op = OP_COMPACT_OK;
      send(fd_km, &op, sizeof(t_operacion), 0);
      int ok_compact;
      recv(fd_km, &ok_compact, sizeof(int), MSG_WAITALL);
      pthread_mutex_unlock(&mutex_km);
      log_info(logger, "## Fin de compactación");

    // Paso 5a: reintentar MEM_ALLOC para el proceso que lo disparó
      pthread_mutex_lock(&mutex_km);
      op = OP_MEM_ALLOC;
      send(fd_km, &op, sizeof(t_operacion), 0);
      send(fd_km, &pcb_original->pid, sizeof(int), 0);
      uint32_t tam_u = (uint32_t)pcb_original->tamanio_pendiente;
      send(fd_km, &tam_u, sizeof(uint32_t), 0);
      send(fd_km, &pcb_original->seg_id_pendiente, sizeof(int), 0);
      int32_t ok_alloc;
      recv(fd_km, &ok_alloc, sizeof(int32_t), MSG_WAITALL);
      pthread_mutex_unlock(&mutex_km);

      if (ok_alloc >= 0){
          log_info(logger, "## PID: %d - Segmento Creado %d - Tamaño: %d (post-compactación)", 
                    pcb_original->pid, pcb_original->seg_id_pendiente,
                    pcb_original->tamanio_pendiente);
      } else {
        log_warning(logger, "## PID: %d - Sin memoria incluso tras compactación", pcb_original->pid);
      }
        encolar_ready(pcb_original);

    // Paso 5b: re-encolar desalojados al frente
        for (int i = 0; i < list_size(lista_desalojados); i++)
            reencolar_ready(list_get(lista_desalojados, i));
        list_clean(lista_desalojados);

      intentar_desuspender();
      compactando = false;
      pthread_mutex_unlock(&mutex_compactacion);
}

void bsod() {
    log_error(logger, "## BSOD - Memoria corrupta. Finalizando sistema.");

    pthread_mutex_lock(&mutex_cpus);
    for (int i = 0; i < list_size(lista_cpus); i++) {
        t_cpu* cpu = list_get(lista_cpus, i);
        if (!cpu->libre && cpu->pcb_actual != NULL)
            cambiar_estado(cpu->pcb_actual, EXIT);
    }
    pthread_mutex_unlock(&mutex_cpus);

    pthread_mutex_lock(&mutex_ready);
    for (int i = 0; i < num_colas; i++) {
        while (!queue_is_empty(colas_ready[i])) {
            t_pcb* pcb = queue_pop(colas_ready[i]);
            cambiar_estado(pcb, EXIT);
        }
    }
    pthread_mutex_unlock(&mutex_ready);

    pthread_mutex_lock(&mutex_block);
    while (!queue_is_empty(cola_block)) {
        t_pcb* pcb = queue_pop(cola_block);
        cambiar_estado(pcb, EXIT);
    }
    pthread_mutex_unlock(&mutex_block);

    pthread_mutex_lock(&mutex_susp);
    while (!queue_is_empty(cola_susp_block)) {
        t_pcb* pcb = queue_pop(cola_susp_block);
        cambiar_estado(pcb, EXIT);
    }
    while (!queue_is_empty(cola_susp_ready)) {
        t_pcb* pcb = queue_pop(cola_susp_ready);
        cambiar_estado(pcb, EXIT);
    }
    pthread_mutex_unlock(&mutex_susp);

    exit(1);
}

void notificar_fin_proceso_km(int pid) {
    pthread_mutex_lock(&mutex_km);
    t_operacion op = OP_FIN_PROCESO;
    send(fd_km, &op, sizeof(t_operacion), 0);
    send(fd_km, &pid, sizeof(int), 0);
    int ok;
    recv(fd_km, &ok, sizeof(int), MSG_WAITALL);
    pthread_mutex_unlock(&mutex_km);
}
