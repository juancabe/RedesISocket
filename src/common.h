/*
** Fichero: common.h
** Autores:
** Juan Calzada Bernal DNI 70919688Q
** Hugo Chalard Collado DNI DNIHUGO
*/

#ifndef COMMON_H
#define COMMON_H

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#define PUERTO 19688
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define TAM_BUFFER_OUT_UDP 5624 /* maximum size of packets to be sent */
#define TAM_BUFFER_IN_UDP 65535 /* maximum size of packets to be received */
#define MAXHOST 256
#define LOG_FILENAME "server_log.txt"
#define RETRIES 2
#define TIMEOUT 6
// #define DEBUG

typedef struct {
  char *response;
  int socket;
  int eport;
} client_return;

bool check_crlf_format(char *buffer, int len) {
  bool found = false;

  if (len < 2) {
#ifdef DEBUG
    fprintf(stderr, "[CHECK CRLF] Buffer too short\n");
#endif
    return false;
  }

  for (int i = 0; i < len; i++) {
    if (buffer[i] == '\n') {
      if (i == 0 || buffer[i - 1] != '\r') {
#ifdef DEBUG
        fprintf(stderr, "[CHECK CRLF] No \\r before \\n on str\n");
#endif
        return false;
      } else {
        found = true;
      }
    }
  }

  if (!found) {
#ifdef DEBUG
    fprintf(stderr, "[CHECK CRLF] No \\n found on str\n");
    buffer[20] = '\0';
    fprintf(stderr, "%s\n", buffer);

#endif
    return false;
  }

  if (buffer[len - 1] != '\n' || buffer[len - 2] != '\r') {
#ifdef DEBUG
    fprintf(stderr, "[CHECK CRLF] Last two characters are not \\r\\n on str\n");
    fprintf(stderr, "%s\n", buffer);
#endif
    return false;
  }

  return true;
}

#endif