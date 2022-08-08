#ifndef CONSOLA_H_
#define CONSOLA_H_

#include <utils.h>

t_log* logger;
t_config* config;
char* pathConfig;

bool getConsoleParameters(int, char **, char**, long);
//void printInstructions(char** inst);

#endif /* CONSOLA_H_ */
