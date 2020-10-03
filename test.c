#include <stdio.h>
#include <stdlib.h>

/*#define EXIT_SUCCESS 0;*/

void help(){
	printf("what am I doing ?\n");
}

int main(){
	char buffer[127];
	scanf("%s", buffer);
	printf("Não quero saber\n");

	help();

	return EXIT_SUCCESS;
}
