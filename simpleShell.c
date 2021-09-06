#include <stdio.h>    
#include <sys/types.h>
#include <unistd.h>
#include <stdlib.h>

int main()
{ 
	pid_t result;
	char comandoShell[30];

	while(1){

	    //Aguarda um comando para executar
	    scanf("%s", comandoShell);

		// pid_t vfork(void);
		result=vfork();

		if(result==-1) {
		  perror("Erro em vfork");
		  exit(0);
		}

		if(!result) {  // filho
			/* sobrepoe o processo atual com um novo processo */
			result=execlp(comandoShell,"hello",NULL);

			perror("Erro em execl");
			exit(0);

		}else {  // pai

			//este wait faz com que o pai espere o filho terminar, logo o processo filho estara rodando em foreground
			wait();
			//printf("Pai retomou a execucao.\n");
			printf("\n");
		}

	}
}