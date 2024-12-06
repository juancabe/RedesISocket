#ifndef COMMON_H
#define COMMON_H

#include <sys/types.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/errno.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <stdbool.h>

#define PUERTO 19688
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define BUFFERSIZE 1024         /* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128
#define LOG_FILENAME "server_log.txt"
#define RETRIES 5
#define TIMEOUT 5

bool check_crlf_format(char *buffer, int len)
{
  bool found = false;

  if (len < 2)
  {
    fprintf(stderr, "[CHECK CRLF] Buffer too short\n");
    return false;
  }

  for (int i = 0; i < len; i++)
  {
    if (buffer[i] == '\n')
    {
      if (i == 0 || !(buffer[i - 1] != '\r'))
      {
        fprintf(stderr, "[CHECK CRLF] No \\r before \\n on str\n");
        return false;
      }
      else
      {
        found = true;
      }
    }
  }

  if (!found)
  {
    fprintf(stderr, "[CHECK CRLF] No \\n found on str\n");
    fprintf(stderr, "%s\n", buffer);
    return false;
  }

  if (buffer[len - 1] != '\n' || buffer[len - 2] != '\r')
  {
    fprintf(stderr, "[CHECK CRLF] Last two characters are not \\r\\n on str\n");
    fprintf(stderr, "%s\n", buffer);
    return false;
  }

  return true;
}

#endif