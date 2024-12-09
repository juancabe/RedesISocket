#ifndef SERVER_UDP_H
#define SERVER_UDP_H

#include "../cliente/client_udp.h"
#include "common_server.h"
#include "compose_finger.h"
#include "parse_client_request.h"
#include <stdio.h>

// In order to fork UDP, we must do the first recv in the parent process
// and then fork the child process to handle the request

char *preprocess_UDP_request(int s, struct sockaddr_in *clientaddr_in, socklen_t *addrlen) {
  // The recv must be done in the parent process, this function will be called
  // in the parent process
  char *buffer = (char *)malloc(TAM_BUFFER_IN_UDP);
  *addrlen = sizeof(struct sockaddr_in);
  ssize_t cc = recvfrom(s, buffer, TAM_BUFFER_IN_UDP - 1, 0, (struct sockaddr *)clientaddr_in, addrlen);
  if (cc == -1) {
    perror("recvfrom");
    free(buffer);
    return NULL;
  }
  buffer[cc] = '\0';
  return buffer;
}

void perrout_UDP(int socket) {
  perror("[server_UDP] ERROR");
  close(socket);
  exit(1);
}

void serverUDP(char *buffer, int s, struct sockaddr_in clientaddr_in, socklen_t addrlen) {
  struct in_addr reqaddr; /* for requested host's address */
  struct hostent *hp;     /* pointer to host info for requested host */
  int nc, errcode;

  struct addrinfo hints, *res;

  memset(&hints, 0, sizeof(hints));
  hints.ai_family = AF_INET;

#ifdef DEBUG
  fprintf(stderr, "[serverUDP] Received message: %s\n", buffer);
#endif

  const char *internal_error = "Internal error\r\n";
  char *internal_error_malloced = (char *)malloc(strlen(internal_error) + 1);
  if (internal_error_malloced == NULL) {
#ifdef DEBUG
    fprintf(stderr, "[server_TCP]: unable to register MALLOC error\n");
#endif
    perrout_UDP(s);
  }

  // Now we must parse client's message and respond to it
  char *username = NULL;
  char *hostname = NULL;
  parse_client_request_return ret = parse_client_request(buffer, &hostname, &username);
  bool username_malloced = true;
  char *response = NULL;
  bool response_malloced = false;

#ifdef DEBUG
  printf("[serverUDP] ret: %d\n", ret);
  if (username)
    printf("[serverUDP] username: %s\n", username);
  else
    printf("[serverUDP] username: NULL\n");
  if (hostname)
    printf("[serverUDP] hostname: %s\n", hostname);
  else
    printf("[serverUDP] hostname: NULL\n");
#endif

  // Now we must compose the response, i.e. call composeFinger
  switch (ret) {
  case USERNAME:
    if (username == NULL) {
      response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
      username_malloced = false;
    } else {
      response = just_one_user_info(username);
      if (response == NULL) {
        response = internal_error_malloced;
        response_malloced = false;
      } else
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
      response_malloced = false;
    } else
      response_malloced = true;
    break;
  case HOSTNAME_REDIRECT:
    // Username should be malloced for move_hostnames
    if (username == NULL) {
      username = (char *)malloc(3);
      if (username == NULL) {
        response = internal_error_malloced;
        response_malloced = false;
        break;
      }
      strcpy(username, "\r\n");
      username_malloced = true;
    } else {
      username = (char *)realloc(username, strlen(username) + 3);
      if (username == NULL) {
        response = internal_error_malloced;
        response_malloced = false;
        break;
      }
      strcat(username, "\r\n");
    }

    if (hostname == NULL) {
      response = "Your request is invalid. Expected {[username][@hostname]\\r\\n}\r\n";
      response_malloced = false;
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
      fprintf(stderr, "calling client_udp(%s, %s)\n", username ? username : "NULL", hostname ? hostname : "NULL");
#endif
      response = client_udp(username, hostname, RETRIES / 2); // Username is the new request
      if (response == NULL) {
        response = internal_error_malloced;
        response_malloced = false;
      } else
        response_malloced = true;
    }
    break;
  default:
    response = "Unknown error\r\n";
    response_malloced = false;
    break;
  }

  if (response == NULL) {
    response = "Error during generating correct response\r\n";
    response_malloced = false;
  } else if (strlen(response) > TAM_BUFFER_OUT_UDP) {
    if (response_malloced)
      free(response);
    response_malloced = false;
    response = "Response doesn't fit in UDP packet\r\n";
  }

#ifdef DEBUG
  printf(("[serverUDP] sending response: %s\n"), response ? response : "NO RESPONSE");
#endif

  addrlen = sizeof(struct sockaddr_in);
  // Now we must send the response to the client
  if (sendto(s, response, strlen(response), 0, (struct sockaddr *)&clientaddr_in, addrlen) != strlen(response)) {
    perrout_UDP(s);
  }

  // Free memory
  if (response_malloced && response)
    free(response);
  if (username_malloced && username)
    free(username);
  if (hostname)
    free(hostname);
  if (buffer)
    free(buffer);

  close(s);
  // ALL Done
}

#endif