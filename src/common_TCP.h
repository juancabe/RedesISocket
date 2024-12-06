#ifndef COMMON_TCP_H
#define COMMON_TCP_H

#include "common.h"

char *TCP_send_and_wait_server_request(int s, char *request, int *response_size)
{
  if (send(s, request, strlen(request), 0) != strlen((request)))
  {
    return NULL;
  }

  // Receive response from server
  const int step_len = 1024;
  int received_len, actual_len = 0;
  char *buffer = malloc(step_len);
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
  }

  *response_size = actual_len;
  return buffer;
}

#endif