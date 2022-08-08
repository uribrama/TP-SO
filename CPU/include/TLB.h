#ifndef TLB_H_
#define TLB_H_

#include <utils.h>
#include <stdint.h>
#include <stdbool.h>

typedef enum algoritmo {
    FIFO,
	LRU
} algoritmo;

typedef struct t_tlb {
	uint32_t num_pag;
	uint32_t num_marco;
} t_tlb;

t_list *tlb;
algoritmo reemplazo_tlb;
extern uint32_t entradas_tlb;

bool hayEspacioEnTLB(void);
void agregarRegTLB(uint32_t, uint32_t);
void actualizarRegTLB(uint32_t, uint32_t);
void reemplazarRegTLB(uint32_t, uint32_t);
int buscarRegTLB(uint32_t);
void vaciarTLB(void);
void logTLB(void);

#endif /* TLB_H_ */
