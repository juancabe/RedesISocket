#include "common_server.h"
#include "server_TCP.h"
#include "server_UDP.h"

int FIN = 0;
void finalizar() { FIN = 1; }

int main(int argc, char *argv[])
{
  int s_TCP, s_UDP; /* connected socket descriptor */
  int ls_TCP;       /* listen socket descriptor */

  int cc; /* contains the number of bytes read */

  struct sigaction sa = {.sa_handler = SIG_IGN}; /* used to ignore SIGCHLD */

  struct sockaddr_in myaddr_in;     /* for local socket address */
  struct sockaddr_in clientaddr_in; /* for peer socket address */
  int addrlen;

  fd_set readmask;     /* máscara para select */
  int numfds, s_mayor; /* número de descriptores */

  struct sigaction vec;

  /* Create the listen socket. */
  ls_TCP = socket(AF_INET, SOCK_STREAM, 0);
  if (ls_TCP == -1)
  {
    perror(argv[0]);
    fprintf(stderr, "%s: unable to create socket TCP\n", argv[0]);
    exit(1);
  }
  /* clear out address structures */
  /* needed for correct bind when using wildcards */
  memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
  memset((char *)&clientaddr_in, 0, sizeof(struct sockaddr_in));

  addrlen = sizeof(struct sockaddr_in); /* size of addresses */

  /* Set up address structure for the listen socket. */
  myaddr_in.sin_family = AF_INET;
  myaddr_in.sin_addr.s_addr = INADDR_ANY;
  myaddr_in.sin_port = htons(PUERTO);

  /* Bind the listen address to the socket. */
  if (bind(ls_TCP, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
  {
    perror(argv[0]);
    fprintf(stderr, "%s: unable to bind address TCP\n", argv[0]);
    exit(1);
  }

  if (listen(ls_TCP, 5) == -1) // 5 es el número máximo de conexiones pendientes
  {
    perror(argv[0]);
    fprintf(stderr, "%s: unable to listen on socket\n", argv[0]);
    exit(1);
  }

  /* Create the socket UDP. */
  s_UDP = socket(AF_INET, SOCK_DGRAM, 0);
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
  setpgrp();

  switch (fork())
  {
  case -1: /* Unable to fork, for some reason. */
    perror(argv[0]);
    fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
    exit(1);

  case 0: /* The child process (daemon) comes here. */
#ifndef DEBUG
    fclose(stdin);
    fclose(stderr);
#endif

    if (sigaction(SIGCHLD, &sa, NULL) == -1)
    {
      perror(" sigaction(SIGCHLD)");
      fprintf(stderr, "%s: unable to register the SIGCHLD signal\n", argv[0]);
      exit(1);
    }

    /* Registrar SIGTERM para la finalizacion ordenada del programa servidor */
    vec.sa_handler = (void *)finalizar;
    vec.sa_flags = 0;
    if (sigaction(SIGTERM, &vec, (struct sigaction *)0) == -1)
    {
      perror(" sigaction(SIGTERM)");
      fprintf(stderr, "%s: unable to register the SIGTERM signal\n", argv[0]);
      exit(1);
    }

    while (!FIN)
    {
      /* Meter en el conjunto de sockets los sockets UDP y TCP */
      FD_ZERO(&readmask);
      FD_SET(ls_TCP, &readmask);
      FD_SET(s_UDP, &readmask);
      /*
      Seleccionar el descriptor del socket que ha cambiado. Deja una marca en el conjunto de sockets (readmask)
      */
      if (ls_TCP > s_UDP)
        s_mayor = ls_TCP;
      else
        s_mayor = s_UDP;

      if ((numfds = select(s_mayor + 1, &readmask, (fd_set *)0, (fd_set *)0, NULL)) < 0)
      {
        if (errno == EINTR)
        {
          FIN = 1;
          close(ls_TCP);
          close(s_UDP);
          perror("\nFinalizando el servidor. Se�al recibida en elect\n ");
        }
      }
      else
      {

        /* Comprobamos si el socket seleccionado es el socket TCP */
        if (FD_ISSET(ls_TCP, &readmask))
        {
          s_TCP = accept(ls_TCP, (struct sockaddr *)&clientaddr_in, &addrlen);
          if (s_TCP == -1)
            exit(1);
          switch (fork())
          {
          case -1: /* Can't fork, just exit. */
            exit(1);
          case 0:          /* Child process comes here. */
            close(ls_TCP); /* Close the listen socket inherited from the daemon. */
            serverTCP(s_TCP, clientaddr_in);
            exit(0);
          default:
            close(s_TCP);
          }
        }
        if (FD_ISSET(s_UDP, &readmask))
        {
          switch (fork())
          {
          case -1:
            exit(1);
          case 0:
            serverUDP(s_UDP, clientaddr_in);
            exit(0);
          default:
            break;
          }
        }
      }
    }
    close(ls_TCP);
    close(s_UDP);

    printf("\nFin de programa servidor!\n");

  default: /* Parent process comes here. */
    exit(0);
  }
}
