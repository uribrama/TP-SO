#ifndef TP_2022_1C_SCALONETASO_PLANIFICADOR_H
#define TP_2022_1C_SCALONETASO_PLANIFICADOR_H

#include <pthread.h>
#include <errno.h>
#include <semaphore.h>
#include <time.h>
#include "KernelUtils.h"

sem_t semPcbready;
sem_t semPcbEjecutando;

pthread_t planificador;
pthread_t planificadorLargo;
pthread_t planificacionMedianoPlazo[1000];
pthread_t hiloBloqueos;
pthread_t hiloSuspended;

time_t tiempoEnvio;

void planificadorLargoPlazo();
void planificadorCortoPlazo();
void planificadorMedianoPlazo();
void planificarLargoPlazo();
void iniciarHiloDeSuspensionYBloqueado();
void fifo();
void sjf();
int posicionDeProcesoEnLista (t_list*, uint32_t);
void pcbBlocked();
void pcbSuspended();

#endif //TP_2022_1C_SCALONETASO_PLANIFICADOR_H
