#ifndef KERNEL_H_
#define KERNEL_H_

#include <pthread.h>
#include <utils.h>
#include <errno.h>
#include "Planificador.h" //Para que funcione en Eclipse, descomentar esto

t_log* logger;
t_config* config;
char* pathConfig;

int conexionMemoria;
int conexionCPUDispatch;
int conexionCPUInterrupt;

void esperarConexion(int);
//void esperarConexionCPU();
void* atenderKernel(void*);
//void* atenderCPUDispatch(void*);
void inicializarEstados();
void iniciarSemaforos();
void enviarDispatchCPU(op_code codigoOperacion, Pcb*);
void enviarInterrupcionACPU(op_code);

void enviarExitConsola(Pcb*);
#endif /* KERNEL_H_ */
