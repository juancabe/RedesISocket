#ifdef COMMON_CLIENT_TCP_H
#define COMMON_CLIENT_TCP_H

#include "../common.h"

// Receive all DATA until client closes connection
static char *receive_data_until_connection_closed(char *hostname, int s)
{
  const int step_len = 1024;
  int received_len, actual_len = 0;
  char *buffer = malloc(step_len);
  if (buffer == NULL)
  {
    errout(hostname);
    return NULL;
  }
  while (received_len = recv(s, buffer + actual_len, step_len, 0))
  {
    if (received_len < 0)
      errout(hostname);

    actual_len += received_len;
    char *tempPtr = buffer;
    buffer = realloc(buffer, actual_len + step_len);
    if (buffer == NULL)
    {
      errout(hostname);
      free(tempPtr);
    }
  }

  return buffer;
}

#endif