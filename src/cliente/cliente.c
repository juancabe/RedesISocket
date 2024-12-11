/*
** Fichero: cliente.c
** Autores:
** Juan Calzada Bernal DNI 70919688Q
** Hugo Chalard Collado DNI DNIHUGO
*/

#include "client_tcp.h"
#include "client_udp.h"

#include <stdio.h>

static int countDigits(int num) {
  int count = 0;

  if (num == 0) // Caso especial para el 0
    return 1;

  if (num < 0) // Considerar números negativos
    num = -num;

  while (num > 0) {
    count++;
    num /= 10;
  }

  return count;
}

int main(int argc, char **argv) {
  if (argc < 2 || argc > 3) {
    fprintf(stderr, "uso: %s {TCP|UDP} [request]\n", argv[0]);
    exit(1);
  }

  // Add \r\n to the request
  char *request = NULL;
  request = malloc((argc == 3 ? strlen(argv[2]) : 0) + 3);
  strcpy(request, argc == 3 ? argv[2] : "");
  strcat(request, "\r\n");

  client_return response = {0};

  if (strcmp(argv[1], "TCP") == 0) {
    response = client_tcp(request, "localhost");
  } else if (strcmp(argv[1], "UDP") == 0) {
    response = client_udp(request, "localhost", TIMEOUT * 2);
  } else {
    fprintf(stderr, "Protocolo no soportado: %s\n", argv[1]);
    exit(1);
  }
  char *filename;
  if (response.eport == 0)
    filename = "noPortAssigned.txt";
  else {
    filename = malloc(countDigits(response.eport) + 5);
    sprintf(filename, "%d.txt", response.eport);
  }
  FILE *file = fopen(filename, "w");
  if (file == NULL) {
    fprintf(stderr, "Error al abrir el archivo de output\n");
    exit(1);
  }

  if (response.response == NULL) {
    fprintf(file, "Error al recibir respuesta\n");
  } else {
    fprintf(file, "%s\n", response.response);
  }

  fclose(file);

  if (response.socket > 0) {
    close(response.socket);
  }

  return 0;
}