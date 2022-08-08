/*
 ============================================================================
 Name        : CPU.c
 Author      : scalonetaSO
 Description : Módulo CPU
 ============================================================================
 */

#include "../include/CPU.h"
//#include "TLB.c" //Include para Uri, sino no le compila. Comentar y descomentar según corresponda.
//#include "MMU.c" //Include para Uri, sino no le compila. Comentar y descomentar según corresponda.

int main(int argc, char** argv) {
    int server_dispatch = 0, server_interrupt = 0;
    tlb = list_create();
    pthread_mutex_init(&mutex_interrupcion, NULL);

    logger = iniciar_logger(CPU);
    log_info(logger, "Inicio log");

    asignarArchivoConfig(argc, argv);
    log_info(logger, "Leo config");

    setearConfiguracionesGenerales();

    //Crear conexion con Memoria
    conexionMemoria = crear_conexion(ip_memoria, puerto_memoria);
    log_info(logger, "Creo conexión con Memoria");
    handshakeConMemoria();

    server_dispatch = iniciar_servidor(ip, puerto_dispatch);
    log_info(logger, "CPU corriendo en %s:%s", ip, puerto_dispatch);
    log_info(logger, "CPU Server listo para recibir PCB");

    server_interrupt = iniciar_servidor(ip, puerto_interrupt);
    log_info(logger, "CPU corriendo en %s:%s", ip, puerto_interrupt);
    log_info(logger, "CPU Server listo para recibir interrupciones");

    esperarConexionDispatch(server_dispatch);
    esperarConexionInterrupt(server_interrupt);


    //destroys
    terminarPrograma(conexionMemoria, logger, config);
    free(configMemoria);
    liberar_conexion(conexionMemoria);
    liberar_conexion(conexionKernel);
    pthread_mutex_destroy(&mutex_interrupcion);

    return EXIT_SUCCESS;
}

void asignarArchivoConfig(int cantidadArgumentos, char** argumentos) {
    pathConfig = string_new();
    if (cantidadArgumentos > 1) {
        string_append(&pathConfig, "./config/");
        string_append(&pathConfig, argumentos[1]);
        string_append(&pathConfig, ".cfg");
    }
    else string_append(&pathConfig, "CPU.cfg");
}

void handshakeConMemoria(void) {
	if (enviarSignal(conexionMemoria, HANDSHAKE_CPU) == -1) {
		log_info(logger, "Error al intentar handshake con Memoria");
		terminarPrograma(conexionMemoria, logger, config);
		exit(EXIT_FAILURE);
	}
	else {
		configMemoria = recibirConfigMemoria(conexionMemoria);
		log_info(logger, "Recibi la configuración de la memoria.\n"
						 "Tamaño de páginas: %d\nEntradas por tabla: %d",
				 configMemoria->tamPagina, configMemoria->entradasTabla);
	}
}

void setearConfiguracionesGenerales(void) {
    config = leer_config(pathConfig);
	log_info(logger, "Leo config");

    if (config_has_property(config, "IP"))
        ip = config_get_string_value(config, "IP");
    else {
        log_error(logger, "Te falta setear la IP en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "RETARDO_NOOP"))
        retardo_noop = config_get_int_value(config, "RETARDO_NOOP");
    else {
        log_error(logger, "Te falta setear RETARDO_NOOP en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "PUERTO_ESCUCHA_DISPATCH")) {
        puerto_dispatch = config_get_string_value(config, "PUERTO_ESCUCHA_DISPATCH");
        log_info(logger, "PUERTO DISPATCH: %s", puerto_dispatch);
    } else {
        log_error(logger, "Te falta setear PUERTO_ESCUCHA_DISPATCH en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "PUERTO_ESCUCHA_INTERRUPT")) {
        puerto_interrupt = config_get_string_value(config, "PUERTO_ESCUCHA_INTERRUPT");
        log_info(logger, "PUERTO INTERRUPT: %s", puerto_interrupt);
    } else {
        log_error(logger, "Te falta setear PUERTO_ESCUCHA_INTERRUPT en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "IP_MEMORIA"))
        ip_memoria = config_get_string_value(config, "IP_MEMORIA");
    else {
        log_error(logger, "Te falta setear IP_MEMORIA en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "PUERTO_MEMORIA"))
        puerto_memoria = config_get_string_value(config, "PUERTO_MEMORIA");
    else {
        log_error(logger, "Te falta setear PUERTO_MEMORIA en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "IP_KERNEL"))
        ip_kernel = config_get_string_value(config, "IP_KERNEL");
    else {
        log_error(logger, "Te falta setear IP_KERNEL en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "PUERTO_KERNEL"))
        puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");
    else {
        log_error(logger, "Te falta setear PUERTO_KERNEL en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "ENTRADAS_TLB"))
		entradas_tlb = config_get_int_value(config, "ENTRADAS_TLB");
    else {
        log_error(logger, "Te falta setear ENTRADAS_TLB en tu config correctamente");
        exit(EXIT_FAILURE);
    }

    if (config_has_property(config, "REEMPLAZO_TLB")) {
    	char *algoritmo_reemplazo_tlb = config_get_string_value(config, "REEMPLAZO_TLB");

		if (strcmp(algoritmo_reemplazo_tlb, "FIFO") == 0)
			reemplazo_tlb = FIFO;
		else if (strcmp(algoritmo_reemplazo_tlb, "LRU") == 0)
			reemplazo_tlb = LRU;
		else {
			log_error(logger, "Algoritmo de reemplazo para la TLB incorrecto");
			exit(EXIT_FAILURE);
		}
    }
    else {
    	log_error(logger, "Te falta setear REEMPLAZO_TLB en tu config correctamente");
    	exit(EXIT_FAILURE);
    }

    log_info(logger, "Memoria IP: %s, PUERTO: %s", ip_memoria, puerto_memoria);
}

//move a un cpu utils
infoMemoria *recibirConfigMemoria(int socket_cliente) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    if (recibir_operacion(socket_cliente) == CONFIG_MEMORIA) {
        recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
        log_info(logger, "Buffer Size: %d", paquete->buffer->size);

        paquete->buffer->stream = malloc(paquete->buffer->size);
        recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

        infoMemoria *configMem = deserializarConfigMemoria(paquete->buffer);

        //log_info(logger, "Recibi la configuración de la memoria. Tamaño de páginas: %d\nEntradas por tabla: %d", configMem->tamPagina, configMem->entradasTabla);

        eliminar_paquete(paquete);
        return configMem;
    } else {
        log_warning(logger, "No se pudo recibir la configuración de la memoria. Finalizo el programa.");
        exit(EXIT_FAILURE);
    }
}

infoMemoria *deserializarConfigMemoria(t_buffer *buffer) {
    infoMemoria *configMemoria = malloc(sizeof(infoMemoria));

    void *stream = buffer->stream;

    memcpy(&(configMemoria->tamPagina), stream, sizeof(configMemoria->tamPagina));
    stream += sizeof(configMemoria->tamPagina);
    memcpy(&(configMemoria->entradasTabla), stream, sizeof(configMemoria->entradasTabla));

    log_info(logger, "Ya deserialice la configuración de la memoria");
    log_info(logger, "Config Memoria - tamPagina = %d", configMemoria->tamPagina);
    log_info(logger, "Config Memoria - entradasTabla = %d", configMemoria->entradasTabla);
    return configMemoria;
}

void esperarConexionDispatch(int server_dispatch) {
    int socket_cliente = esperar_cliente(server_dispatch);
    pthread_t kernel_dispatch;
    log_info(logger, "Creo thread para atender el puerto Dispatch");
    pthread_create(&kernel_dispatch, NULL, (void *) atenderDispatchKernel, (void *) socket_cliente);
    pthread_detach(kernel_dispatch);
}

void *atenderDispatchKernel(void *argument) {
    int socket = (int) argument;
    int pidAnterior = -1;
    while (1) {
        int cod_op = recibir_operacion(socket);
        log_debug(logger, "Atender kernel Dispatch: Código Operación: %d", cod_op);

        switch (cod_op) {
            case KERNEL_OP: {
                Pcb *pcb = recibirPcb(socket);
                log_info(logger, "Me llego un PCB");
                logInfoPCB(pcb);

                //obtengo las instrucciones del pcb y las guardo temporalmente
                parseAndStorePCBInstructions(pcb);

                if (pidAnterior != pcb->pid) {
                	pidAnterior = pcb->pid;
                	vaciarTLB();
                }

                cicloInstruccion(pcb, socket);
                break;
            }
            case -1: {
                log_error(logger, "El cliente %d se desconecto. Terminando servidor", socket);
                return (void *) EXIT_FAILURE;
            }
            default: {
                log_warning(logger, "Operacion desconocida. Nada de hacer trampita");
                exit(EXIT_FAILURE);
            }
        }
    }
}

void esperarConexionInterrupt(int server_interrupt) {
    int socket_cliente = esperar_cliente(server_interrupt);
    pthread_t kernel_interrupt;
    log_info(logger, "Creo thread para atender el puerto interrupt");
    pthread_create(&kernel_interrupt, NULL, (void *) atenderInterruptKernel, (void *) socket_cliente);
    pthread_join(kernel_interrupt, NULL);
}

void *atenderInterruptKernel(void *argument) {
    int socket = (int) argument;

    while (1) {
        int cod_op = recibir_operacion(socket);
        log_info(logger, "Atender interrupcion kernel: Código Operacion: %d", cod_op);

        switch (cod_op) {
            case CPU_INTERRUPT:
            	log_info(logger, "Me llego una interrupción");
            	pthread_mutex_lock(&mutex_interrupcion);
            	interrupcion = 1;
            	pthread_mutex_unlock(&mutex_interrupcion);
                break;
            case -1:
                log_error(logger, "El cliente %d se desconecto. Terminando servidor", socket);
                return (void *) EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida. Nada de hacer trampita");
                exit(EXIT_FAILURE);
        }
    }
}

void cicloInstruccion(Pcb *pcb, int socket) {
    uint32_t programCounter;
    uint32_t valor = (uint32_t) NULL;
    Instruccion *instruccionActual;

    do {
        programCounter = fetch(pcb);

        instruccionActual = decode(instruccionesPCB, programCounter);

        if (instruccionActual->tipo == COPY)
            valor = fetchOperands(pcb->tablaPaginas, instruccionActual->arguments[1]);

        execute(pcb, instruccionActual, valor, socket);
    } while (!checkInterrupt(pcb) && !(instruccionActual->tipo == IO || instruccionActual->tipo == EXIT));

    list_destroy_and_destroy_elements(instruccionesPCB->instruccion, (void*) destroyInstruction);
}

uint32_t fetch(Pcb *pcb) {
    log_info(logger, "Ciclo de Instrucción - Fetch: PC = %d", pcb->programCounter);

	return pcb->programCounter;
}

Instruccion *decode(Instrucciones* instrucciones, uint32_t programCounter) {
    Instruccion *proximaInstruccion = list_get(instrucciones->instruccion, programCounter);

    log_info(logger, "Ciclo de Instrucción - Decode: Instrucción = %d | Param 1: %d | "
                     "Param 2: %d", proximaInstruccion->tipo,
					 proximaInstruccion->arguments[0],
					 proximaInstruccion->arguments[1]);

    return proximaInstruccion;
}

uint32_t fetchOperands(uint32_t tablaPaginas, uint32_t dir_logica) {
    uint32_t dir_fisica = 0;
    //Obtener el valor en memoria del argumento 2 que es la dirección lógica
    dir_fisica = traducirDireccion(tablaPaginas, dir_logica);
    log_info(logger, "Ciclo de Instrucción - Fetch Operands");

    return obtenerValor(dir_fisica);
}

void execute(Pcb *pcb, Instruccion *instruccion, uint32_t valor, int socket) {
    switch (instruccion->tipo) {
        case NO_OP: {
            log_info(logger, "Ciclo de Instrucción - Execute: NO_OP | RETARDO_NOOP: %d", retardo_noop);
            usleep(retardo_noop * 1000); //multiplico por 1000 para convertirlo a milisegundos
            pcb->programCounter++;
            break;
        }
        case IO: {
            log_info(logger, "Ciclo de Instrucción - Execute: I/O");
            //Devolver PCB actualizado y el tiempo de bloqueo en milisegundos
            int tiempoBloqueo = instruccion->arguments[0];
            log_info(logger, "Ciclo de Instrucción - Execute: I/O | Tiempo Bloqueo: %d", tiempoBloqueo);
            pcb->programCounter++;
            conexionKernel = crear_conexion(ip_kernel, puerto_kernel);
            enviarPCB(pcb, PCB_IO, conexionKernel);
            //TODO: una vez que funcionen los cambios, eliminar esto
            //send(conexionKernel, &tiempoBloqueo, sizeof(tiempoBloqueo), (int) NULL);
            enviarMensaje(string_itoa(tiempoBloqueo), conexionKernel, TIEMPO_BLOQUEO_IO);
            close(conexionKernel);
            break;
        }
        case READ: {
            log_info(logger, "Ciclo de Instrucción - Execute: READ");
            //Leer el valor en memoria de la dirección lógica indicada en el argumento 1
            uint32_t dir_fisica = 0;
            dir_fisica = traducirDireccion(pcb->tablaPaginas, instruccion->arguments[0]);
            valor = obtenerValor(dir_fisica);
            log_info(logger, "Valor leído: %d", valor);
            pcb->programCounter++;
            break;
        }
        case COPY: {
            log_info(logger, "Ciclo de Instrucción - Execute: COPY");
            //Copiar el valor (obtenido en la instrucción fetchOperands) en memoria en la dirección
            //lógica indicada en el argumento 1
            uint32_t dir_fisica = traducirDireccion(pcb->tablaPaginas, instruccion->arguments[0]);

            if (escribirValor(dir_fisica, valor) == 0)
                log_info(logger, "Se escribió el valor: %d correctamente en la dirección física: %d",
                		 valor, dir_fisica);
            else
                log_info(logger, "Hubo un error al escribir el valor: %d en la dirección física: %d",
                		 valor, dir_fisica);

            pcb->programCounter++;
            break;
        }
        case WRITE: {
            log_info(logger, "Ciclo de Instrucción - Execute: WRITE");
            //Escribir el valor del argumento 2 en memoria en la dirección lógica indicada en el argumento 1
            uint32_t dir_fisica = traducirDireccion(pcb->tablaPaginas, instruccion->arguments[0]);

            if (escribirValor(dir_fisica, instruccion->arguments[1]) == 0)
                log_info(logger, "Se escribió el valor: %d correctamente en la dirección física: %d",
                		 instruccion->arguments[1], dir_fisica);
            else
                log_info(logger, "Hubo un error al escribir el valor: %d en la dirección física: %d",
                		 instruccion->arguments[1], dir_fisica);

            pcb->programCounter++;
            break;
        }
        case EXIT: {
            log_info(logger, "Ciclo de Instrucción - Execute: EXIT");
            //Devolver el PCB actualizado al Kernel con el código de operación EXIT_PCB
            conexionKernel = crear_conexion(ip_kernel, puerto_kernel);
            enviarPCB(pcb, EXIT_PCB, conexionKernel);
            close(conexionKernel);
            break;
        }
        default: {
            log_info(logger, "ERROR! Instrucción desconocida!");
            break;
        }
    }
}

uint32_t checkInterrupt(Pcb* pcb) {
	uint32_t val = 0;

    pthread_mutex_lock(&mutex_interrupcion);
    val = interrupcion;
    pthread_mutex_unlock(&mutex_interrupcion);

    if (val) {
        log_info(logger, "Interrupcion: enviando PCB a Kernel");
        conexionKernel = crear_conexion(ip_kernel, puerto_kernel);
        enviarPCB(pcb, PCB_INTERRUPT, conexionKernel);
        pthread_mutex_lock(&mutex_interrupcion);
        interrupcion = 0;
        pthread_mutex_unlock(&mutex_interrupcion);
        close(conexionKernel);
    }

    return val;
}

//hacer el enviar codigo y despues los send esta matando a memoria porque le llega todo mal
//una forma seria probar enviando en el "flags" no signal, que significa que va a enviar mas y no termino...
uint32_t traducirDireccion(uint32_t tabla_pagina_1er_nivel, uint32_t dir_logica) {
	int marco = 0;
    uint32_t num_pag = calcularNumeroPagina(dir_logica, configMemoria->tamPagina);
    uint32_t desplazamiento = calcularDesplazamiento(dir_logica, num_pag, configMemoria->tamPagina);

    if ((marco = buscarRegTLB(num_pag)) == -1) {
    	//TLB Miss!
		uint32_t entrada_tabla_1er_nivel = calcularEntradaTabla(num_pag, configMemoria->entradasTabla, 1);
		uint32_t entrada_tabla_2do_nivel = calcularEntradaTabla(num_pag, configMemoria->entradasTabla, 2);

		//Envío a memoria el número de tabla de 1er nivel y el índice de la tabla para obtener
		//el número de tabla de 2do nivel
		/*TODO: una vez que funcionen los cambios, eliminar esto
		enviarCodigoOperacion(conexionMemoria, TRADUCCION_DIRECCION);
		//send(conexionMemoria, &tabla_pagina_1er_nivel, sizeof(tabla_pagina_1er_nivel), (int) NULL);
		//send(conexionMemoria, &entrada_tabla_1er_nivel, sizeof(entrada_tabla_1er_nivel), (int) NULL);
		//Probar si funciona con estas flags
		send(conexionMemoria, &tabla_pagina_1er_nivel, sizeof(tabla_pagina_1er_nivel), MSG_MORE);
		send(conexionMemoria, &entrada_tabla_1er_nivel, sizeof(entrada_tabla_1er_nivel), MSG_EOR);
		*/
		SolicitudTraduccion *solicitudTraduccion = malloc(sizeof(SolicitudTraduccion));
		solicitudTraduccion->nroTabla = tabla_pagina_1er_nivel;
		solicitudTraduccion->entradaSolicitada = entrada_tabla_1er_nivel;
		uint32_t tamPaquete = sizeof(solicitudTraduccion->nroTabla) + sizeof(solicitudTraduccion->entradaSolicitada);
		paquete(conexionMemoria, solicitudTraduccion, tamPaquete, TRADUCCION_TABLA_NIVEL1);

		/*TODO: una vez que funcionen los cambios, eliminar esto
		recv(conexionMemoria, &tabla_pagina_2do_nivel, sizeof(tabla_pagina_2do_nivel), MSG_WAITALL);
		log_info(logger, "Tabla Página 2do Nivel: %d", tabla_pagina_2do_nivel);
		*/
		uint32_t tabla_pagina_2do_nivel = (uint32_t) atoi(recibirUnPaqueteConUnMensaje(conexionMemoria, true));

		//Envío a memoria el índice de la tabla para obtener el marco de la memoria
		/*TODO: una vez que funcionen los cambios, eliminar esto
		//No envío el número de tabla de 2do nivel porque la memoria ya lo tiene del paso anterior. NO APLICA MÁS ESTO PQ MANDO UN STRUCT
		//send(conexionMemoria, &tabla_pagina_2do_nivel, sizeof(tabla_pagina_2do_nivel), (int) NULL);
		send(conexionMemoria, &entrada_tabla_2do_nivel, sizeof(entrada_tabla_2do_nivel), MSG_EOR);
		*/
		solicitudTraduccion->nroTabla = tabla_pagina_2do_nivel;
		solicitudTraduccion->entradaSolicitada = entrada_tabla_2do_nivel;
		paquete(conexionMemoria, solicitudTraduccion, tamPaquete, TRADUCCION_TABLA_NIVEL2);

		/*TODO: una vez que funcionen los cambios, eliminar esto
		recv(conexionMemoria, &marco, sizeof(marco), MSG_WAITALL);
		*/
		marco = atoi(recibirUnPaqueteConUnMensaje(conexionMemoria, true));
		actualizarRegTLB(num_pag, (uint32_t) marco);
		free(solicitudTraduccion);
    }

    log_info(logger, "Marco: %d", marco);
    logTLB();

    //Devuelvo la dirección física
    return calcularDireccionFisica((uint32_t) marco, configMemoria->tamPagina, desplazamiento);
}

uint32_t obtenerValor(uint32_t dir_fisica) {
	/*TODO: una vez que funcionen los cambios, eliminar esto
    enviarCodigoOperacion(conexionMemoria, LEER_VALOR_MEMORIA);
    send(conexionMemoria, &dir_fisica, sizeof(dir_fisica), (int) NULL);
    */
	enviarMensaje(string_itoa(dir_fisica), conexionMemoria, LEER_VALOR_MEMORIA);
	/*TODO: una vez que funcionen los cambios, eliminar esto
    //leer del void* de la memoria el valor que hay en la dir_fisica
    uint32_t valor = 0;
    recv(conexionMemoria, &valor, sizeof(valor), MSG_WAITALL);
    return valor;
    */
    return (uint32_t) atoi(recibirUnPaqueteConUnMensaje(conexionMemoria, true));
}

uint32_t escribirValor(uint32_t dir_fisica, uint32_t valor) {
	/*TODO: una vez que funcionen los cambios, eliminar esto
    enviarCodigoOperacion(conexionMemoria, ESCRIBIR_VALOR_MEMORIA);
	//Le envío la dirección física a donde tiene que escribir el valor
	send(conexionMemoria, &dir_fisica, sizeof(dir_fisica), (int) NULL);
	//Le envío el valor a escribir
	send(conexionMemoria, &valor, sizeof(valor), (int) NULL);
	*/
	EscribirValorMemoria *escribirValorMemoria = malloc(sizeof(EscribirValorMemoria));
	escribirValorMemoria->dir_fisica = dir_fisica;
	escribirValorMemoria->valor = valor;
	uint32_t tamPaquete = sizeof(escribirValorMemoria->dir_fisica) + sizeof(escribirValorMemoria->valor);
	paquete(conexionMemoria, escribirValorMemoria, tamPaquete, ESCRIBIR_VALOR_MEMORIA);
	free(escribirValorMemoria);

	/*TODO: una vez que funcionen los cambios, eliminar esto
	//Recibo el resultado de la escritura. OK = 0, ERROR = -1
	uint32_t result = 0;
	recv(conexionMemoria, &result, sizeof(result), MSG_WAITALL);
	return result;
	*/
	return (uint32_t) atoi(recibirUnPaqueteConUnMensaje(conexionMemoria, true));
}

// --------------------- parsing instructions --------------------

void parseAndStorePCBInstructions(Pcb* pcb) {
    char **listInstrucciones = string_split(pcb->instrucciones, "\n");
    printInstructions(listInstrucciones);
    instruccionesPCB = parseInstructions(listInstrucciones);
    free(listInstrucciones);
}

Instrucciones *parseInstructions(char **listInstructions) {
    Instrucciones *instrucciones = malloc(sizeof(Instrucciones));
    instrucciones->instruccion = list_create();
    Instruccion *inst;

    if (!string_array_is_empty(listInstructions)) {
        for (int i = 0; i < string_array_size(listInstructions); i++) {
            inst = getInstruccion(splitInstruction(listInstructions[i]));
            if (inst != NULL) {
                if (inst->tipo == NO_OP) {
                    while (inst->arguments[0] > 0) {
                        Instruccion *instruccionAux = malloc(sizeof(Instruccion));
                        instruccionAux->tipo = NO_OP;
                        instruccionAux->arguments[0] = -1;
                        instruccionAux->arguments[1] = -1;
                        list_add(instrucciones->instruccion, instruccionAux);
                        inst->arguments[0]--;
                    }
                } else
                    list_add(instrucciones->instruccion, inst);
            }
        }
    }
    return instrucciones;
}

Instruccion *getInstruccion(char **splitedInstruction) {
    if (string_array_size(splitedInstruction) > 0) {
        char *tipo = splitedInstruction[0];

        char *arg1 = "-1";
        char *arg2 = "-1";
        if (string_array_size(splitedInstruction) == 2)
            arg1 = splitedInstruction[1];
        if (string_array_size(splitedInstruction) == 3) {
            arg1 = splitedInstruction[1];
            arg2 = splitedInstruction[2];
        }

        Instruccion instr;
        Instruccion *ptr_instr = malloc(sizeof(Instruccion));

        if (strcmp(tipo, "NO_OP") == 0)
            instr.tipo = NO_OP;
        else if (strcmp(tipo, "I/O") == 0)
            instr.tipo = IO;
        else if (strcmp(tipo, "WRITE") == 0)
            instr.tipo = WRITE;
        else if (strcmp(tipo, "COPY") == 0)
            instr.tipo = COPY;
        else if (strcmp(tipo, "READ") == 0)
            instr.tipo = READ;
        else if (strcmp(tipo, "EXIT") == 0)
            instr.tipo = EXIT;
        else
            return ptr_instr = NULL;

        if (arg1 || arg1 >= 0)
            ptr_instr->arguments[0] = atoi(arg1);
        else
            instr.arguments[0] = (int) NULL;
        if (arg2 || arg2 >= 0)
            ptr_instr->arguments[1] = atoi(arg2);
        else
            instr.arguments[1] = (int) NULL;

        ptr_instr->tipo = instr.tipo;

        return ptr_instr;
    }

    return NULL;
}

char **splitInstruction(char *leida) {
    return string_split(leida, " ");
}

void printInstructions(char **listaInstrucciones) {
    for (int i = 0; i < string_array_size(listaInstrucciones); i++) {
        printf("Instruccion %d: %s\n", i+1, listaInstrucciones[i]);
    }
}

void destroyInstruction(Instruccion* instruccion) {
    free(instruccion);
}
