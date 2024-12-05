#include "common.h"
#include "server_TCP.h"
#include "server_UDP.h"

int FIN = 0;
void finalizar() { FIN = 1; }

int main(argc, argv)
int argc;
char *argv[];
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

  char buffer[BUFFERSIZE]; /* buffer for packets to be read into */

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

  /* Now, all the initialization of the server is
   * complete, and any user errors will have already
   * been detected.  Now we can fork the daemon and
   * return to the user.  We need to do a setpgrp
   * so that the daemon will no longer be associated
   * with the user's control terminal.  This is done
   * before the fork, so that the child will not be
   * a process group leader.  Otherwise, if the child
   * were to open a terminal, it would become associated
   * with that terminal as its control terminal.  It is
   * always best for the parent to do the setpgrp.
   */
  setpgrp();

  switch (fork())
  {
  case -1: /* Unable to fork, for some reason. */
    perror(argv[0]);
    fprintf(stderr, "%s: unable to fork daemon\n", argv[0]);
    exit(1);

  case 0: /* The child process (daemon) comes here. */

    /* Close stdin and stderr so that they will not
     * be kept open.  Stdout is assumed to have been
     * redirected to some logging file, or /dev/null.
     * From now on, the daemon will not report any
     * error messages.  This daemon will loop forever,
     * waiting for connections and forking a child
     * server to handle each one.
     */
    fclose(stdin);
    fclose(stderr);

    /* Set SIGCLD to SIG_IGN, in order to prevent
     * the accumulation of zombies as each child
     * terminates.  This means the daemon does not
     * have to make wait calls to clean them up.
     */
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
          /* Note that addrlen is passed as a pointer
           * so that the accept call can return the
           * size of the returned address.
           */
          /* This call will block until a new
           * connection arrives.  Then, it will
           * return the address of the connecting
           * peer, and a new socket descriptor, s,
           * for that connection.
           */
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
          default: /* Daemon process comes here. */
            /* The daemon needs to remember
             * to close the new accept socket
             * after forking the child.  This
             * prevents the daemon from running
             * out of file descriptor space.  It
             * also means that when the server
             * closes the socket, that it will
             * allow the socket to be destroyed
             * since it will be the last close.
             */
            close(s_TCP);
          }
        } /* De TCP*/
        /* Comprobamos si el socket seleccionado es el socket UDP */
        if (FD_ISSET(s_UDP, &readmask))
        {
          /* This call will block until a new
           * request arrives.  Then, it will
           * return the address of the client,
           * and a buffer containing its request.
           * BUFFERSIZE - 1 bytes are read so that
           * room is left at the end of the buffer
           * for a null character.
           */

          serverUDP(s_UDP, clientaddr_in);
        }
      }
    } /* Fin del bucle infinito de atenci�n a clientes */
    /* Cerramos los sockets UDP y TCP */
    close(ls_TCP);
    close(s_UDP);

    printf("\nFin de programa servidor!\n");

  default: /* Parent process comes here. */
    exit(0);
  }
}
