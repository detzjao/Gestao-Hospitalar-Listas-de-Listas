#include <stdio.h>
#include <conio2.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include "tadfilashospital.h"

/* ========= Funções auxiliares da interface ========= */

void pausa() {
    printf("\n\nPressione qualquer tecla para continuar...");
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
        printf("Nao foi possivel abrir o arquivo %s.\n", nomeArq);
        printf("A simulacao podera ser feita apenas com pacientes cadastrados manualmente.\n");
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

void mostrarFilas(FILAS_PRIORIDADE *filas) {
    int i;
    printf("FILAS DE TRIAGEM (por prioridade)\n");
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        DESCRITOR_FILA *d = &(filas->fila[i]);
        printf("  %-9s: %2d paciente(s) aguardando\n",
               prioridadeToString(i),
               d->qtde);
    }
    printf("\n");
}

void mostrarMedicos(LISTA_MEDICOS *lista) {
    MEDICO *m;

    if (lista == NULL || lista->inicio == NULL) {
        printf("Nao ha medicos/salas cadastrados.\n\n");
        return;
    }

    printf("MEDICOS / SALAS\n");
    m = lista->inicio;
    while (m != NULL) {
        printf("  Medico %d - ", m->id);
        if (m->ocupado && m->atual != NULL) {
            printf("Ocupado com %s (%s), restante: %d\n",
                   m->atual->nome,
                   prioridadeToString(m->atual->prioridade),
                   m->tempoRestante);
        } else {
            printf("Livre\n");
        }
        m = m->prox;
    }
    printf("\n");
}

void mostrarEstatisticasFinais(int totalAtendidos[NUM_PRIORIDADES],
                               long somaEspera[NUM_PRIORIDADES],
                               FILAS_PRIORIDADE *filas,
                               LISTA_MEDICOS *lista) {
    int i;
    int restantesFila = 0;
    int emTratamento = 0;
    MEDICO *m;

    printf("\n===== ESTATISTICAS FINAIS =====\n\n");

    printf("Pacientes tratados por classificacao de risco:\n");
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        printf("  %-9s: %d paciente(s)\n",
               prioridadeToString(i),
               totalAtendidos[i]);
    }
    printf("\n");

    printf("Tempo medio de espera por classificacao (unidades de tempo):\n");
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        double media = 0.0;
        if (totalAtendidos[i] > 0) {
            media = (double)somaEspera[i] / (double)totalAtendidos[i];
        }
        printf("  %-9s: %.2f\n",
               prioridadeToString(i),
               media);
    }
    printf("\n");

    for (i = 0; i < NUM_PRIORIDADES; i++) {
        restantesFila += filas->fila[i].qtde;
    }
    printf("Pacientes restantes na fila: %d\n", restantesFila);

    m = lista->inicio;
    while (m != NULL) {
        if (m->ocupado && m->atual != NULL) emTratamento++;
        m = m->prox;
    }
    printf("Pacientes ainda em tratamento: %d\n\n", emTratamento);

    if (lista != NULL && lista->inicio != NULL) {
        printf("Pacientes tratados por cada Medico/Sala:\n");
        m = lista->inicio;
        while (m != NULL) {
            int totalMedico = 0;
            for (i = 0; i < NUM_PRIORIDADES; i++) {
                totalMedico += m->atendidosPorPrioridade[i];
            }
            printf("  Medico %d: %d pacientes (V=%d, A=%d, Vd=%d)\n",
                   m->id,
                   totalMedico,
                   m->atendidosPorPrioridade[PRIO_VERMELHO],
                   m->atendidosPorPrioridade[PRIO_AMARELO],
                   m->atendidosPorPrioridade[PRIO_VERDE]);
            m = m->prox;
        }
        printf("\n");
    }
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
    printf("CADASTRO MANUAL DE PACIENTE\n\n");
    printf("Classificacao de risco:\n");
    printf("  1 - Vermelho (emergencia)\n");
    printf("  2 - Amarelo  (urgencia)\n");
    printf("  3 - Verde    (nao urgente)\n");
    printf("Opcao: ");
    scanf("%d", &opc);
    fflush(stdin);

    if (opc == 1) prioridade = PRIO_VERMELHO;
    else if (opc == 2) prioridade = PRIO_AMARELO;
    else prioridade = PRIO_VERDE;

    printf("Tempo de tratamento estimado (unidades de tempo): ");
    scanf("%d", &tempo);
    fflush(stdin);

    printf("Nome do paciente: ");
    lerLinha(nome, TAM_NOME);
    printf("Queixa principal: ");
    lerLinha(queixa, TAM_QUEIXA);
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

    printf("\nPaciente inserido na fila %s!\n", prioridadeToString(prioridade));
    pausa();
}

/* ========= Configuração dos médicos ========= */

void configurarMedicos(LISTA_MEDICOS *lista) {
    int qtd, i;
    if (lista == NULL) return;

    inicializarListaMedicos(lista);

    clrscr();
    printf("CONFIGURACAO INICIAL\n\n");
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

    srand((unsigned int)time(NULL));

    inicializarFilas(&filas);
    inicializarListaMedicos(&listaMedicos);

    configurarMedicos(&listaMedicos);

    printf("\nDuracao maxima da simulacao (em unidades de tempo): ");
    scanf("%d", &duracaoMax);
    fflush(stdin);
    if (duracaoMax <= 0) duracaoMax = 50;

    printf("\nDeseja carregar pacientes a partir de arquivo texto?\n");
    printf("  1 - Sim (arquivo pacientes.txt)\n");
    printf("  2 - Nao (vou cadastrar manualmente)\n");
    printf("Opcao: ");
    scanf("%d", &opc);
    fflush(stdin);

    if (opc == 1) {
        carregarPacientesArquivo("pacientes.txt", &filas, &proxIdPaciente);
    }

    while (1) {
        /* Avanca uma unidade de tempo de atendimento */
        avancarTempo(&listaMedicos, totalAtendidos, somaEspera);

        /* Atribui novos pacientes aos medicos livres */
        alocarPacientes(&listaMedicos, &filas, tempoAtual);

        /* Interface da simulacao */
        clrscr();
        textcolor(LIGHTCYAN);
        printf("SIMULACAO DE PRONTO-SOCORRO - GESTAO HOSPITALAR\n");
        printf("Tempo atual: %d   |   Duracao maxima: %d\n\n", tempoAtual, duracaoMax);

        textcolor(WHITE);
        mostrarFilas(&filas);
        mostrarMedicos(&listaMedicos);

        printf("Opcoes:\n");
        printf("  [ENTER] - Avancar 1 unidade de tempo\n");
        printf("  [C]     - Cadastrar novo paciente manualmente\n");
        printf("  [F]     - Finalizar simulacao agora\n\n");
        printf("Escolha: ");

        int ch = getch();
        if (ch == 13) { /* ENTER */
            tempoAtual++;
        } else if (ch == 'c' || ch == 'C') {
            cadastrarPacienteManual(&filas, &proxIdPaciente, tempoAtual);
        } else if (ch == 'f' || ch == 'F') {
            break;
        }

        if (tempoAtual >= duracaoMax) {
            break;
        }

        /* Parada natural: sem pacientes esperando e nenhum medico ocupado */
        if (!haPacientes(&filas) && !medicosOcupados(&listaMedicos)) {
            break;
        }
    }

    clrscr();
    mostrarEstatisticasFinais(totalAtendidos, somaEspera, &filas, &listaMedicos);

    /* Libera memoria dinamica (apenas delete) */
    limparFilas(&filas);
    limparListaMedicos(&listaMedicos);

    pausa();
}

int main() {
    executarSimulacao();
    return 0;
}
