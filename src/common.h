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

#endif