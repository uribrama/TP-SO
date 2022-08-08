#ifndef TP_2022_1C_SCALONETASO_MEMORIAUTILS_H
#define TP_2022_1C_SCALONETASO_MEMORIAUTILS_H

#include <utils.h>
#include <fcntl.h>
#include <math.h> //@ema ojeli con esta lib si no te funka yo tuve que hacer que la compile adhiriendo al modulo la lib con nombre "m" segun lei en foros

t_log *logger;
t_config *config;
infoMemoria *configMemoria;

//Configuracion global
//char* ipKernel;
//char* puertoKernel;
char* ip;
char* puerto;

uint32_t entradasTabla;

uint32_t tamMemoria;
char *pathSwap;
uint32_t retardoSwap;
uint32_t retardoMemoria;
uint32_t framesPorProceso;

void *memory;

uint32_t cantidadFrames;
uint32_t* frameDePaginas;

uint32_t punteroClock[100]; // todo esta bien la cantidad ?

t_list* tablaPaginasPrincipal;
t_list* tablaPaginasNivel2;


t_list* listaSwap;
sem_t semSwap; //sem interno de sync swap
sem_t semEsperarSwap; //sem binario activa y desactiva con el inicio/fin de la action

typedef enum codigoSwap {
    NUEVO,
    SUSPENDIDO,
    FIN,
    LEER_PAGINA_SWAP,
    ESCRIBIR_PAGINA_SWAP
} codigoSwap;

typedef struct EstructuraProceso {
    uint32_t id;
    uint32_t archivo;
    uint32_t *tabla1;
    uint32_t size;
    uint32_t cantidadTablas2;
    uint32_t cantidadPaginas;
    uint32_t numeroTabla;
    t_list *listaFramePagina;
} EstructuraProceso;

typedef struct TablaNivel2 {
    uint32_t frame;
    uint32_t presencia;
    uint32_t uso;
    uint32_t modificado;
    uint32_t numeroTabla1;
} TablaNivel2;

typedef struct FramePagina {
    uint32_t frame;
    uint32_t pagina;
    TablaNivel2 *tablaNivel2;
} FramePagina;

typedef enum AlgoritmoReemplazo {
    CLOCK,
    CLOCKM
} AlgoritmoReemplazo;

AlgoritmoReemplazo algoritmoReemplazo;


uint32_t nuevoProceso(Pcb*);
void setearConfiguracionesGenerales(t_config *);
void crearTablas(EstructuraProceso*, Pcb*);
void resetearTablas2 (EstructuraProceso*);
void liberarFrames(EstructuraProceso *);
uint32_t obtenerPrimerFrameLibre();
void setEntradaTabla (TablaNivel2*, uint32_t, uint32_t, uint32_t, uint32_t);
void finProceso(uint32_t);
bool suspenderProceso(uint32_t numeroTabla);
t_list* crearDataParaEscribirEnSwap(EstructuraProceso*);
uint32_t tabla1ATabla2(uint32_t, uint32_t);
uint32_t tabla2AFrame(uint32_t, uint32_t);
void cargarPaginaEnMemoria(uint32_t, uint32_t, TablaNivel2 *);
uint32_t obtenerNumeroPagina (EstructuraProceso*, uint32_t, uint32_t);
uint32_t leer(uint32_t);
TablaNivel2* obtenerEntradaSegunDireccionFisica(int);
uint32_t escribir(uint32_t, uint32_t);


#endif //TP_2022_1C_SCALONETASO_MEMORIAUTILS_H
