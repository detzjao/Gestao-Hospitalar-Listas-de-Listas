#ifndef TAD_FILAS_HOSPITAL_CPP
#define TAD_FILAS_HOSPITAL_CPP

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Constantes de prioridade */
#define PRIO_VERMELHO 0
#define PRIO_AMARELO  1
#define PRIO_VERDE    2
#define NUM_PRIORIDADES 3

/* Tamanhos máximos de textos */
#define TAM_NOME    50
#define TAM_QUEIXA  80
#define TAM_DATA    20

typedef struct paciente {
    int id;
    int prioridade;            /* 0 = vermelho, 1 = amarelo, 2 = verde */
    int tempoAtendimento;      /* tempo total de tratamento */
    char nome[TAM_NOME];
    char queixa[TAM_QUEIXA];
    char dataChegada[TAM_DATA];
    int tempoChegada;          /* instante em que entrou na fila */
    int tempoInicio;           /* instante em que entrou em atendimento */
    struct paciente *prox;
} PACIENTE;

typedef struct {
    PACIENTE *inicio;
    PACIENTE *fim;
    int qtde;
} DESCRITOR_FILA;

typedef struct {
    DESCRITOR_FILA fila[NUM_PRIORIDADES];
} FILAS_PRIORIDADE;

/* Lista duplamente encadeada de médicos/salas */
typedef struct medico {
    int id;
    int ocupado;                       /* 0 = livre, 1 = atendendo */
    PACIENTE *atual;
    int tempoRestante;
    int atendidosPorPrioridade[NUM_PRIORIDADES];

    struct medico *ant;
    struct medico *prox;
} MEDICO;

typedef struct {
    MEDICO *inicio;
    MEDICO *fim;
    int qtde;
} LISTA_MEDICOS;

/* ========= Funções de apoio ========= */

const char *prioridadeToString(int prioridade) {
    switch (prioridade) {
        case PRIO_VERMELHO: return "Vermelho";
        case PRIO_AMARELO:  return "Amarelo";
        case PRIO_VERDE:    return "Verde";
        default:            return "Desconhecida";
    }
}

/* ========= Fila de pacientes ========= */

void inicializarFilas(FILAS_PRIORIDADE *f) {
    int i;
    if (f == NULL) return;
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        f->fila[i].inicio = NULL;
        f->fila[i].fim    = NULL;
        f->fila[i].qtde   = 0;
    }
}

PACIENTE *criarPaciente(int id,
                        int prioridade,
                        int tempoAtendimento,
                        const char *nome,
                        const char *queixa,
                        const char *dataChegada,
                        int tempoChegada) {
    PACIENTE *p = new PACIENTE;

    p->id = id;
    p->prioridade = prioridade;
    p->tempoAtendimento = tempoAtendimento;

    /* Copia segura de strings */
    strncpy(p->nome, nome, TAM_NOME - 1);
    p->nome[TAM_NOME - 1] = '\0';

    strncpy(p->queixa, queixa, TAM_QUEIXA - 1);
    p->queixa[TAM_QUEIXA - 1] = '\0';

    strncpy(p->dataChegada, dataChegada, TAM_DATA - 1);
    p->dataChegada[TAM_DATA - 1] = '\0';

    p->tempoChegada = tempoChegada;
    p->tempoInicio = -1;
    p->prox = NULL;
    
    return p;
}

PACIENTE *desenfileirarPaciente(DESCRITOR_FILA *desc) {
    PACIENTE *p;
    if (desc == NULL || desc->inicio == NULL) return NULL;

    p = desc->inicio;
    desc->inicio = p->prox;
    if (desc->inicio == NULL) {
        desc->fim = NULL;
    }
    p->prox = NULL;
    desc->qtde--;
    return p;
}

void enfileirarPaciente(FILAS_PRIORIDADE *filas, PACIENTE *p) {
    DESCRITOR_FILA *desc;
    if (filas == NULL || p == NULL) return;

    if (p->prioridade < 0 || p->prioridade >= NUM_PRIORIDADES) {
        p->prioridade = PRIO_VERDE;
    }

    desc = &(filas->fila[p->prioridade]);
    p->prox = NULL;

    if (desc->inicio == NULL) {
        desc->inicio = p;
        desc->fim = p;
    } else {
        desc->fim->prox = p;
        desc->fim = p;
    }
    desc->qtde++;
}

PACIENTE *obterProximoPaciente(FILAS_PRIORIDADE *filas) {
    int pr;
    if (filas == NULL) return NULL;

    for (pr = PRIO_VERMELHO; pr <= PRIO_VERDE; pr++) {
        DESCRITOR_FILA *desc = &(filas->fila[pr]);
        if (desc->inicio != NULL) {
            return desenfileirarPaciente(desc);
        }
    }
    return NULL;
}

int haPacientes(FILAS_PRIORIDADE *filas) {
    int i;
    if (filas == NULL) return 0;
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        if (filas->fila[i].inicio != NULL) return 1;
    }
    return 0;
}

void limparFilas(FILAS_PRIORIDADE *filas) {
    int i;
    if (filas == NULL) return;

    for (i = 0; i < NUM_PRIORIDADES; i++) {
        DESCRITOR_FILA *desc = &(filas->fila[i]);
        while (desc->inicio != NULL) {
            PACIENTE *p = desenfileirarPaciente(desc);
            delete p;              /* só delete, sem free */
        }
    }
}

/* ========= Lista de médicos ========= */

void inicializarListaMedicos(LISTA_MEDICOS *lista) {
    if (lista == NULL) return;
    lista->inicio = NULL;
    lista->fim = NULL;
    lista->qtde = 0;
}

MEDICO *adicionarMedico(LISTA_MEDICOS *lista, int id) {
    MEDICO *m;
    int i;

    if (lista == NULL) return NULL;

    m = new MEDICO;
    m->id = id;
    m->ocupado = 0;
    m->atual = NULL;
    m->tempoRestante = 0;
    for (i = 0; i < NUM_PRIORIDADES; i++) {
        m->atendidosPorPrioridade[i] = 0;
    }
    m->ant = lista->fim;
    m->prox = NULL;

    if (lista->fim != NULL) {
        lista->fim->prox = m;
    } else {
        lista->inicio = m;
    }
    lista->fim = m;
    lista->qtde++;

    return m;
}

/* Só remove médico se ele estiver livre */
int removerMedico(LISTA_MEDICOS *lista, int id) {
    MEDICO *m;
    if (lista == NULL) return 0;

    m = lista->inicio;
    while (m != NULL) {
        if (m->id == id) {
            if (m->ocupado && m->atual != NULL) {
                return 0; /* não remove médico ocupado */
            }

            if (m->ant != NULL) {
                m->ant->prox = m->prox;
            } else {
                lista->inicio = m->prox;
            }

            if (m->prox != NULL) {
                m->prox->ant = m->ant;
            } else {
                lista->fim = m->ant;
            }

            delete m;
            lista->qtde--;
            return 1;
        }
        m = m->prox;
    }

    return 0;
}

int medicosOcupados(LISTA_MEDICOS *lista) {
    MEDICO *m;
    if (lista == NULL) return 0;

    m = lista->inicio;
    while (m != NULL) {
        if (m->ocupado && m->atual != NULL) return 1;
        m = m->prox;
    }
    return 0;
}

/* Atribui pacientes a médicos livres, respeitando prioridades */
void alocarPacientes(LISTA_MEDICOS *lista,
                     FILAS_PRIORIDADE *filas,
                     int tempoAtual) {
    MEDICO *m;
    if (lista == NULL || filas == NULL) return;

    m = lista->inicio;
    while (m != NULL) {
        if (!m->ocupado && haPacientes(filas)) {
            PACIENTE *p = obterProximoPaciente(filas);
            if (p == NULL) break;

            p->tempoInicio = tempoAtual;
            m->atual = p;
            m->tempoRestante = p->tempoAtendimento;
            m->ocupado = 1;
        }
        m = m->prox;
    }
}

/* Avança uma unidade de tempo de atendimento de todos os médicos */
void avancarTempo(LISTA_MEDICOS *lista,
                  int totalAtendidos[NUM_PRIORIDADES],
                  long somaEspera[NUM_PRIORIDADES]) {
    MEDICO *m;
    if (lista == NULL) return;

    m = lista->inicio;
    while (m != NULL) {
        if (m->ocupado && m->atual != NULL) {
            m->tempoRestante--;
            if (m->tempoRestante <= 0) {
                PACIENTE *p = m->atual;
                int pr = p->prioridade;
                int espera;

                if (pr < 0 || pr >= NUM_PRIORIDADES) pr = PRIO_VERDE;

                totalAtendidos[pr]++;

                espera = p->tempoInicio - p->tempoChegada;
                if (espera < 0) espera = 0;
                somaEspera[pr] += espera;

                m->atendidosPorPrioridade[pr]++;

                delete p;
                m->atual = NULL;
                m->tempoRestante = 0;
                m->ocupado = 0;
            }
        }
        m = m->prox;
    }
}

void limparListaMedicos(LISTA_MEDICOS *lista) {
    MEDICO *m, *prox;
    if (lista == NULL) return;

    m = lista->inicio;
    while (m != NULL) {
        prox = m->prox;
        if (m->atual != NULL) {
            delete m->atual;
        }
        delete m;
        m = prox;
    }
    lista->inicio = NULL;
    lista->fim = NULL;
    lista->qtde = 0;
}

#endif

