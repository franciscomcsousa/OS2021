#include "tecnicofs-client-api.h"
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>

int sockfd_global;

int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  bzero((char *)addr, sizeof(struct sockaddr_un));
  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

int tfsCreate(char *filename, char nodeType) {
  int servlen;
  struct sockaddr_un serv_addr;
  char buffer[100];
  strcpy(buffer,"Hello World");
  servlen = setSockAddrUn("/tmp/server", &serv_addr);

  if (sendto(sockfd_global, buffer, strlen(buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  if (recvfrom(sockfd_global, buffer, sizeof(buffer), 0, 0, 0) < 0) {
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

  if ((sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) { 
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }

  char socket_client[100],pid_s[100],client[100];
  int pid;
  strcpy(client,"/tmp/client");
  pid = (int)getpid();
  sprintf(pid_s,"%d",pid);
  strcpy(socket_client,strcat(client, pid_s));
  

  unlink(socket_client);
  clilen = setSockAddrUn (socket_client, &client_addr);
  if (bind(sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  } 

  sockfd_global = sockfd;

  return 0;
}

int tfsUnmount() {
  return -1;
}
