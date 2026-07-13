#include "swapear.h"
void manejar_operacion(t_operacion op){
    switch(op){
        case OP_SUSPEND:{
            int ret = 0;
            uint32_t direccion, bloque;
            uint8_t* contenido = calloc(1, tamanio_bloque);
            recv(socket_km,&direccion,sizeof(uint32_t),MSG_WAITALL);
            bloque = direccion / tamanio_bloque;
            int res1 = recv(socket_km,contenido,tamanio_bloque,MSG_WAITALL);
            int res2 = escribir_archivo(direccion,contenido);
            if(res1<0 || res2<0){
                ret --;
                free(contenido);
                send(socket_km,&ret,sizeof(int),0);
                log_error(logger, "Error escribiendo en bloque %d \n",bloque);
                break;
            }
            send(socket_km,&ret,sizeof(int),0);
            free(contenido);
            log_info(logger, "Bloque %d escrito \n",bloque);
            break;
        }

        case OP_DESUSPEND:{
            int ret = 0;
            uint32_t direccion,bloque;
            uint8_t* contenido = calloc(1,tamanio_bloque);
            recv(socket_km,&direccion,sizeof(uint32_t),MSG_WAITALL);
            bloque = direccion / tamanio_bloque;
            if(leer_archivo(direccion,contenido)<0){
                ret --;
                free(contenido);
                send(socket_km,&ret,sizeof(int),0);
                log_error(logger,"Error leyendo bloque %d \n",bloque);
                break;
            }
            send(socket_km,&ret,sizeof(int),0);
            if (send(socket_km,contenido,tamanio_bloque,0)<0){
                free(contenido);
                log_error(logger,"Error enviando bloque %d \n",bloque);
                break;
            }
            free(contenido);
            log_info(logger,"Bloque %d leido \n",bloque);
            break;
        }
        default:
            log_info(logger, "Operacion desconocida solicitada \n");
            break;
    }
}


int escribir_archivo(uint32_t direccion, uint8_t* contenido){
    FILE* archivo = fopen(path_SWAP,"r+b");
    if (archivo == NULL){
        return -1;
    }
    if (fseek(archivo, direccion, SEEK_SET) != 0) {
        fclose(archivo);
        return -1;
    }
    if(fwrite(contenido,1,tamanio_bloque,archivo)!=tamanio_bloque){
        fclose(archivo);
        return -1;
    }
    fclose(archivo);
    return 0;
}
int leer_archivo (uint32_t direccion, uint8_t* contenido){
    FILE* archivo = fopen(path_SWAP, "rb");
    if (archivo == NULL){
        return -1;
    }
    if (fseek(archivo, direccion, SEEK_SET) != 0) {
        fclose(archivo);
        return -1;
    }
    if(fread(contenido, 1, tamanio_bloque, archivo)!=tamanio_bloque){
        fclose(archivo);
        return -1;
    }
    fclose(archivo);
    return 0;
}