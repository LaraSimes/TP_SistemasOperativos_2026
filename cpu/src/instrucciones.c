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

int ejecutar(char* instruccion, t_contexto* ctx, int fd_ks, t_log* logger, int pid){ //instrucciones que no usan mmu
    
    char nombre[32] = {0};
    char arg1[32]   = {0};
    char arg2[32]   = {0};
    sscanf(instruccion, "%s %s %s", nombre, arg1, arg2);

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

    // Instrucciones de memoria — Check 3
    if (strcmp(nombre, "MOV_IN")   == 0 || 
        strcmp(nombre, "MOV_OUT")  == 0 ||
        strcmp(nombre, "COPY_MEM") == 0) {
        log_warning(logger, "Instruccion de memoria pendiente para Check 3: %s", nombre);
        ctx->pc++;
        return CONTINUAR;
    }

    // Syscalls — protocolo con KS pendiente
    // Por ahora solo loguear, no mandar nada
    log_warning(logger, "Syscall pendiente de implementar: %s", nombre);
    ctx->pc++;
    return SYSCALL;
}

