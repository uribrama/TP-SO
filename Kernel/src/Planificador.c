/*
 * Name: planificador.c
*  Author: scalonetaSO
 */

#include "../include/Planificador.h"

/*void inicioEstructuraPlanificacion(){//Tengo que recibir la lista de procesos -> (ya estan iniciadas las listas de estados para que seria esto)
    log_info(logger, "Iniciando Planificacion");
    //planificadorCortoPlazo();
    //planificadorMedianoPlazo();
    //planificadorLargoPlazo();
}*/

void planificadorCortoPlazo() {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    if (tipoPlanificador == FIFO) {
        pthread_create(&planificador, &attr, (void *) fifo, NULL);
        log_info(logger, "Inicio planificador corto plazo con fifo");
    } else {
        pthread_create(&planificador, &attr, (void *) sjf, NULL);
        log_info(logger, "Inicio planificador corto plazo con sjf");
    }
}

void iniciarHiloDeSuspensionYBloqueado() {
    pthread_create(&hiloBloqueos, NULL, (void*) &pcbBlocked, NULL);
    pthread_detach(hiloBloqueos);

    pthread_create(&hiloSuspended, NULL, (void*) &pcbSuspended, NULL);
    pthread_detach(hiloSuspended);
}
void planificadorLargoPlazo() {
    pthread_create(&planificadorLargo, NULL, (void *) planificarLargoPlazo, NULL);
    log_info(logger, "Inicio planificador largo plazo");
    pthread_detach(planificadorLargo);
}

void planificadorMedianoPlazo(Pcb* pcb) {
    // SUSPENDED_READY -> READY
    log_info(logger, "Empieza el contador para la suspensión del proceso %d", pcb->pid);
    usleep(tiempoMaxBloqueado * 1000);
    pthread_mutex_lock(&mutexPlanifMedianoPlazo);
    int posicion = posicionDeProcesoEnLista(estadoBlockedIO, pcb->pid);
    if (posicion >= 0) {
        //se suspende proceso por exceso de tiempo blockeado
        enviarMensaje(string_itoa(pcb->tablaPaginas), conexionMemoria, MEMORY_SUSPEND);
        char *buffer = recibirUnPaqueteConUnMensaje(conexionMemoria, true);
        if (strcmp(buffer, "OK") == 0) {
            log_info(logger, "Proceso %d suspendido en memoria correctamente", pcb->pid);
            cambiarDeEstado(BLOCKED, BLOCKEDSUSP, pcb);

            sem_post(&semCantidadProcesosEnSuspendedBlocked);
            sem_post(&contadorGradoMultiProgramacion);
        } else log_info(logger, "Suspensión del proceso %d DENEGADA por memoria", pcb->pid);
    }
    pthread_mutex_unlock(&mutexPlanifMedianoPlazo);
}

void planificarLargoPlazo() {
    while (1) {
        sem_wait(&semPlanifLargoPlazo);
        sem_wait(&semPermitidoParaNew);
        sem_post(&semPermitidoParaNew);
        sem_wait(&contadorGradoMultiProgramacion);
        log_info(logger, "Entro un pcb al Planificacion largo plazo");
        if (list_size(estadoNew) > 0) { // NO TENDRIA SENTIDO ESTA CONDICION, si entro es porque semPlanifLargoPlazo se envio un post al crear el pcb antes.
            if (list_size(estadoSuspendedReady) == 0) {
                Pcb *pcb = list_get(estadoNew, 0);

                log_info(logger, "Procesos en ready: %d", estadoReady->elements_count);
                enviarPCB(pcb, MEMORY_INIT, conexionMemoria);

                char *buffer = recibirUnPaqueteConUnMensaje(conexionMemoria, true);
                int nroPagina = atoi(buffer);
                log_info(logger, "Continuo planif largo plazo, recibi la pagina %u", nroPagina);
                pcb->tablaPaginas = nroPagina;

                cambiarDeEstado(NEW, READY, pcb);
                sem_post(&semPcbready); // aviso que hay alguien en ready

                if (tipoPlanificador == SJF && estadoExec->elements_count > 0) {
                    enviarInterrupcionACPU(CPU_INTERRUPT);
                    //si es SFJ, cuando pasa a READY se deberá enviar una Interrupción al proceso CPU
                    //Al recibir el PCB del proceso en ejecución, se calcularán las estimaciones correspondientes
                }

                free(buffer);
                //free(pcb)
            } else
                sem_post(&contadorGradoMultiProgramacion);
        }
    }
}

void fifo() {
    while (1) {
        sem_wait(&semPcbready);
        sem_wait(&semPcbEjecutando);
        if (estadoReady->elements_count > 0) {
            Pcb * pcb = list_get(estadoReady, 0);
            cambiarDeEstado(READY, EXEC, pcb);
            log_info(logger, "Pasa pcb %u a exec por FIFO", pcb->pid);
            //sem_post(&semPcbEjecutando);
            enviarDispatchCPU(KERNEL_OP, pcb);
        }
    }
}

void sjf() {
    /**
     * logica para ordenar lista de estadoReady segun rafaga mas chica
     */
    bool rafagaMenor(Pcb *unPcb, Pcb *otroPcb) {
        return unPcb->estimacionRafaga < otroPcb->estimacionRafaga;
    }

    while (1) {
        sem_wait(&semPcbready);
        sem_wait(&semPcbEjecutando);
        estadoReady = list_sorted(estadoReady, (void *) rafagaMenor);
        Pcb *pcb;
        Pcb *pcbExecuting;
        if (estadoReady->elements_count > 0) {
            pcb = list_get(estadoReady, 0);
            tiempoEnvio = time(NULL);
            cambiarDeEstado(READY, EXEC, pcb);
            log_info(logger, "Pasa pcb %u a exec por SJF", pcb->pid);
            //sem_post(&semPcbEjecutando);
            enviarDispatchCPU(KERNEL_OP, pcb);
        }
    }
}


void pcbBlocked() {
    while (1) {
        sem_wait(&semCantidadProcesosEsperandoDesbloqueo);

        PcbBlockeo *pcbBlockeo = list_remove(listaPendientesDeDesbloqueo, 0);
        Pcb *pcb = pcbBlockeo->pcb;

        log_info(logger, "El proceso %d comienza a hacer su IO", pcb->pid);
        usleep(pcbBlockeo->tiempoBloqueo * 1000);
        pthread_mutex_lock(&mutexPlanifMedianoPlazo);

        int posicion = posicionDeProcesoEnLista(estadoBlockedIO, pcb->pid);
        if (posicion >= 0) {
            cambiarDeEstado(BLOCKED, READY, pcb);

            log_info(logger, "El proceso %d finaliza de hacer su IO y vuelve a ready", pcb->pid);
            sem_post(&semPcbready);

            if (tipoPlanificador == SJF)
                enviarInterrupcionACPU(CPU_INTERRUPT);
        }
        else {
            posicion = posicionDeProcesoEnLista(estadoBlockedSusp, pcb->pid);
            if (posicion >= 0) {
                cambiarDeEstado(BLOCKEDSUSP, SUSPENDEDREADY, pcb);
                if (list_size(estadoSuspendedReady) == 1)
                    sem_wait(&semPermitidoParaNew);

                log_info(logger, "El proceso %d finaliza de hacer su IO pero está suspendido", pcb->pid);
                sem_wait(&semCantidadProcesosEnSuspendedBlocked);
                sem_post(&semCantidadProcesosEnSuspendedReady);
            } else
                log_error(logger, "Falta el proceso proceso %d .....", pcb->pid);
        }
            pthread_mutex_unlock(&mutexPlanifMedianoPlazo);

        free(pcbBlockeo); //todo validar que este bien limpiarlo y no borre el pcb dentro
    }
}

void pcbSuspended() {
    while (1) {
        sem_wait(&semCantidadProcesosEnSuspendedReady);
        sem_wait(&contadorGradoMultiProgramacion);
        Pcb *pcb = list_get(estadoSuspendedReady, 0);
        log_info(logger, "El proceso %d vuelve a ready luego de estar suspendido", pcb->pid);
        cambiarDeEstado(SUSPENDEDREADY, READY, pcb);

        sem_post(&semPcbready);
        if (list_size(estadoSuspendedReady) == 0)
            sem_post(&semPermitidoParaNew);

        if (tipoPlanificador == SJF) {
            enviarInterrupcionACPU(CPU_INTERRUPT);
        }
    }
}

void actualizarEstimacion(Pcb *pcb, double rafaga) {
    // Est(n+1) = alfa r(n) + (1 - alfa) Est(n)
    double estimacionNueva = alfa * rafaga + (1 - alfa) * pcb->estimacionRafaga;
    log_info(logger, "Actualizo la estimacion del proceso %d: pasa de %f a %f milisegundos", pcb->pid, pcb->estimacionRafaga, estimacionNueva);
    pcb->estimacionRafaga = estimacionNueva;
    tiempoEnvio = 0;
}

void desalojarPcb(Pcb *pcb) {
    //calculo el tiempo ejecutado real
    time_t tiempoRecepcion = time(NULL);
    double tiempoRafaga = difftime(tiempoRecepcion, tiempoEnvio) * 1000;
    cambiarDeEstado(EXEC, READY, pcb);
    //todo esta bien actualizar la estimacion si es un desalojo por nuevo pcb recibido ?
    actualizarEstimacion(pcb, tiempoRafaga);
    // permito la continuacion de del planif corto plazo para seleccionar el proximo pcb segun estimacion nueva
    sem_post(&semPcbEjecutando);
    sem_post(&semPcbready);
}

int posicionDeProcesoEnLista (t_list* lista, uint32_t id) {
    Pcb* pcb;
    for (int i=0; i<list_size(lista); i++) {
        pcb = list_get(lista,i);
        if (pcb->pid == id)
            return i;
    }
    return -1;
}
