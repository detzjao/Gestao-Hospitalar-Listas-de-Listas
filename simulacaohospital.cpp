#include <stdio.h>
#include <conio2.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "tadfilashospital.h"

#define LARGURA_TELA 80
#define ALTURA_TELA  25

void pausa() {
    gotoxy(3, 24);
    textcolor(LIGHTGRAY);
    printf("Pressione qualquer tecla para continuar...");
    getch();
}

/* Ler linha com fgets e tirar o '\n' do final */
void lerLinha(char *dest, int tam) {
    if (fgets(dest, tam, stdin) != NULL) {
        int len = (int)strlen(dest);
        if (len > 0 && dest[len - 1] == '\n') {
            dest[len - 1] = '\0';
        }
    } else {
        if (tam > 0) dest[0] = '\0';
    }
}

/* ========= Moldura e centralização ========= */

void desenhaMoldura(int esquerda, int topo, int direita, int base) {
    int x, y;

    textcolor(LIGHTCYAN);

    /* Cantos */
    gotoxy(esquerda, topo);   printf("%c", 201); /* ? */
    gotoxy(direita,  topo);   printf("%c", 187); /* ? */
    gotoxy(esquerda, base);   printf("%c", 200); /* ? */
    gotoxy(direita,  base);   printf("%c", 188); /* ? */

    /* Linhas horizontais */
    for (x = esquerda + 1; x < direita; x++) {
        gotoxy(x, topo); printf("%c", 205);  /* ? */
        gotoxy(x, base); printf("%c", 205);  /* ? */
    }

    /* Linhas verticais */
    for (y = topo + 1; y < base; y++) {
        gotoxy(esquerda, y); printf("%c", 186); /* ? */
        gotoxy(direita,  y); printf("%c", 186); /* ? */
    }

    textcolor(WHITE);
}

void centralizarTexto(int linha, const char *texto) {
    int col = (LARGURA_TELA - (int)strlen(texto)) / 2;
    if (col < 1) col = 1;
    gotoxy(col, linha);
    printf("%s", texto);
}

/* Converte texto do arquivo para código de prioridade */
int lerPrioridadeTexto(const char *texto) {
    char copia[20];
    int i;

    if (texto == NULL) return PRIO_VERDE;

    strncpy(copia, texto, 19);
    copia[19] = '\0';

    for (i = 0; copia[i] != '\0'; i++) {
        copia[i] = (char)tolower(copia[i]);
    }

    if (strstr(copia, "vermelho") != NULL) return PRIO_VERMELHO;
    if (strstr(copia, "amarelo")  != NULL) return PRIO_AMARELO;
    return PRIO_VERDE;
}

/* ========= Entrada de dados (arquivo) ========= */
/* Formato de cada linha:
   Classificacao,TempoTratamento,Nome,Queixa,Data */
void carregarPacientesArquivo(const char *nomeArq,
                              FILAS_PRIORIDADE *filas,
                              int *proxId) {
    FILE *f;
    char linha[256];
    int tempoChegada = 0;

    if (filas == NULL || proxId == NULL) return;

    f = fopen(nomeArq, "r");
    if (f == NULL) {
        clrscr();
        desenhaMoldura(2, 1, 79, 24);
        centralizarTexto(3, "ARQUIVO NAO ENCONTRADO");
        gotoxy(4, 6);
        printf("Nao foi possivel abrir o arquivo %s.", nomeArq);
        gotoxy(4, 8);
        printf("A simulacao podera ser feita apenas com pacientes cadastrados manualmente.");
        pausa();
        return;
    }

    while (fgets(linha, sizeof(linha), f) != NULL) {
        char *token;
        char classificacao[30];
        char tempoStr[10];
        char nome[TAM_NOME];
        char queixa[TAM_QUEIXA];
        char data[TAM_DATA];
        int prioridade;
        int tempoAtendimento;

        if (linha[0] == '\n' || linha[0] == '\r' || linha[0] == '\0')
            continue;

        if (linha[strlen(linha) - 1] == '\n')
            linha[strlen(linha) - 1] = '\0';

        token = strtok(linha, ",");
        if (token == NULL) continue;
        strncpy(classificacao, token, sizeof(classificacao) - 1);
        classificacao[sizeof(classificacao) - 1] = '\0';

        token = strtok(NULL, ",");
        if (token == NULL) continue;
        strncpy(tempoStr, token, sizeof(tempoStr) - 1);
        tempoStr[sizeof(tempoStr) - 1] = '\0';

        token = strtok(NULL, ",");
        if (token == NULL) continue;
        strncpy(nome, token, sizeof(nome) - 1);
        nome[sizeof(nome) - 1] = '\0';

        token = strtok(NULL, ",");
        if (token == NULL) continue;
        strncpy(queixa, token, sizeof(queixa) - 1);
        queixa[sizeof(queixa) - 1] = '\0';

        token = strtok(NULL, ",");
        if (token == NULL) strcpy(data, "s/data");
        else {
            strncpy(data, token, sizeof(data) - 1);
            data[sizeof(data) - 1] = '\0';
        }

        prioridade = lerPrioridadeTexto(classificacao);
        tempoAtendimento = atoi(tempoStr);

        /* intervalo aleatorio entre chegadas, de 1 a 3 unidades de tempo */
        tempoChegada += (rand() % 3) + 1;

        PACIENTE *p = criarPaciente(*proxId,
                                    prioridade,
                                    tempoAtendimento,
                                    nome,
                                    queixa,
                                    data,
                                    tempoChegada);

        enfileirarPaciente(filas, p);
        (*proxId)++;
    }

    fclose(f);
}

/* ========= Visualização do estado ========= */

void mostrarFilas(FILAS_PRIORIDADE *filas, int x, int y) {
    int i;
    gotoxy(x, y++);
    printf("FILAS DE TRIAGEM (por prioridade)");
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        DESCRITOR_FILA *d = &(filas->fila[i]);
        gotoxy(x, y++);
        printf("  %-9s: %2d paciente(s) aguardando",
               prioridadeToString(i),
               d->qtde);
    }
}

void mostrarMedicos(LISTA_MEDICOS *lista, int x, int y) {
    MEDICO *m;

    if (lista == NULL || lista->inicio == NULL) {
        gotoxy(x, y);
        printf("Nao ha medicos/salas cadastrados.");
        return;
    }

    gotoxy(x, y++);
    printf("MEDICOS / SALAS");
    m = lista->inicio;
    while (m != NULL) {
        gotoxy(x, y++);
        printf("  Medico %d - ", m->id);
        if (m->ocupado && m->atual != NULL) {
            printf("Ocupado com %s (%s), restante: %d",
                   m->atual->nome,
                   prioridadeToString(m->atual->prioridade),
                   m->tempoRestante);
        } else {
            printf("Livre");
        }
        m = m->prox;
    }
}

void mostrarEstatisticasFinais(int totalAtendidos[NUM_PRIORIDADES],
                               long somaEspera[NUM_PRIORIDADES],
                               FILAS_PRIORIDADE *filas,
                               LISTA_MEDICOS *lista) {
    int i;
    int restantesFila = 0;
    int emTratamento = 0;
    MEDICO *m;

    clrscr();
    desenhaMoldura(2, 1, 79, 24);
    centralizarTexto(3, "RELATORIO FINAL DA SIMULACAO");

    gotoxy(4, 5);
    printf("Pacientes tratados por classificacao de risco:");
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        gotoxy(4, 6 + i);
        printf("  %-9s: %d paciente(s)",
               prioridadeToString(i),
               totalAtendidos[i]);
    }

    gotoxy(4, 10);
    printf("Tempo medio de espera por classificacao (unidades de tempo):");
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        double media = 0.0;
        if (totalAtendidos[i] > 0) {
            media = (double)somaEspera[i] / (double)totalAtendidos[i];
        }
        gotoxy(4, 11 + i);
        printf("  %-9s: %.2f",
               prioridadeToString(i),
               media);
    }

    for (i = 0; i < NUM_PRIORIDADES; i++) {
        restantesFila += filas->fila[i].qtde;
    }
    gotoxy(4, 15);
    printf("Pacientes restantes na fila: %d", restantesFila);

    m = lista->inicio;
    while (m != NULL) {
        if (m->ocupado && m->atual != NULL) emTratamento++;
        m = m->prox;
    }
    gotoxy(4, 16);
    printf("Pacientes ainda em tratamento: %d", emTratamento);

    if (lista != NULL && lista->inicio != NULL) {
        int linha = 18;
        gotoxy(4, linha++);
        printf("Pacientes tratados por cada Medico/Sala:");
        m = lista->inicio;
        while (m != NULL) {
            int totalMedico = 0;
            for (i = 0; i < NUM_PRIORIDADES; i++) {
                totalMedico += m->atendidosPorPrioridade[i];
            }
            gotoxy(4, linha++);
            printf("  Medico %d: %d pacientes (V=%d, A=%d, Vd=%d)",
                   m->id,
                   totalMedico,
                   m->atendidosPorPrioridade[PRIO_VERMELHO],
                   m->atendidosPorPrioridade[PRIO_AMARELO],
                   m->atendidosPorPrioridade[PRIO_VERDE]);
            m = m->prox;
        }
    }

    pausa();
}

/* ========= Cadastro manual de paciente ========= */

void cadastrarPacienteManual(FILAS_PRIORIDADE *filas,
                             int *proxId,
                             int tempoAtual) {
    int opc;
    int prioridade;
    int tempo;
    char nome[TAM_NOME];
    char queixa[TAM_QUEIXA];
    char data[TAM_DATA];

    if (filas == NULL || proxId == NULL) return;

    clrscr();
    desenhaMoldura(2, 1, 79, 24);
    centralizarTexto(3, "CADASTRO MANUAL DE PACIENTE");

    gotoxy(4, 6);
    printf("Classificacao de risco:");
    gotoxy(4, 7);
    printf("  1 - Vermelho (emergencia)");
    gotoxy(4, 8);
    printf("  2 - Amarelo  (urgencia)");
    gotoxy(4, 9);
    printf("  3 - Verde    (nao urgente)");
    gotoxy(4, 11);
    printf("Opcao: ");
    scanf("%d", &opc);
    fflush(stdin);

    if (opc == 1) prioridade = PRIO_VERMELHO;
    else if (opc == 2) prioridade = PRIO_AMARELO;
    else prioridade = PRIO_VERDE;

    gotoxy(4, 13);
    printf("Tempo de tratamento estimado (unidades de tempo): ");
    scanf("%d", &tempo);
    fflush(stdin);

    gotoxy(4, 15);
    printf("Nome do paciente: ");
    lerLinha(nome, TAM_NOME);
    gotoxy(4, 16);
    printf("Queixa principal: ");
    lerLinha(queixa, TAM_QUEIXA);
    gotoxy(4, 17);
    printf("Data de chegada (texto livre): ");
    lerLinha(data, TAM_DATA);

    PACIENTE *p = criarPaciente(*proxId,
                                prioridade,
                                tempo,
                                nome,
                                queixa,
                                data,
                                tempoAtual);

    enfileirarPaciente(filas, p);
    (*proxId)++;

    gotoxy(4, 19);
    printf("Paciente inserido na fila %s!", prioridadeToString(prioridade));
    pausa();
}

/* ========= Configuração dos médicos ========= */

void configurarMedicos(LISTA_MEDICOS *lista) {
    int qtd, i;
    if (lista == NULL) return;

    inicializarListaMedicos(lista);

    clrscr();
    desenhaMoldura(2, 1, 79, 24);
    centralizarTexto(3, "CONFIGURACAO INICIAL DE MEDICOS");

    gotoxy(4, 6);
    printf("Quantidade de Medicos/Salas disponiveis: ");
    scanf("%d", &qtd);
    fflush(stdin);

    if (qtd <= 0) qtd = 1;

    for (i = 1; i <= qtd; i++) {
        adicionarMedico(lista, i);
    }
}

/* ========= Loop principal da simulação ========= */

void executarSimulacao() {
    FILAS_PRIORIDADE filas;
    LISTA_MEDICOS listaMedicos;

    int totalAtendidos[NUM_PRIORIDADES] = {0, 0, 0};
    long somaEspera[NUM_PRIORIDADES] = {0, 0, 0};

    int proxIdPaciente = 1;
    int duracaoMax;
    int opc;
    int tempoAtual = 0;
    int modoAvanco; /* 1 = manual, 2 = automatico */

    srand((unsigned int)time(NULL));

    inicializarFilas(&filas);
    inicializarListaMedicos(&listaMedicos);

    configurarMedicos(&listaMedicos);

    clrscr();
    desenhaMoldura(2, 1, 79, 24);
    centralizarTexto(3, "CONFIGURACAO DA SIMULACAO");

    gotoxy(4, 6);
    printf("Duracao maxima da simulacao (em unidades de tempo): ");
    scanf("%d", &duracaoMax);
    fflush(stdin);
    if (duracaoMax <= 0) duracaoMax = 50;

    gotoxy(4, 8);
    printf("Modo de avancar o tempo:");
    gotoxy(4, 9);
    printf("  1 - Manual (ENTER para avancar)");
    gotoxy(4, 10);
    printf("  2 - Automatico (1 segundo por passo)");
    gotoxy(4, 12);
    printf("Opcao: ");
    scanf("%d", &modoAvanco);
    fflush(stdin);
    if (modoAvanco != 2) modoAvanco = 1;

    gotoxy(4, 14);
    printf("Deseja carregar pacientes a partir de arquivo texto?");
    gotoxy(4, 15);
    printf("  1 - Sim (arquivo pacientes.txt)");
    gotoxy(4, 16);
    printf("  2 - Nao (vou cadastrar manualmente)");
    gotoxy(4, 18);
    printf("Opcao: ");
    scanf("%d", &opc);
    fflush(stdin);

    if (opc == 1) {
        carregarPacientesArquivo("pacientes.txt", &filas, &proxIdPaciente);
    }

    /* Loop principal */
    while (1) {
        /* Avanca uma unidade de tempo de atendimento */
        avancarTempo(&listaMedicos, totalAtendidos, somaEspera);

        /* Atribui novos pacientes aos medicos livres */
        alocarPacientes(&listaMedicos, &filas, tempoAtual);

        /* Interface da simulacao */
        clrscr();
        desenhaMoldura(2, 1, 79, 24);
        centralizarTexto(2, "SIMULACAO DE PRONTO-SOCORRO - GESTAO HOSPITALAR");

        {
            char linhaTempo[80];
            sprintf(linhaTempo,
                    "Tempo atual: %d   |   Duracao maxima: %d",
                    tempoAtual, duracaoMax);
            centralizarTexto(4, linhaTempo);
        }

        /* Mostrar filas e medicos dentro da moldura */
        mostrarFilas(&filas, 4, 6);
        mostrarMedicos(&listaMedicos, 4, 11);

        if (modoAvanco == 1) {
            /* Modo manual */
            centralizarTexto(20,
                "[ENTER] - Avancar  |  [C] - Cadastrar paciente  |  [F] - Finalizar");
            gotoxy(4, 22);
            printf("Escolha: ");

            int ch = getch();
            if (ch == 13) { /* ENTER */
                tempoAtual++;
            } else if (ch == 'c' || ch == 'C') {
                cadastrarPacienteManual(&filas, &proxIdPaciente, tempoAtual);
            } else if (ch == 'f' || ch == 'F') {
                break;
            }
        } else {
            /* Modo automatico */
            centralizarTexto(20,
                "Modo automatico: simulacao em andamento...");
            Sleep(1000); /* 1 segundo por passo */
            tempoAtual++;
        }

        if (tempoAtual >= duracaoMax) {
            break;
        }

        /* Parada natural: sem pacientes esperando e nenhum medico ocupado */
        if (!haPacientes(&filas) && !medicosOcupados(&listaMedicos)) {
            break;
        }
    }

    mostrarEstatisticasFinais(totalAtendidos, somaEspera, &filas, &listaMedicos);

    /* Libera memoria dinamica (apenas delete) */
    limparFilas(&filas);
    limparListaMedicos(&listaMedicos);
}

int main() {
    executarSimulacao();
    return 0;
}

