#ifndef SERVER_UDP_H
#define SERVER_UDP_H

#include "common_server.h"

/*
 *				S E R V E R U D P
 *
 *	This is the actual server routine that the daemon forks to
 *	handle each individual connection.  Its purpose is to receive
 *	the request packets from the remote client, process them,
 *	and return the results to the client.  It will also write some
 *	logging information to stdout.
 *
 */
void serverUDP(int s, struct sockaddr_in clientaddr_in)
{
  struct in_addr reqaddr; /* for requested host's address */
  struct hostent *hp;     /* pointer to host info for requested host */
  int nc, errcode;

  struct addrinfo hints, *res;

  int addrlen;

  addrlen = sizeof(struct sockaddr_in);

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;

  if (1)
  {

    char request[TAM_BUFFER_IN_UDP];
    ssize_t cc = recvfrom(s, request, TAM_BUFFER_IN_UDP, 0,
                          (struct sockaddr *)&clientaddr_in, &addrlen);
    if (cc == -1)
    {
      perror("[UDP RECEIVE] Error");
      printf("%d: recvfrom error\n", s);
      exit(1);
    }
    /* Make sure the message received is
     * null terminated.
     */
    request[cc] = '\0';
    FILE *outLog = fopen(LOG_FILENAME, "a+");
    fprintf(outLog, "[UDP SERVER] RECEIVED\n%s\n", request);
    fclose(outLog);
  }
  else
  {
    /* Treat the message as a string containing a hostname. */
    /* Esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta. */
    // errcode = getaddrinfo(request, NULL, &hints, &res);
    if (errcode != 0)
    {
      /* Name was not found.  Return a
       * special value signifying the error. */
      reqaddr.s_addr = ADDRNOTFOUND;
    }
    else
    {
      /* Copy address of host into the return request. */
      reqaddr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
    }
    freeaddrinfo(res);

    nc = sendto(s, &reqaddr, sizeof(struct in_addr),
                0, (struct sockaddr *)&clientaddr_in, addrlen);
    if (nc == -1)
    {
      perror("serverUDP");
      printf("%s: sendto error\n", "serverUDP");
      return;
    }
  }
}

#endif