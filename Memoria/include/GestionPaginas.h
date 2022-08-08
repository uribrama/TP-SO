#ifndef TP_2022_1C_SCALONETASO_GESTIONPAGINAS_H
#define TP_2022_1C_SCALONETASO_GESTIONPAGINAS_H

#include "Swap.h"

void cargarPaginaEnFrameNuevo(EstructuraProceso *, uint32_t, TablaNivel2*);
void cargarPaginaReemplazando(EstructuraProceso *, uint32_t, TablaNivel2 *);
FramePagina *obtenerFramePaginaVictima(EstructuraProceso *);

#endif //TP_2022_1C_SCALONETASO_GESTIONPAGINAS_H
