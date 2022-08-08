/*
 ============================================================================
 Name        : Consola.c
 Author      : scalonetaSO
 Description : Módulo Consola
 ============================================================================
 */

#include "../include/Consola.h"

int main(int argCount, char **argVector) {
    //ejecutarlo como ./Debug/Consola ./ruta/a/mi/archivo.txt
    char *path, *ip_kernel, *puerto_kernel, *instrucciones;
    int conexion;
    Proceso *proceso = malloc(sizeof(Proceso));

    //todo fixme
    if (!getConsoleParameters(argCount, argVector, &path, 2)) return EXIT_FAILURE;

    for (int i = 0; i < argCount; i++) {
        printf("Valor Arg[%d]: %s\n", i, argVector[i]);
    }

    // get arguments values
    proceso->tamProceso = atoi(argVector[2]);

    path = (char *) malloc(strlen(argVector[1]) * sizeof(char) + 1);

    if (path != NULL)
        strcpy(path, argVector[1]);
    else
        return EXIT_FAILURE;

    printf("path file instructions: %s \n", path);

    instrucciones = archivo_leer(path);
    free(path);

    //t_paquete* paquete;
    logger = iniciar_logger(CONSOLA);
    log_info(logger, "Inicio log");

    pathConfig = "./config/Consola.cfg";
    config = leer_config(pathConfig);
    log_info(logger, "Leo config");

    ip_kernel = config_get_string_value(config, "IP_KERNEL");
    puerto_kernel = config_get_string_value(config, "PUERTO_KERNEL");

    log_info(logger, "Kernel IP: %s, PUERTO: %s", ip_kernel, puerto_kernel);

    conexion = crear_conexion(ip_kernel, puerto_kernel);

    proceso->instrucciones = instrucciones;

    uint32_t tamPaquete = sizeof(proceso->tamProceso) + sizeof(uint32_t) + strlen(instrucciones) + 1;
    paquete(conexion, proceso, tamPaquete, PROCESO);
    free(proceso);
    free(instrucciones);

    log_info(logger, "Espero hasta el exit de Kernel pacientemente :)");
    char *buffer = recibirUnPaqueteConUnMensaje(conexion, true);
    if (strcmp(buffer, "OK") == 0) {
        log_info(logger, "Trabajo realizado, cerrando esta consola");
        terminarPrograma(conexion, logger, config);
    }

    return EXIT_SUCCESS;
}

bool getConsoleParameters(int argCount, char **arguments, char **path, long size) {
    if (argCount < 3)
        return false;

    // todo validate valid path
    path = &arguments[1];
    size = strtol(arguments[2], 0, 0);

    return true;
}

/*void printInstructions(char** listaInstrucciones) {
    for(int i = 0; i < string_array_size(listaInstrucciones); i++) {
        printf("Instrucción %d: %s\n", i, listaInstrucciones[i]);
    }
}*/
