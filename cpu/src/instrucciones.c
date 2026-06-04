#include "main.h"

uint32_t get_registro(t_contexto* ctx, char* nombre){
    if (strcmp(nombre, "AX") == 0)  return ctx->ax;
    if (strcmp(nombre, "BX") == 0)  return ctx->bx;
    if (strcmp(nombre, "CX") == 0)  return ctx->cx;
    if (strcmp(nombre, "DX") == 0)  return ctx->dx;
    if (strcmp(nombre, "EAX") == 0) return ctx->eax;
    if (strcmp(nombre, "EBX") == 0) return ctx->ebx;
    if (strcmp(nombre, "ECX") == 0) return ctx->ecx;
    if (strcmp(nombre, "EDX") == 0) return ctx->edx;
    if (strcmp(nombre, "SI") == 0)  return ctx->si;
    if (strcmp(nombre, "DI") == 0)  return ctx->di;
    if (strcmp(nombre, "PC") == 0)  return ctx->pc;
    return 0;
}

void set_registro(t_contexto* ctx, char* nombre, uint32_t valor){
    if (strcmp(nombre, "AX") == 0)  { ctx->ax  = (uint8_t)valor;  return; } //cambia el formato a 8 bits 
    if (strcmp(nombre, "BX") == 0)  { ctx->bx  = (uint8_t)valor;  return; }
    if (strcmp(nombre, "CX") == 0)  { ctx->cx  = (uint8_t)valor;  return; }
    if (strcmp(nombre, "DX") == 0)  { ctx->dx  = (uint8_t)valor;  return; }
    if (strcmp(nombre, "EAX") == 0) { ctx->eax = valor;            return; }
    if (strcmp(nombre, "EBX") == 0) { ctx->ebx = valor;            return; }
    if (strcmp(nombre, "ECX") == 0) { ctx->ecx = valor;            return; }
    if (strcmp(nombre, "EDX") == 0) { ctx->edx = valor;            return; }
    if (strcmp(nombre, "SI") == 0)  { ctx->si  = valor;            return; }
    if (strcmp(nombre, "DI") == 0)  { ctx->di  = valor;            return; }
    if (strcmp(nombre, "PC") == 0)  { ctx->pc  = valor;            return; }
}


static int sizeof_registro(char* nombre) {
    if (strcmp(nombre,"AX")==0 || strcmp(nombre,"BX")==0 ||
        strcmp(nombre,"CX")==0 || strcmp(nombre,"DX")==0) return 1;
    return 4;
}

int ejecutar(char* instruccion, t_contexto* ctx, t_log* logger, int pid, int fd_km, int* tipo_io_out, int* param_io_out, char nombre_mutex_out[32], uint32_t* dir_logica_out, int* seg_id_out, char archivo_proc_out[256]){ //instrucciones que no usan mmu
    
    char nombre[32] = {0};
    char arg1[32]   = {0};
    char arg2[32]   = {0};
    sscanf(instruccion, "%s %s %s", nombre, arg1, arg2);

    //LOG OOBLIGATORIO
    log_info(logger, "## PID: %d - Ejecutando: %s - %s %s", pid, nombre, arg1, arg2);
    
    //NOOP
    if(strcmp(nombre, "NOOP") == 0){
        ctx -> pc++;
        return CONTINUAR;
    }

    //SET
    if(strcmp(nombre, "SET") == 0){
        set_registro(ctx, arg1, atoi(arg2));
        // Si SET modifica PC directamente, no incrementar
        if (strcmp(arg1, "PC") != 0) ctx->pc++;
        return CONTINUAR;
    }

    //SUM
    if (strcmp(nombre, "SUM") == 0) {
        uint32_t res = get_registro(ctx, arg1) + get_registro(ctx, arg2);
        set_registro(ctx, arg1, res);
        ctx->pc++;
        return CONTINUAR;
    }

    //SUB
    if (strcmp(nombre, "SUB") == 0) {
        uint32_t res = get_registro(ctx, arg1) - get_registro(ctx, arg2);
        set_registro(ctx, arg1, res);
        ctx->pc++;
        return CONTINUAR;
    }

    //JNZ
    if (strcmp(nombre, "JNZ") == 0) {
        if (get_registro(ctx, arg1) != 0) {
            ctx->pc = (uint32_t)atoi(arg2); // PC modificado, no incrementar
        } else {
            ctx->pc++;
        }
        return CONTINUAR;
    }

    //EXIT
    if (strcmp(nombre, "EXIT") == 0) {
        return FIN_PROCESO;
    }

    //MOV_IN
    if (strcmp(nombre, "MOV_IN") == 0) {
        uint32_t dir_logica = ctx->si;
        int size = sizeof_registro(arg1);

        t_operacion op = OP_READ_MEM;
        send(fd_km, &op,         sizeof(t_operacion), 0);
        send(fd_km, &pid,        sizeof(int),         0);
        send(fd_km, &dir_logica, sizeof(uint32_t),    0);
        send(fd_km, &size,       sizeof(int),         0);

        int ok;
        recv(fd_km, &ok, sizeof(int), MSG_WAITALL);
        if (ok != 1) return SEG_FAULT;

        uint32_t valor = 0;
        recv(fd_km, &valor, size, MSG_WAITALL);
        set_registro(ctx, arg1, valor);

        log_info(logger, "PID: %d - Acción: LEER - Dirección Física: %u - Valor: %u", pid, dir_logica, valor);
        ctx->pc++;
        return CONTINUAR;
    }

    //MOV_OUT
    if (strcmp(nombre, "MOV_OUT") == 0) {
        uint32_t dir_logica = ctx->di;
        int size = sizeof_registro(arg1);
        uint32_t valor = get_registro(ctx, arg1);

        t_operacion op = OP_WRITE_MEM;
        send(fd_km, &op,         sizeof(t_operacion), 0);
        send(fd_km, &pid,        sizeof(int),         0);
        send(fd_km, &dir_logica, sizeof(uint32_t),    0);
        send(fd_km, &size,       sizeof(int),         0);
        send(fd_km, &valor,      size,                0);

        int ok;
        recv(fd_km, &ok, sizeof(int), MSG_WAITALL);
        if (ok != 1) return SEG_FAULT;

        log_info(logger, "PID: %d - Acción: ESCRIBIR - Dirección Física: %u - Valor: %u", pid, dir_logica, valor);
        ctx->pc++;
        return CONTINUAR;
    }

    //COPY_MEM
    if (strcmp(nombre, "COPY_MEM") == 0) {
        int size = (int)get_registro(ctx, arg1);
        uint32_t dir_src = ctx->si;
        uint32_t dir_dst = ctx->di;

        t_operacion op = OP_READ_MEM;
        send(fd_km, &op,      sizeof(t_operacion), 0);
        send(fd_km, &pid,     sizeof(int),         0);
        send(fd_km, &dir_src, sizeof(uint32_t),    0);
        send(fd_km, &size,    sizeof(int),         0);

        int ok;
        recv(fd_km, &ok, sizeof(int), MSG_WAITALL);
        if (ok != 1) return SEG_FAULT;

        uint8_t* buffer = malloc(size);
        recv(fd_km, buffer, size, MSG_WAITALL);

        op = OP_WRITE_MEM;
        send(fd_km, &op,      sizeof(t_operacion), 0);
        send(fd_km, &pid,     sizeof(int),         0);
        send(fd_km, &dir_dst, sizeof(uint32_t),    0);
        send(fd_km, &size,    sizeof(int),         0);
        send(fd_km, buffer,   size,                0);
        free(buffer);

        recv(fd_km, &ok, sizeof(int), MSG_WAITALL);
        if (ok != 1) return SEG_FAULT;

        ctx->pc++;
        return CONTINUAR;
    }

    //SYSCALLS

    //SLEEP
    if (strcmp(nombre, "SLEEP") == 0) {
        *tipo_io_out  = SYSCALL_IO_SLEEP;
        *param_io_out = atoi(arg1);
        *dir_logica_out = 0;
        ctx->pc++;
        return SYSCALL;
    }

    //STDOUT
    if (strcmp(nombre, "STDOUT") == 0) {
        *tipo_io_out  = SYSCALL_IO_STDOUT;
        *dir_logica_out = get_registro(ctx, arg1);
        *param_io_out = (int)get_registro(ctx, arg2); // Check 3 define qué mandar
        ctx->pc++;
        return SYSCALL;
    }
    //STDIN
    if (strcmp(nombre, "STDIN") == 0) {
        *tipo_io_out  = SYSCALL_IO_STDIN;
        *dir_logica_out = get_registro(ctx, arg1);
        *param_io_out = (int)get_registro(ctx, arg2);
        ctx->pc++;
        return SYSCALL;
    }

    // INSTRUCCIONES MUTEX CREATE
    if (strcmp(nombre, "MUTEX_CREATE") == 0 ||
    strcmp(nombre, "MUTEX_LOCK")   == 0 ||
    strcmp(nombre, "MUTEX_UNLOCK") == 0) {
    if      (strcmp(nombre, "MUTEX_CREATE") == 0) *tipo_io_out = SYSCALL_MUTEX_CREATE;
    else if (strcmp(nombre, "MUTEX_LOCK")   == 0) *tipo_io_out = SYSCALL_MUTEX_LOCK;
    else                                           *tipo_io_out = SYSCALL_MUTEX_UNLOCK;
    strncpy(nombre_mutex_out, arg1, 31);
    ctx->pc++;
    return SYSCALL;
}

    //SYSCALLS NO IMPLEMENTADAS

    //MEM_ALOC
    if (strcmp(nombre, "MEM_ALLOC") == 0) {
        *tipo_io_out  = SYSCALL_MEM_ALLOC;
        *seg_id_out   = atoi(arg1);   // id del segmento
        *param_io_out = atoi(arg2);   // tamaño
        ctx->pc++;
        return SYSCALL;
    }

    //MEM_FREE
    if (strcmp(nombre, "MEM_FREE") == 0) {
        *tipo_io_out = SYSCALL_MEM_FREE;
        *seg_id_out  = atoi(arg1);    // id del segmento
        ctx->pc++;
        return SYSCALL;
    }

    //INIT_PROC
    if (strcmp(nombre, "INIT_PROC") == 0) {
        *tipo_io_out  = SYSCALL_INIT_PROC;
        strncpy(archivo_proc_out, arg1, 255);  // nombre del archivo
        *param_io_out = atoi(arg2);            // prioridad
        ctx->pc++;
        return SYSCALL;
    }

}

