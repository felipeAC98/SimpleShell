#include <stdio.h>    
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "auxiliar/aux_func.h"


void child_hand(int sigNum){
	pid_t pidFilho;
	int status;

	pidFilho=waitpid(-1, &status , 0);

	if(pidFilho!=-1){
		printf("o filho %d terminou com status %d \n",pidFilho, WEXITSTATUS(status));
	}

}

int main()
{ 
	pid_t shell_pgid;
	pid_t result;
	pid_t res;
	int shell_terminal = STDIN_FILENO;

	if(signal(SIGCHLD, child_hand)==SIG_ERR) {
	perror("Erro capturando tratamento do sinal");
	return(0);
	}

	/*if(signal(SIGTSTP, SIG_IGN)==SIG_ERR) {
	perror("Erro capturando tratamento do sinal");
	return(0);
	}*/

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

		// verifica se o programa irá rodar em foreground ou background
		int bg = verificaExecucaoBG(&argv,nParametros);

		if(bg != 0)
		{	
			argv[bg] = NULL;
			nParametros=bg;
		}

		int saidaArquivo = 0;
		for(int i = 1; i<nParametros; i++)
		{
			if(strcmp(argv[i],">") == 0)
			{
				saidaArquivo = i;
				argv[i] = NULL;
			}
		}

		//faz a separacao dos comandos e parametros recebidos
		//interpretaEntrada(&buffer, tamanhoEntrada,  &prog);

		// faz o fork do processo
		result=vfork();

		if(result==-1) {
		  perror("Erro em vfork");
		  exit(0);
		}

		if(!result) {  // filho

			if(saidaArquivo!=0)
			{	
				char fileName[50] = "";
				strcat(fileName, argv[saidaArquivo+1]);
				int output_fds = open(fileName, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
				argv[saidaArquivo+1] = NULL;
				res = dup2(output_fds, STDOUT_FILENO);
			}

			//passando para o filho o comando que deve ser executado
			result=execvp(prog,argv);

			perror("Erro em execl");
			exit(0);

		}else {  // pai

			// verificando se é para o programa rodar em background
			if(bg == 0)
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

