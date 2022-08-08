/*
 ============================================================================
 Name        : TLB.c
 Author      : scalonetaSO
 Description : Módulo CPU: MMU
 ============================================================================
 */

#include "../include/MMU.h"

uint32_t calcularNumeroPagina(uint32_t dir_logica, uint32_t tam_pag) {
	uint32_t num_pag = (uint32_t) floor(dir_logica / tam_pag);
	log_info(logger, "Num. pag: %d", num_pag);

	return num_pag;
}

uint32_t calcularDesplazamiento(uint32_t dir_logica, uint32_t num_pag, uint32_t tam_pag) {
	uint32_t desplazamiento = dir_logica - num_pag * tam_pag;
	log_info(logger, "Desplazamiento: %d", desplazamiento);

	return desplazamiento;
}

uint32_t calcularEntradaTabla(uint32_t num_pag, uint32_t cant_entradas_tabla, uint32_t nivel) {
	uint32_t entrada_tabla = 0;
	if (nivel == 1) {
		entrada_tabla = (uint32_t) floor(num_pag / cant_entradas_tabla);
		log_info(logger, "Entrada Tabla 1er Nivel: %d", entrada_tabla);
	} else if (nivel == 2) {
		entrada_tabla = num_pag % cant_entradas_tabla;
		log_info(logger, "Entrada Tabla 2do Nivel: %d", entrada_tabla);
	}
	else {
		log_info(logger, "ERROR! Tabla de nivel incorrecta");
	}

	return entrada_tabla;
}

uint32_t calcularDireccionFisica(uint32_t marco, uint32_t tam_pag, uint32_t desplazamiento) {
	uint32_t dir_fisica = marco * tam_pag + desplazamiento;
	log_info(logger, "Dirección Física: %d", dir_fisica);

	return dir_fisica;
}

