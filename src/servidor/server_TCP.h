#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include "common.h"

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
    // Receive all data the client wants
    // to send until he closes his SENDING connection
    // (he can still receive our response)
    const int step_len = 1024;
    int received_len, actual_len = 0;
    char *buffer = malloc(step_len);
    while (received_len = recv(s, buffer + actual_len, step_len, 0))
    {
      if (received_len < 0)
        errout(hostname);

      actual_len += received_len;
      char *tempPtr = buffer;
      buffer = realloc(buffer, actual_len + step_len);
      if (buffer == NULL)
      {
        errout(hostname);
        free(tempPtr);
      }
    }

    /*
    EXAMPLE OF EXECUTION OF THE LOOP
    Receiving 2*step_len data
  recv()
      1. actual_len = 0, reading step_len bytes
        a) actual_len -> actual_len'
        b) buffer alloc enough for next step_len bytes
        (lets suppose received_len == 1024)
      2. actual_len = 1024, reading step_len bytes
        a) actual_len -> actual_len'
        b) buffer alloc enough for next step_len bytes
      3. recv returns 0, end of loop
    */

    // Now we must parse client's message and respond to it
    // TODO

    FILE *outLog = fopen(LOG_FILENAME, "a");
    fprintf(outLog, "[TCP SERVER] RECEIVED\n");
    fprintf(outLog, "Client message:\n");
    fprintf(outLog, "\n%s\n", buffer);
    free(buffer);
    fclose(outLog);
  }
  else
  { // Example's approach
    while (len = recv(s, buf, TAM_BUFFER, 0))
    {
      if (len == -1)
        errout(hostname); /* error from recv */

      /* The reason this while loop exists is that there
       * is a remote possibility of the above recv returning
       * less than TAM_BUFFER bytes.  This is because a recv returns
       * as soon as there is some data, and will not wait for
       * all of the requested data to arrive.  Since TAM_BUFFER bytes
       * is relatively small compared to the allowed TCP
       * packet sizes, a partial receive is unlikely.  If
       * this example had used 2048 bytes requests instead,
       * a partial receive would be far more likely.
       * This loop will keep receiving until all TAM_BUFFER bytes
       * have been received, thus guaranteeing that the
       * next recv at the top of the loop will start at
       * the begining of the next request.
       */
      while (len < TAM_BUFFER)
      {
        len1 = recv(s, &buf[len], TAM_BUFFER - len, 0);
        if (len1 == -1)
          errout(hostname);
        len += len1;
      }
      /* Increment the request count. */
      reqcnt++;
      /* Send a response back to the client. */
      if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER)
        errout(hostname);
    }
  }
  close(s);

  time(&timevar);
  printf("Completed %s port %u, %d requests, at %s\n",
         hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
}

#endif