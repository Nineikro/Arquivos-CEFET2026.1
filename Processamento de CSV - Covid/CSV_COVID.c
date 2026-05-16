#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define READ_BUF_SIZE  65536
#define MAX_COLS       300
#define MAX_LINHA      8192

typedef struct {
    char   linha[MAX_LINHA];   
    int    pos;                
} CSVParser;

static void csv_init(CSVParser* p) {
    memset(p, 0, sizeof(CSVParser));
}

static int csv_split(char* linha, char** cols, int max_cols) {
    int ncols = 0;
    char* p   = linha;

    while (ncols < max_cols) {
        cols[ncols++] = p;

        if (*p == '"') {
            p++;
            cols[ncols - 1] = p; 
            while (*p && !(*p == '"' && *(p + 1) != '"')) {
                if (*p == '"' && *(p + 1) == '"') p++;
                p++;
            }
            if (*p == '"') *p++ = '\0'; 
        } else {
            while (*p && *p != ',') p++;
        }

        if (*p == ',') {
            *p++ = '\0';  
        } else {
            break;        
        }
    }
    return ncols;
}

static void csv_processar_bloco(CSVParser* p, const char* bloco, int tam,
                                void (*callback)(char**, int, void*),
                                void* userData)
{
    char* cols[MAX_COLS];

    for (int i = 0; i < tam; i++) {
        char c = bloco[i];

        if (c == '\r') continue; 
        if (c == '\n') {
            p->linha[p->pos] = '\0';
            if (p->pos > 0) {
                int ncols = csv_split(p->linha, cols, MAX_COLS);
                callback(cols, ncols, userData);
            }
            p->pos = 0;
        } else {
            if (p->pos < MAX_LINHA - 1) {
                p->linha[p->pos++] = c;
            }
        }
    }
}

#define COL_PAIS         2
#define COL_DATA         3
#define COL_NOVOS_CASOS  5
#define COL_TOTAL_MORTES 7
#define MIN_COLS         8   

typedef struct {
    int       totalLinhasLidas;
    long      maxNovosCasosBrasil;
    char      dataRecordeBrasil[20];
    long long totalMortesBrasil;
} DadosCovid;

static void processarLinha(char** cols, int ncols, void* userData) {
    DadosCovid* dados = (DadosCovid*) userData;
    if (dados->totalLinhasLidas == 0) {
        dados->totalLinhasLidas++;
        return;
    }
    if (ncols < MIN_COLS) {
        dados->totalLinhasLidas++;
        return;
    }

    char* pais          = cols[COL_PAIS];
    char* data          = cols[COL_DATA];
    char* novosCasosStr = cols[COL_NOVOS_CASOS];
    char* totalMortesStr= cols[COL_TOTAL_MORTES];

    if (strcmp(pais, "Brazil") == 0) {
        if (novosCasosStr != NULL && strlen(novosCasosStr) > 0) {
            long novosCasos = atol(novosCasosStr);
            if (novosCasos > 0 && novosCasos > dados->maxNovosCasosBrasil) {
                dados->maxNovosCasosBrasil = novosCasos;
                strncpy(dados->dataRecordeBrasil, data,
                        sizeof(dados->dataRecordeBrasil) - 1);
                dados->dataRecordeBrasil[sizeof(dados->dataRecordeBrasil) - 1] = '\0';
            }
        }
        if (totalMortesStr != NULL && strlen(totalMortesStr) > 0) {
            long long mortes = atoll(totalMortesStr);
            if (mortes > dados->totalMortesBrasil) {
                dados->totalMortesBrasil = mortes;
            }
        }
    }

    dados->totalLinhasLidas++;
}

int main(void) {
    DadosCovid meusDados;
    memset(&meusDados, 0, sizeof(DadosCovid));
    strcpy(meusDados.dataRecordeBrasil, "N/A");

    char* buf = (char*) malloc(READ_BUF_SIZE);
    if (!buf) {
        fprintf(stderr, "Erro: falha ao alocar buffer de leitura.\n");
        return EXIT_FAILURE;
    }

    CSVParser csv;
    csv_init(&csv);

    FILE* f = fopen("owid-covid-data.csv", "rb");
    if (!f) {
        fprintf(stderr, "Erro: nao foi possivel abrir owid-covid-data.csv\n");
        fprintf(stderr, "Verifique se o arquivo esta na mesma pasta do executavel.\n");
        free(buf);
        return EXIT_FAILURE;
    }

    printf("Processando o arquivo CSV, por favor aguarde...\n");

    int qt;
    while ((qt = (int) fread(buf, 1, READ_BUF_SIZE, f)) > 0) {
        csv_processar_bloco(&csv, buf, qt, processarLinha, &meusDados);
    }
    fclose(f);
    csv_processar_bloco(&csv, "\n", 1, processarLinha, &meusDados);

    printf("\n================ TRABALHO DE COVID ================\n");
    printf("Total de linhas processadas no CSV : %d\n",  meusDados.totalLinhasLidas);
    printf("---------------------------------------------------\n");
    printf("Dados extraidos do Brasil:\n");
    printf("  > Maior numero de novos casos em 24h : %ld (Data: %s)\n",
           meusDados.maxNovosCasosBrasil, meusDados.dataRecordeBrasil);
    printf("  > Total acumulado de mortes           : %lld\n",
           meusDados.totalMortesBrasil);
    printf("===================================================\n");

    free(buf);
    return 0;
}