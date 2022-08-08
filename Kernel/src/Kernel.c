/*
 ============================================================================
 Name        : Kernel.c
 Author      : scalonetaSO
 Description : Módulo Kernel
 ============================================================================
 */

#include "../include/Kernel.h"
//#include "KernelUtils.c" //Includes para Uri, sino no le compila. Comentar y descomentar según corresponda.
//#include "Planificador.c" //Includes para Uri, sino no le compila. Comentar y descomentar según corresponda.

int main(int argc, char** argv) {
    int server_fd; //fd = file descriptor

    logger = iniciar_logger(KERNEL);
    log_info(logger, "Inicio log");

    asignarArchivoConfig(argc, argv);
    config = leer_config(pathConfig);
    log_info(logger, "Leo config");

    setearConfiguracionesGenerales(config);
    server_fd = iniciar_servidor(ip, puerto);

    log_info(logger, "Kernel corriendo en %s:%s", ip, puerto);

    log_info(logger, "Kernel Server listo para recibir un proceso");

    inicializarEstados();
    iniciarSemaforos();

    planificadorCortoPlazo();
    planificadorLargoPlazo();
    iniciarHiloDeSuspensionYBloqueado();

    //crear conexion a CPU
    conexionMemoria = crear_conexion(ipMemoria, puertoMemoria);
    conexionCPUDispatch = crear_conexion(ipCPU, puertoCPUDispatch);
    conexionCPUInterrupt = crear_conexion(ipCPU, puertoCPUInterrupt);

    esperarConexion(server_fd);

    //destroys
    terminarPrograma(conexionMemoria, logger, config);
    liberar_conexion(conexionCPUDispatch);
    liberar_conexion(conexionCPUInterrupt);

    pthread_mutex_destroy(&mutex_cambio_est);
    pthread_mutex_destroy(&mutexPlanifMedianoPlazo);
    sem_destroy(&semPcbEjecutando);
    sem_destroy(&semPcbready);
    sem_destroy(&semPlanifLargoPlazo);
    sem_destroy(&semCantidadProcesosEsperandoDesbloqueo);
    sem_destroy(&contadorGradoMultiProgramacion);
    sem_destroy(&semCantidadProcesosEnSuspendedBlocked);
    sem_destroy(&semCantidadProcesosEsperandoDesbloqueo);
    sem_destroy(&semPermitidoParaNew);
    sem_destroy(&semCantidadProcesosEnSuspendedReady);
    list_destroy_and_destroy_elements(estadoReady, (void*) free);
    list_destroy_and_destroy_elements(estadoExec, (void*) free);
    list_destroy_and_destroy_elements(estadoNew, (void*) free);
    list_destroy_and_destroy_elements(estadoExit, (void*) free);
    list_destroy_and_destroy_elements(estadoBlockedSusp, (void*) free);
    list_destroy_and_destroy_elements(estadoBlockedIO, (void*) free);
    list_destroy_and_destroy_elements(estadoSuspendedReady, (void*) free);
    list_destroy_and_destroy_elements(listaPendientesDeDesbloqueo, (void*) free);

    return EXIT_SUCCESS;
}

void asignarArchivoConfig(int cantidadArgumentos, char** argumentos) {
    pathConfig = string_new();
    if (cantidadArgumentos > 1) {
        string_append(&pathConfig, "./config/");
        string_append(&pathConfig, argumentos[1]);
        string_append(&pathConfig, ".cfg");
    }
    else string_append(&pathConfig, "Kernel.cfg");
}

void esperarConexion(int server_fd) {
    while (1) {
        int socket_cliente = esperar_cliente(server_fd);
        pthread_create(&kernel, NULL, (void *) atenderKernel, (void *) socket_cliente);
        pthread_detach(kernel);
    }
}

/*void esperarConexionCPU() {
    pthread_t CPUDispatch;
    pthread_create(&CPUDispatch, NULL, (void *) atenderCPUDispatch, (void *) conexionCPUDispatch);
    pthread_detach(CPUDispatch);
}*/

void *atenderKernel(void *argument) {
    int socket = (int) argument;

    while (1) {
        int cod_op = recibir_operacion(socket);
        log_debug(logger, "Atender kernel: Código Operacion: %d", cod_op);

        switch (cod_op) {
            case PROCESO: {
                log_info(logger, "Me llego un proceso\n");
                Proceso *proceso = recibirProceso(socket);
                //printProceso(proceso);
                asignarPcb(proceso->instrucciones, proceso->tamProceso, socket);
                free(proceso);
                break;
            }
            case PCB_IO: {
                log_info(logger, "Recibo el pcb por una entrada/salida desde CPU");
                Pcb *pcb = recibirPcb(socket);
                logInfoPCB(pcb);

                int tiempoBloqueo = atoi(recibirUnPaqueteConUnMensaje(socket, true));
                log_info(logger, "Recibí el Tiempo de Bloqueo: %d", tiempoBloqueo);

                if (tipoPlanificador == SJF) {
                    time_t tiempoRecepcion = time(NULL);
                    double tiempoRafaga = difftime(tiempoRecepcion, tiempoEnvio) * 1000;
                    actualizarEstimacion(pcb, tiempoRafaga);
                }

                cambiarDeEstado(EXEC, BLOCKED, pcb);

                PcbBlockeo* pcbBlockeo = malloc(sizeof(PcbBlockeo));
                pcbBlockeo->pcb = pcb;
                pcbBlockeo->tiempoBloqueo = tiempoBloqueo;

                list_add(listaPendientesDeDesbloqueo, pcbBlockeo);
                log_info(logger, "Pongo en bloqueado al proceso %d", pcb->pid);
                pthread_create(&planificacionMedianoPlazo[pcb->pid], NULL, (void *) &planificadorMedianoPlazo, pcb);
                pthread_detach(planificacionMedianoPlazo[pcb->pid]);

                sem_post(&semCantidadProcesosEsperandoDesbloqueo);
                sem_post(&semPcbEjecutando);
                break;
            }
            case EXIT_PCB: {
                log_info(logger, "Pcb va a pasar a exit");
                Pcb *pcb = recibirPcb(socket);
                exitPcb(pcb);
                break;
            }
            case PCB_INTERRUPT: {
                Pcb *pcb = recibirPcb(socket);
                log_info(logger, "Desalojo pcb ejecutando: %u", pcb->pid);
                desalojarPcb(pcb);
                break;
            }
            /*case 0:
                break;
            */
            case -1: {
                log_debug(logger, "El cliente %u se desconecto. Terminando servidor", socket);
                return (void *) EXIT_FAILURE;
            }
            default: {
                log_warning(logger, "Operacion desconocida. Nada de hacer trampita");
                //abort();
            }
        }
    }

}

void inicializarEstados() {
    estadoNew = list_create();
    estadoReady = list_create();
    estadoBlockedIO = list_create();
    estadoBlockedSusp = list_create();
    estadoExec = list_create(); // recordar config: grado de multitarea (cuantos a la vez)
    estadoSuspendedReady = list_create();
    estadoExit = list_create();

    listaPendientesDeDesbloqueo = list_create();
}

void iniciarSemaforos() {
    sem_init(&semPcbready, 0, 0); // BINARIO - HAY ALGUN PCB EN READY? SI/NO
    // sem_init(&sem_pcb_exec, 0, 0); // BINARIO - HAY ALGUN PCB EN READY? SI/NO
    //sem_init(&sem_multitarea, 0, gradoMultitarea); // CONTADOR
    sem_init(&semPcbEjecutando, 0, 1);
    sem_init(&contadorGradoMultiProgramacion, 0, gradoMultiprogramacion);
    sem_init(&semCantidadProcesosEnSuspendedBlocked, 0 , 0);
    sem_init(&semCantidadProcesosEsperandoDesbloqueo, 0, 0);
    sem_init(&semPermitidoParaNew, 0,1);
    sem_init(&semCantidadProcesosEnSuspendedReady, 0 , 0);

    pthread_mutex_init(&mutexPlanifMedianoPlazo, NULL);

    pthread_mutex_init(&mutex_cambio_est,NULL); // MUTEX - PROTEGE LA OPERACION DE SACER PCB DE UN ESTADO Y PONERLOS EN OTRO
    sem_init(&semPlanifLargoPlazo, 0, 0);
}

void enviarDispatchCPU(op_code codigoOperacion, Pcb *pcb) {
    log_info(logger, "Envio pcb a cpu");
    enviarPCB(pcb, codigoOperacion, conexionCPUDispatch);
}

void enviarInterrupcionACPU(op_code codigoOperacion) {
    log_info(logger, "Envio interrupcion a cpu");
    enviarCodigoOperacion(conexionCPUInterrupt, codigoOperacion);
}

void enviarExitConsola(Pcb *pcb) {
    log_info(logger, "Envio signal de terminar consola");
    enviarMensaje("OK", atoi(pcb->conexionConsola), 0);
}

/*void* atenderCPUDispatch(void *argument) {
    int socket = (int) argument;

    while (1) {
        log_info(logger, "Atender CPU Dispatch");

        int cod_op = recibir_operacion(socket);

        switch (cod_op) {
            case EXIT_PCB: {
                log_info(logger, "Pcb va a pasar a exit:\n");
                Pcb *pcb = recibirPcb(socket);
                //todo abstraer en metodo de kernelUtils
                if (pcb->estado == EXEC) {
                    //eliminarDeEstado(estadoExec, pcb);
                    cambiarDeEstado(EXEC, (Estado) EXIT, pcb); //revisar si es necesario el estado exit o es al pedin (despues se acumularian en exit)
                    //sem avisando que salio de exec...
                    sem_post(&semPcbEjecutando);
                    enviarSolicitudAMemoria(MEMORY_EXIT);
                    enviarExitConsola(pcb);
                }
                case PCB_CPU: {
                    log_info(logger, "Recibi el pcb de vuelta desde CPU");
                    Pcb *pcb = recibirPcb(conexionCPUDispatch);
                    log_info(logger, "Me llego un PCB");
                    logInfoPCB(pcb);
                    int tiempoBloqueo = 0;
                    recv(conexionCPUDispatch, &tiempoBloqueo, sizeof(tiempoBloqueo), MSG_WAITALL);
                    log_info(logger, "Recibí el Tiempo de Bloqueo: %d", tiempoBloqueo);

                    //se recibe pcb de cpu, se tiene que pasar a ready y se envia post de no ejecutar mas
                    sem_post(&semPcbEjecutando);
                    break;
                }
                case 0:
                    break;
                case -1: {
                    log_error(logger, "El cliente %u se desconecto. Terminando servidor", socket);
                    return (void *) EXIT_FAILURE;
                }
                default: {
                    log_warning(logger, "Operacion desconocida. Nada de hacer trampita");
                    abort();
                }
            }
        }
    }
*/

// esto es para obtener si hubo un error, en caso que pueda servir: printf("read error ocurred:! %s\n", strerror(errno));
