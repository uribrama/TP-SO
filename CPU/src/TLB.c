/*
 ============================================================================
 Name        : TLB.c
 Author      : scalonetaSO
 Description : Módulo CPU: TLB
 ============================================================================
 */

#include "../include/TLB.h"

bool hayEspacioEnTLB(void) {
	return (list_size(tlb) < entradas_tlb);
}

void agregarRegTLB(uint32_t pag, uint32_t marco) {
	t_tlb* nuevo_reg_tlb = malloc(sizeof(t_tlb));
	nuevo_reg_tlb->num_pag = pag;
	nuevo_reg_tlb->num_marco = marco;
	list_add_in_index(tlb, 0, nuevo_reg_tlb);
}

void actualizarRegTLB(uint32_t pag, uint32_t marco) {
	if (hayEspacioEnTLB())
		agregarRegTLB(pag, marco);
	else
		reemplazarRegTLB(pag, marco);
}

void reemplazarRegTLB(uint32_t pag, uint32_t marco) {
	char *algoritmo = string_new();
	t_tlb* nuevo_reg_tlb = malloc(sizeof(t_tlb));
	t_tlb* reg_a_eliminar;

	nuevo_reg_tlb->num_pag = pag;
	nuevo_reg_tlb->num_marco = marco;
	reg_a_eliminar = list_remove(tlb, (list_size(tlb)-1));
	list_add_in_index(tlb, 0, nuevo_reg_tlb);

	if (reemplazo_tlb == FIFO)
		string_append(&algoritmo, "FIFO");
	else //LRU
		string_append(&algoritmo, "LRU");

	log_info(logger, "Algoritmo %s: se reemplaza la pag|marco %d|%d por %d|%d",
			 algoritmo, reg_a_eliminar->num_pag, reg_a_eliminar->num_marco,
			 nuevo_reg_tlb->num_pag, nuevo_reg_tlb->num_marco);
	free(reg_a_eliminar);
}

int buscarRegTLB(uint32_t num_pag) {
	t_tlb* reg_tlb;
	for (int i = 0; i < tlb->elements_count; i++) {
		reg_tlb = list_get(tlb, i);
		if (reg_tlb->num_pag == num_pag) {
			if (reemplazo_tlb == LRU) {
				reg_tlb = list_remove(tlb, i);
				list_add_in_index(tlb, 0, reg_tlb);
			}
			log_info(logger, "TLB Hit! Pag: %d | Marco: %d",
					reg_tlb->num_pag, reg_tlb->num_marco);
			return reg_tlb->num_marco;
		}
	}
	log_info(logger, "TLB Miss!");
	return -1;
}

void vaciarTLB(void) {
    list_clean_and_destroy_elements(tlb, (void*)free);
    log_info(logger, "TLB vaciada!");
}

void logTLB(void) {
	int i = 0;
	log_info(logger, "Contenido TLB:");
	log_info(logger, "%11s | %10s | %10s", "Indice", "Pag", "Marco");
	while(i < list_size(tlb)) {
		t_tlb* elem = list_get(tlb, i);
		log_info(logger, " %10d | %10d | %10d", i, elem->num_pag, elem->num_marco);
		i++;
	}
}
