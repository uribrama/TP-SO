/*
 ============================================================================
 Name        : Swap.c
 Author      : scalonetaSO
 Description : Módulo CPU: SWAP
 ============================================================================
 */

#include "../include/Swap.h"
//#include <unistd.h>
//#include <sys/types.h>

void iniciarSwap() {
    iniciarCarpetaSwap();
    if (pthread_create(&hiloSwap, NULL, (void *) &procesarSolicitudesSwap, NULL) != 0) {
        log_error(logger, "Error al crear el hilo del swap");
    } else log_info(logger, "Swap iniciado correctamente");
}

void procesarSolicitudesSwap() {
    SolicitudSwap *solicitud;
    while (1) {
        sem_wait(&semSwap);
        solicitud = list_remove(listaSwap, 0);
        switch (solicitud->codigo) {
            case NUEVO:
                crearSwap(solicitud->numeroTabla);
                break;
            case SUSPENDIDO:
                suspenderSwap(solicitud->numeroTabla, solicitud->listaEscribirSuspended);
                break;
            case FIN:
                eliminarSwap(solicitud->numeroTabla);
                break;
            case LEER_PAGINA_SWAP:
                leerPaginaSwap(solicitud->numeroTabla, solicitud->paginaLectura, solicitud->bufferSolicitud);
                break;
            case ESCRIBIR_PAGINA_SWAP:
                leerEscribirPaginaSwap(solicitud->numeroTabla, solicitud->paginaLectura, solicitud->bufferSolicitud,
                                       solicitud->paginaEscritura);
                break;
            default:
                log_info(logger, "Solicitud desconocida");
        }
        destruirSolicitudSwap(solicitud);
        usleep(retardoSwap * 1000);
        sem_post(&semEsperarSwap);
    }
}

void crearSolicitudSwap(codigoSwap codigo, uint32_t numeroTabla, t_list *listaSusp, void **buffer, uint32_t pagLectura, uint32_t pagEscritura) {
    log_info(logger, "Crear solicitud swap codigo %u", codigo);
    SolicitudSwap *solicitud = malloc(sizeof(SolicitudSwap));
    solicitud->codigo = codigo;
    solicitud->numeroTabla = numeroTabla;
    solicitud->listaEscribirSuspended = list_create();
    if (codigo == SUSPENDIDO) {
        list_destroy(solicitud->listaEscribirSuspended);
        solicitud->listaEscribirSuspended = listaSusp;
    }
    else if (codigo == LEER_PAGINA_SWAP) {
        solicitud->bufferSolicitud = buffer;
        solicitud->paginaLectura = pagLectura;
    }
    else if (codigo == ESCRIBIR_PAGINA_SWAP) {
        solicitud->bufferSolicitud = buffer;
        solicitud->paginaLectura = pagLectura;
        solicitud->paginaEscritura = pagEscritura;
    }
    list_add(listaSwap, solicitud);
    sem_post(&semSwap);
}

void crearSwap(uint32_t numeroTabla) {
    EstructuraProceso *proceso = list_get(tablaPaginasPrincipal, numeroTabla);
    uint32_t id = proceso->id;
    char *ruta = obtenerPath(id);
    proceso->archivo = open(ruta, O_CREAT | O_RDWR, S_IRUSR | S_IWUSR);
    if (proceso->archivo != -1) {
        ftruncate(proceso->archivo, proceso->size);
        log_info(logger, "Archivo swap del proceso %d fue creado correctamente", id);
    } else log_info(logger, "No se pudo crear el archivo swap del proceso %d", id);
    log_info(logger, "Archivo swap creado para pcb en %s", ruta);
    free(ruta);
}

void eliminarSwap(uint32_t numeroTabla) {
    EstructuraProceso *proceso = list_get(tablaPaginasPrincipal, numeroTabla);
    uint32_t id = proceso->id;
    char *ruta = obtenerPath(id);
    int resultadoClose = close(proceso->archivo);
    int resultadoRemove = remove(ruta);
    if (resultadoClose || resultadoRemove)
        log_info(logger, "No se pudo eliminar el archivo swap del proceso %d", id);
    else
        log_info(logger, "Archivo swap del proceso %d fue eliminado correctamente", id);
    free(ruta);
}

void suspenderSwap(uint32_t numeroTabla, t_list *escrituras) {
    EstructuraProceso *est = list_get(tablaPaginasPrincipal, numeroTabla);
    EscrituraSwap *escrituraSwap;
    int i;
    for (i = 0; i < list_size(escrituras); i++) {
        escrituraSwap = list_get(escrituras, i);
        escribirSwap(est->archivo, escrituraSwap->pagina, escrituraSwap->buffer);
    }
    log_info(logger, "Paginas del proceso %d guardadas en swap correctamente tras suspensión", est->id);
}

void leerPaginaSwap(uint32_t numeroTabla, uint32_t pagina, void **bufferSolicitud) {
    EstructuraProceso *est = list_get(tablaPaginasPrincipal, numeroTabla);
    free(*bufferSolicitud);
    *bufferSolicitud = leerSwap(est->archivo, pagina);
    log_info(logger, "Se leyo la pagina en swap");
}

void leerEscribirPaginaSwap(uint32_t numeroTabla, uint32_t paginaLectura, void **bufferSolicitud, uint32_t paginaEscritura) {
    EstructuraProceso *est = list_get(tablaPaginasPrincipal, numeroTabla);
    escribirSwap(est->archivo, paginaEscritura, *bufferSolicitud);
    free(*bufferSolicitud);
    log_info(logger, "Se escribio la pagina reemplazada en el swap");
    usleep(retardoSwap * 1000);
    *bufferSolicitud = leerSwap(est->archivo, paginaLectura);
    log_info(logger, "Se leyo la pagina nueva en el swap");
}

void *leerSwap(uint32_t archivo, uint32_t pagina) {
    void *buf = malloc(configMemoria->tamPagina);
    lseek(archivo, configMemoria->tamPagina * pagina, SEEK_SET);
    read(archivo, buf, configMemoria->tamPagina);
    return buf;
}

void escribirSwap(uint32_t archivo, uint32_t pagina, void *buffer) {
    lseek(archivo, configMemoria->tamPagina * pagina, SEEK_SET);
    write(archivo, buffer, configMemoria->tamPagina);
}

char *obtenerPath(uint32_t id) {
    char *ruta = string_new();
    char *numero = string_itoa(id);
    string_append(&ruta, pathSwap);
    string_append(&ruta, "/");
    string_append(&ruta, numero);
    string_append(&ruta, ".swap");
    free(numero);
    return ruta;
}

void iniciarCarpetaSwap() {
    mkdir(pathSwap, S_IRWXU);
}

void destruirSolicitudSwap(SolicitudSwap *solicitud) {
    list_destroy_and_destroy_elements(solicitud->listaEscribirSuspended, (void *) destruirEscrituraSwap);
    free(solicitud);
}

void destruirEscrituraSwap(EscrituraSwap *escritura) {
    free(escritura->buffer);
    free(escritura);
}
