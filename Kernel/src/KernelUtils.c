#include "../include/KernelUtils.h"

void setearConfiguracionesGenerales(t_config *configFile) {
    if (config_has_property(configFile, "IP"))
        ip = config_get_string_value(configFile, "IP");
    else {
        log_error(logger, "Te falta setear la IP en tu configFile correctamente");
        abort();
    }

    if (config_has_property(configFile, "PUERTO_ESCUCHA"))
        puerto = config_get_string_value(configFile, "PUERTO_ESCUCHA");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }

    if (config_has_property(configFile, "GRADO_MULTIPROGRAMACION"))
        gradoMultiprogramacion = config_get_int_value(configFile, "GRADO_MULTIPROGRAMACION");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (config_has_property(configFile, "ALGORITMO_PLANIFICACION")) {
        char *algoritmoPlanif = config_get_string_value(configFile, "ALGORITMO_PLANIFICACION");
        if (strcmp(algoritmoPlanif, "SJF") == 0)
            tipoPlanificador = SJF;
        else //asigno por default FIFO si no
            //if(strcmp(algoritmoPlanif, "FIFO") == 0)
            tipoPlanificador = FIFO;
    } else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (tipoPlanificador == SJF) {
        if (config_has_property(configFile, "ESTIMACION_INICIAL")) {
            estimacionInicialRafaga = config_get_int_value(configFile, "ESTIMACION_INICIAL");
        } else {
            log_error(logger, "Te falta setear una configFile correctamente");
            abort();
        }
    }
    if (config_has_property(configFile, "TIEMPO_MAXIMO_BLOQUEADO"))
        tiempoMaxBloqueado = config_get_int_value(configFile, "TIEMPO_MAXIMO_BLOQUEADO");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (config_has_property(configFile, "IP_MEMORIA"))
        ipMemoria = config_get_string_value(configFile, "IP_MEMORIA");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (config_has_property(configFile, "PUERTO_MEMORIA"))
        puertoMemoria = config_get_string_value(configFile, "PUERTO_MEMORIA");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (config_has_property(configFile, "IP_CPU"))
        ipCPU = config_get_string_value(configFile, "IP_CPU");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (config_has_property(configFile, "PUERTO_CPU_DISPATCH"))
        puertoCPUDispatch = config_get_string_value(configFile, "PUERTO_CPU_DISPATCH");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (config_has_property(configFile, "PUERTO_CPU_INTERRUPT"))
        puertoCPUInterrupt = config_get_string_value(configFile, "PUERTO_CPU_INTERRUPT");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
    if (config_has_property(configFile, "ALFA"))
        alfa = config_get_double_value(configFile, "ALFA");
    else {
        log_error(logger, "Te falta setear una configFile correctamente");
        abort();
    }
}

Proceso *recibirProceso(int socket_cliente) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    Proceso *proceso = deserializarProceso(paquete->buffer);

    log_info(logger, "Ya recibi el proceso con tamaño %d", proceso->tamProceso);

    eliminar_paquete(paquete);
    return proceso;
}

Proceso *deserializarProceso(t_buffer *buffer) {
    Proceso *proceso = malloc(sizeof(Proceso));

    void *stream = buffer->stream;
    uint32_t tamanioInstrucciones = 0;

    memcpy(&(proceso->tamProceso), stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    memcpy(&tamanioInstrucciones, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    proceso->instrucciones = malloc(tamanioInstrucciones);

    memcpy(proceso->instrucciones, stream, tamanioInstrucciones);

    //log_info(logger, "Ya deserialice el proceso %d \n", proceso->tamProceso);
    return proceso;
}

void printProceso(Proceso *proc) {
    log_info(logger, "Tamaño del proceso %d", proc->tamProceso);
    char **instrucciones = string_split(proc->instrucciones, "\n");
    log_info(logger, "Cant. instrucciones: %d", string_array_size(instrucciones));
    for (int i = 0; i < string_array_size(instrucciones); i++) {
        log_info(logger, "Instrucción %d: %s", i+1, instrucciones[i]);
    }
}

void asignarPcb(char *instrucciones, uint32_t tamanio, int socket) {
    log_info(logger, "Asignando nuevo pcb");
    Pcb *pcb = malloc(sizeof(Pcb));
    pcb->pid = generador_id;
    pcb->tamanio = tamanio;
    pcb->instrucciones = instrucciones;
    pcb->programCounter = 0;
    pcb->estimacionRafaga = estimacionInicialRafaga;
    pcb->estado = NEW;
    pcb->conexionConsola = string_itoa(socket);
    generador_id++;

    agregarAEstado(estadoNew, pcb);
    sem_post(&semPlanifLargoPlazo);
}

void exitPcb(Pcb* pcb) {
    if (pcb->estado == EXEC) {
        cambiarDeEstado(EXEC, QUIT, pcb); //revisar si es necesario el estado exit o es al pedin (despues se acumularian en exit)
        //sem avisando que salio de exec...
        sem_post(&semPcbEjecutando);
        sem_post(&contadorGradoMultiProgramacion);

        //enviar mensaje a Memoria y Consola de que se quitea el pcb
        enviarMensaje(string_itoa(pcb->tablaPaginas), conexionMemoria, MEMORY_EXIT);
        enviarExitConsola(pcb);

        free(pcb);
    }
}


/**
 * estadoViejo
 * estadoNuevo
 * pcb -> pcb a cambiar de estadoViejo a estadoNuevo
 * Se elimina de la lista de estadoViejo (correspondiente) y se agrega a la lista
 * de estado nuevo.
 * Ademas se actualiza luego el estado actual del pcb al que se paso estadoNuevo
 */
void cambiarDeEstado(Estado estadoViejo, Estado estadoNuevo, Pcb *pcb) {
    //todo alguna validacion requiere aca ?

    if (estaEnEstado(pcb, estadoViejo)) {
        t_list *listaEstadoViejo = obtenerListaSegunEstado(estadoViejo);
        t_list *listaEstadoNuevo = obtenerListaSegunEstado(estadoNuevo);
        eliminarDeEstado(listaEstadoViejo, pcb);
        agregarAEstado(listaEstadoNuevo, pcb);
        actualizarEstadoDePcb(pcb, estadoNuevo);

        log_info(logger, "Se cambio el pcb id %d de estado. Ahora su estado es %u ", pcb->pid, pcb->estado);
    }
}

bool estaEnEstado(Pcb *pcb, Estado estado) {
    return pcb->estado == estado;
}

void agregarAEstado(t_list *listaEstado, Pcb *pcb) {
    pthread_mutex_lock(&mutex_cambio_est);
    list_add(listaEstado, pcb);
    pthread_mutex_unlock(&mutex_cambio_est);
}

void eliminarDeEstado(t_list *listaEstado, Pcb *pcb) {
    bool esElPcb(void *elPcb) {
        Pcb *otroPcb = elPcb;
        return otroPcb->pid == pcb->pid;
    }

    pthread_mutex_lock(&mutex_cambio_est);
    list_remove_by_condition(listaEstado, esElPcb);
    pthread_mutex_unlock(&mutex_cambio_est);
}

t_list *obtenerListaSegunEstado(Estado estado) {
    switch (estado) {
        case NEW:
            return estadoNew;
        case READY:
            return estadoReady;
        case BLOCKED:
            return estadoBlockedIO;
        case BLOCKEDSUSP:
            return estadoBlockedSusp;
        case EXEC:
            return estadoExec;
        case SUSPENDEDREADY:
            return estadoSuspendedReady;
        case QUIT:
            return estadoExit;
        default:
            log_error(logger, "El listado de estado %u no existe...", estado);
            return NULL;
    }
}

void actualizarEstadoDePcb(Pcb *pcb, Estado nuevoEstado) {
    pthread_mutex_lock(&mutex_cambio_est);
    pcb->estado = nuevoEstado;
    pthread_mutex_unlock(&mutex_cambio_est);
}
