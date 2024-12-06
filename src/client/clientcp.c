#include "../common.h"
#include "../common_TCP.h"

int main(argc, argv)
int argc;
char *argv[];
{
	int s; /* connected socket descriptor */
	struct addrinfo hints, *res;
	long timevar;										/* contains time returned by time() */
	struct sockaddr_in myaddr_in;		/* for local socket address */
	struct sockaddr_in servaddr_in; /* for server socket address */
	int addrlen, i, j, errcode;
	/* This example uses TAM_BUFFER byte messages. */
	char buf[TAM_BUFFER];

	if (argc != 2)
	{
		fprintf(stderr, "Usage:  %s <remote host>\n", argv[0]);
		exit(1);
	}

	/* Create the socket. */
	s = socket(AF_INET, SOCK_STREAM, 0);
	if (s == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to create socket\n", argv[0]);
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
	errcode = getaddrinfo(argv[1], NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		fprintf(stderr, "%s: No es posible resolver la IP de %s\n",
						argv[0], argv[1]);
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
		perror(argv[0]);
		fprintf(stderr, "%s: unable to connect to remote\n", argv[0]);
		exit(1);
	}
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		perror(argv[0]);
		fprintf(stderr, "%s: unable to read socket address\n", argv[0]);
		exit(1);
	}

	/* Print out a startup message for the user. */
	time(&timevar);
	printf("Connected to %s on port %u at %s",
				 argv[1], ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));

	if (1)
	{
		// Send request to server (function must be shared between server and client)
		char *request = "i0919688\r\n";
		int response_size;
		char *response = TCP_send_and_wait_server_request(s, request, &response_size);
		/*
		// Add null terminator to response
		response = realloc(response, response_size + 1);
		response[response_size] = '\0';
		printf("[CLIENT TCP] Message received: %s\n", response);
		*/
		// Print server response

		printf("[CLIENT TCP] Response size: %d\n", response_size);

		char *tempPtr = response;
		do
		{
			printf("%c", *tempPtr);
			tempPtr++;
		} while (*tempPtr != '\0');
		printf("\n");
	}
	else
	{ // EXAMPLE
		for (i = 1; i <= 5; i++)
		{
			*buf = i;
			if (send(s, buf, TAM_BUFFER, 0) != TAM_BUFFER)
			{
				fprintf(stderr, "%s: Connection aborted on error ", argv[0]);
				fprintf(stderr, "on send number %d\n", i);
				exit(1);
			}
		}
		if (shutdown(s, 1) == -1)
		{
			perror(argv[0]);
			fprintf(stderr, "%s: unable to shutdown socket\n", argv[0]);
			exit(1);
		}
		while (i = recv(s, buf, TAM_BUFFER, 0))
		{
			if (i == -1)
			{
				perror(argv[0]);
				fprintf(stderr, "%s: error reading result\n", argv[0]);
				exit(1);
			}
			while (i < TAM_BUFFER)
			{
				j = recv(s, &buf[i], TAM_BUFFER - i, 0);
				if (j == -1)
				{
					perror(argv[0]);
					fprintf(stderr, "%s: error reading result\n", argv[0]);
					exit(1);
				}
				i += j;
			}
			/* Print out message indicating the identity of this reply. */
			printf("Received result number %d\n", *buf);
		}
	}

	/* Print message indicating completion of task. */
	time(&timevar);
	printf("All done at %s", (char *)ctime(&timevar));
}
