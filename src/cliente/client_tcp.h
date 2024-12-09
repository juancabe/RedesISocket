#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H

#include "../common_TCP.h"
#include "common_client.h"

#define MAX_RESPONSE_SIZE 1000000000 // 1GB

void handler_ctcp(int signum) {
#ifdef DEBUG
  printf("[client_TCP]Alarma recibida \n");
#endif
}

// Returns string malloced with response from server, if malloc fails, returns
// NULL
// Request MUST be a null terminated string
char *TCP_send_close_send_and_wait_server_request(int s, char *request,
                                                  int *response_size) {

#ifdef DEBUG
  fprintf(stderr, "Sending request: %s\n", request);
#endif
  if (send(s, request, strlen(request), 0) != strlen((request))) {
    return NULL;
  }
#ifdef DEBUG
  fprintf(stderr, "Request sent\n");
#endif
  // Do this in order to gracefully close connection later
  // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
  if (shutdown(s, SHUT_WR) == -1) {
    return NULL;
  }

  // Receive response from server, until he closes connection
  const int step_len = 1024;
  int received_len, actual_len = 0;
  char *buffer = (char *)malloc(step_len);
  bool received = false;
#ifdef DEBUG
  fprintf(stderr, "Receiving response\n");
#endif
  const char *connection_problem = "Connection problem\r\n";
  char *connection_problem_malloced =
      (char *)malloc(strlen(connection_problem) + 1);
  if (connection_problem_malloced == NULL) {
    return NULL;
  }
  strcpy(connection_problem_malloced, connection_problem);

  // Receive until server closes connection or timeout
  alarm(TIMEOUT);
  while ((received_len = recv(s, buffer + actual_len, step_len, 0)) &&
         actual_len < MAX_RESPONSE_SIZE) {
    if (received_len < 0) {
      free(buffer);
      return connection_problem_malloced;
    }
    received = true;
#ifdef DEBUG
    fprintf(stderr, "Received %d bytes\n", received_len);
#endif

    actual_len += received_len;
    char *tempPtr = buffer;
    buffer = (char *)realloc(buffer, actual_len + step_len);
    if (buffer == NULL) {
      free(tempPtr);
      return NULL;
    }
  }
  alarm(0);

  free(connection_problem_malloced);
  *response_size = actual_len;
  return received ? (check_crlf_format(buffer, actual_len) ? buffer : NULL)
                  : NULL;
}

char *client_tcp(char *req, char *hostname) {
  int s; /* connected socket descriptor */
  struct addrinfo hints, *res;
  long timevar;                   /* contains time returned by time() */
  struct sockaddr_in myaddr_in;   /* for local socket address */
  struct sockaddr_in servaddr_in; /* for server socket address */
  int i, j, errcode;
  socklen_t addrlen;

  const char *internal_error = "Internal error\r\n";
  char *internal_error_malloced = (char *)malloc(strlen(internal_error) + 1);
  if (internal_error_malloced == NULL) {
    return NULL;
  }

  struct sigaction vec;
  vec.sa_handler = handler_ctcp;
  vec.sa_flags = 0;

  if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1) {
    perror("[client_TCP] sigaction(SIGALRM)");
#ifdef DEBUG
    fprintf(stderr, "[client_TCP]: unable to register the SIGALRM signal\n");
#endif
    return internal_error_malloced;
  }
  /* Create the socket. */
  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) {
#ifdef DEBUG
    fprintf(stderr, "[client_tcp] unable to create socket\n");
#endif
    size_t return_len = strlen("Error creating socket to reach ") +
                        strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr,
              "[client_tcp] Error creating socket to reach {hostname}\r\n");
#endif
      return internal_error_malloced;
    }
    sprintf(return_str, "Error creating socket to reach %s\r\n", hostname);
    return return_str;
  }

  /* clear out address structures */
  memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
  memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

  /* Set up the peer address to which we will connect. */
  servaddr_in.sin_family = AF_INET;

  /* Get the host information for the hostname that the
   * user passed in. */
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  /* esta funci�n es la recomendada para la compatibilidad con IPv6
   * gethostbyname queda obsoleta*/
  errcode = getaddrinfo(hostname, NULL, &hints, &res);
  if (errcode != 0) {
/* Name was not found.  Return a
 * special value signifying the error. */
#ifdef DEBUG
    fprintf(stderr, "[client_tcp] No es posible resolver la IP de %s\n",
            hostname);
#endif
    size_t return_len = strlen("Error resolving hostname ") + strlen(hostname) +
                        strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_tcp] Error resolving hostname {hostname}\r\n");
#endif
      freeaddrinfo(res);
      return internal_error_malloced;
    }
    sprintf(return_str, "Error resolving hostname %s\r\n", hostname);
    freeaddrinfo(res);
    return return_str;
  } else {
    /* Copy address of host */
    servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
  }
  freeaddrinfo(res);

  /* puerto del servidor en orden de red*/
  servaddr_in.sin_port = htons(PUERTO);
  if (connect(s, (const struct sockaddr *)&servaddr_in,
              sizeof(struct sockaddr_in)) == -1) {
#ifdef DEBUG
    fprintf(stderr, "[client_tcp] unable to connect to remote\n");
#endif
    size_t return_len =
        strlen("Error connecting to ") + strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_tcp] Error connecting to {hostname}\r\n");
#endif
      return internal_error_malloced;
    }
    sprintf(return_str, "Error connecting to %s\r\n", hostname);
    return return_str;
  }
  addrlen = sizeof(struct sockaddr_in);
  if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
#ifdef DEBUG
    fprintf(stderr, "[client_tcp] unable to read socket address\n");
#endif
    size_t return_len = strlen("Error reading socket address\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_tcp] Error reading socket address\n");
#endif
      return internal_error_malloced;
    }
    sprintf(return_str, "Error reading socket address\n");
    return return_str;
  }

/* Print out a startup message for the user. */
#ifdef DEBUG
  time(&timevar);
  printf("Connected to localhost on port %u at %s", ntohs(myaddr_in.sin_port),
         (char *)ctime(&timevar));
#endif

  // Send request to server
  int response_size;
  char *response =
      TCP_send_close_send_and_wait_server_request(s, req, &response_size);
  if (response == NULL) {
#ifdef DEBUG
    fprintf(stderr, "[client_tcp] Error receiving response\n");
#endif
    size_t return_len = strlen("Error receiving response\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_tcp] Error receiving response\n");
#endif
      return internal_error_malloced;
    }
    sprintf(return_str, "Error receiving response\n");
    return return_str;
  }

  // Add null terminator to response
  response = (char *)realloc(response, response_size + 1);
  response[response_size] = '\0';

  /* Print message indicating completion of task. */
#ifdef DEBUG
  time(&timevar);
  printf("All done at %s", (char *)ctime(&timevar));
#endif

  free(internal_error_malloced);

  return response;
}

#endif