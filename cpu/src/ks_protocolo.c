#include "main.h"

void devolver_a_ks(int fd_ks, int pid, int resultado, t_contexto* ctx, int tipo_io, int param_io, uint32_t dir_logica, char* nombre_mutex, int seg_id, char* archivo_proc, t_log* logger){
  send(fd_ks, &resultado, sizeof(int), 0);
  send(fd_ks, &pid, sizeof(int), 0);
  send(fd_ks, ctx, sizeof(t_contexto), 0);

  if (resultado == SYSCALL) {
    end(fd_ks, &tipo_io, sizeof(int), 0);                                        
                                                                                   
    if (tipo_io == SYSCALL_IO_SLEEP) {                                           
      send(fd_ks, &param_io, sizeof(int), 0);
    } else if (tipo_io == SYSCALL_IO_STDIN || tipo_io == SYSCALL_IO_STDOUT) {                                 
      send(fd_ks, &dir_logica, sizeof(uint32_t), 0);
      send(fd_ks, &param_io, sizeof(int), 0);
    } else if (tipo_io <= SYSCALL_MUTEX_UNLOCK){
      char nombre[32] = {0};
      strncpy(nombre, nombre_mutex, 31);
      send(fd_ks, nombre, 32, 0);
    } else if (tipo_io == SYSCALL_MEM_ALLOC) {
      send(fd_ks, &seg_id,   sizeof(int), 0);   // id segmento
      send(fd_ks, &param_io, sizeof(int), 0);   // tamaño
    } else if (tipo_io == SYSCALL_MEM_FREE) {
      send(fd_ks, &seg_id, sizeof(int), 0);     // id segmento
    } else if (tipo_io == SYSCALL_INIT_PROC) {
      send(fd_ks, &param_io, sizeof(int), 0);   // prioridad
      int len = strlen(archivo_proc) + 1;
      send(fd_ks, &len,        sizeof(int), 0);
      send(fd_ks, archivo_proc, len,        0);
    }
  }

    log_info(logger, "## PID: %d - Devuelto a KS con resultado %d", pid, resultado);
}