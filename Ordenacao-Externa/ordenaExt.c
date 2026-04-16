//Alunos: Carlos Felipe de A. Pereira e Nina K. E. Fonseca

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h> //Adiciona aleatoriedade nos números sorteados para popular o arquivo

//Função de comparação para o qsort
int compara (const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

//Função para intercalar dois arquivos ordenados
void intercala (const char *file1, const char *file2, const char *out_file) {
    FILE *f1 = fopen(file1, "rb");
    FILE *f2 = fopen(file2, "rb");
    FILE *out = fopen(out_file, "wb");

    if (!f1 || !f2 || !out) {
        printf("Erro ao abrir arquivos para merge.\n");
        exit(1);
    }

    int val1, val2;
    size_t read1 = fread(&val1, sizeof(int), 1, f1);
    size_t read2 = fread(&val2, sizeof(int), 1, f2);

    //Intercala enquanto os dois arquivos tiverem elementos
    while (read1 == 1 && read2 == 1) {
        if (val1 <= val2) {
            fwrite(&val1, sizeof(int), 1, out);
            read1 = fread(&val1, sizeof(int), 1, f1);
        } else {
            fwrite(&val2, sizeof(int), 1, out);
            read2 = fread(&val2, sizeof(int), 1, f2);
        }
    }

    //Grava o restante do arquivo 1 e 2
    while (read1 == 1) {
        fwrite(&val1, sizeof(int), 1, out);
        read1 = fread(&val1, sizeof(int), 1, f1);
    }
    while (read2 == 1) {
        fwrite(&val2, sizeof(int), 1, out);
        read2 = fread(&val2, sizeof(int), 1, f2);
    }

    fclose(f1);
    fclose(f2);
    fclose(out);
}

//Dividir, ler, ordenar na memória e salvar
void ordenaExterna (const char *nomeArq, int k) {
    FILE *input = fopen(nomeArq, "rb");
    if (!input) {
        printf("Erro ao abrir arquivo de entrada.\n");
        return;
    }

    //Descobrir o tamanho total do arquivo
    fseek(input, 0, SEEK_END);
    long tamanho = ftell(input);
    rewind(input);
    
    long elementos = tamanho / sizeof(int);
    long tamanhoBloco = elementos / k;

    printf("- Total de elementos: %ld\n", elementos);
    printf("- Numero de blocos (K): %d\n", k);

    //Criar os blocos iniciais ordenados
    for (int i = 0; i < k; i++) {
        long tamanhoBlocoAtual = tamanhoBloco;
        if (i == k - 1) {
            tamanhoBlocoAtual += (elementos % k);
        }

        //Ler bloco para a memória
        int *buffer = (int*) malloc(tamanhoBlocoAtual * sizeof(int));
        fread(buffer, sizeof(int), tamanhoBlocoAtual, input);

        //Ordenar na memória
        qsort(buffer, tamanhoBlocoAtual, sizeof(int), compara );

        //Salvar bloco ordenado em arquivo temporário
        char nomeArqTemp[256];
        sprintf(nomeArqTemp, "passe0_block%d.bin", i);
        FILE *arqTemp = fopen(nomeArqTemp, "wb");
        fwrite(buffer, sizeof(int), tamanhoBlocoAtual, arqTemp);
        fclose(arqTemp);

        free(buffer);
    }
    fclose(input);

    //Intercalação dos blocos dois a dois
    int kAtual = k;
    int passe = 0;

    while (kAtual > 1) {
        int kProximo = 0;
        
        for (int i = 0; i < kAtual; i += 2) {
            char file1[256], file2[256], out_file[256];
            sprintf(file1, "passe%d_block%d.bin", passe, i);

            if (i + 1 < kAtual) {
                sprintf(file2, "passe%d_block%d.bin", passe, i + 1);
                sprintf(out_file, "passe%d_block%d.bin", passe + 1, kProximo);
                
                intercala (file1, file2, out_file);
                remove(file1);
                remove(file2);
            } else {
                //Número ímpar de arquivos: apenas renomear/passar o último pro próximo ciclo
                sprintf(out_file, "passe%d_block%d.bin", passe + 1, kProximo);
                rename(file1, out_file); 
            }
            kProximo++;
        }
        kAtual = kProximo;
        passe++;
    }

    char arqFinalTemp[256];
    sprintf(arqFinalTemp, "passe%d_block0.bin", passe);
    rename(arqFinalTemp, "arquivoOrdenado.bin");
    
    printf("Ordenacao concluida! Arquivo final: 'arquivoOrdenado.bin'\n");
}

//Funções auxiliares
void geradorArqAleatorio(const char *nomeArq, int qteElementos) {
    FILE *f = fopen(nomeArq, "wb");
    for (int i = 0; i < qteElementos; i++) {
        int r = rand() % 10000;
        fwrite(&r, sizeof(int), 1, f);
    }
    fclose(f);
}

void imprimePrimeirosElementos(const char *nomeArq, int contador) {
    FILE *f = fopen(nomeArq, "rb");
    if (!f) return;
    int val;
    printf("Primeiros %d elementos do arquivo: ", contador);
    for (int i = 0; i < contador && fread(&val, sizeof(int), 1, f) == 1; i++) {
        printf("%d ", val);
    }
    printf("\n");
    fclose(f);
}

int main() {
    const char *arquivoInput = "dadosDesordenados.bin";
    int elementos = 100; 
    int kBlocos = 4; 
    srand(time(NULL)); //Função para aleatorizar os números do arquivo

    //Gerador de números aleatórios para popular o arquivo
    geradorArqAleatorio(arquivoInput, elementos);
    printf("Arquivo de entrada gerado.\n");
    imprimePrimeirosElementos(arquivoInput, 10);

    //Execução do algoritmo
    ordenaExterna (arquivoInput, kBlocos);

    //Verificação
    imprimePrimeirosElementos("arquivoOrdenado.bin", 10);

    return 0;
}
