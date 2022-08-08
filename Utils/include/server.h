#ifndef SERVER_H_
#define SERVER_H_

#include "../include/utils.h"

void* recibir_buffer(int*, int);
int iniciar_servidor(char*, char*);
int esperar_cliente(int);
void* atender_cliente(void*);
t_list* recibir_paquete(int);
void recibir_mensaje(int);
int recibir_operacion(int);

#endif /* SERVER_H_ */
