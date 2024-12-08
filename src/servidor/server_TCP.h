#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include "common_server.h"
#include "../common_TCP.h"
#include "compose_finger.h"
#include "parse_client_request.h"

#include "../cliente/client_tcp.h"

void serverTCP(int s, struct sockaddr_in clientaddr_in)
{
  char SERVER_NAME[] = "serverTCP";
  int reqcnt = 0;                /* keeps count of number of requests */
  char remote_hostname[MAXHOST]; /* remote host's name string */

  struct hostent *hp; /* pointer to host info for remote host */

  struct linger linger; /* allow a lingering, graceful close; */
                        /* used when setting SO_LINGER */

  int status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in),
                           remote_hostname, MAXHOST, NULL, 0, 0);
  if (status && (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), remote_hostname, MAXHOST) == NULL))
  {
    perror(" inet_ntop \n");
  }
#ifdef DEBUG
  /* Log a startup message. */
  long timevar; /* contains time returned by time() */
  time(&timevar);
  printf("Startup from %s port %u at %s",
         remote_hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));
#endif

  /* Set the socket for a lingering, graceful close.
   * This will cause a final close of this socket to wait until all of the
   * data sent on it has been received by the remote host.
   */
  linger.l_onoff = 1;
  linger.l_linger = 1;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger,
                 sizeof(linger)) == -1)
  {
    errout("First ERROUT");
  }

  // First message should be one line
  char *buffer = receive_one_message(remote_hostname, s);
  reqcnt++;
  if (buffer == NULL)
  {
    errout("Second ERROUT");
  }

  // Now we must parse client's message and respond to it
  char *username = NULL;
  bool username_malloced = false;
  char *hostname = NULL;
  parse_client_request_return ret = parse_client_request(buffer, &hostname, &username);
  username_malloced = true;
  char *response = NULL;
  bool response_malloced = false;

  // Now we must compose the response, i.e. call composeFinger
  switch (ret)
  {
  case USERNAME:
    if (username == NULL)
    {
      response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
      username_malloced = false;
    }
    else
    {
      response = just_one_user_info(username);
      response_malloced = true;
    }
    break;
  case ERROR:
    response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
    break;
  case NO_USERNAME_NO_HOSTNAME:
    response = all_users_info();
    response_malloced = true;
    break;
  case HOSTNAME_REDIRECT:
    if (username == NULL) // Username not provided by client
    {
      username = "\r\n";
      username_malloced = false;
    }
    if (hostname == NULL)
    {
      response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
    }
    else
    {
      response = client_tcp(username, hostname); // Username is the new request
      response_malloced = true;
    }
    break;
  default:
    response = "Unknown error\r\n";
    break;
  }

  if (response == NULL)
  {
    response = "Error during generating correct response\r\n";
    response_malloced = false;
  }

  // Now we must send the response to the client
  if (send(s, response, strlen(response), 0) != strlen(response))
  // \0 no se envia, acaba con  \r\n
  {
    errout("3rd ERROUT");
  }
  // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
  // Close sending channel, the client alredy closed theirs
  if (shutdown(s, SHUT_WR) == -1)
  {
    errout("4th ERROUT");
  }
  // Now we must close the connection
  close(s);

#ifdef DEBUG
  time(&timevar);
  printf("Completed %s port %u, %d requests, at %s\n",
         hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
#endif
  // Free memory
  if (response_malloced)
    free(response);
  if (username_malloced && (username != NULL))
    free(username);
  if (hostname != NULL)
    free(hostname);
}

#endif