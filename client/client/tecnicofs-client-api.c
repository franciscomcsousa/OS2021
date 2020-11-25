#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int client_sockfd;
char* client_socket_name;
extern char* serverName;

/*----------------------------------------------------------------------AUX-FUNCTIONS------------------------------------------------------------------------------------------------*/
/**
 * Clears socket address.
*/
int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/**
 * Creates socket name based on a standart name plus the pid of the client.
*/
void createSocketName(char* client_socket_name){
  int pid = (int) getpid();
  char socket_name[MAX_SOCKET_NAME];
  char pid_string[MAX_FILE_NAME];

  /* converts pid from int to string */
  sprintf(pid_string,"%d",pid);

  strcpy(socket_name,CLIENT_SOCKET_NAME);

  /* concatenate socket_name and pid_string to create unique name */
  strcpy(client_socket_name,strcat(socket_name,pid_string));
}

/*----------------------------------------------------------------------API-FUNCTIONS------------------------------------------------------------------------------------------------*/

int tfsCreate(char *filename, char nodeType) {
  int servlen;
  struct sockaddr_un serv_addr;
  char buffer[MAX_INPUT_SIZE];

  /* to avoid destroying filename */
  strcpy(buffer,filename);
 
  servlen = setSockAddrUn(serverName, &serv_addr);

  if (sendto(client_sockfd, buffer, strlen(buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(client_sockfd, buffer, sizeof(buffer), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  printf("Recebeu resposta do servidor: %s\n", buffer);

  return 0;
}

int tfsDelete(char *path) {
  return -1;
}

int tfsMove(char *from, char *to) {
  return -1;
}

int tfsLookup(char *path) {
  return -1;
}

int tfsPrint(){
  return -1;
}

int tfsMount(char * sockPath) {
  int sockfd;
  socklen_t clilen;
  struct sockaddr_un client_addr;
  char socket_name[MAX_SOCKET_NAME];

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) { 
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }
  
  createSocketName(socket_name);

  unlink(socket_name);
  clilen = setSockAddrUn (socket_name, &client_addr);

  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  } 

  /* makes it global so that other functions can use it */
  client_sockfd = sockfd;
  client_socket_name = socket_name;

  return 0;
}

int tfsUnmount() {

  close(client_sockfd);
  unlink(client_socket_name);
  return 0;
}
