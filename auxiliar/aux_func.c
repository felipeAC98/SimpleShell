char interpretaEntrada(char *buffer, int tamanhoEntrada, char *prog){

	for(int i=0; i<tamanhoEntrada; i++){
		prog[i]=buffer[i];
	}

	//printf("tamanhoEntrada: %d \n",tamanhoEntrada);
	//printf("buffer: %s \n",buffer);

	return prog;
}

void separaStrings(char *buffer, char *argv[], char *prog, int tamanhoEntrada, int nParametros){

	int argumentoAtual=0;
	char delimitador[] = " ";
	char *parametro = strtok(buffer, delimitador);


	while(parametro != NULL)
	{

		if(argumentoAtual==0)
			strcpy(prog, parametro);
		//strcpy(argv[argumentoAtual], parametro);
		argv[argumentoAtual]=parametro;
		//printf("argv %s\n",argv[argumentoAtual]);

		argumentoAtual++;
		parametro = strtok(NULL, delimitador);
	}

	argv[argumentoAtual]=NULL;
	//printf("prog: %s\n",prog);
	//printf("parametro: %s\n",parametro);

}

int verificaExecucaoBG(char *argv[], int nParametros){

	//Procurando a primeira ocorrencia do simbolo & para retornar informando que a execucao em background sera necessaria
	for(int i=1; i<nParametros;i++){
		if(strcmp(argv[i],"&") == 0){
			return i;
		}
	}

	return 0;

}