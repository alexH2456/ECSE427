#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <string.h>
#include <signal.h>

// Alexander Harris - 260688155
//
//TODO: Set up child process creation using fork() and waitpid(), write execvp function for ls/pwd/cd/cat/cp/fg/jobs/exit and background(&), 

int getcmd(char *prompt, char *args[], int *background)
{
	int length, i = 0;
	char *token, *loc;
	char *line = NULL;
	size_t linecap = 0;
	printf("%s", prompt);
	length = getline(&line, &linecap, stdin);
	if(length <= 0){
		exit(-1);
	}

	if((loc = index(line, '&')) != NULL){
		*background = 1;
		*loc = ' ';
	}
	else{
		*background = 0;
	}

	while((token = strsep(&line, " \t\n")) != NULL){
		for(int j = 0; j < strlen(token); j++){
			if(token[j] <= 32){
				token[j] = '\0';
			}
		}
		if(strlen(token) > 0){
			args[i++] = token;
		}
		return i;
	}
}

int main(void)
{
	time_t now;
	srand((unsigned int) (time(&now)));

	char *args[20];
	int bg;
	while(1){
		bg = 0;
		memset(args, 0, sizeof(args));
		int cnt = getcmd("\n>> ", args, &bg);
		
		//Increases execution time for syscalls
		/*int w, rem;
		w =  rand() % 10;
		rem = sleep(w);
		while(rem != 0){
			rem = sleep(rem);
		}*/

		if(args[0] != NULL){
			printf("%s\n", args[0]);
		} 
		else{
			printf(" ");
		}
	}
}