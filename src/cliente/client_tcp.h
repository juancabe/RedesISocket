#ifndef CLIENT_TCP_H
#define CLIENT_TCP_H

#include "common_client.h"
#include "../common_TCP.h"

int client_tcp(char *req)
{
	int s; /* connected socket descriptor */
	struct addrinfo hints, *res;
	long timevar;										/* contains time returned by time() */
	struct sockaddr_in myaddr_in;		/* for local socket address */
	struct sockaddr_in servaddr_in; /* for server socket address */
	int addrlen, i, j, errcode;

	/* Create the socket. */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
	{
		fprintf(stderr, "client_tcp: unable to create socket\n");
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	/* Set up the peer address to which we will connect. */
	servaddr_in.sin_family = AF_INET;

	/* Get the host information for the hostname that the
	 * user passed in. */
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	/* esta funci�n es la recomendada para la compatibilidad con IPv6 gethostbyname queda obsoleta*/
	errcode = getaddrinfo(HOSTNAME, NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		fprintf(stderr, "[client_tcp]: No es posible resolver la IP de localhost\n");
		exit(1);
	}
	else
	{
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);

	/* puerto del servidor en orden de red*/
	servaddr_in.sin_port = htons(PUERTO);
	if (connect(s, (const struct sockaddr *)&servaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		fprintf(stderr, "[client_tcp]: unable to connect to remote\n");
		exit(1);
	}
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		fprintf(stderr, "[client_tcp]: unable to read socket address\n");
		exit(1);
	}

/* Print out a startup message for the user. */
#ifdef DEBUG
	time(&timevar);
	printf("Connected to localhost on port %u at %s", ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));
#endif

	// Send request to server
	int response_size;
	char *response = TCP_send_close_send_and_wait_server_request(s, req, &response_size);
	if (response == NULL)
	{
		fprintf(stderr, "[CLIENT TCP] Error receiving response\n");
		exit(1);
	}

	// Add null terminator to response
	response = realloc(response, response_size + 1);
	response[response_size] = '\0';
	printf("%s", response);
	free(response);

	/* Print message indicating completion of task. */
#ifdef DEBUG
	time(&timevar);
	printf("All done at %s", (char *)ctime(&timevar));
#endif
}

#endif