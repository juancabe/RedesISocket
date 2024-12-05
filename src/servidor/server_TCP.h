#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include "common.h"
#include "common_tcp.h"
#include "compose_finger.h"

/*
 *				S E R V E R T C P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */

void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
  int reqcnt = 0;         /* keeps count of number of requests */
  char buf[TAM_BUFFER];   /* This example uses TAM_BUFFER byte messages. */
  char hostname[MAXHOST]; /* remote host's name string */

  int len, len1, status;
  struct hostent *hp; /* pointer to host info for remote host */
  long timevar;       /* contains time returned by time() */

  struct linger linger; /* allow a lingering, graceful close; */
                        /* used when setting SO_LINGER */

  /* Look up the host information for the remote host
   * that we have connected with.  Its internet address
   * was returned by the accept call, in the main
   * daemon loop above.
   */

  status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
                       hostname, MAXHOST, NULL, 0, 0);
  if (status)
  {
    /* The information is unavailable for the remote
     * host.  Just format its internet address to be
     * printed out in the logging information.  The
     * address will be shown in "internet dot format".
     */
    /* inet_ntop para interoperatividad con IPv6 */
    if (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), hostname, MAXHOST) == NULL)
      perror(" inet_ntop \n");
  }
  /* Log a startup message. */
  time(&timevar);
  /* The port number must be converted first to host byte
   * order before printing.  On most hosts, this is not
   * necessary, but the ntohs() call is included here so
   * that this program could easily be ported to a host
   * that does require it.
   */
  printf("Startup from %s port %u at %s",
         hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));

  /* Set the socket for a lingering, graceful close.
   * This will cause a final close of this socket to wait until all of the
   * data sent on it has been received by the remote host.
   */
  linger.l_onoff = 1;
  linger.l_linger = 1;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
                 sizeof(linger)) == -1)
  {
    errout(hostname);
  }

  if (1)
  {

    // First message should be one line
    char *buffer = receive_one_message(hostname, s);
    if (buffer == NULL)
    {
      errout(hostname);
    }

    // Now we must parse client's message and respond to it
    // TODO

    // Now we must compose the response, i.e. call composeFinger
    // TODO

    // Now we must send the response to the client
    // TODO

    // Now we must close the connection
    // TODO
  }

  close(s);

  time(&timevar);
  printf("Completed %s port %u, %d requests, at %s\n",
         hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
}

#endif