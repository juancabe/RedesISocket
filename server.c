#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define PUERTO 19688
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define BUFFERSIZE 1024         /* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128
#define LOG_FILENAME "server_log.txt"
extern int errno;
int main(int argc, char **argv)
{
  int ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
  if (ls_TCP == -1)
  {
    perror(argv[0]);
    fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
    exit(1);
  }
  struct sockaddr_in myaddr_in;     /* for local socket address */
  struct sockaddr_in clientaddr_in; /* for peer socket address */

  memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
  memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

  int addrlen = sizeof(struct sockaddr_in);
  myaddr_in.sin_family = AF_INET; /* familia de direcciones ipv4 */
  myaddr_in.sin_addr.s_addr = INADDR_ANY;
  myaddr_in.sin_port = htons(PUERTO);

  //      TCP
  /* Bind the listen address to the socket. */
  if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
  {
    perror(argv[0]);
    fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
    exit(1);
  }
  /* Initiate the listen on the socket so remote users
   * can connect.  The listen backlog is set to 5, which
   * is the largest currently supported.
   */
  if (listen(ls_TCP, 5) == -1)
  {
    perror(argv[0]);
    fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
    exit(1);
  }

  //      UDP
  /* Create the socket UDP. */
  int s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
  if (s_UDP == -1)
  {
    perror(argv[0]);
    printf("%s: unable to create socket UDP\n", argv[0]);
    exit(1);
  }
  /* Bind the server's address to the socket. */
  if (bind(s_UDP, (struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
  {
    perror(argv[0]);
    printf("%s: unable to bind address UDP\n", argv[0]);
    exit(1);
  }
  setpgrp(); // Para que no se cierre el servidor al cerrar la terminal
}