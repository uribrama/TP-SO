#include "../include/server.h"
//#include "../include/kernelUtils.h"

int iniciar_servidor(char *ip, char *puerto) {
    int socket_servidor;

    struct addrinfo hints, *servinfo, *p;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    getaddrinfo(ip, puerto, &hints, &servinfo);

    for (p = servinfo; p != NULL; p = p->ai_next) {
        if ((socket_servidor = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1)
            continue;

        uint32_t yes =1;
        setsockopt(socket_servidor,SOL_SOCKET,SO_REUSEPORT, &yes, sizeof(int));

        if (bind(socket_servidor, p->ai_addr, p->ai_addrlen) == -1) {
            close(socket_servidor);
            continue;
        }
        break;
    }

    if (listen(socket_servidor, SOMAXCONN) == -1) {
        close(socket_servidor);
        return EXIT_FAILURE;
    }

    freeaddrinfo(servinfo);

    log_trace(logger, "Listo para escuchar a mi cliente");

    return socket_servidor;
}

int esperar_cliente(int socket_servidor) {
    struct sockaddr_in dir_cliente;
    uint32_t tam_direccion = sizeof(struct sockaddr_in);

    int socket_cliente = accept(socket_servidor, (void *) &dir_cliente, &tam_direccion);

    //log_info(logger, "Se conecto un cliente!");
    log_info(logger, "Se conecto un cliente en un socket nuevo: %d", socket_cliente);

    return socket_cliente;
}

void *atender_cliente(void *argument) {
    int socket = (int) argument;

    while (1) {
        int cod_op = recibir_operacion(socket);

        switch (cod_op) {
            case MENSAJE:
                recibir_mensaje(socket);
                break;
            case PAQUETE:;
                t_list *lista;
                lista = recibir_paquete(socket);
                log_info(logger, "Me llegaron los siguientes valores:\n");
                list_iterate(lista, (void *) iterator);
                break;
            case -1:
                log_error(logger, "El cliente se desconecto. Terminando servidor");
                return (void *) EXIT_FAILURE;
            default:
                log_warning(logger, "Operacion desconocida. Nada de hacer trampita");
                //break;
                //return (void*) -1;
                abort();
        }
    }
}

int recibir_operacion(int socket_cliente) {
    int cod_op;
    if (recv(socket_cliente, &cod_op, sizeof(int), MSG_WAITALL) != 0)
        return cod_op;
    else {
        close(socket_cliente);
        return -1;
    }
}

void *recibir_buffer(int *size, int socket_cliente) {
    void *buffer;

    recv(socket_cliente, size, sizeof(int), MSG_WAITALL);
    buffer = malloc(*size);
    recv(socket_cliente, buffer, *size, MSG_WAITALL);

    return buffer;
}

void recibir_mensaje(int socket_cliente) {
    int size;
    char *buffer = recibir_buffer(&size, socket_cliente);
    log_info(logger, "Me llego el mensaje %s", buffer);
    free(buffer);
}

t_list *recibir_paquete(int socket_cliente) {
    int size;
    int desplazamiento = 0;
    void *buffer;
    t_list *valores = list_create();
    int tamanio;

    buffer = recibir_buffer(&size, socket_cliente);
    while (desplazamiento < size) {
        memcpy(&tamanio, buffer + desplazamiento, sizeof(int));
        desplazamiento += sizeof(int);
        char *valor = malloc(tamanio);
        memcpy(valor, buffer + desplazamiento, tamanio);
        desplazamiento += tamanio;
        list_add(valores, valor);
    }
    free(buffer);
    return valores;
}


