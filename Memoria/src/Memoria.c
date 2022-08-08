/*
 ============================================================================
 Name        : Memoria.c
 Author      : scalonetaSO
 Description : Módulo Memoria
 ============================================================================
 */

#include "../include/Memoria.h"
//#include "MemoriaUtils.c" //Include para Uri, sino no le compila. Comentar y descomentar según corresponda.

int main(int argc, char** argv) {
    int server_fd;

    logger = iniciar_logger(MEMORIA);
    log_info(logger, "Inicio log");

    asignarArchivoConfig(argc, argv);
    config = leer_config(pathConfig);
    log_info(logger, "Leo config");

    setearConfiguracionesGenerales(config);

    server_fd = iniciar_servidor(ip, puerto);
    log_info(logger, "Memoria corriendo en %s:%s", ip, puerto);

    inicializarMemoria();
    inicializarEstructuras();
    iniciarSwap();

    esperarConexion(server_fd);

    finalizarMemoria(server_fd);

    return EXIT_SUCCESS;
}

void asignarArchivoConfig(int cantidadArgumentos, char** argumentos) {
    pathConfig = string_new();
    if (cantidadArgumentos > 1) {
        string_append(&pathConfig, "./config/");
        string_append(&pathConfig, argumentos[1]);
        string_append(&pathConfig, ".cfg");
    }
    else string_append(&pathConfig, "Memoria.cfg");
}

void esperarConexion(int server_fd) {
    while (1) {
        int cliente_fd = esperar_cliente(server_fd);
        pthread_create(&memoria, NULL, (void *) atenderClienteMemoria, (void *) cliente_fd);
        pthread_detach(memoria);
    }
}

void *atenderClienteMemoria(void *socket_hilo) {
    int socket = (int) socket_hilo;

    while (1) {
        int cod_op = recibir_operacion(socket);
        log_info(logger, "Atender memoria: Código Operacion: %d", cod_op);

        switch (cod_op) {
            //////KERNEL CODES ////
            case MEMORY_INIT: {
                log_info(logger, "Me llego un proceso nuevo desde kernel");
                Pcb *pcb = recibirPcb(socket);
                uint32_t numeroTabla = nuevoProceso(pcb);
                enviarMensaje(string_itoa(numeroTabla), socket, 0);
                log_info(logger, "Numero de tabla enviado %u", numeroTabla );
                break;
            }

            case MEMORY_SUSPEND: {
                log_info(logger, "Me llego la suspensión de un proceso desde Kernel");
                char *numeroTabla = recibirUnPaqueteConUnMensaje(socket, false); //como ya obtuvo el codigoOperacion arriba, le mando false ya que no lo tiene mas
                suspenderProceso(atoi(numeroTabla));
                enviarMensaje("OK", socket, 0);
                log_info(logger,"Suspension de memoria terminada");
                break;
            }

            case MEMORY_EXIT: {
                log_info(logger,"Llega finalizacion de un proceso desde kernel");
                char* numeroTabla = recibirUnPaqueteConUnMensaje(socket, false); //como ya obtuvo el codigoOperacion arriba, le mando false ya que no lo tiene mas
                finProceso(atoi(numeroTabla));
                break;
            }
            ///////////////////////////////////

            case HANDSHAKE_CPU: {
                uint32_t tamPaquete = sizeof(configMemoria->tamPagina) + sizeof(configMemoria->entradasTabla);
                paquete(socket, configMemoria, tamPaquete, CONFIG_MEMORIA);
                break;
            }

            case TRADUCCION_TABLA_NIVEL1: {
                log_info(logger, "La CPU solicita un numero de tabla de 2do nivel");
                SolicitudTraduccion* solicitudTraduccion = deserializarSolicitudTraduccion(socket);
                uint32_t nroTabla2 = tabla1ATabla2(solicitudTraduccion->nroTabla, solicitudTraduccion->entradaSolicitada);
                usleep(retardoMemoria * 1000);

                //Responder numero de tabla 2
                enviarMensaje(string_itoa(nroTabla2), socket, 0);
                log_info(logger, "Enviando numero de tabla de 2do nivel al CPU %u", nroTabla2);
                break;
            }
            case TRADUCCION_TABLA_NIVEL2: {
                log_info(logger, "El CPU me solicita un numero de frame");
                SolicitudTraduccion* solicitudTraduccion = deserializarSolicitudTraduccion(socket);
                uint32_t frame = tabla2AFrame(solicitudTraduccion->nroTabla, solicitudTraduccion->entradaSolicitada);
                usleep(retardoMemoria * 1000);

                //Responder numero de frame
                enviarMensaje(string_itoa(frame), socket, 0);
                log_info(logger, "Enviando numero de frame al CPU %d", frame);
                break;
            }

            case LEER_VALOR_MEMORIA: {
                log_info(logger,"Llega solicitud de lectura de CPU");
                char* dirFisica = recibirUnPaqueteConUnMensaje(socket, false); //ya leyo el codigo de operacion previamente asi que le manda false para que no lo lea
                log_info(logger, "Recibí la dirección física: %s", dirFisica);

                uint32_t lectura = leer(atoi(dirFisica));
                usleep(retardoMemoria * 1000);

                enviarMensaje(string_itoa(lectura), socket, 0);
                log_info(logger,"Lectura enviada al CPU: %u" , lectura);
                break;
            }
            case ESCRIBIR_VALOR_MEMORIA: {
                log_info(logger,"Llega solicitud de escritura de CPU ");  // codigo,direccionFisica(uint32_t),valor(uint32_t)
                EscribirValorMemoria* escribirValorMemoria = deserializarSolicitudEscribirMemoria(socket);
                log_info(logger, "Recibí la dirección física: %u", escribirValorMemoria->dir_fisica);
                uint32_t resultado = escribir(escribirValorMemoria->dir_fisica, escribirValorMemoria->valor);

                //respondo con el resultado (0) de que salio todo ok
                usleep(retardoMemoria * 1000);
                enviarMensaje(string_itoa(resultado), socket, 0);
                log_info(logger,"Escritura realizada correctamente");
                break;
            }
            /*case 0:
                break;*/
            case -1: {
                log_debug(logger, "El cliente se desconecto. Terminando servidor");
                return (void *) EXIT_FAILURE;
            }
            default: {
                log_warning(logger, "Operacion desconocida. No quieras meter la pata");
                // exit(EXIT_FAILURE); todo no hace mas falta que tire un exit
            }
        }
    }
}

void inicializarMemoria() {
    memory = malloc(tamMemoria);

    cantidadFrames = (int) (tamMemoria / configMemoria->tamPagina);
    frameDePaginas = malloc(sizeof(int) * cantidadFrames);
    for (int i=0; i<cantidadFrames; i++) {
        frameDePaginas[i]=-1;
    }
}

void inicializarEstructuras() {
    listaSwap = list_create();
    tablaPaginasPrincipal = list_create();
    tablaPaginasNivel2 = list_create();
    sem_init(&semSwap,1,0);
    sem_init(&semEsperarSwap,1,0);
}

void finalizarMemoria(int server) {
    log_info(logger,"Finalizando el modulo memoria");

    terminarPrograma(server, logger, config);
    free(configMemoria);
    log_destroy(logger);

    free(memory);
    free(pathSwap);
    free(frameDePaginas);
    destruirTablas();
}

void destruirTablas() {
    list_destroy_and_destroy_elements(tablaPaginasPrincipal, (void *) destruirEstructuras);
    list_destroy_and_destroy_elements(tablaPaginasNivel2, (void *) destruirTabla2);
    list_destroy_and_destroy_elements(listaSwap, (void *) destruirSolicitudSwap);
}

void destruirEstructuras(EstructuraProceso *proceso) {
    list_destroy_and_destroy_elements(proceso->listaFramePagina, (void *) free);
    free(proceso->tabla1);
    free(proceso);
}

void destruirTabla2(t_list *tabla2) {
    list_destroy_and_destroy_elements(tabla2, (void *) free);
}
