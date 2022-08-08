#include "../include/GestionPaginas.h"

void cargarPaginaEnFrameNuevo(EstructuraProceso *proceso, uint32_t numeroPagina, TablaNivel2 *entrada) {
    FramePagina *framePagina = malloc(sizeof(FramePagina));
    int frameLibre = obtenerPrimerFrameLibre();
    frameDePaginas[frameLibre] = proceso->numeroTabla;
    framePagina->frame = frameLibre;
    framePagina->pagina = numeroPagina;
    framePagina->tablaNivel2 = entrada;
    list_add(proceso->listaFramePagina, framePagina);
    setEntradaTabla(entrada, frameLibre, 1, 1, 0);
    void *buf = malloc(configMemoria->tamPagina);
    crearSolicitudSwap(LEER_PAGINA_SWAP, proceso->numeroTabla, (t_list *) NULL, &buf, numeroPagina, (int) NULL);
    sem_wait(&semEsperarSwap);
    memcpy(memory + configMemoria->tamPagina * framePagina->frame, buf, configMemoria->tamPagina);
    free(buf);
}

void cargarPaginaReemplazando(EstructuraProceso *proceso, uint32_t numeroPagina, TablaNivel2 *entrada) {
    FramePagina *framePagina = obtenerFramePaginaVictima(proceso);
    TablaNivel2 *entradaVieja = framePagina->tablaNivel2;
    int frameViejo = framePagina->frame;
    int paginaVieja = framePagina->pagina;
    int modificadoViejo = entradaVieja->modificado;
    framePagina->pagina = numeroPagina;
    framePagina->tablaNivel2 = entrada;
    setEntradaTabla(entradaVieja, -1, 0, 0, 0);
    setEntradaTabla(entrada, frameViejo, 1, 1, 0);
    void *buf = malloc(configMemoria->tamPagina);
    if (modificadoViejo == 1) {
        memcpy(buf, memory + configMemoria->tamPagina * frameViejo, configMemoria->tamPagina);
        crearSolicitudSwap(ESCRIBIR_PAGINA_SWAP, proceso->numeroTabla, (t_list *) NULL, &buf, numeroPagina, paginaVieja);
    } else
        crearSolicitudSwap(LEER_PAGINA_SWAP, proceso->numeroTabla, (t_list *) NULL, &buf, numeroPagina, (int) NULL);
    sem_wait(&semEsperarSwap);
    memcpy(memory + configMemoria->tamPagina * framePagina->frame, buf, configMemoria->tamPagina);
    free(buf);
}

FramePagina *obtenerFramePaginaVictima(EstructuraProceso *proceso) {
    int i;
    int flag = 1;
    int vuelta = -1;
    FramePagina *framePagina;
    TablaNivel2 *entradaParaClock;
    if (algoritmoReemplazo == CLOCK) {
        for (i = punteroClock[proceso->id]; flag > 0; i++) {
            if (i == list_size(proceso->listaFramePagina)) i = 0;
            framePagina = list_get(proceso->listaFramePagina, i);
            entradaParaClock = framePagina->tablaNivel2;
            if (entradaParaClock->uso == 0) {
                punteroClock[proceso->id] = i;
                flag = 0;
            } else entradaParaClock->uso = 0;
        }
    } else
        if (algoritmoReemplazo == CLOCKM) {
        for (i = punteroClock[proceso->id]; flag > 0; i++) {
            if (i == list_size(proceso->listaFramePagina))
                i = 0;

            if (vuelta == -1)
                vuelta = 0;
            else if (vuelta == 0 && i == punteroClock[proceso->id])
                vuelta = 1;

            framePagina = list_get(proceso->listaFramePagina, i);
            entradaParaClock = framePagina->tablaNivel2;

            if (vuelta == 0) {
                if (entradaParaClock->uso == 0 && entradaParaClock->modificado == 0) {
                    punteroClock[proceso->id] = i;
                    flag = 0;
                }
            } else
                if (vuelta == 1) {
                    if (entradaParaClock->uso == 0) {
                        punteroClock[proceso->id] = i;
                        flag = 0;
                    } else
                        entradaParaClock->uso = 0;
                }
        }
    }
        else log_error(logger, "Algoritmo de reemplazo desconocido");

    return framePagina;
}
