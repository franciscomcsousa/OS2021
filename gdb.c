#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(){

    char buffer[127];

    printf("Copyright (C) 2020 Free Software Foundation, Inc.\nLicense GPLv3+: GNU GPL version 3 or later <http://gnu.org/licenses/gpl.html>\nThis is free software: you are free to change and redistribute it.\nThere is NO WARRANTY, to the extent permitted by law.\nType ""show copying"" and ""show warranty"" for details.\nThis GDB was configured as ""x86_64-linux-gnu"".\nType ""show configuration"" for configuration details.\nFor bug reporting instructions, please see:\n<http://www.gnu.org/software/gdb/bugs/>.\nFind the GDB manual and other documentation resources online at:\n    <http://www.gnu.org/software/gdb/documentation/>.\n\nFor help, type ""help"".\nType ""apropos word"" to search for commands related to ""word"".\n(gdb)");
    
    scanf("%s", buffer);

	char* text = malloc(sizeof(char)*(strlen(buffer)+1));
	strcpy(text,buffer);

    printf("Não quero saber\n");

    return(EXIT_SUCCESS);
}