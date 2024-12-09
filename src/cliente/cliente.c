#include "client_tcp.h"
#include "client_udp.h"

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

  char *response = NULL;

  if (strcmp(argv[1], "TCP") == 0) {
    response = client_tcp(request, "localhost");
  } else if (strcmp(argv[1], "UDP") == 0) {
    response = client_udp(request, "localhost", TIMEOUT);
  } else {
    fprintf(stderr, "Protocolo no soportado: %s\n", argv[1]);
    exit(1);
  }

  if (response == NULL) {
    fprintf(stderr, "Error al recibir respuesta\n");
    exit(1);
  } else {
#ifdef SEND_BIG_CHUNK
    printf("Response length: %ld\n", strlen(response));
#else
    printf("%s", response);
#endif
  }

  return 0;
}