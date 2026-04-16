#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Definição da estrutura do registro de endereço
typedef struct _Endereco Endereco;
struct _Endereco {
    char logradouro[72];
    char bairro[72];
    char cidade[72];
    char uf[72];
    char sigla[2];
    char cep[8];
    char lixo[2]; // Geralmente usado para quebra de linha (\r\n) ou alinhamento na base original
};

int main(int argc, char** argv) {
    FILE *f;
    Endereco e;
    long posicao, inicio, fim, meio;
    int comparacoes = 0;

    // Verifica se o usuário passou o CEP como argumento
    if(argc != 2) {
        fprintf(stderr, "Uso correto: %s [CEP_SEM_TRACO]\n", argv[0]);
        fprintf(stderr, "Exemplo: %s 22041001\n", argv[0]);
        return 1;
    }

    // Abre o arquivo em modo de leitura binária
    f = fopen("cep_ordenado.dat", "rb");
    if(!f) {
        fprintf(stderr, "Erro: Não foi possível abrir o arquivo 'cep_ordenado.dat'.\n");
        return 1;
    }

    // Move o ponteiro para o final do arquivo para descobrir o tamanho em bytes
    fseek(f, 0, SEEK_END);
    posicao = ftell(f);
    rewind(f); // Retorna o ponteiro para o início do arquivo

    // Define os limites inicial e final para a busca binária
    inicio = 0;
    fim = (posicao / sizeof(Endereco)) - 1;

    printf("Tamanho do Arquivo: %ld bytes\n", posicao);
    printf("Tamanho do Registro: %lu bytes\n", sizeof(Endereco));
    printf("Total de Registros: %ld\n\n", fim + 1);

    // Loop da Busca Binária
    while(inicio <= fim) {
        comparacoes++;
        meio = (inicio + fim) / 2;

        // Posiciona o cursor de leitura exatamente no registro do 'meio'
        fseek(f, meio * sizeof(Endereco), SEEK_SET);
        
        // Lê o registro na posição atual para a variável 'e'
        fread(&e, sizeof(Endereco), 1, f);

        // Compara o CEP buscado com o CEP lido
        // Usamos strncmp limitando a 8 caracteres, pois o CEP no arquivo pode não terminar com '\0'
        int cmp = strncmp(argv[1], e.cep, 8);

        if(cmp == 0) {
            printf("--- CEP ENCONTRADO! ---\n");
            // Imprime usando precisão para evitar ler lixo de memória caso falte o '\0'
            printf("Logradouro: %.72s\nBairro: %.72s\nCidade: %.72s\nUF: %.72s\nSigla: %.2s\nCEP: %.8s\n",
                   e.logradouro, e.bairro, e.cidade, e.uf, e.sigla, e.cep);
            printf("\nTotal de comparacoes no disco: %d\n", comparacoes);
            
            fclose(f);
            return 0;
        } 
        else if(cmp < 0) {
            // Se o CEP buscado é menor, ele deve estar na metade inferior (esquerda)
            fim = meio - 1;
        } 
        else {
            // Se o CEP buscado é maior, ele deve estar na metade superior (direita)
            inicio = meio + 1;
        }
    }

    printf("O CEP %s não foi encontrado no arquivo.\n", argv[1]);
    printf("Total de comparacoes no disco: %d\n", comparacoes);

    fclose(f);
    return 0;
}