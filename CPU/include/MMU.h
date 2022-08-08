#ifndef MMU_H_
#define MMU_H_

#include <utils.h>
#include <stdint.h>
#include <math.h> //Hay que agregar en el GCC C Linker > Libraries, la library (-l) "m"

uint32_t calcularNumeroPagina(uint32_t, uint32_t);
uint32_t calcularDesplazamiento(uint32_t, uint32_t, uint32_t);
uint32_t calcularEntradaTabla(uint32_t, uint32_t, uint32_t);
uint32_t calcularDireccionFisica(uint32_t, uint32_t, uint32_t);

#endif /* MMU_H_ */
