#ifndef COMMON_SERVER_H
#define COMMON_SERVER_H

#include "../common.h"

extern int errno;

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
  printf("Connection with %s aborted on error\n", hostname);
  exit(1);
}

#endif