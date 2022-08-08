#ifndef TP_2022_1C_SCALONETASO_KERNELUTILS_H
#define TP_2022_1C_SCALONETASO_KERNELUTILS_H

#include <utils.h>
#include "Kernel.h"

typedef enum PlanificadorCortoPlazo {
    FIFO,
    SJF
} PlanificadorCortoPlazo;

typedef struct PcbBlockeo {
    Pcb* pcb;
    uint32_t tiempoBloqueo;
} PcbBlockeo;

uint32_t generador_id;

// SEMAFOROS MUTEX
pthread_mutex_t mutex_cambio_est;
pthread_mutex_t mutexPlanifMedianoPlazo;
//

sem_t semPlanifLargoPlazo;
sem_t contadorGradoMultiProgramacion;
sem_t semCantidadProcesosEnSuspendedBlocked;
sem_t semCantidadProcesosEsperandoDesbloqueo;
sem_t semPermitidoParaNew;
sem_t semCantidadProcesosEnSuspendedReady;


t_list* listaPendientesDeDesbloqueo;

//Configuracion global (podria ser un struct que se comparte desde aca a todos los files, pero deberia inicializarse aca, si no mantener asi params)
char* ip;
int gradoMultiprogramacion;
PlanificadorCortoPlazo tipoPlanificador;
double estimacionInicialRafaga;
int tiempoMaxBloqueado;
char *ipMemoria;
char *puertoMemoria;
char *ipCPU;
char *puertoCPUDispatch;
char *puertoCPUInterrupt;
char *puerto;
double alfa;


void printProceso(struct Proceso*);

struct Proceso* recibirProceso(int);
Proceso* deserializarProceso(struct t_buffer*);

bool esElPcb(void *);

void asignarPcb(char*, uint32_t, int);
bool estaEnEstado(Pcb*, Estado);
t_list* obtenerListaSegunEstado(Estado);
void cambiarDeEstado(Estado estadoViejo, Estado estadoNuevo, Pcb*);
void eliminarDeEstado(t_list*, Pcb*);
void agregarAEstado(t_list*, Pcb*);
void actualizarEstadoDePcb(Pcb*, Estado);
void setearConfiguracionesGenerales(t_config*);
void exitPcb(Pcb*);
void desalojarPcb(Pcb*);

#endif //TP_2022_1C_SCALONETASO_KERNELUTILS_H
