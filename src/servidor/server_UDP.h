#ifndef SERVER_UDP_H
#define SERVER_UDP_H

#include "common_server.h"

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

  char buffer[TAM_BUFFER_IN_UDP];
  ssize_t cc = recvfrom(s, buffer, TAM_BUFFER_IN_UDP, 0, (struct sockaddr *)&clientaddr_in, &addrlen);

  // Now we must parse client's message and respond to it
  char *username = NULL;
  char *hostname = NULL;
  parse_client_request_return ret = parse_client_request(buffer, &hostname, &username);
  char *response = NULL;

  // Now we must compose the response, i.e. call composeFinger
  switch (ret)
  {
  case USERNAME:
    response = just_one_user_info(username);
    break;
  case ERROR:
    response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
    break;
  case NO_USERNAME_NO_HOSTNAME:
    response = all_users_info();
    break;
  case HOSTNAME_REDIRECT:
    // TODO
    exit(-1);
    break;
  }

  // Now we must check that the response fits in UDP packet
  // TODO
  bool freed = false;
  if (response == NULL)
  {
    freed = true;
    response = "No response\r\n";
  }
  else if (strlen(response) > TAM_BUFFER_OUT_UDP)
  {
    freed = true;
    free(response);
    response = "Response doesn't fit in UDP packet\r\n";
  }

#ifdef DEBUG
  printf(("[serverUDP] sending response: %s\n"), response ? response : "NO RESPONSE");
#endif

  // Now we must send the response to the client
  if (sendto(s, response, strlen(response), 0, (struct sockaddr *)&clientaddr_in, addrlen) != strlen(response))
  {
    errout("3rd ERROUT");
  }

  // Free memory
  if (!freed && response)
  {
    free(response);
  }
  if (username)
  {
    free(username);
  }
  if (hostname)
  {
    free(hostname);
  }

  // ALL Done
}

#endif