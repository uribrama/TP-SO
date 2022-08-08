#include "../include/utils.h"

void paquete(int conexion, void *contenidoPaquete, uint32_t tam, op_code codOperacion) {
    t_paquete *paquete = crear_paquete(codOperacion);
    agregar_a_paquete(paquete, contenidoPaquete, tam);
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);
}

void paqueteVacio(int conexion, op_code codOperacion) {
    t_paquete *paquete = crear_paquete(codOperacion);
    enviar_paquete(paquete, conexion);
    eliminar_paquete(paquete);
}

void *serializar_paquete(t_paquete *paquete, int bytes) {
    void *magic = malloc(bytes);
    int desplazamiento = 0;

    memcpy(magic + desplazamiento, &(paquete->codigo_operacion), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, &(paquete->buffer->size), sizeof(int));
    desplazamiento += sizeof(int);
    memcpy(magic + desplazamiento, paquete->buffer->stream, paquete->buffer->size);
    desplazamiento += paquete->buffer->size;

    return magic;
}

int crear_conexion(char *ip, char *puerto) {
    struct addrinfo hints;
    struct addrinfo *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &server_info);

    int socket_cliente = socket(server_info->ai_family, server_info->ai_socktype, server_info->ai_protocol);

    if (connect(socket_cliente, server_info->ai_addr, server_info->ai_addrlen) == -1)
        printf("error opening connection to %s %s", ip, puerto);

    freeaddrinfo(server_info);

    return socket_cliente;
}

void enviar_mensaje(char *mensaje, int socket_cliente) {
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = MENSAJE;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + 2 * sizeof(int);

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}


void crear_buffer(t_paquete *paquete) {
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = 0;
    paquete->buffer->stream = NULL;
}

t_paquete * crear_paquete(op_code codOperacion) { //todo podemos recibir un codigo de operacion y asignarlo en la linea de abajo
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->codigo_operacion = codOperacion;
    crear_buffer(paquete);
    return paquete;
}


/**
 *
 * @param socketConexion: int
 * @param conCodigoOperacion: true/false
 * @return char*
 */
char *recibirUnPaqueteConUnMensaje(int socketConexion, bool conCodigoOperacion) {
    if(conCodigoOperacion)
        recibir_operacion(socketConexion); // no se usa solo es para recibirlo y sacarlo del recv buffer
    int size = 0;
    return recibirBufferSimple(&size, socketConexion);
}

void *recibirBufferSimple(int *size, int socket_cliente) {
    void *buffer;

    recv(socket_cliente, size, sizeof(uint32_t), MSG_WAITALL); // el resultado se guarda en size
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);
    return buffer;
}

void agregar_a_paquete(t_paquete *paquete, void *contenido, uint32_t tamanio) {
    paquete->buffer->size = tamanio;
    //log_info(logger, "Buffer Size: %d", paquete->buffer->size);

    void *stream = malloc(paquete->buffer->size);
    int offset = 0; // Desplazamiento

    //todo dependiento el codigo de operacion puedo generalizar el tamaño de paquete segun su contenido
    if (paquete->codigo_operacion == PROCESO) {
        uint32_t tamMensaje = 0;
        Proceso *proceso = contenido;

        memcpy(stream + offset, &(proceso->tamProceso), sizeof(proceso->tamProceso));
        offset += sizeof(proceso->tamProceso);
        tamMensaje = strlen(proceso->instrucciones) + 1;

        memcpy(stream + offset, &tamMensaje, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy(stream + offset, proceso->instrucciones, tamMensaje);

        paquete->buffer->stream = stream;
    } else if (paquete->codigo_operacion == CONFIG_MEMORIA) {
        infoMemoria *configMemoria = contenido;
        //log_info(logger, "sizeof infoMemoria: %d", sizeof(infoMemoria));
        //log_info(logger, "sizeof tamPagina: %d", sizeof(configMemoria->tamPagina));
        //log_info(logger, "sizeof entradasTabla: %d", sizeof(configMemoria->entradasTabla));

        memcpy(stream + offset, &(configMemoria->tamPagina), sizeof(configMemoria->tamPagina));
        offset += sizeof(configMemoria->tamPagina);
        memcpy(stream + offset, &(configMemoria->entradasTabla), sizeof(configMemoria->entradasTabla));

        //log_info(logger, "tamPagina: %d", configMemoria->tamPagina);
        //log_info(logger, "entradasTabla: %d", configMemoria->entradasTabla);

        paquete->buffer->stream = stream;
    } else if (paquete->codigo_operacion == KERNEL_OP ||
               paquete->codigo_operacion == PCB_IO ||
               paquete->codigo_operacion == PCB_INTERRUPT ||
               paquete->codigo_operacion == EXIT_PCB ||
               paquete->codigo_operacion == MEMORY_INIT) {
        Pcb *pcb = contenido;
        log_debug(logger, "Agrego a Paquete con Código de Operación: %d", paquete->codigo_operacion);

        memcpy(stream + offset, &(pcb->estado), sizeof(Estado));
        offset += sizeof(Estado);

        memcpy(stream + offset, &(pcb->pid), sizeof(pcb->pid));
        offset += sizeof(pcb->pid);

        uint32_t tamInstrucciones = strlen(pcb->instrucciones) + 1;

        memcpy(stream + offset, &tamInstrucciones, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy(stream + offset, pcb->instrucciones, tamInstrucciones);
        offset += tamInstrucciones;

        memcpy(stream + offset, &(pcb->estimacionRafaga), sizeof(pcb->estimacionRafaga));
        offset += sizeof(pcb->estimacionRafaga);

        memcpy(stream + offset, &(pcb->tamanio), sizeof(pcb->tamanio));
        offset += sizeof(pcb->tamanio);

        memcpy(stream + offset, &(pcb->programCounter), sizeof(pcb->programCounter));
        offset += sizeof(pcb->programCounter);

        memcpy(stream + offset, &(pcb->tablaPaginas), sizeof(pcb->tablaPaginas));
        offset += sizeof(pcb->tablaPaginas);

        memcpy(stream + offset, &(pcb->conexionConsola), sizeof(pcb->conexionConsola));

        uint32_t tamConexionConsola = strlen(pcb->conexionConsola) + 1;

        memcpy(stream + offset, &tamConexionConsola, sizeof(uint32_t));
        offset += sizeof(uint32_t);

        memcpy(stream + offset, pcb->conexionConsola, tamConexionConsola);

        paquete->buffer->stream = stream;
    } else if (paquete->codigo_operacion == TRADUCCION_TABLA_NIVEL1 ||
    		   paquete->codigo_operacion == TRADUCCION_TABLA_NIVEL2) {
    	SolicitudTraduccion *solicitudTraduccion = contenido;

    	memcpy(stream + offset, &(solicitudTraduccion->nroTabla), sizeof(solicitudTraduccion->nroTabla));
    	offset += sizeof(solicitudTraduccion->nroTabla);
    	memcpy(stream + offset, &(solicitudTraduccion->entradaSolicitada), sizeof(solicitudTraduccion->entradaSolicitada));

    	paquete->buffer->stream = stream;
    } else if (paquete->codigo_operacion == ESCRIBIR_VALOR_MEMORIA) {
        EscribirValorMemoria *solicitudEscritura = contenido;

        memcpy(stream + offset, &(solicitudEscritura->dir_fisica), sizeof(solicitudEscritura->dir_fisica));
        offset += sizeof(solicitudEscritura->dir_fisica);
        memcpy(stream + offset, &(solicitudEscritura->valor), sizeof(solicitudEscritura->valor));

        paquete->buffer->stream = stream;
    }
    else {
        paquete->buffer->stream = realloc(paquete->buffer->stream, paquete->buffer->size + tamanio + sizeof(int));

        memcpy(paquete->buffer->stream + paquete->buffer->size, &tamanio, sizeof(int));
        memcpy(paquete->buffer->stream + paquete->buffer->size + sizeof(int), contenido, tamanio);

        paquete->buffer->size += tamanio + sizeof(int);
    }
}

void enviarMensaje(char *mensaje, int socket_cliente, op_code operacion) {
    t_paquete *paquete = malloc(sizeof(t_paquete));

    paquete->codigo_operacion = operacion;
    paquete->buffer = malloc(sizeof(t_buffer));
    paquete->buffer->size = strlen(mensaje) + 1;
    paquete->buffer->stream = malloc(paquete->buffer->size);
    memcpy(paquete->buffer->stream, mensaje, paquete->buffer->size);

    int bytes = paquete->buffer->size + sizeof(int) + sizeof(op_code); // tam del mensaje en si + tam del parametro "size" + tam del parametro de op_code

    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
    eliminar_paquete(paquete);
}

void enviar_paquete(t_paquete *paquete, int socket_cliente) {
    int bytes = paquete->buffer->size + 2 * sizeof(int);
    void *a_enviar = serializar_paquete(paquete, bytes);

    send(socket_cliente, a_enviar, bytes, 0);

    free(a_enviar);
}

void eliminar_paquete(t_paquete *paquete) {
    free(paquete->buffer->stream);
    free(paquete->buffer);
    free(paquete);
}

void liberar_conexion(int socket_cliente) {
    close(socket_cliente);
}

void iterator(char *value) {
    printf("%s\n", value);
}

//todo eliminar no lo vamos a usar en el tp
/*void leer_consola(t_log* logger, t_paquete* paquete) {
	char* leido;

	leido = readline(">");
	while(strncmp(leido, "", 1) != 0) {
		log_info(logger, "%s", leido);
		agregar_a_paquete(paquete, leido, strlen(leido) + 1);
		leido = readline(">");
	}

	free(leido);
}*/

FILE *abrirArchivo(char *path, char *mode) {
    return fopen(path, mode);
}

int cerrarArchivo(FILE *file) {
    return fclose(file);
}

//todo eliminar ya que estamos usando la función archivo_leer
/*
char* leerArchivo(FILE* file) {
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    while ((read = getline(&line, &len, file)) != -1) {
        printf("Retrieved line of length %zu:\n", read);
       printf("%s", line);
    }
    if (line)
        free(line);
}
*/

int archivo_obtenerTamanio(char *path) {
    FILE *f = abrirArchivo(path, "r");
    fseek(f, 0L, SEEK_END);
    int i = ftell(f);
    cerrarArchivo(f);
    return i;
    /*int fd = open(path, O_RDONLY, 0777);
    struct stat statbuf;
    fstat(fd, &statbuf);
    close(fd);
    return statbuf.st_size;*/
}

char *archivo_leer(char *path) {
    FILE *archivo = abrirArchivo(path, "r");

    int tamanio = archivo_obtenerTamanio(path);
    char *texto = malloc(tamanio + 1);

    fread(texto, tamanio, sizeof(char), archivo);

    cerrarArchivo(archivo);

    texto[tamanio] = '\0';

    return texto;
}

void terminarPrograma(int conexion, t_log *logger, t_config *config) {
    liberar_conexion(conexion);
    log_info(logger, "Programa terminado");
    log_destroy(logger);
    config_destroy(config);
}

int enviarSignal(int socketDestino, int header) {
	return send(socketDestino, &header, sizeof(int), MSG_NOSIGNAL);
}

//TODO: eliminar, no se usa
/*
int recibirSignal(int socketOrigen) {
    int header;
    int bytesRecibidos;
    if ((bytesRecibidos = recv(socketOrigen, &header, sizeof(int), MSG_NOSIGNAL)) <= 0) {
        return 0;
    } else {
        return header;
    }
}
*/

void enviarPCB(Pcb *pcb, op_code codOperacion, int socket) {
    uint32_t tamPaquete = sizeof(pcb->estado) + sizeof(pcb->pid) + sizeof(uint32_t) +
                          (strlen(pcb->instrucciones) + 1) +
                          sizeof(pcb->estimacionRafaga) +
                          sizeof(pcb->tamanio) + sizeof(pcb->programCounter) +
                          sizeof(pcb->tablaPaginas) + (strlen(pcb->conexionConsola) + 1) + sizeof(pcb->conexionConsola);
    paquete(socket, pcb, tamPaquete, codOperacion);
    log_info(logger, "Envié el PCB");
}

Pcb *recibirPcb(int socket_cliente) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);
    log_debug(logger, "Buffer Size: %d", paquete->buffer->size);

    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    Pcb *pcb = deserializarPcb(paquete->buffer);

    eliminar_paquete(paquete);
    return pcb;
}

Pcb *deserializarPcb(t_buffer *buffer) {
    Pcb *pcb = malloc(sizeof(Pcb));

    void *stream = buffer->stream;

    memcpy(&(pcb->estado), stream, sizeof(pcb->estado));
    stream += sizeof(pcb->estado);

    memcpy(&(pcb->pid), stream, sizeof(pcb->pid));
    stream += sizeof(pcb->pid);

    uint32_t tamanioInstrucciones = 0;

    memcpy(&tamanioInstrucciones, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    pcb->instrucciones = malloc(tamanioInstrucciones);

    memcpy(pcb->instrucciones, stream, tamanioInstrucciones);
    stream += tamanioInstrucciones;

    memcpy(&(pcb->estimacionRafaga), stream, sizeof(pcb->estimacionRafaga));
    stream += sizeof(double);

    memcpy(&(pcb->tamanio), stream, sizeof(pcb->tamanio));
    stream += sizeof(uint32_t);

    memcpy(&(pcb->programCounter), stream, sizeof(pcb->programCounter));
    stream += sizeof(uint32_t);

    memcpy(&(pcb->tablaPaginas), stream, sizeof(pcb->tablaPaginas));
    stream += sizeof(uint32_t);

    memcpy(&(pcb->conexionConsola), stream, sizeof(pcb->conexionConsola));

    uint32_t tamanioConexionConsola = 0;

    memcpy(&tamanioConexionConsola, stream, sizeof(uint32_t));
    stream += sizeof(uint32_t);

    pcb->conexionConsola = malloc(tamanioConexionConsola);

    memcpy(pcb->conexionConsola, stream, tamanioConexionConsola);
    //todo conexionConsola se estaba serializando/desezerialiando mal... lo tuve que convertir a char y ahora funka flama

    return pcb;
}

SolicitudTraduccion* deserializarSolicitudTraduccion(int socket_cliente) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    SolicitudTraduccion *solicitudTraduccion = recibirSolicitudTraduccion(paquete->buffer);

    eliminar_paquete(paquete);
    return solicitudTraduccion;
}

SolicitudTraduccion* recibirSolicitudTraduccion(t_buffer* buffer) {
    SolicitudTraduccion *solicitudTraduccion = malloc(sizeof(SolicitudTraduccion));

    void *stream = buffer->stream;

    memcpy(&(solicitudTraduccion->nroTabla), stream, sizeof(solicitudTraduccion->nroTabla));
    stream += sizeof(solicitudTraduccion->nroTabla);

    memcpy(&(solicitudTraduccion->entradaSolicitada), stream, sizeof(solicitudTraduccion->entradaSolicitada));

    return solicitudTraduccion;
}

EscribirValorMemoria* deserializarSolicitudEscribirMemoria(int socket_cliente) {
    t_paquete *paquete = malloc(sizeof(t_paquete));
    paquete->buffer = malloc(sizeof(t_buffer));

    recv(socket_cliente, &(paquete->buffer->size), sizeof(int), MSG_WAITALL);

    paquete->buffer->stream = malloc(paquete->buffer->size);
    recv(socket_cliente, paquete->buffer->stream, paquete->buffer->size, MSG_WAITALL);

    EscribirValorMemoria *solicitud = recibirSolicitudEscrituraMemoria(paquete->buffer);

    eliminar_paquete(paquete);
    return solicitud;
}

EscribirValorMemoria* recibirSolicitudEscrituraMemoria(t_buffer* buffer) {
    EscribirValorMemoria *solicitud = malloc(sizeof(EscribirValorMemoria));

    void *stream = buffer->stream;

    memcpy(&(solicitud->dir_fisica), stream, sizeof(solicitud->dir_fisica));
    stream += sizeof(solicitud->dir_fisica);

    memcpy(&(solicitud->valor), stream, sizeof(solicitud->valor));

    return solicitud;
}

void logInfoPCB(Pcb *pcb) {
    log_info(logger, "Estado: %d \n "
                     "PID: %d \n "
                     "Est. Ráfaga: %f \n "
                     "Tamaño: %d \n "
                     "Program Counter: %d \n "
                     "Tabla Paginas: %d \n "
                     "Conexión Consola: %s",
             pcb->estado, pcb->pid, pcb->estimacionRafaga, pcb->tamanio, pcb->programCounter, pcb->tablaPaginas,
             pcb->conexionConsola);
    //logInstruccionesPCB(pcb->instrucciones); ya las loguea cuando guarda las instrucciones tempooralmente
}

//Función mejorada de paqueteVacio
int enviarCodigoOperacion(int conexion, int codOperacion) {
    return send(conexion, &codOperacion, sizeof(codOperacion), MSG_NOSIGNAL);
}
