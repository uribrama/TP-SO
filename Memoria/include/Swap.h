#ifndef TP_2022_1C_SCALONETASO_SWAP_H
#define TP_2022_1C_SCALONETASO_SWAP_H

#include <utils.h>
#include <pthread.h>
#include <sys/stat.h>
#include "MemoriaUtils.h"

typedef struct {
    codigoSwap codigo;
    uint32_t numeroTabla;
    t_list* listaEscribirSuspended;
    uint32_t paginaLectura;
    void** bufferSolicitud;
    uint32_t paginaEscritura;
} SolicitudSwap;

typedef struct {
    uint32_t pagina;
    void *buffer;
} EscrituraSwap;

void procesarSolicitudesSwap();
void iniciarSwap();
char *obtenerPath(uint32_t id);
void crearSwap(uint32_t numeroTabla);
void eliminarSwap(uint32_t numeroTabla);
void suspenderSwap(uint32_t numeroTabla, t_list* escrituras);
void escribirSwap(uint32_t archivo, uint32_t pagina, void* buffer);
void leerPaginaSwap(uint32_t numeroTabla, uint32_t pagina, void** bufferSolicitud);
void leerEscribirPaginaSwap(uint32_t numeroTabla, uint32_t paginaLectura, void** bufferSolicitud, uint32_t paginaEscritura);
void* leerSwap(uint32_t archivo, uint32_t pagina);
void crearSolicitudSwap(codigoSwap codigo, uint32_t numeroTabla, t_list* listaSusp, void** buffer, uint32_t pagLectura, uint32_t pagEscritura);
void destruirSolicitudSwap(SolicitudSwap* solicitud);
void destruirEscrituraSwap(EscrituraSwap* escritura);
void iniciarCarpetaSwap();

pthread_t hiloSwap;

#endif //TP_2022_1C_SCALONETASO_SWAP_H
