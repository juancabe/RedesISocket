#include "client_tcp.h"

int main(int argc, char **argv)
{
  if (argc != 3)
  {
    fprintf(stderr, "uso: %s {TCP|UDP} [request]\n", argv[0]);
    exit(1);
  }

  // Add \r\n to the request
  char *request = malloc(strlen(argv[2]) + 3);
  strcpy(request, argv[2]);
  strcat(request, "\r\n");

  if (strcmp(request, "TCP") == 0)
  {
    client_tcp(argv[2]);
  }
  else if (strcmp(request, "UDP") == 0)
  {
    // TODO client_udp(argv[2]);
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