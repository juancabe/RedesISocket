#ifndef CLIENT_UDP_H
#define CLIENT_UDP_H
#include "common_client.h"

extern int errno;

void handler_cudp(int signum) {
#ifdef DEBUG
  printf("[client_UDP]Alarma recibida \n");
#endif
}

client_return client_udp(char *request, char *hostname, int timeout) {
  int errcode;
  int retry = RETRIES;
  int s;
  struct sockaddr_in myaddr_in;
  struct sockaddr_in servaddr_in;
  struct in_addr reqaddr;
  socklen_t addrlen;
  int n_retry;
  client_return ret;
  ret.eport = 0;

  struct addrinfo hints, *res;

  if (strlen(request) > TAM_BUFFER_OUT_UDP) {
#ifdef DEBUG
    fprintf(stderr, "[client_udp]: request too long\n");
#endif
    // return "Request too long when trying to reach {hostname}\r\n"; return
    // this but alloced and correctly formated
    size_t return_len = strlen("Request too long when trying to reach ") + strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)(char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_udp]: Error creating error message\r\n");
#endif
      ret.response = NULL;
      ret.socket = -1;
      return ret;
    }
    sprintf(return_str, "Request too long when trying to reach %s\r\n", hostname);
    ret.response = return_str;
    ret.socket = -1;
    return ret;
  }

  /* Create the socket. */
  s = socket(AF_INET, SOCK_DGRAM, 0);
  if (s == -1) {
#ifdef DEBUG
    fprintf(stderr, "[client_udp]: unable to create socket\n");
#endif
    // return "Error creating socket to reach {hostname}\r\n"; return this but
    // alloced and correctly formated
    size_t return_len = strlen("Error creating socket to reach ") + strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_udp]: Error creating socket to reach {hostname}\r\n");
#endif
      ret.response = NULL;
      ret.socket = -1;
      return ret;
    }
    sprintf(return_str, "Error creating socket to reach %s\r\n", hostname);
    ret.response = return_str;
    ret.socket = -1;
    return ret;
  }

  /* clear out address structures */
  memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
  memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

  myaddr_in.sin_family = AF_INET;
  myaddr_in.sin_port = 0;
  myaddr_in.sin_addr.s_addr = INADDR_ANY;
  if (bind(s, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1) {
#ifdef DEBUG
    fprintf(stderr, "[client_udp]: unable to bind socket\n");
#endif
    // return "Error binding socket to reach {hostname}\r\n"; return this but
    // alloced and correctly formated
    size_t return_len = strlen("Error binding socket to reach ") + strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_udp]: Error binding socket to reach {hostname}\r\n");
#endif
      ret.response = NULL;
      ret.socket = -1;
      return ret;
    }
    sprintf(return_str, "Error binding socket to reach %s\r\n", hostname);
    ret.response = return_str;
    ret.socket = -1;
    return ret;
  }

  addrlen = sizeof(struct sockaddr_in);
  if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1) {
#ifdef DEBUG
    fprintf(stderr, "[client_udp]: unable to read socket address\n");
#endif
    size_t return_len = strlen("Error reading socket address to reach ") + strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_udp]: Error reading socket address to reach {hostname}\r\n");
#endif
      ret.response = NULL;
      ret.socket = -1;
      return ret;
    }
    sprintf(return_str, "Error reading socket address to reach %s\r\n", hostname);
    ret.response = return_str;
    ret.socket = -1;
    return ret;
  }
  ret.eport = ntohs(myaddr_in.sin_port);

#ifdef DEBUG
  /* Print out a startup message for the user. */
  long timevar;
  time(&timevar);
  printf("[client_udp]: Socket binded to port %d", ntohs(myaddr_in.sin_port));
#endif

  /* Set up the server address. */
  servaddr_in.sin_family = AF_INET;
  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;
  errcode = getaddrinfo(hostname, NULL, &hints, &res);
  if (errcode != 0) {
/* Name was not found.  Return a
 * special value signifying the error. */
#ifdef DEBUG
    fprintf(stderr, "[client_udp]: No es posible resolver la IP de %s\n", hostname);
#endif
    size_t return_len = strlen("No es posible resolver la IP de ") + strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_udp]: Error resolving IP of {hostname}\r\n");
#endif
      ret.response = NULL;
      ret.socket = s;
      return ret;
    }
    sprintf(return_str, "No es posible resolver la IP de %s\r\n", hostname);
    ret.response = return_str;
    ret.socket = s;
    return ret;
  } else {
    /* Copy address of host */
    servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
  }
  freeaddrinfo(res);
  servaddr_in.sin_port = htons(PUERTO); // orden de red

  /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
  struct sigaction vec;
  vec.sa_handler = handler_cudp;
  vec.sa_flags = 0;
  if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1) {
    perror("[client_UDP] sigaction(SIGALRM)");
#ifdef DEBUG
    fprintf(stderr, "[client_udp]: unable to register the SIGALRM signal\n");
#endif
    size_t return_len = strlen("Functioning error to reach ") + strlen(hostname) + strlen("\r\n") + 1;
    char *return_str = (char *)malloc(return_len);
    if (return_str == NULL) {
#ifdef DEBUG
      fprintf(stderr, "[client_udp]: Error registering SIGALRM signal to reach "
                      "{hostname}\r\n");
#endif
      ret.response = NULL;
      ret.socket = s;
      return ret;
    }
    sprintf(return_str, "Functioning error to reach %s\r\n", hostname);
    ret.response = return_str;
    ret.socket = s;
    return ret;
  }

  n_retry = retry;
  alarm(timeout);
  while (n_retry > 0) {
    /* Send the request to the nameserver. */
    if (sendto(s, request, strlen(request), 0, (struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1) {
#ifdef DEBUG
      fprintf(stderr, "[client_udp]: unable to send request\n");
#endif
      size_t return_len = strlen("Error sending request to reach ") + strlen(hostname) + strlen("\r\n") + 1;
      char *return_str = (char *)malloc(return_len);
      if (return_str == NULL) {
#ifdef DEBUG
        fprintf(stderr, "[client_udp]: Error sending request to reach {hostname}\r\n");
#endif
        ret.response = NULL;
        ret.socket = s;
        return ret;
      }
      sprintf(return_str, "Error sending request to reach %s\r\n", hostname);
      ret.response = return_str;
      ret.socket = s;
      return ret;
    }

    alarm(timeout);
    char req_response[TAM_BUFFER_IN_UDP];
    ssize_t req_response_len = 0;
    /* Wait for the reply to come in. */
    if ((req_response_len = recvfrom(s, req_response, TAM_BUFFER_IN_UDP, 0, (struct sockaddr *)&servaddr_in, &addrlen)) == -1) {
      if (errno == EINTR) {
#ifdef DEBUG
        printf("attempt %d (retry %d).\n", n_retry, retry);
#endif
        n_retry--;
      } else {
#ifdef DEBUG
        printf("Unable to get response from %s\n", hostname);
#endif
#ifdef DEBUG
        printf("[client_udp] after %d attempts.\n", retry);
#endif
        size_t return_len = strlen("Unable to get response from ") + strlen(hostname) + 1;
        char *return_str = (char *)malloc(return_len);
        if (return_str == NULL) {
#ifdef DEBUG
          fprintf(stderr, "[client_udp]: Error getting response from {hostname}\r\n");
#endif
          ret.response = NULL;
          ret.socket = s;
          return ret;
        }
        sprintf(return_str, "Unable to get response from %s", hostname);
        ret.response = return_str;
        ret.socket = s;
        return ret;
      }
    } else {
      alarm(0); // Cancel the alarm
      if (check_crlf_format(req_response, req_response_len)) {
        /* Print out response. */
        char *with_null = (char *)malloc(req_response_len + 1);
        strncpy(with_null, req_response, req_response_len);
        with_null[req_response_len] = '\0';
        ret.response = with_null;
        ret.socket = s;
        return ret;
      } else {
#ifdef DEBUG
        printf("[client_udp] Error receiving response: format incorrect\n");
#endif
      }
      break;
    }
  }

  if (n_retry == 0) {
#ifdef DEBUG
    printf("Unable to get response from");
    printf("[client_udp] after %d attempts.\n", retry);
#endif
  }

  /* Close the socket. */
  close(s);
  size_t return_len = strlen("Unable to get response from ") + strlen(hostname) + 1;
  char *return_str = (char *)malloc(return_len);
  if (return_str == NULL) {
#ifdef DEBUG
    fprintf(stderr, "[client_udp]: Error getting response from {hostname}\r\n");
#endif
    ret.response = NULL;
    ret.socket = -1;
  }
  sprintf(return_str, "Unable to get response\r\n");
  ret.response = return_str;
  ret.socket = -1;
  return ret;
}

#endif