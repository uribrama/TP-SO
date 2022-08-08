#ifndef UTILS_H_
#define UTILS_H_

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <signal.h>
#include <unistd.h>
#include <netdb.h>
#include <readline/readline.h>
#include <commons/log.h>
#include <commons/config.h>
#include <commons/string.h>
#include <commons/collections/list.h>
#include <string.h>
#include "server.h"
#include "config.h"
#include "log.h"
#include <semaphore.h>

typedef enum op_code {
    MENSAJE,
    PAQUETE,
    PROCESO,
    EXIT_PCB,
    KERNEL_OP,
    MEMORY_INIT,
    MEMORY_EXIT,
    MEMORY_SUSPEND,
    CPU_INTERRUPT,
    PCB_INTERRUPT,
    CPU_EXIT,
    PCB_IO,
    TIEMPO_BLOQUEO_IO,
    TRADUCCION_DIRECCION,
    LEER_VALOR_MEMORIA,
    ESCRIBIR_VALOR_MEMORIA,
    CONFIG_MEMORIA,
    HANDSHAKE_CPU,
    TRADUCCION_TABLA_NIVEL1,
    TRADUCCION_TABLA_NIVEL2
} op_code;

typedef struct t_buffer {
    int size;
    void *stream;
} t_buffer;

typedef struct {
    op_code codigo_operacion;
    t_buffer *buffer;
} t_paquete;

t_log *logger;

typedef enum {
    CONSOLA,
    KERNEL,
    CPU,
    MEMORIA
} modulo;

typedef enum Type {
    NO_OP,
    IO,
    READ,
    COPY,
    WRITE,
    EXIT
} Type;

typedef enum Estado {
    NEW,
    READY,
    EXEC,
    BLOCKED,
    BLOCKEDSUSP,
    SUSPENDEDREADY,
    QUIT
} Estado;

typedef struct Proceso {
    uint32_t tamProceso;
    char *instrucciones;
} Proceso;

typedef struct infoMemoria {
    int tamPagina;
    uint32_t entradasTabla;
} infoMemoria;

typedef struct SolicitudTraduccion {
    uint32_t nroTabla;
    uint32_t entradaSolicitada;
} SolicitudTraduccion;

typedef struct EscribirValorMemoria {
    uint32_t dir_fisica;
    uint32_t valor;
} EscribirValorMemoria;

//variables globales
t_list *estadoNew;
t_list *estadoReady;
t_list *estadoBlockedIO;
t_list *estadoBlockedSusp;
t_list *estadoExec;
t_list *estadoExit;
t_list *estadoSuspendedReady;

pthread_t kernel;
pthread_t cpu;
pthread_t memoria;
//

pthread_mutex_t mutex_cambio_est;

typedef struct Instruccion {
    Type tipo;
    uint32_t arguments[2];
} Instruccion;

typedef struct Instrucciones {
    t_list *instruccion;
} Instrucciones;

typedef struct Pcb {
    Estado estado;
    uint32_t pid;  //Identificador del proceso
    char *instrucciones;
    double estimacionRafaga; //planificar los procesos en el algoritmo SRT, (valor inicial definido por archivo de configuración)
    uint32_t tamanio;
    uint32_t programCounter; //número de la próxima instrucción a ejecutar.
    uint32_t tablaPaginas; // cuando el proceso pase a estado READY se carga
    char *conexionConsola;
} Pcb;

void asignarArchivoConfig(int, char**);

void paquete(int, void *, uint32_t, op_code);

void paqueteVacio(int, op_code);

int crear_conexion(char *, char *);

void enviar_mensaje(char *, int);

t_paquete *crear_paquete(op_code);

void agregar_a_paquete(t_paquete *, void *, uint32_t);

void enviar_paquete(t_paquete *, int);

void eliminar_paquete(t_paquete *);

void liberar_conexion(int);

void iterator(char *);

void leer_consola(t_log *, t_paquete *);

FILE *abrirArchivo(char *, char *);

int cerrarArchivo(FILE *file);

int archivo_obtenerTamanio(char *);

char *archivo_leer(char *);

void terminarPrograma(int, t_log *, t_config *);

void enviarMensaje(char *, int, op_code);

void *recibirBufferSimple(int *size, int);

char *recibirUnPaqueteConUnMensaje(int, bool);

void *serializar_paquete(t_paquete *, int);

int enviarSignal(int, int);

Pcb *recibirPcb(int);

Pcb *deserializarPcb(t_buffer *);

void logInfoPCB(Pcb *);

void logInstruccionesPCB(char *);

void enviarPCB(Pcb *, op_code, int);

int enviarCodigoOperacion(int, int);

SolicitudTraduccion* deserializarSolicitudTraduccion(int);

SolicitudTraduccion* recibirSolicitudTraduccion(t_buffer*);

EscribirValorMemoria* deserializarSolicitudEscribirMemoria(int);

EscribirValorMemoria* recibirSolicitudEscrituraMemoria(t_buffer*);

#endif /* UTILS_H_ */
