#include <stdio.h>    
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>
#include<signal.h>
#include <string.h>

int leituraTerminal(char *buffer, int *nParametros);
char interpretaEntrada(char *buffer, int tamanhoEntrada, char *prog);

int main()
{ 
	pid_t shell_pgid;
	pid_t result;
	int shell_terminal = STDIN_FILENO;

	while(1){
		int nParametros=1;

		//Inserindo o shell em foreground - vai ficar preso aqui enquanto nao for fg
		while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ())){
			kill (- shell_pgid, SIGTTIN);
		}

		//Alocando um tamanho maximo para a entrada do terminal - precisa verificar sobre alocacao dinamica
		char buffer[20];
		int tamanhoEntrada = leituraTerminal(&buffer, &nParametros);
		
		//Criando variavel de argumentos
		char *argv[nParametros+1];

		//variavel na qual vai ser salvo o programa digitado, futuramente sera substituido por uma matriz de argumentos tipo argv
		char prog[tamanhoEntrada];

		separaStrings(&buffer, &argv, &prog, tamanhoEntrada, nParametros);


		//faz a separacao dos comandos e parametros recebidos
		//interpretaEntrada(&buffer, tamanhoEntrada,  &prog);

		// faz o fork do processo
		result=vfork();

		if(result==-1) {
		  perror("Erro em vfork");
		  exit(0);
		}

		if(!result) {  // filho
			//passando para o filho o comando que deve ser executado
			result=execlp(prog,prog,NULL);;

			perror("Erro em execl");
			exit(0);

		}else {  // pai

			//este wait faz com que o pai espere o filho terminar, logo o processo filho estara rodando em foreground
			waitpid(result, NULL, 0);
			//printf("Pai retomou a execucao.\n");
			printf("\n");
		}

	}
}

int leituraTerminal(char *buffer, int *nParametros){

	//alocando um buffer na memoria
	int c;
	int tamanhoEntrada=0;

	printf("$ ");

	while(1){
		c = getchar();

		//inserindo no buffer na posicao "tamanhoEntrada" o proximo caracter lido
		buffer[tamanhoEntrada] = c;

		if(c == EOF || c=='\n'){
			buffer[tamanhoEntrada] = '\0';
			return tamanhoEntrada;
		}
		else if(c == ' '){
			*nParametros=*nParametros+1;	
		}

		tamanhoEntrada++;

	}

}

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