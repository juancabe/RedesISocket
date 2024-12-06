#ifndef COMMON_TCP_H
#define COMMON_TCP_H

#include "common.h"

char *TCP_send_and_wait_server_request(int s, char *request, int *response_size)
{
  if (send(s, request, strlen(request), 0) != strlen((request)))
  {
    return NULL;
  }

  // Receive response from server, until he closes connection
  const int step_len = 1024;
  int received_len, actual_len = 0;
  char *buffer = malloc(step_len);
  bool received = false;
  while (received_len = recv(s, buffer + actual_len, step_len, 0))
  {
    received = true;
    printf("Received %d bytes\n", received_len);
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

  *response_size = actual_len;
  return received ? buffer : NULL;
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

  return buffer;
}

#endif