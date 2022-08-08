#ifndef CPU_H_
#define CPU_H_

#include <pthread.h> //Hay que agregar en el GCC C Linker > Libraries, la library (-l) "pthread"
#include <utils.h> //Hay que agregar en el GCC C Linker > Libraries, la library (-l) "Utils"
#include <unistd.h>
#include "../include/TLB.h"
#include "../include/MMU.h"

t_log* logger;
t_config* config;

char* pathConfig;

//Configs Generales
uint32_t entradas_tlb;
uint32_t retardo_noop;
char *puerto_dispatch;
char *puerto_interrupt;
char *ip_memoria;
char *puerto_memoria;
char *ip_kernel;
char *puerto_kernel;

Instrucciones* instruccionesPCB;


uint32_t interrupcion = 0;
int conexionMemoria = 0;
int conexionKernel = 0;
char* ip;
infoMemoria *configMemoria;

pthread_mutex_t mutex_interrupcion;

void setearConfiguracionesGenerales(void);
void handshakeConMemoria(void);
infoMemoria* recibirConfigMemoria(int);
infoMemoria* deserializarConfigMemoria(t_buffer*);
void esperarConexionDispatch(int);
void* atenderDispatchKernel(void*);
void esperarConexionInterrupt(int);
void* atenderInterruptKernel(void*);
void cicloInstruccion(Pcb*, int);
uint32_t fetch(Pcb*);
Instruccion* decode(Instrucciones *, uint32_t);
uint32_t fetchOperands(uint32_t, uint32_t);
void execute(Pcb*, Instruccion*, uint32_t, int);
uint32_t checkInterrupt(Pcb*);
uint32_t traducirDireccion(uint32_t, uint32_t);
uint32_t obtenerValor(uint32_t);
uint32_t escribirValor(uint32_t, uint32_t);
void printInstructions(char**);
char** splitInstruction(char*);
Instruccion* getInstruccion(char**);
Instrucciones *parseInstructions(char **);
void parseAndStorePCBInstructions(Pcb*);
double obtenerTiempoEjecucion(void);
//void agregarRegTLB(t_list*, uint32_t, uint32_t);
//bool existePagEnTLB(t_list*, uint32_t);
//void logTLB(t_list*);
void destroyInstruction(Instruccion*);

//void enviarPCB(Pcb*, op_code, int);

#endif /* CPU_H_ */
