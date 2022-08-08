#include "../include/config.h"

t_config* leer_config(char* path)
{
	t_config* config;

    config = config_create(path);

	if(config == NULL) {
		printf("No se pudo leer el archivo de configuración");
		exit(EXIT_FAILURE);
	}

	return config;
}
