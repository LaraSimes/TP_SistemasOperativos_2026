#include "planificador.h"

extern t_log* logger;
extern void registrar_proceso_en_km(int pid, char* path);

// Colas globales
t_queue* cola_new;
t_queue* cola_ready;
t_queue* cola_block;

// Listas globales
t_list* lista_cpus;
t_list* lista_ios;
t_list* lista_mutex;

// Mutex
pthread_mutex_t mutex_ready  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_block  = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_cpus   = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_ios    = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex_mutex  = PTHREAD_MUTEX_INITIALIZER;

// PID autoincremental
static int siguiente_pid = 0;
static pthread_mutex_t mutex_pid = PTHREAD_MUTEX_INITIALIZER;

void iniciar_planificador() { // Crea todas las colas y listas vacías al arrancar
    cola_new   = queue_create();
    cola_ready = queue_create();
    cola_block = queue_create();
    lista_cpus = list_create();
    lista_ios  = list_create();
    lista_mutex = list_create();
}

t_pcb* crear_pcb(int pid, char* path, int prioridad) {
    t_pcb* pcb = malloc(sizeof(t_pcb));
    pcb->pid = pid;
    pcb->prioridad = prioridad;
    pcb->estado = NEW;
    pcb->path_instrucciones = strdup(path);
    pcb->param_io = 0;
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
    pthread_mutex_lock(&mutex_ready);
    queue_push(cola_ready, pcb);
    pthread_mutex_unlock(&mutex_ready);
    wake_dispatcher();
}

t_pcb* desencolar_ready() {
    pthread_mutex_lock(&mutex_ready);
    t_pcb* pcb = queue_pop(cola_ready);
    pthread_mutex_unlock(&mutex_ready);
    return pcb;
}

void encolar_block(t_pcb* pcb) {
    cambiar_estado(pcb, BLOCK);
    pthread_mutex_lock(&mutex_block);
    queue_push(cola_block, pcb);
    pthread_mutex_unlock(&mutex_block);
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
    t_io_registrada* encontrada = NULL;
    for (int i = 0; i < list_size(lista_ios); i++) {
        t_io_registrada* io = list_get(lista_ios, i);
        if (io->tipo == tipo) {
            encontrada = io;
            break;
        }
    }
    pthread_mutex_unlock(&mutex_ios);
    return encontrada;
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