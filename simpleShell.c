#include <stdio.h>    
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

int leituraTerminal(char *buffer, int *nParametros);
char interpretaEntrada(char *buffer, int tamanhoEntrada, char *prog);
void verificaTipoExecucao(char *argv[], int nParametros, int *bg, int *saidaArquivo, int *pPipe);
void child_hand(int sigNum);



int main()
{ 
	pid_t shell_pgid;
	pid_t res;
	pid_t _result;
	int shell_terminal = STDIN_FILENO;
	signal(SIGCHLD, child_hand);
	/*if(signal(SIGCHLD, child_hand)==SIG_ERR) {
	perror("Erro capturando tratamento do sinal");
	return(0);
	}*/

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

		int bg = 0; // verifica se o programa irá rodar em foreground ou background
		int saidaArquivo = 0;
		int pPipe = 0;

		//Variaveis extra necessarias para caso de programa com pipe
		char *argv2[nParametros+1];
		char prog2[tamanhoEntrada];

		verificaTipoExecucao(&argv,nParametros, &bg, &saidaArquivo, &pPipe);

		//faz a separacao dos comandos e parametros recebidos
		//interpretaEntrada(&buffer, tamanhoEntrada,  &prog);

		// faz o fork do processo
		_result=fork();

		if(_result==-1) {
		  perror("Erro em vfork");
		  exit(0);
		}

		if(!_result) {  // filho

			if(saidaArquivo!=0)
			{	
				char fileName[50] = "";
				strcat(fileName, argv[saidaArquivo+1]);
				int output_fds = open(fileName, O_WRONLY | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
				argv[saidaArquivo+1] = NULL;
				res = dup2(output_fds, STDOUT_FILENO);
			}

			if(pPipe!=0){

				int pipefd[2];
				int resDup, resultPipe;

				if (pipe2(pipefd, O_CLOEXEC) == -1) {
					perror("Abertura do pipe");
					exit(0);
				}

				resultPipe=fork();
				if(resultPipe){
					//close(pipefd[1]);

					//redirecionando a entrada do filho para ser a saida do pai
					resDup = dup2(pipefd[0],STDIN_FILENO);

					//passando o programa e parametros para o inicio do vetor argv para o processo filho 
					strcpy(prog2, argv[pPipe+1]);

					int pArgumento=0;

					for(pArgumento=pPipe+1; pArgumento<nParametros; pArgumento++){
						argv2[pArgumento-pPipe-1]=argv[pArgumento];
					}

					argv2[nParametros-pPipe-1]=NULL;

					//passando para o filho o comando que deve ser executado
					execvp(prog2,argv2);
					exit(0);

				}else{
					//close(pipefd[0]);
					//redirecionando a saida do pai para a entrada do filho
					resDup = dup2(pipefd[1],STDOUT_FILENO);

					execvp(prog,argv);
					exit(0);
				}
			}

			//passando para o filho o comando que deve ser executado
			execvp(prog,argv);
			perror("Erro em execl");
			exit(0);

		}else {  // pai

			// verificando se é para o programa rodar em background
			if(bg == 0){
				//este wait faz com que o pai espere o filho terminar, logo o processo filho estara rodando em foreground
				waitpid(_result, NULL, 0);
			}
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

void verificaTipoExecucao(char *argv[], int nParametros, int *bg, int *saidaArquivo, int *pPipe){

	//Procurando a primeira ocorrencia do simbolo & para retornar informando que a execucao em background sera necessaria
	for(int i=1; i<nParametros;i++){
		if(strcmp(argv[i],"&") == 0 ){
			*bg=i;
			argv[i] = NULL;
		}
		else if(strcmp(argv[i],">") == 0 ){
			*saidaArquivo=i;
			argv[i] = NULL;
		}
		else if(strcmp(argv[i],"|") == 0){
			*pPipe=i;
			argv[i] = NULL;
		}


	}
	

}

void child_hand(int sigNum){
	pid_t pidFilho;
	int status;

	// pidFilho=wait(&status);
	pidFilho=waitpid(-1, &status, WNOHANG);
	
	if (WIFEXITED(status)){
		printf("o filho %d terminou com status %d e o sinal foi %d \n",pidFilho, status, sigNum);
		fflush(stdout);
	}
}