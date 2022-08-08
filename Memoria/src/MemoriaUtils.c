#include "../include/MemoriaUtils.h"
#include "../include/GestionPaginas.h"
//#include "GestionPaginas.c" //Include para Uri, sino no le compila. Comentar y descomentar según corresponda.
//#include "Swap.c" //Include para Uri, sino no le compila. Comentar y descomentar según corresponda.

void setearConfiguracionesGenerales(t_config *configFile) {
    configMemoria = malloc(sizeof(infoMemoria));

    if (config_has_property(configFile, "PUERTO_ESCUCHA"))
        puerto = config_get_string_value(configFile, "PUERTO_ESCUCHA");
    else {
        log_error(logger, "Te falta setear PUERTO_ESCUCHA correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "TAM_PAGINA"))
        configMemoria->tamPagina = config_get_int_value(configFile, "TAM_PAGINA");
    else {
        log_error(logger, "Te falta setear una config correctamente");
        exit(EXIT_FAILURE);
    }
    if (config_has_property(configFile, "ENTRADAS_POR_TABLA"))
        configMemoria->entradasTabla = config_get_int_value(configFile, "ENTRADAS_POR_TABLA");
    else {
        log_error(logger, "Te falta setear una config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "IP"))
        ip = config_get_string_value(configFile, "IP");
    else {
        log_error(logger, "Te falta setear la IP en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "TAM_MEMORIA"))
        tamMemoria = config_get_int_value(configFile, "TAM_MEMORIA");
    else {
        log_error(logger, "Te falta setear TAM_MEMORIA correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "PATH_SWAP"))
        pathSwap = strdup(config_get_string_value(configFile, "PATH_SWAP"));
    else {
        log_error(logger, "Te falta setear PATH_SWAP correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "ALGORITMO_REEMPLAZO")) {
        char *algoritmo = strdup(config_get_string_value(configFile, "ALGORITMO_REEMPLAZO"));
        if (strcmp(algoritmo, "CLOCK") == 0)
            algoritmoReemplazo = CLOCK;
        else
            algoritmoReemplazo = CLOCKM;
    } else {
        log_error(logger, "Te falta setear ALGORITMO_REEMPLAZO correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "MARCOS_POR_PROCESO"))
        framesPorProceso = config_get_int_value(configFile, "MARCOS_POR_PROCESO");
    else {
        log_error(logger, "Te falta setear MARCOS_POR_PROCESO correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "RETARDO_MEMORIA"))
        retardoMemoria = config_get_int_value(configFile, "RETARDO_MEMORIA");
    else {
        log_error(logger, "Te falta setear RETARDO_MEMORIA correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(configFile, "RETARDO_SWAP"))
        retardoSwap = config_get_int_value(configFile, "RETARDO_SWAP");
    else {
        log_error(logger, "Te falta setear RETARDO_SWAP correctamente");
        exit(EXIT_FAILURE);
    }

    log_info(logger, "Tamaño de páginas: %d\nEntradas por tabla: %d", configMemoria->tamPagina, configMemoria->entradasTabla);
}

uint32_t nuevoProceso(Pcb *pcb) {
    log_info(logger, "Creando nuevo proceso");
    uint32_t numeroTabla;
    EstructuraProceso *proceso = malloc(sizeof(EstructuraProceso));
    proceso->id = pcb->pid;
    proceso->size = pcb->tamanio;
    proceso->listaFramePagina = list_create();
    numeroTabla = list_size(tablaPaginasPrincipal);
    proceso->numeroTabla = numeroTabla;
    crearTablas(proceso, pcb);
    list_add(tablaPaginasPrincipal, proceso);
    crearSolicitudSwap(NUEVO, numeroTabla, (t_list*) NULL, (void*) NULL, (int) NULL, (int) NULL);
    sem_wait(&semEsperarSwap);
    free(pcb);
    return numeroTabla;
}

void finProceso(uint32_t numeroTabla) {
    log_info(logger, "Creando solicitud de fin, numero tabla: %u", numeroTabla);
    EstructuraProceso *proceso = list_get(tablaPaginasPrincipal, numeroTabla);
    liberarFrames(proceso);
    crearSolicitudSwap(FIN, numeroTabla, (t_list *) NULL, (void *) NULL, (int) NULL, (int) NULL);
    sem_wait(&semEsperarSwap);
    free(proceso); //todo verificar si esta bien qeu haga un free aca
}

bool suspenderProceso(uint32_t numeroTabla) {
    log_info(logger, "Suspendiendo proceso, numero tabla: %u", numeroTabla);
    bool resultado = 0;

    EstructuraProceso * proceso = list_get(tablaPaginasPrincipal,numeroTabla);
    t_list* lista = crearDataParaEscribirEnSwap(proceso);
    crearSolicitudSwap(SUSPENDIDO, numeroTabla, lista, (void*) NULL, (int) NULL, (int) NULL);
    liberarFrames(proceso);
    resetearTablas2(proceso);
    sem_wait(&semEsperarSwap);
    return resultado;
}

uint32_t tabla1ATabla2(uint32_t numeroTabla1, uint32_t entrada) {
	log_debug(logger, "Numero de tabla 1: %d | Numero entrada: %d", numeroTabla1, entrada);
    uint32_t numeroTabla2;
    EstructuraProceso * est = list_get(tablaPaginasPrincipal,numeroTabla1);
    numeroTabla2 = est->tabla1[entrada];
    return numeroTabla2;
}

uint32_t tabla2AFrame(uint32_t numeroTabla2, uint32_t numeroEntrada) {
	log_debug(logger, "Numero de tabla 2: %d | Numero entrada: %d", numeroTabla2, numeroEntrada);
    uint32_t frame = 0;
    t_list* tabla2 = list_get(tablaPaginasNivel2,numeroTabla2);
    TablaNivel2 * entrada = list_get(tabla2,numeroEntrada);
    entrada->uso=1;
    if (entrada->presencia == 0) cargarPaginaEnMemoria(numeroTabla2, numeroEntrada, entrada);
    frame = entrada->frame;
    return frame;
}

void crearTablas(EstructuraProceso *proceso, Pcb *pcb) { // todo revisar
    int i;
    t_list *tabla2;
    double cantPaginas = (double) pcb->tamanio / (double) configMemoria->tamPagina;
    proceso->cantidadPaginas = ceil(cantPaginas); //retorna la cantidad de paginas que ocupa el tamaño del proceso (siempre es round top)
    cantPaginas = (double) proceso->cantidadPaginas / (double) configMemoria->entradasTabla;
    proceso->cantidadTablas2 = ceil(cantPaginas);
    proceso->tabla1 = malloc(sizeof(int) * proceso->cantidadTablas2);
    for (i = 0; i < proceso->cantidadTablas2; i++) {
        proceso->tabla1[i] = list_size(tablaPaginasNivel2);
        tabla2 = list_create();
        list_add(tablaPaginasNivel2, tabla2);
    }
    resetearTablas2(proceso);
}

void resetearTablas2(EstructuraProceso *proceso) {    // revisar esto
    uint32_t indexTablaProceso;
    t_list *tabla2;
    TablaNivel2 *tablaNivel2;
    for (int i = 0; i < proceso->cantidadTablas2; i++) {
        indexTablaProceso = proceso->tabla1[i];
        tabla2 = list_get(tablaPaginasNivel2, indexTablaProceso);
        if (list_size(tabla2) > 0) list_clean_and_destroy_elements(tabla2, (void *) free);
        for (int j = 0; j < configMemoria->entradasTabla; j++) {
            tablaNivel2 = malloc(sizeof(TablaNivel2));
            setEntradaTabla(tablaNivel2, -1, 0, 0, 0);
            tablaNivel2->numeroTabla1 = proceso->numeroTabla;
            list_add(tabla2, tablaNivel2);
        }
    }
}

//para fin y suspender proceso
void liberarFrames(EstructuraProceso *proceso) {
    uint32_t aux;
    FramePagina *framePagina;
    for (int i = 0; i < list_size(proceso->listaFramePagina); i++) {
        framePagina = list_get(proceso->listaFramePagina, i);
        aux = framePagina->frame;
        frameDePaginas[aux] = -1;
        free(framePagina);
    }
    list_clean(proceso->listaFramePagina);
}

void cargarPaginaEnMemoria(uint32_t numeroTabla2, uint32_t numeroEntrada, TablaNivel2 *entrada) {
    EstructuraProceso *proceso = list_get(tablaPaginasPrincipal, entrada->numeroTabla1);
    int numeroPagina = obtenerNumeroPagina(proceso, numeroTabla2, numeroEntrada);
    if (list_size(proceso->listaFramePagina) < framesPorProceso) {
    	log_info(logger, "Cargando pagina %d en frame nuevo del proceso %d", numeroPagina, proceso->id);
        cargarPaginaEnFrameNuevo(proceso, numeroPagina, entrada);
    } else {
    	log_info(logger, "Reemplazo pagina %d en frame del proceso %d", numeroPagina, proceso->id);
        cargarPaginaReemplazando(proceso, numeroPagina, entrada);
    }
}


uint32_t obtenerPrimerFrameLibre() {
    for (int i = 0; i < cantidadFrames; i++) {
        if (frameDePaginas[i] == -1) return i;
    }
    return -1;
}

void setEntradaTabla(TablaNivel2 *tabla, uint32_t frame, uint32_t presencia, uint32_t uso, uint32_t modificado) {
    tabla->frame = frame;
    tabla->presencia = presencia;
    tabla->uso = uso;
    tabla->modificado = modificado;
}


t_list *crearDataParaEscribirEnSwap(EstructuraProceso *proceso) {
    t_list *listData = list_create();
    int i;
    FramePagina *framePagina;
    EscrituraSwap *escrituraSwap;
    void *bufferData;
    for (i = 0; i < list_size(proceso->listaFramePagina); i++) {
        framePagina = list_get(proceso->listaFramePagina, i);
        escrituraSwap = malloc(sizeof(EscrituraSwap));
        escrituraSwap->pagina = framePagina->pagina;
        bufferData = malloc(configMemoria->tamPagina);
        memcpy(bufferData, memory + configMemoria->tamPagina * framePagina->frame, configMemoria->tamPagina);
        escrituraSwap->buffer = bufferData;
        list_add(listData, escrituraSwap);
    }
    return listData;
}

uint32_t obtenerNumeroPagina(EstructuraProceso *proceso, uint32_t numeroTabla2, uint32_t numeroEntrada) {
    uint32_t numeroPagina, aux;

    for (int i = 0; i < proceso->cantidadTablas2; i++) {
        if (proceso->tabla1[i] == numeroTabla2) aux = i;
    }
    numeroPagina = aux * configMemoria->entradasTabla + numeroEntrada;
    return numeroPagina;
}


////////////////////// LECTURAS Y ESCRITURAS /////////////////////////////////////////////////////////////////////////////7

uint32_t leer(uint32_t direccionFisica) {
    uint32_t lectura;
    memcpy(&lectura, memory + direccionFisica, sizeof(uint32_t));
    TablaNivel2 *entrada = obtenerEntradaSegunDireccionFisica(direccionFisica);
    entrada->uso = 1;
    return lectura;
}

uint32_t escribir(uint32_t direccionFisica, uint32_t valor) {
    uint32_t respuesta = 0;
    memcpy(memory + direccionFisica, &valor, sizeof(uint32_t));
    TablaNivel2 *entrada = obtenerEntradaSegunDireccionFisica(direccionFisica);
    entrada->uso = 1;
    entrada->modificado = 1;
    return respuesta;
}

TablaNivel2 *obtenerEntradaSegunDireccionFisica(int direccionFisica) {
    int numeroFrame = floor(direccionFisica / configMemoria->tamPagina);
    uint32_t numeroTabla1 = frameDePaginas[numeroFrame];
    EstructuraProceso *proceso = list_get(tablaPaginasPrincipal, numeroTabla1);
    FramePagina *framePagina;
    uint32_t pagina;
    for (int i = 0; i < list_size(proceso->listaFramePagina); i++) {
        framePagina = list_get(proceso->listaFramePagina, i);

        if (framePagina->frame == numeroFrame)
            pagina = framePagina->pagina;
    }

    int entradaTabla1 = floor(pagina / configMemoria->entradasTabla);
    int entradaTabla2 = pagina - entradaTabla1 * configMemoria->entradasTabla;

    t_list *tabla2 = list_get(tablaPaginasNivel2, proceso->tabla1[entradaTabla1]);
    TablaNivel2 *entrada = list_get(tabla2, entradaTabla2);
    return entrada;
}
