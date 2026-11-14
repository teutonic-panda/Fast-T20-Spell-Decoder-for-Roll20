#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_LINHAS 2000
#define MAX_TAM_LINHA 1024
#define MAX_BUFFER 16384
#define MAX_MAGIAS 200

void trim(char *str) {
    char *start = str;
    while (isspace((unsigned char)*start)) start++;
    if (start != str) memmove(str, start, strlen(start) + 1);

    size_t len = strlen(str);
    while (len > 0 && isspace((unsigned char)str[len - 1])) {
        str[--len] = '\0';
    }
}

int eh_classificacao(const char *linha) {
    if (strncmp(linha, "Arcana ", 7) == 0 ||
        strncmp(linha, "Divina ", 7) == 0 ||
        strncmp(linha, "Universal ", 10) == 0) {
        if (strchr(linha, '(') != NULL && strchr(linha, ')') != NULL) {
            return 1;
        }
    }
    return 0;
}

int eh_chave_cabecalho(const char *linha) {
    const char *chaves[] = {
        "Execução:", "Alcance:", "Alvo:", "Alvos:",
        "Área:", "Efeito:", "Duração:", "Resistência:"
    };
    for (int i = 0; i < 8; i++) {
        if (strncmp(linha, chaves[i], strlen(chaves[i])) == 0) {
            return 1;
        }
    }
    return 0;
}

int eh_aprimoramento(const char *linha) {
    if (strncmp(linha, "Truque:", 7) == 0) return 1;
    if (linha[0] == '+' && isdigit((unsigned char)linha[1])) {
        if (strstr(linha, "PM:") != NULL) return 1;
    }
    return 0;
}

void processar_magia(char linhas[][MAX_TAM_LINHA], int inicio, int fim) {
    char nome_magia[MAX_TAM_LINHA];
    strcpy(nome_magia, linhas[inicio]);

    char cabecalho[MAX_BUFFER] = "";
    char descricao[MAX_BUFFER] = "";
    char buffer_paragrafo[MAX_BUFFER] = "";

    snprintf(cabecalho, sizeof(cabecalho), "*%s*\n", linhas[inicio + 1]);

    int lendo_cabecalho = 1;

    for (int i = inicio + 2; i < fim; i++) {
        char *linha = linhas[i];

                if (lendo_cabecalho && eh_chave_cabecalho(linha)) {
            char *parte = strtok(linha, ";");
            
            while (parte != NULL) {
                trim(parte);
                
                if (strlen(parte) > 0) {
                    size_t len_p = strlen(parte);
                    if (parte[len_p - 1] == '.') {
                        parte[len_p - 1] = '\0';
                    }

                    if (strncmp(parte, "Resistência:", 12) == 0) {
                        char *conteudo = parte + 12;
                        while (isspace((unsigned char)*conteudo)) conteudo++;

                        char pericia[128] = "";
                        char efeito[512] = "";
                        char *espaco = strchr(conteudo, ' ');

                        if (espaco != NULL) {
                            size_t tam_pericia = espaco - conteudo;
                            if (tam_pericia >= sizeof(pericia)) tam_pericia = sizeof(pericia) - 1;
                            strncpy(pericia, conteudo, tam_pericia);
                            pericia[tam_pericia] = '\0';
                            strncpy(efeito, espaco + 1, sizeof(efeito) - 1);
                            efeito[sizeof(efeito) - 1] = '\0';
                        } else {
                            strncpy(pericia, conteudo, sizeof(pericia) - 1);
                            pericia[sizeof(pericia) - 1] = '\0';
                        }

                        if (pericia[0] != '\0') {
                            pericia[0] = (char)toupper((unsigned char)pericia[0]);
                        }

                        char linha_res[MAX_TAM_LINHA];
                        snprintf(linha_res, sizeof(linha_res), "**Resistência:** %s CD [[ @{cdtotal} ]] %s;\n", pericia, efeito);
                        strcat(cabecalho, linha_res);
                    } else {
                        char *dois_pontos = strchr(parte, ':');
                        if (dois_pontos != NULL) {
                            *dois_pontos = '\0';
                            char *chave = parte;
                            char *valor = dois_pontos + 1;
                            while (isspace((unsigned char)*valor)) valor++;

                            char linha_cab[MAX_TAM_LINHA];
                            snprintf(linha_cab, sizeof(linha_cab), "**%s:** %s;\n", chave, valor);
                            strcat(cabecalho, linha_cab);
                        }
                    }
                }
                parte = strtok(NULL, ";");
            }
        } else {
            lendo_cabecalho = 0;

            if (eh_aprimoramento(linha)) {
                if (strlen(buffer_paragrafo) > 0) {
                    strcat(descricao, buffer_paragrafo);
                    strcat(descricao, "\n\n");
                    buffer_paragrafo[0] = '\0';
                }
                char *dois_pontos = strchr(linha, ':');
                *dois_pontos = '\0';
                snprintf(buffer_paragrafo, sizeof(buffer_paragrafo), "**%s:**%s", linha, dois_pontos + 1);
            } else {
                size_t len_buf = strlen(buffer_paragrafo);
                if (len_buf > 0 &&
                    (buffer_paragrafo[len_buf - 1] == '.' || buffer_paragrafo[len_buf - 1] == '!') &&
                    isupper((unsigned char)linha[0]) && len_buf > 60) {
                    
                    strcat(descricao, buffer_paragrafo);
                    strcat(descricao, "\n\n");
                    strcpy(buffer_paragrafo, linha);
                } else {
                    if (len_buf > 0 && buffer_paragrafo[len_buf - 1] == '-') {
                        buffer_paragrafo[len_buf - 1] = '\0';
                        strcat(buffer_paragrafo, linha);
                    } else {
                        if (len_buf > 0) strcat(buffer_paragrafo, " ");
                        strcat(buffer_paragrafo, linha);
                    }
                }
            }
        }
    }

    if (strlen(buffer_paragrafo) > 0) {
        strcat(descricao, buffer_paragrafo);
    }

    size_t len_cab = strlen(cabecalho);
    if (len_cab > 0 && cabecalho[len_cab - 1] == '\n') {
        cabecalho[len_cab - 1] = '\0';
    }

    // Imprime o resultado separado visualmente para facilitar a cópia
    printf("\n==================== %s ====================\n\n", nome_magia);
    printf("}}\n");
    printf("&{template:t20-info}{{infoname=Cabeçalho}}{{description=%s}}\n\n", cabecalho);
    printf("&{template:t20-info}{{infoname=Descrição}}{{description=%s\n", descricao);
}

int main() {
    static char linhas[MAX_LINHAS][MAX_TAM_LINHA];
    int total_linhas = 0;

    // Lê direto da caixa de Input (Stdin) do compilador online
    while (total_linhas < MAX_LINHAS && fgets(linhas[total_linhas], MAX_TAM_LINHA, stdin)) {
        trim(linhas[total_linhas]);
        if (strlen(linhas[total_linhas]) > 0) {
            total_linhas++;
        }
    }

    int indices_classificacao[MAX_MAGIAS];
    int total_magias = 0;

    for (int i = 0; i < total_linhas && total_magias < MAX_MAGIAS; i++) {
        if (eh_classificacao(linhas[i])) {
            indices_classificacao[total_magias++] = i;
        }
    }

    if (total_magias == 0) {
        printf("Nenhuma magia encontrada. Cole o texto na caixa de Input (Stdin) antes de rodar.\n");
        return 1;
    }

    for (int m = 0; m < total_magias; m++) {
        int pos_class = indices_classificacao[m];
        int inicio = (pos_class > 0) ? pos_class - 1 : pos_class;
        int fim = (m + 1 < total_magias) ? indices_classificacao[m + 1] - 1 : total_linhas;

        processar_magia(linhas, inicio, fim);
    }

    printf("\n========================================================\n");
    return 0;
}
