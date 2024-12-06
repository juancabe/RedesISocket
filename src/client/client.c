#include "client_tcp.h"

int main(int argc, char **argv)
{
  if (argc < 2 || argc > 3)
  {
    fprintf(stderr, "uso: %s {TCP|UDP} [request]\n", argv[0]);
    exit(1);
  }

  // Add \r\n to the request
  char *request = NULL;
  request = malloc((argc == 3 ? strlen(argv[2]) : 0) + 3);
  strcpy(request, argc == 3 ? argv[2] : "");
  strcat(request, "\r\n");

  if (strcmp(argv[1], "TCP") == 0)
  {
    client_tcp(request);
  }
  else if (strcmp(argv[1], "UDP") == 0)
  {
    // TODO client_udp(request);
    {
      fprintf(stderr, "Error: TODO, no implementado\n");
      exit(1);
    }
  }
  else
  {
    fprintf(stderr, "Protocolo no soportado: %s\n", argv[1]);
    exit(1);
  }
}