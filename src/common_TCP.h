#ifndef COMMON_TCP_H
#define COMMON_TCP_H

#include "common.h"

// Request MUST be a null terminated string
char *TCP_send_and_wait_server_request(int s, char *request, int *response_size)
{
#ifdef DEBUG
  fprintf(stderr, "Sending request: %s\n", request);
#endif
  if (send(s, request, strlen(request), 0) != strlen((request)))
  {
    return NULL;
  }
#ifdef DEBUG
  fprintf(stderr, "Request sent\n");
#endif

  // Close sending channel
  /* TODO: Do this to avoid server waiting for more data and allow him to receive dynamically
  if (shutdown(s, SHUT_WR) == -1)
  {
    return NULL;
  }
  */

  // Receive response from server, until he closes connection
  const int step_len = 1024;
  int received_len, actual_len = 0;
  char *buffer = malloc(step_len);
  bool received = false;
#ifdef DEBUG
  fprintf(stderr, "Receiving response\n");
#endif
  // Receive until server closes connection
  while (received_len = recv(s, buffer + actual_len, step_len, 0))
  {
    // TODO timeout
    received = true;
#ifdef DEBUG
    fprintf(stderr, "Received %d bytes\n", received_len);
#endif
    if (received_len < 0)
      return NULL;

    actual_len += received_len;
    char *tempPtr = buffer;
    buffer = realloc(buffer, actual_len + step_len);
    if (buffer == NULL)
    {
      free(tempPtr);
      return NULL;
    }
  }

  if (received && check_crlf_format(buffer, actual_len) == false)
  {
    return NULL;
  }

  *response_size = actual_len;
  return received ? (check_crlf_format(buffer, actual_len) ? buffer
                                                           : NULL)
                  : NULL;
}

// Receive one message, i.e. until \r\n
static char *receive_one_message(char *hostname, int s)
{
  const int step_len = 1024;
  int received_len, actual_len = 0;
  char *buffer = malloc(step_len);
  if (buffer == NULL)
  {
    return NULL;
  }
  while (received_len = recv(s, buffer + actual_len, step_len, 0))
  {
    // TODO timeout
    if (received_len < 0)
      return NULL;

    actual_len += received_len;
    char *tempPtr = buffer;
    buffer = realloc(buffer, actual_len + step_len);
    if (buffer == NULL)
    {
      free(tempPtr);
      return NULL;
    }

    if (strstr(buffer, "\r\n") != NULL)
    {
      break;
    }
  }

  if (check_crlf_format(buffer, actual_len) == false)
  {
    return NULL;
  }

  return buffer;
}

#endif