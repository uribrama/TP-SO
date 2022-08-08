#ifndef MEMORIA_H
#define MEMORIA_H

#include <pthread.h>
#include <utils.h>
#include "GestionPaginas.h"

char* pathConfig;

void esperarConexion(int);

void *atenderClienteMemoria(void *);

void inicializarMemoria(void);

void inicializarEstructuras();

void finalizarMemoria(int);

void destruirTablas();

void destruirEstructuras(EstructuraProceso *);

void destruirTabla2(t_list *);

#endif /* MEMORIA_H */
