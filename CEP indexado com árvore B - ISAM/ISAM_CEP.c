#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SLOTS_POR_PAGINA  3    
#define MAX_PAGINAS       100    
#define CEP_LEN           9     
#define OFFSET_INVALIDO   -1L

typedef struct {
    char cep[CEP_LEN];
    long offset_dados;   
    int  ocupado;        
} SlotDado;

typedef struct {
    SlotDado slots[SLOTS_POR_PAGINA];
    long     prox_overflow;  
} PaginaDados;

typedef struct {
    char cep_minimo[CEP_LEN]; 
    long offset_pagina;       
} EntradaIndice;


typedef struct {
    FILE*         arquivo;
    EntradaIndice indice[MAX_PAGINAS];
    int           num_paginas;
} ContextoISAM;

static void salvar_pagina(ContextoISAM* ctx, long offset, PaginaDados* pag) {
    fseek(ctx->arquivo, offset, SEEK_SET);
    fwrite(pag, sizeof(PaginaDados), 1, ctx->arquivo);
    fflush(ctx->arquivo);
}

static void ler_pagina(ContextoISAM* ctx, long offset, PaginaDados* pag) {
    fseek(ctx->arquivo, offset, SEEK_SET);
    fread(pag, sizeof(PaginaDados), 1, ctx->arquivo);
}

static long get_novo_offset(ContextoISAM* ctx) {
    fseek(ctx->arquivo, 0, SEEK_END);
    return ftell(ctx->arquivo);
}

static int comparar_slot(const void* a, const void* b) {
    return strcmp(((SlotDado*)a)->cep, ((SlotDado*)b)->cep);
}

void isam_carga_inicial(ContextoISAM* ctx, SlotDado* registros, int total) {
    ctx->num_paginas = 0;

    qsort(registros, total, sizeof(SlotDado), comparar_slot);

    int i = 0;
    while (i < total && ctx->num_paginas < MAX_PAGINAS) {
        PaginaDados pag;
        memset(&pag, 0, sizeof(PaginaDados));
        pag.prox_overflow = OFFSET_INVALIDO;

        int slots_usados = 0;
        while (slots_usados < SLOTS_POR_PAGINA && i < total) {
            pag.slots[slots_usados]         = registros[i];
            pag.slots[slots_usados].ocupado = 1;
            slots_usados++;
            i++;
        }

        long offset_pag = get_novo_offset(ctx);
        salvar_pagina(ctx, offset_pag, &pag);

        strncpy(ctx->indice[ctx->num_paginas].cep_minimo,
                pag.slots[0].cep, CEP_LEN - 1);
        ctx->indice[ctx->num_paginas].cep_minimo[CEP_LEN - 1] = '\0';
        ctx->indice[ctx->num_paginas].offset_pagina = offset_pag;
        ctx->num_paginas++;
    }

    printf("Carga inicial concluida: %d pagina(s), %d registro(s) indexados.\n",
           ctx->num_paginas, total);
}

static int busca_binaria_indice(ContextoISAM* ctx, const char* cep_alvo) {
    int esq = 0, dir = ctx->num_paginas - 1, resultado = 0;
    while (esq <= dir) {
        int meio = (esq + dir) / 2;
        int cmp  = strcmp(cep_alvo, ctx->indice[meio].cep_minimo);
        if (cmp == 0) return meio;
        if (cmp  > 0) { resultado = meio; esq = meio + 1; }
        else            dir = meio - 1;
    }
    return resultado;
}

long isam_buscar(ContextoISAM* ctx, const char* cep_alvo) {
    if (ctx->num_paginas == 0) {
        printf("Indice vazio. Execute a carga inicial primeiro.\n");
        return OFFSET_INVALIDO;
    }

    int  idx_pag = busca_binaria_indice(ctx, cep_alvo);
    long offset  = ctx->indice[idx_pag].offset_pagina;

    while (offset != OFFSET_INVALIDO) {
        PaginaDados pag;
        ler_pagina(ctx, offset, &pag);

        for (int i = 0; i < SLOTS_POR_PAGINA; i++) {
            if (pag.slots[i].ocupado &&
                strcmp(pag.slots[i].cep, cep_alvo) == 0) {
                return pag.slots[i].offset_dados; 
            }
        }
        offset = pag.prox_overflow; 
    }

    return OFFSET_INVALIDO; 
}

void isam_inserir(ContextoISAM* ctx, SlotDado novo) {
    if (ctx->num_paginas == 0) {
        fprintf(stderr, "Erro: execute a carga inicial antes de inserir.\n");
        return;
    }

    int  idx_pag         = busca_binaria_indice(ctx, novo.cep);
    long offset          = ctx->indice[idx_pag].offset_pagina;
    long offset_anterior = OFFSET_INVALIDO;
    PaginaDados pag;

    while (offset != OFFSET_INVALIDO) {
        ler_pagina(ctx, offset, &pag);

        for (int i = 0; i < SLOTS_POR_PAGINA; i++) {
            if (!pag.slots[i].ocupado) {
                pag.slots[i]         = novo;
                pag.slots[i].ocupado = 1;
                salvar_pagina(ctx, offset, &pag);
                printf("Inserido CEP %s em slot livre na pagina offset=%ld.\n",
                       novo.cep, offset);
                return;
            }
        }
        offset_anterior = offset;
        offset          = pag.prox_overflow;
    }

    PaginaDados pag_overflow;
    memset(&pag_overflow, 0, sizeof(PaginaDados));
    pag_overflow.prox_overflow  = OFFSET_INVALIDO;
    pag_overflow.slots[0]       = novo;
    pag_overflow.slots[0].ocupado = 1;

    long offset_novo = get_novo_offset(ctx);
    salvar_pagina(ctx, offset_novo, &pag_overflow);

    ler_pagina(ctx, offset_anterior, &pag);
    pag.prox_overflow = offset_novo;
    salvar_pagina(ctx, offset_anterior, &pag);

    printf("Inserido CEP %s em nova pagina de OVERFLOW (offset=%ld).\n",
           novo.cep, offset_novo);
}

void isam_dump(ContextoISAM* ctx) {
    printf("\n======= DUMP ISAM =======\n");
    printf("Indice primario (%d entradas):\n", ctx->num_paginas);
    for (int p = 0; p < ctx->num_paginas; p++) {
        printf("  [%2d] CEP minimo: %-9s | offset pagina: %ld\n",
               p, ctx->indice[p].cep_minimo, ctx->indice[p].offset_pagina);
    }

    printf("\nPaginas de dados:\n");
    for (int p = 0; p < ctx->num_paginas; p++) {
        long offset = ctx->indice[p].offset_pagina;
        int  nivel  = 0;
        while (offset != OFFSET_INVALIDO) {
            PaginaDados pag;
            ler_pagina(ctx, offset, &pag);

            printf("  Pagina %d %s (offset=%ld): ",
                   p, nivel == 0 ? "[Principal]" : "[Overflow] ", offset);

            int algum = 0;
            for (int i = 0; i < SLOTS_POR_PAGINA; i++) {
                if (pag.slots[i].ocupado) {
                    printf("%s ", pag.slots[i].cep);
                    algum = 1;
                }
            }
            if (!algum) printf("(vazia)");
            printf("\n");

            offset = pag.prox_overflow;
            nivel++;
        }
    }
    printf("=========================\n\n");
}

int main() {
    ContextoISAM ctx;
    memset(&ctx, 0, sizeof(ContextoISAM));
    ctx.num_paginas = 0;

    ctx.arquivo = fopen("isam_indice.dat", "w+b");
    if (!ctx.arquivo) {
        fprintf(stderr, "Erro ao criar isam_indice.dat\n");
        return EXIT_FAILURE;
    }
    SlotDado registros[] = {
        {"01001000", 1500, 0},
        {"13050020", 5210, 0},
        {"22041001", 3450, 0},
        {"60060000", 11000, 0},
        {"90040000", 8900, 0},
    };
    int total = sizeof(registros) / sizeof(registros[0]);

    isam_carga_inicial(&ctx, registros, total);
    isam_dump(&ctx);
    printf("--- Buscas ---\n");
    const char* ceps_busca[] = {"22041001", "60060000", "99999999"};
    for (int i = 0; i < 3; i++) {
        long off = isam_buscar(&ctx, ceps_busca[i]);
        if (off != OFFSET_INVALIDO)
            printf("CEP %s encontrado | offset dados: %ld\n", ceps_busca[i], off);
        else
            printf("CEP %s nao encontrado.\n", ceps_busca[i]);
    }
    printf("\n--- Insercoes pos-carga ---\n");
    SlotDado novos[] = {
        {"05010000", 2000, 0},
        {"07030000", 2500, 0},
        {"08050000", 3000, 0}, 
    };
    for (int i = 0; i < 3; i++) {
        isam_inserir(&ctx, novos[i]);
    }

    isam_dump(&ctx);

    fclose(ctx.arquivo);
    return 0;
}