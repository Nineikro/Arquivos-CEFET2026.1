//Alunos: Carlos Felipe de A. Pereira e Nina K. E. Fonseca

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct _Endereco {
    char logradouro[72];
    char bairro[72];
    char cidade[72];
    char uf[72];
    char sigla[2];
    char cep[8];
    char lixo[2];
} Endereco;

typedef struct _Indice {
    char cep[8];
    long posicao;
} Indice;

int comparaIndice(const void *a, const void *b) {
    return strncmp(((Indice*)a)->cep, ((Indice*)b)->cep, 8);
}

int main(int argc, char** argv) {
    FILE *fOriginal, *fIndice;
    Endereco e;
    Indice *indices;
    long qtdRegistros, i;

    if (argc != 2) {
        fprintf(stderr, "USO: %s [CEP_PARA_BUSCA]\n", argv[0]);
        return 1;
    }

    fOriginal = fopen("cep.dat", "rb");
    if (fOriginal == NULL) {
        fprintf(stderr, "Erro ao abrir cep.dat. Certifique-se de que o arquivo esta no diretorio.\n");
        return 1;
    }

    fseek(fOriginal, 0, SEEK_END);
    qtdRegistros = ftell(fOriginal) / sizeof(Endereco);
    rewind(fOriginal);

    indices = (Indice*) malloc(qtdRegistros * sizeof(Indice));
    if (indices == NULL) {
        fprintf(stderr, "Erro de alocacao de memoria para o indice.\n");
        fclose(fOriginal);
        return 1;
    }

    printf("1. Lendo cep.dat e criando estrutura de indice (%ld registros)...\n", qtdRegistros);
    for (i = 0; i < qtdRegistros; i++) {
        fread(&e, sizeof(Endereco), 1, fOriginal);
        strncpy(indices[i].cep, e.cep, 8);
        indices[i].posicao = i;            
    }

    printf("2. Ordenando o indice em memoria...\n");
    qsort(indices, qtdRegistros, sizeof(Indice), comparaIndice);

    printf("3. Salvando indice ordenado em 'cep-indice.dat'...\n");
    fIndice = fopen("cep-indice.dat", "wb");
    if (fIndice != NULL) {
        fwrite(indices, sizeof(Indice), qtdRegistros, fIndice);
        fclose(fIndice);
    } else {
        fprintf(stderr, "Aviso: Nao foi possivel criar o arquivo cep-indice.dat\n");
    }

    printf("4. Realizando busca binaria pelo CEP: %s\n", argv[1]);
    long inicio = 0;
    long fim = qtdRegistros - 1;
    long meio;
    long posicaoEncontrada = -1;
    int comparacoes = 0;

    char alvo[8];
    strncpy(alvo, argv[1], 8);

    while (inicio <= fim) {
        comparacoes++;
        meio = inicio + (fim - inicio) / 2;
        
        int cmp = strncmp(alvo, indices[meio].cep, 8);

        if (cmp == 0) {
            posicaoEncontrada = indices[meio].posicao;
            break;
        } else if (cmp < 0) {
            fim = meio - 1;
        } else {
            inicio = meio + 1;
        }
    }


    if (posicaoEncontrada != -1) {
        fseek(fOriginal, posicaoEncontrada * sizeof(Endereco), SEEK_SET);
        fread(&e, sizeof(Endereco), 1, fOriginal);

        printf("\n=> CEP Encontrado! (Foram necessarias %d comparacoes no indice)\n", comparacoes);
        printf("------------------------------------------------------------\n");
        printf("Logradouro: %.72s\n", e.logradouro);
        printf("Bairro:     %.72s\n", e.bairro);
        printf("Cidade:     %.72s\n", e.cidade);
        printf("UF:         %.72s\n", e.uf);
        printf("Sigla:      %.2s\n", e.sigla);
        printf("CEP:        %.8s\n", e.cep);
        printf("------------------------------------------------------------\n");
    } else {
        printf("\n=> CEP %s nao encontrado.\n", argv[1]);
    }

    free(indices);
    fclose(fOriginal);

    return 0;
}
