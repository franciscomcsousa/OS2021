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

/**
 * Resets and set socket address.
 * @param path: socket name
 * @param addr: socket address
*/
int setSockAddrUn(char *path, struct sockaddr_un *addr) {

  if (addr == NULL)
    return 0;

  /* set addr bytes to zero */
  bzero((char *)addr, sizeof(struct sockaddr_un));

  addr->sun_family = AF_UNIX;
  strcpy(addr->sun_path, path);

  return SUN_LEN(addr);
}

/**
 * Creates socket name based on a standart name plus the pid of the client.
*/
void createSocketName(){
  
  char pid_string[MAX_FILE_NAME];
  char socket_name[MAX_SOCKET_NAME];

  /* converts pid from int to string */
  sprintf(pid_string,"%d",getpid());

  strcpy(socket_name,CLIENT_SOCKET_NAME);

  /* concatenate socket_name and pid_string to create unique name */
  strcpy(client_socket_name,strcat(socket_name,pid_string));
}


int tfsCreate(char *filename, char nodeType) {

  int servlen,result;
  char buffer[MAX_INPUT_SIZE];
  struct sockaddr_un serv_addr;

  sprintf(buffer,"c %s %c",filename,nodeType);
  servlen = setSockAddrUn(serverName, &serv_addr);

  /* sends command to serv_addr */
  if (sendto(client_sockfd, buffer, strlen(buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  /* receive server response */
  if (recvfrom(client_sockfd, &result, sizeof(result), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  return result;
}

int tfsDelete(char *path) {

  int servlen,result;
  char buffer[MAX_INPUT_SIZE];
  struct sockaddr_un serv_addr;

  sprintf(buffer,"d %s",path);
  servlen = setSockAddrUn(serverName, &serv_addr);

  /* sends command to serv_addr */
  if (sendto(client_sockfd, buffer, strlen(buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0) {
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  } 

  /* receive server response */
  if (recvfrom(client_sockfd, &result, sizeof(result), 0, 0, 0) < 0) {
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  } 

  return result;
}

int tfsMove(char *from, char *to) {
  
  int servlen,result;
  char buffer[MAX_INPUT_SIZE];
  struct sockaddr_un serv_addr;

  sprintf(buffer, "m %s %s",from,to);
  servlen = setSockAddrUn(serverName, &serv_addr);  

  /* sends command to serv_addr */
  if (sendto(client_sockfd, buffer, strlen(buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0){
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  }

  /* receive server response */
  if (recvfrom(client_sockfd, &result, sizeof(result),0,0,0) < 0){
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }

  return result;
}

int tfsLookup(char *path) {

  int servlen,result;
  char buffer[MAX_INPUT_SIZE];
  struct sockaddr_un serv_addr;

  sprintf(buffer, "l %s", path);
  servlen = setSockAddrUn(serverName, &serv_addr);  

  /* sends command to serv_addr */
  if (sendto(client_sockfd, buffer, strlen(buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0){
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  }

  /* receive server response */
  if (recvfrom(client_sockfd, &result, sizeof(result),0,0,0) < 0){
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }

  return result;
}

int tfsPrint(char *file){

  int servlen,result;
  char buffer[MAX_INPUT_SIZE];
  struct sockaddr_un serv_addr;

  sprintf(buffer, "p %s", file);
  servlen = setSockAddrUn(serverName, &serv_addr);  

  /* sends command to serv_addr */
  if (sendto(client_sockfd, buffer, strlen(buffer)+1, 0, (struct sockaddr *) &serv_addr, servlen) < 0){
    perror("client: sendto error");
    exit(EXIT_FAILURE);
  }

  /* receive server response */
  if (recvfrom(client_sockfd, &result, sizeof(result),0,0,0) < 0){
    perror("client: recvfrom error");
    exit(EXIT_FAILURE);
  }

  return result;
}

int tfsMount(char * sockPath) {

  socklen_t clilen;
  struct sockaddr_un client_addr;
  client_socket_name = (char*) malloc(sizeof(char)*(MAX_SOCKET_NAME+1));

  /* creates a socket of domain UNIX and type DATAGRAM */
  if ((client_sockfd = socket(AF_UNIX, SOCK_DGRAM, 0) ) < 0) { 
    perror("client: can't open socket");
    exit(EXIT_FAILURE);
  }
  
  createSocketName();
  unlink(client_socket_name);

  /* clears and set the socket address */
  clilen = setSockAddrUn (client_socket_name, &client_addr);

  /* binds a name to the socket, this name is specified by the server_addr */
  if (bind(client_sockfd, (struct sockaddr *) &client_addr, clilen) < 0) {
    perror("client: bind error");
    exit(EXIT_FAILURE);
  } 
  return EXIT_SUCCESS;
}

int tfsUnmount() {

  close(client_sockfd);
  unlink(client_socket_name);
  free(client_socket_name);
  return EXIT_SUCCESS;
}
