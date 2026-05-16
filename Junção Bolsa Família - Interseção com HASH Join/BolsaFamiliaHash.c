#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define HASH_SIZE 900001
#define SEM_PROXIMO 0xFFFFFFFFFFFFFFFFUL

typedef struct {
    char nis[15];
    char nome[72];
    char valor[10];
    char mes[8];
} RegistroMapeado;

typedef struct {
    char chave[8];
    unsigned long posicao;
    unsigned long prox;     
    char ocupado;       
} RegistroIndice;

unsigned long calcular_hash(const char* chave) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*chave++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % HASH_SIZE;
}

void inicializar_arquivo_indice(const char* nome_indice) {
    FILE* fi = fopen(nome_indice, "wb");
    if (!fi) {
        fprintf(stderr, "Erro ao criar o arquivo de indice '%s'.\n", nome_indice);
        return;
    }

    RegistroIndice indice_vazio;
    memset(&indice_vazio, 0, sizeof(RegistroIndice));
    indice_vazio.prox    = SEM_PROXIMO;
    indice_vazio.ocupado = 0;

    for (int i = 0; i < HASH_SIZE; i++) {
        fwrite(&indice_vazio, sizeof(RegistroIndice), 1, fi);
    }
    fclose(fi);
    printf("Arquivo de indice inicializado: %s\n", nome_indice);
}

void inserir_no_hash(FILE* fi, RegistroMapeado reg, unsigned long pos_dados) {
    unsigned long p      = calcular_hash(reg.nis);
    unsigned long offset = p * sizeof(RegistroIndice);

    RegistroIndice ind_atual;
    fseek(fi, offset, SEEK_SET);
    fread(&ind_atual, sizeof(RegistroIndice), 1, fi);

    if (!ind_atual.ocupado) {
        strncpy(ind_atual.chave, reg.nis, 7);
        ind_atual.chave[7]  = '\0';
        ind_atual.posicao   = pos_dados;
        ind_atual.prox      = SEM_PROXIMO;
        ind_atual.ocupado   = 1;

        fseek(fi, offset, SEEK_SET);
        fwrite(&ind_atual, sizeof(RegistroIndice), 1, fi);
    } else {
        unsigned long offset_atual = offset;
        while (ind_atual.prox != SEM_PROXIMO) {
            offset_atual = ind_atual.prox;
            fseek(fi, offset_atual, SEEK_SET);
            fread(&ind_atual, sizeof(RegistroIndice), 1, fi);
        }

        fseek(fi, 0, SEEK_END);
        unsigned long novo_offset = ftell(fi);

        RegistroIndice novo_nodo;
        memset(&novo_nodo, 0, sizeof(RegistroIndice));
        strncpy(novo_nodo.chave, reg.nis, 7);
        novo_nodo.chave[7]  = '\0';
        novo_nodo.posicao   = pos_dados;
        novo_nodo.prox      = SEM_PROXIMO;
        novo_nodo.ocupado   = 1;
        fwrite(&novo_nodo, sizeof(RegistroIndice), 1, fi);

        ind_atual.prox = novo_offset;
        fseek(fi, offset_atual, SEEK_SET);
        fwrite(&ind_atual, sizeof(RegistroIndice), 1, fi);
    }
}

void executar_intersecao_join(const char* nome_dados_A,
                               const char* nome_indice_A,
                               const char* nome_dados_B) {
    FILE* fA  = fopen(nome_dados_A,  "rb");
    FILE* fiA = fopen(nome_indice_A, "rb");
    FILE* fB  = fopen(nome_dados_B,  "rb");

    if (!fA || !fiA || !fB) {
        fprintf(stderr, "Erro ao abrir arquivos para o HASH JOIN.\n");
        if (fA)  fclose(fA);
        if (fiA) fclose(fiA);
        if (fB)  fclose(fB);
        return;
    }

    printf("\n--- RESULTADO DA INTERSECAO (HASH JOIN) ---\n");

    RegistroMapeado regB;
    while (fread(&regB, sizeof(RegistroMapeado), 1, fB) > 0) {
        unsigned long p      = calcular_hash(regB.nis);
        unsigned long offset = p * sizeof(RegistroIndice);

        RegistroIndice indA;
        fseek(fiA, offset, SEEK_SET);
        fread(&indA, sizeof(RegistroIndice), 1, fiA);

        while (indA.ocupado) {
            if (strncmp(indA.chave, regB.nis, 7) == 0) {
                fseek(fA, indA.posicao * sizeof(RegistroMapeado), SEEK_SET);
                RegistroMapeado regA;
                fread(&regA, sizeof(RegistroMapeado), 1, fA);

                if (strcmp(regA.nis, regB.nis) == 0) {
                    printf("Intersecao! NIS: %-15s | Nome: %-40s | Valor Anterior: %-8s | Valor Atual: %s\n",
                           regA.nis, regA.nome, regA.valor, regB.valor);
                }
            }

            if (indA.prox == SEM_PROXIMO) break;
            fseek(fiA, indA.prox, SEEK_SET);
            fread(&indA, sizeof(RegistroIndice), 1, fiA);
        }
    }

    fclose(fA);
    fclose(fiA);
    fclose(fB);
}

int main() {
    const char* arq_dados_A  = "bolsa_familia_mes1.dat";
    const char* arq_indice_A = "bolsa_familia_hashA.dat";
    const char* arq_dados_B  = "bolsa_familia_mes2.dat";

    printf("Inicializando arquivo de indices hash em disco...\n");
    inicializar_arquivo_indice(arq_indice_A);

    FILE* fA = fopen(arq_dados_A, "wb");
    if (!fA) {
        fprintf(stderr, "Erro ao criar '%s'.\n", arq_dados_A);
        return EXIT_FAILURE;
    }
    RegistroMapeado r1 = {"12345678901", "Fulano da Silva",     "600.00", "202602"};
    RegistroMapeado r2 = {"98765432100", "Ciclano de Souza",    "700.00", "202602"};
    fwrite(&r1, sizeof(RegistroMapeado), 1, fA);
    fwrite(&r2, sizeof(RegistroMapeado), 1, fA);
    fclose(fA);

    FILE* fi = fopen(arq_indice_A, "r+b");
    if (!fi) {
        fprintf(stderr, "Erro ao abrir indice para escrita.\n");
        return EXIT_FAILURE;
    }
    inserir_no_hash(fi, r1, 0);
    inserir_no_hash(fi, r2, 1);
    fclose(fi);
    printf("Tabela A indexada via Hash com sucesso.\n");

    FILE* fB = fopen(arq_dados_B, "wb");
    if (!fB) {
        fprintf(stderr, "Erro ao criar '%s'.\n", arq_dados_B);
        return EXIT_FAILURE;
    }
    RegistroMapeado rB1 = {"12345678901", "Fulano da Silva",      "650.00", "202603"}; // intersecao
    RegistroMapeado rB2 = {"11122233344", "Beltrano de Oliveira", "600.00", "202603"}; // nao esta em A
    fwrite(&rB1, sizeof(RegistroMapeado), 1, fB);
    fwrite(&rB2, sizeof(RegistroMapeado), 1, fB);
    fclose(fB);

    executar_intersecao_join(arq_dados_A, arq_indice_A, arq_dados_B);

    return 0;
}