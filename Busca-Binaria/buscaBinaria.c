//Alunos: Carlos Felipe de A. Pereira e Nina K. E. Fonseca

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

typedef struct _Endereco Endereco;
struct _Endereco {
    char logradouro[72];
    char bairro[72];
    char cidade[72];
    char uf[72];
    char sigla[2];
    char cep[8];
    char lixo[2];
};

int main(int argc, char** argv) {
    FILE *f;
    Endereco e;
    long posicao, inicio, fim, meio;
    int comparacoes = 0;

    if(argc != 2) {
        fprintf(stderr, "Uso correto: %s [CEP_SEM_TRACO]\n", argv[0]);
        fprintf(stderr, "Exemplo: %s 22041001\n", argv[0]);
        return 1;
    }

    f = fopen("cep_ordenado.dat", "rb");
    if(!f) {
        fprintf(stderr, "Erro: Não foi possível abrir o arquivo 'cep_ordenado.dat'.\n");
        return 1;
    }

    fseek(f, 0, SEEK_END);
    posicao = ftell(f);
    rewind(f);

    inicio = 0;
    fim = (posicao / sizeof(Endereco)) - 1;

    printf("Tamanho do Arquivo: %ld bytes\n", posicao);
    printf("Tamanho do Registro: %lu bytes\n", sizeof(Endereco));
    printf("Total de Registros: %ld\n\n", fim + 1);

    while(inicio <= fim) {
        comparacoes++;
        meio = (inicio + fim) / 2;

        fseek(f, meio * sizeof(Endereco), SEEK_SET);
        fread(&e, sizeof(Endereco), 1, f);

        int cmp = strncmp(argv[1], e.cep, 8);

        if(cmp == 0) {
            printf("--- CEP ENCONTRADO! ---\n");
            printf("Logradouro: %.72s\nBairro: %.72s\nCidade: %.72s\nUF: %.72s\nSigla: %.2s\nCEP: %.8s\n",
                   e.logradouro, e.bairro, e.cidade, e.uf, e.sigla, e.cep);
            printf("\nTotal de comparacoes no disco: %d\n", comparacoes);
            
            fclose(f);
            return 0;
        } 
        else if(cmp < 0) {
            fim = meio - 1;
        } 
        else {
            inicio = meio + 1;
        }
    }

    printf("O CEP %s não foi encontrado no arquivo.\n", argv[1]);
    printf("Total de comparacoes no disco: %d\n", comparacoes);

    fclose(f);
    return 0;
}
