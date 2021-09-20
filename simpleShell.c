/*

Código desenvolvido por:
Felipe Alves Cordeiro 			RA: 744335
Leonardo H Fernandes da Silva 	RA: 744340

O intuito desse projeto é chamar syscalls do Sistema Operacional
e simular o funcionamento de um shell.

*/

// Importando bibliotecas necessárias
#include <stdio.h>    
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#define LENBUFFER 255
#define NUMMAXPIPE 10

// Header das funções criadas para auxiliar durante o código
int leituraTerminal(char *buffer, int *nParametros);
void verificaTipoExecucao(char *argv[], int nParametros, int *bg, int *saidaArquivo, int pPipe[], int *nPipes);
void child_hand(int sigNum);
void separaStrings(char *buffer, char *argv[], char *prog, int tamanhoEntrada);



int main()
{ 	
	pid_t shell_pgid; // Variável que armazena o PID do shell
	pid_t res; // Variável que armazena o retorno do dup2 para redirecionar a saída para arquivos
	pid_t result; // Variável que armazena o retorno do fork
	int shell_terminal = STDIN_FILENO; // Variável utilizada para deixar o código sempre em foreground
	int pipefd[2],pipefd2[2]; // Variáveis utilizadas na abertura do pipe

	// Instalando a rotina child_hand para sinais recebidos do tipo SIGCHLD
	if(signal(SIGCHLD, child_hand)==SIG_ERR) {
		perror("Erro capturando tratamento do sinal");
		return(0);
	}

	// Ignorando sinal relacionado ao CTRL+Z
	if(signal(SIGTSTP,SIG_IGN)==SIG_ERR)
	    perror("Erro ignorando SIGTSTP");

	while(1){
		int nParametros=1; // Variável que armazena o número de parâmetros de um comando
		int nProcs=1; // Sempre tem pelo menos 1 processo, a menos que tenhamos pipe

		// Inserindo o shell em foreground - vai ficar preso aqui enquanto nao for fg
		while (tcgetpgrp (shell_terminal) != (shell_pgid = getpgrp ())){
			kill (- shell_pgid, SIGTTIN);
		}

		// Alocando um tamanho máximo para a entrada do terminal
		char buffer[LENBUFFER]; // Buffer que armazena a entrada digitada no shell
		int tamanhoEntrada = leituraTerminal(&buffer, &nParametros); // Obtendo a entrada e colocando no buffer


		// Criando variável de argumentos
		char *argv[nParametros+1];

		// Variável na qual vai ser salvo o programa digitado, futuramente sera substituido por uma matriz de argumentos tipo argv
		char prog[tamanhoEntrada];

		// Variáveis utilizadas principalmente quando existirem pipes
		char *argv2[nParametros+1];
		char prog2[tamanhoEntrada];

		// Separando os argumentos por espaço, colocando no argv e definindo o programa que será executado em prog 
		separaStrings(&buffer, &argv, &prog, tamanhoEntrada);

		if (argv[0]!=NULL){

			int bg = 0; // "flag" para verificar se o programa irá rodar em foreground ou background
			int saidaArquivo = 0; // "flag" para verificar se a saída do programa irá para um arquivo
			int pPipe[NUMMAXPIPE]; // Vetor que armazena as posições que existem pipes

			// Verficando se o programa irá ter pipes, rodará em background ou redirecionará a saída para um arquivo
			verificaTipoExecucao(&argv,nParametros, &bg, &saidaArquivo, &pPipe, &nProcs);

			// Executando todos o programas que existirem na mesma linha
			for(int progAt=0;progAt<nProcs;progAt++){

				// Abrindo pipes, se necessário, quando existir mais de um programa, exceto para o último
				if(nProcs>1 && progAt<nProcs-1){
					
					// Criando pipes de forma alterna, para que um programa intermediário 
					// não tente ler e escrever no mesmo pipe
					if(progAt%2==0){
						if (pipe(pipefd2) == -1) {
							perror("Abertura do pipe");
							exit(0);
						}
					}
					else{
						if (pipe(pipefd) == -1) {
							perror("Abertura do pipe");
							exit(0);
						}
					}
				}

				// Criação do filho do shell
				result=fork();

				if(result==-1) {
				  perror("Erro em fork");
				  exit(0);
				}

				if(!result) {  // Código que o filho irá executar

					// Se tiver pipe no comando, irá redirecionar as entradas e saídas quando necessário
					if(nProcs>1){ 
						int resDup; // Variável que armazena o retorno do dup2

						// Caso seja um programa que não seja o primeiro daqueles passados como parâmetros
						if(progAt>=1){
							if(progAt%2==0){
								close(pipefd2[0]);
								// Redirecionando a entrada do filho para ser o read do pipe
								resDup = dup2(pipefd[0],STDIN_FILENO);
								close(pipefd[0]);
							}
							else{
								close(pipefd[0]);
								resDup = dup2(pipefd2[0],STDIN_FILENO);
								close(pipefd2[0]);
							}

						}
						// Caso não seja o último programa daqueles passados como parâmetros
						if(progAt<nProcs-1){

							if(progAt%2==0){
								// Redirecionando a saida do filho para ser o write do pipefd2
								resDup = dup2(pipefd2[1],STDOUT_FILENO);
							}
							else{
								resDup = dup2(pipefd[1],STDOUT_FILENO);
							}

						}
					}

					int pArgumento=-1; // Variável de controle para inserção dos parâmetros necessários para executar o programa

					// Passando para argv2 os argumentos do processo atual
					if(nProcs>1){
						// pPipe[pipeAt] retorna a posicao do pipe atual e ao somar um, é possível pegar o programa que será executado
						strcpy(prog2, argv[pPipe[progAt]+1]);
						do{
							pArgumento++;
							argv2[pArgumento]=argv[pArgumento+pPipe[progAt]+1];
						}while(argv2[pArgumento]!=NULL); // Loop para passar os parâmetros do programa atual de argv para argv2
					}
					else{ // Caso que só tem um programa
						strcpy(prog2, prog);
						do{
							pArgumento++;
							argv2[pArgumento]=argv[pArgumento];
						}while(argv2[pArgumento]!=NULL);
					}

					// Só irá redirecionar a saída se, e somente se, for o último programa a ser executado
					if(saidaArquivo!=0 && progAt==nProcs-1)
					{	
						char fileName[50] = "";
						strcat(fileName, argv[saidaArquivo+1]); // Passando o nome do arquivo que será criado para fileName
						int output_fds = open(fileName, O_WRONLY | O_CREAT | O_CLOEXEC, S_IRUSR | S_IWUSR);
						res = dup2(output_fds, STDOUT_FILENO); // Redirecionando a saída do programa para o arquivo
					}

					// Executando o programa
					execvp(prog2,argv2);
					perror("Erro em execvp");
					exit(0);

				}else {  // Shell
					// Fechando os pipes de forma alternada
					if(progAt%2==0 && progAt<nProcs-1){
						close(pipefd2[1]);
					}
					else if( progAt<nProcs-1){
						close(pipefd[1]);
					}
					
					// Verificando se é para o programa rodar em background
					if(bg == 0){
						// Este wait faz com que o pai espere o filho terminar, logo o processo filho estará rodando em foreground
						waitpid(result, NULL, 0);
					}else if(progAt==nProcs-1) {
						printf("[%d]\n",result);
					}
				}
			}
		}
	}
}

// Função que lê o que foi escrito no terminal, insere no buffer e retorna o tamanho da entrada
int leituraTerminal(char *buffer, int *nParametros){

	int c; // Variável que armazena cada caractere
	int tamanhoEntrada=0; // Variável de retorno que contém o tamanho da entrada digitada

	printf("$ ");

	while(tamanhoEntrada <= LENBUFFER){
		c = getchar(); // Lendo cada caractere 

		// Inserindo no buffer na posicao "tamanhoEntrada" o próximo caractere lido
		buffer[tamanhoEntrada] = c;

		// Verificando se a entrada terminou
		if(c=='\n'){
			buffer[tamanhoEntrada] = '\0';
			return tamanhoEntrada;
		}
		else if(c == ' '){
			*nParametros=*nParametros+1;	
		}

		tamanhoEntrada++;

	}
	return tamanhoEntrada;

}

// Função que separa os argumentos por espaço, colocando no argv e definindo o programa que será executado em prog 
void separaStrings(char *buffer, char *argv[], char *prog, int tamanhoEntrada){

	int argumentoAtual=0; // variável de controle para passar os parâmetros para o argv
	char delimitador[] = " "; // Definindo o delimitador utilizado
	char *parametro = strtok(buffer, delimitador); // Lendo a primeira palavra do buffer

	// Loop que coloca os parâmetros no argv
	while(parametro != NULL)
	{
		// armazenando o programa que será executado
		if(argumentoAtual==0)
			strcpy(prog, parametro);
		argv[argumentoAtual]=parametro;

		argumentoAtual++;
		parametro = strtok(NULL, delimitador); // Lendo a próxima palavra do buffer
	}

	argv[argumentoAtual]=NULL;

}

// Função que verifica se o programa irá ter pipes, rodará em background ou redirecionará a saída para um arquivo
void verificaTipoExecucao(char *argv[], int nParametros, int *bg, int *saidaArquivo, int pPipe[], int *nProg){

	pPipe[0]=-1;// Definindo que o primeiro programa comeca na posicao -1 pois será incrementado depois

	// Procurando a ocorrência dos seguintes símbolos
	// - &: para retornar informando que a execuçãoo em background será necessária
	// - |: para retornar que existem pipes e a posição de cada um
	// - >: para retornar informando que será necessário redirecionar a saída
	// Caso um desses símbolos for encontrado, coloca-se NULL no lugar 
	// para que ele não seja enviado como parâmetro do processo
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
			pPipe[*nProg]=i;
			*nProg=*nProg+1;
			argv[i] = NULL;
		}
	}

}

// Função que não deixa os filhos do shell que são colocados em background se tornarem zumbis
void child_hand(int sigNum){
	pid_t pidFilho;
	int status;

	pidFilho=waitpid(-1, &status, WNOHANG); // Utilizando o NOHANG para o pai NÃO ficar parado esperando o retorno de um filho em background
	
	if (WIFEXITED(status)){
		//printf("o filho %d terminou com status %d e o sinal foi %d \n",pidFilho, status, sigNum);
		fflush(stdout);
	}
}