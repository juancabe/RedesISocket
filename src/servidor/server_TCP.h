#ifndef SERVER_TCP_H
#define SERVER_TCP_H

#include "../common_TCP.h"
#include "common_server.h"
#include "compose_finger.h"
#include "parse_client_request.h"

#include "../cliente/client_tcp.h"
#include <unistd.h>
void handler_server(int signum) {
#ifdef DEBUG
  printf("[server_TCP]Alarma recibida \n");
#endif
}

// perror, close socket, exit
void perrout_TCP(int socket) {
  perror("[server_TCP] ERROR");
  close(socket);
  exit(1);
}

// Receive one message, i.e. until \r\n
static char *receive_one_message(char *hostname, int s) {
  const int step_len = 1024;
  int received_len, actual_len = 0;
  char *buffer = (char *)malloc(step_len);
  if (buffer == NULL) {
    return NULL;
  }

  // Receive until \r\n, peer closes connection or timeout
  alarm(TIMEOUT);
  while ((received_len = recv(s, buffer + actual_len, step_len, 0))) {
    if (received_len < 0)
      return NULL;

    actual_len += received_len;
    char *tempPtr = buffer;
    buffer = (char *)realloc(buffer, actual_len + step_len);
    if (buffer == NULL) {
      free(tempPtr);
      return NULL;
    }

    if (strstr(buffer, "\r\n") != NULL) {
      break;
    }
  }
  alarm(0);

  if (check_crlf_format(buffer, actual_len) == false) {
#ifdef DEBUG
    fprintf(stderr, "[server_TCP] Bad format in\n");
#endif
    return NULL;
  }

  return buffer;
}

void serverTCP(int s, struct sockaddr_in clientaddr_in) {
  char SERVER_NAME[] = "serverTCP";
  int reqcnt = 0;                /* keeps count of number of requests */
  char remote_hostname[MAXHOST]; /* remote host's name string */

  struct hostent *hp; /* pointer to host info for remote host */

  /* Registrar SIGALRM para no quedar bloqueados en los recvfrom */

  const char *internal_error = "Internal error\r\n";
  char *internal_error_malloced = (char *)malloc(strlen(internal_error) + 1);
  if (internal_error_malloced == NULL) {
#ifdef DEBUG
    fprintf(stderr, "[server_TCP]: unable to register MALLOC error\n");
#endif
    perrout_TCP(s);
  }

  struct sigaction vec;
  vec.sa_handler = handler_server;
  vec.sa_flags = 0;

  if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1) {
    perror("[server_TCP] sigaction(SIGALRM)");
#ifdef DEBUG
    fprintf(stderr, "[server_TCP]: unable to register the SIGALRM signal\n");
#endif
    perrout_TCP(s);
  }

  int status = getnameinfo((struct sockaddr *)&clientaddr_in, sizeof(clientaddr_in), remote_hostname, MAXHOST, NULL, 0, 0);
  if (status && (inet_ntop(AF_INET, &(clientaddr_in.sin_addr), remote_hostname, MAXHOST) == NULL)) {
    perror(" inet_ntop \n");
#ifdef DEBUG
    fprintf(stderr, "%s: unable to get name\n", SERVER_NAME);
#endif
    perrout_TCP(s);
  }
#ifdef DEBUG
  /* Log a startup message. */
  long timevar; /* contains time returned by time() */
  time(&timevar);
  printf("Startup from %s port %u at %s", remote_hostname, ntohs(clientaddr_in.sin_port), (char *)ctime(&timevar));
#endif

  /* Set the socket for a lingering, graceful close.
   * This will cause a final close of this socket to wait (1 sec) until all of the
   * data sent on it has been received by the remote host if the remote host
   * has closed its socket before all of the data has been received.
  struct linger linger;

  linger.l_onoff = 1;
  linger.l_linger = 1;
  if (setsockopt(s, SOL_SOCKET, SO_LINGER, &linger, sizeof(linger)) == -1) {
    perrout_TCP(s);
  }
   */

  // First message should be one line
  char *buffer = receive_one_message(remote_hostname, s);
  reqcnt++;
  if (buffer == NULL) {
    perrout_TCP(s);
  }

  // Now we must parse client's message and respond to it
  char *username = NULL;
  bool username_malloced = false;
  char *hostname = NULL;
  parse_client_request_return ret = parse_client_request(buffer, &hostname, &username);
  free(buffer);
  username_malloced = true;
  char *response = NULL;
  bool response_malloced = false;

  // Now we must compose the response, i.e. call composeFinger
  switch (ret) {
  case USERNAME:
    if (username == NULL) {
      response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
      username_malloced = false;
    } else {
      response = just_one_user_info(username);
      response_malloced = true;
    }
    break;
  case ERROR:
    response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
    break;
  case NO_USERNAME_NO_HOSTNAME:
    response = all_users_info();
    if (response == NULL) {
      response = internal_error_malloced;
    }
    response_malloced = true;
    break;
  case HOSTNAME_REDIRECT:
    // Username should be malloced and ending with \r\n
    if (username == NULL) {
      username = (char *)malloc(3);
      if (username == NULL) {
        response = internal_error_malloced;
        response_malloced = true;
        break;
      }
      strcpy(username, "\r\n");
      username_malloced = true;
    } else {
      username = (char *)realloc(username, strlen(username) + 3);
      if (username == NULL) {
        response = internal_error_malloced;
        response_malloced = true;
        break;
      }
      strcat(username, "\r\n");
    }
    if (hostname == NULL) {
      response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
    } else {
#ifdef DEBUG
      if (move_hostnames(&username, &hostname)) {
        fprintf(stderr, "Hostnames moved\n");
      } else {
        fprintf(stderr, "Hostnames not moved\n");
      }
#else
      move_hostnames(&username, &hostname);
#endif
#ifdef DEBUG
      fprintf(stderr, "calling client_tcp(%s, %s)\n", username ? username : "NULL", hostname ? hostname : "NULL");
#endif
      response = client_tcp(username, hostname); // Username is the new request
      response_malloced = true;
    }

    break;
  default:
    response = "Unknown error\r\n";
    break;
  }

  if (response == NULL) {
    response = "Error during generating correct response\r\n";
    response_malloced = false;
  }
#ifdef DEBUG
  fprintf(stderr, "About to send %ld bytes\n", strlen(response));
#endif
  // Now we must send the response to the client
  if (send(s, response, strlen(response), 0) != strlen(response))
  // \0 no se envia, acaba con  \r\n
  {
    perrout_TCP(s);
  }
#ifdef DEBUG
  fprintf(stderr, "Response sent\n");
#endif
  // https://blog.netherlabs.nl/articles/2009/01/18/the-ultimate-so_linger-page-or-why-is-my-tcp-not-reliable
  // Close sending channel, the client alredy closed theirs
  if (shutdown(s, SHUT_WR) == -1) {
    perrout_TCP(s);
  }
#ifdef IFF_DEBUG
  fprintf(stderr, "Shutdown sent\n");
#endif
  // Now we must close the connection
  close(s);

#ifdef DEBUG
  time(&timevar);
  printf("Completed %s port %u, %d requests, at %s\n", hostname, ntohs(clientaddr_in.sin_port), reqcnt, (char *)ctime(&timevar));
#endif
  // Free memory
  if (response_malloced)
    free(response);
  if (username_malloced && (username != NULL))
    free(username);
  if (hostname != NULL)
    free(hostname);

  return;
}

#endif