#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/*#define EXIT_SUCCESS 0;*/

void help(){
	printf("what am I doing ?\n");
}

void testedois(){
	printf("socorro\n");
}

void SO(){
	printf("SO pain level is: 1%%\n");
}

/* Creates a segmentation fault error
damn entao eh assim que se criam descricoes wow mlg 420
360 no scope
 */
void explosion(){
	char choice[127];
	char bomb[2];

	printf("Do you want to get a Segmentation fault error ?\nyes:1 no:2\n");
	scanf("%s",choice);

	switch(atoi(choice)){
		case 1:
			*(int*)0 = 0;
			break;
		
		case 2:
			printf("get trolled\n");
			*(int*)0 = 0;
			break;
	}
}

int main(){

	char buffer[127];

	scanf("%s", buffer);

	char* text = malloc(sizeof(char)*(strlen(buffer)+1));
	strcpy(text,buffer);
	printf("You said: %s\n",text);

	printf("Não quero saber\n");

	help();
	testedois();
	SO();

	explosion();

	free(text);
	return EXIT_SUCCESS;
}
