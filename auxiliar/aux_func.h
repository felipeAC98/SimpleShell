#include "aux_func.c"
#ifndef aux_func
#define aux_func

extern int leituraTerminal(char *buffer, int *nParametros);
extern char interpretaEntrada(char *buffer, int tamanhoEntrada, char *prog);
extern int verificaExecucaoBG(char *argv[], int nParametros);

#endif