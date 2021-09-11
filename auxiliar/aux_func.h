#include "aux_func.c"
#ifndef aux_func
#define aux_func

extern int leituraTerminal(char *buffer, int *nParametros);
extern char interpretaEntrada(char *buffer, int tamanhoEntrada, char *prog);
extern int verificaExecucaoBG(char *argv[], int nParametros);
extern void separaStrings(char *buffer, char *argv[], char *prog, int tamanhoEntrada, int nParametros);

#endif