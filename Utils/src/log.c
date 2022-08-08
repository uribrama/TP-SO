#include "../include/log.h"

t_log* iniciar_logger(int logger_code) {
	t_log* logger;
	char* modulo;

	switch(logger_code) {
		case CONSOLA:
			logger = log_create("Consola.log", "Consola", 1, LOG_LEVEL_INFO);
			modulo = "Consola";
			break;
		case KERNEL:
			logger = log_create("Kernel.log", "Kernel", 1, LOG_LEVEL_INFO);
			modulo = "Kernel";
			break;
		case CPU:
			logger = log_create("CPU.log", "CPU", 1, LOG_LEVEL_INFO);
			modulo = "CPU";
			break;
		case MEMORIA:
			logger = log_create("Memoria.log", "Memoria", 1, LOG_LEVEL_INFO);
			modulo = "Memoria";
			break;
		default:
			logger = NULL;
	}

	if(logger == NULL) {
		printf("No se pudo crear el log: %s", modulo);
		exit(EXIT_FAILURE);
	}

	return logger;
}
