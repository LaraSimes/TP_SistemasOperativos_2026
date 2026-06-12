#include "main.h"



static int traducir(t_contexto* ctx, uint32_t dir_logica, int size,
                    int seg_max, uint32_t* dir_fisica_out) {
    int num_seg = dir_logica / seg_max;
    int offset  = dir_logica % seg_max;

    for (int i = 0; i < list_size(ctx->segmentos); i++) {
        t_segmento* s = list_get(ctx->segmentos, i);
        if ((int)s->id_segmento == num_seg) {
            if (offset + size > (int)s->tamanio) return -1; // SEG_FAULT
            *dir_fisica_out = s->base + offset;
            return 0; // OK
        }
    }
    return -1; // segmento no encontrado → SEG_FAULT
}