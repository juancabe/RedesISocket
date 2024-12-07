#ifndef CLIENT_UDP_H
#define CLIENT_UDP_H
#include "common_client.h"

extern int errno;

void handler()
{
	printf("Alarma recibida \n");
}

int client_udp(char *request)
{
	int errcode;
	int retry = RETRIES;
	int s;
	struct sockaddr_in myaddr_in;
	struct sockaddr_in servaddr_in;
	struct in_addr reqaddr;
	int addrlen, n_retry;
	struct sigaction vec;
	struct addrinfo hints, *res;

	if (strlen(request) > TAM_BUFFER_OUT_UDP)
	{
		fprintf(stderr, "[client_udp]: request too long\n");
		exit(1);
	}

	/* Create the socket. */
	s = socket(AF_INET, SOCK_DGRAM, 0);
	if (s == -1)
	{
		fprintf(stderr, "[client_udp]: unable to create socket\n");
		exit(1);
	}

	/* clear out address structures */
	memset((char *)&myaddr_in, 0, sizeof(struct sockaddr_in));
	memset((char *)&servaddr_in, 0, sizeof(struct sockaddr_in));

	myaddr_in.sin_family = AF_INET;
	myaddr_in.sin_port = 0;
	myaddr_in.sin_addr.s_addr = INADDR_ANY;
	if (bind(s, (const struct sockaddr *)&myaddr_in, sizeof(struct sockaddr_in)) == -1)
	{
		fprintf(stderr, "[client_udp]: unable to bind socket\n");
		exit(1);
	}
	addrlen = sizeof(struct sockaddr_in);
	if (getsockname(s, (struct sockaddr *)&myaddr_in, &addrlen) == -1)
	{
		fprintf(stderr, "[client_udp]: unable to read socket address\n");
		exit(1);
	}

#ifdef DEBUG
	/* Print out a startup message for the user. */
	long timevar;
	time(&timevar);
	printf("Connected to %s on port %u at %s", HOSTNAME, ntohs(myaddr_in.sin_port), (char *)ctime(&timevar));
#endif

	/* Set up the server address. */
	servaddr_in.sin_family = AF_INET;
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	errcode = getaddrinfo(HOSTNAME, NULL, &hints, &res);
	if (errcode != 0)
	{
		/* Name was not found.  Return a
		 * special value signifying the error. */
		fprintf(stderr, "[client_udp]: No es posible resolver la IP de %s\n", HOSTNAME);
		exit(1);
	}
	else
	{
		/* Copy address of host */
		servaddr_in.sin_addr = ((struct sockaddr_in *)res->ai_addr)->sin_addr;
	}
	freeaddrinfo(res);
	servaddr_in.sin_port = htons(PUERTO); // orden de red

	/* Registrar SIGALRM para no quedar bloqueados en los recvfrom */
	vec.sa_handler = (void *)handler;
	vec.sa_flags = 0;
	if (sigaction(SIGALRM, &vec, (struct sigaction *)0) == -1)
	{
		perror(" sigaction(SIGALRM)");
		fprintf(stderr, "[client_udp]: unable to register the SIGALRM signal\n");
		exit(1);
	}

	n_retry = RETRIES;

	while (n_retry > 0)
	{

		{
			/* Send the request to the nameserver. */
			if (sendto(s, request, strlen(request), 0, (struct sockaddr *)&servaddr_in,
								 sizeof(struct sockaddr_in)) == -1)
			{
				fprintf(stderr, "[client_udp]: unable to send request\n");
				exit(1);
			}

			alarm(TIMEOUT);
			char req_response[TAM_BUFFER_IN_UDP];
			ssize_t req_response_len = 0;
			/* Wait for the reply to come in. */
			if ((req_response_len = recvfrom(s, req_response, TAM_BUFFER_IN_UDP, 0, (struct sockaddr *)&servaddr_in, &addrlen)) == -1)
			{
				if (errno == EINTR)
				{
					/* Alarm went off and aborted the receive.
					 * Need to retry the request if we have
					 * not already exceeded the retry limit.
					 */
					printf("attempt %d (retries %d).\n", n_retry, RETRIES);
					n_retry--;
				}
				else
				{
					printf("Unable to get response from");
					exit(1);
				}
			}
			else
			{
				alarm(0); // Cancel the alarm
				if (check_crlf_format(req_response, req_response_len))
				{
					/* Print out response. */
					char *with_null = malloc(req_response_len + 1);
					strncpy(with_null, req_response, req_response_len);
					with_null[req_response_len] = '\0';
					// printf("%s\n", with_null);
					free(with_null);
				}
				else
				{
					printf("[client_udp] Error receiving response: format incorrect\n");
				}
				break;
			}
		}
	}

	if (n_retry == 0)
	{
		printf("Unable to get response from");
		printf("[client_udp] after %d attempts.\n", RETRIES);
	}
}

#endif