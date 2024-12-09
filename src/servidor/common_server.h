#ifndef COMMON_SERVER_H
#define COMMON_SERVER_H

#include "../common.h"

extern int errno;

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname) {
  printf("Connection with %s aborted on error\n", hostname);
  exit(1);
}

// Function to, when various hostnames are specified, move next hostnames to request
/*
                 user input: user@host1@host2@host3
  program input to function: username = user\r\n | hostname = host1@host2@host3
  program output of function: username = user@host2@host3\r\n | hostname = host1
*/
bool move_hostnames(char **username, char **hostname) {
  // username = user\r\n hostname = host1@host2@host3
  char *hostname_ptr = *hostname;

  while (*hostname_ptr != '\0' && *hostname_ptr != '@') {
    hostname_ptr++;
  }
  if (*hostname_ptr == '\0') {
    return false;
  }

  *hostname_ptr = '\0'; // hostname = host1
  hostname_ptr++;       // hostname_ptr = host2@host3

  // username = user\r\n hostname = host1
  // new username = user@host2@host3\r\n
  //                        it counts the \r\n    it lacks one @            @   \0
  size_t new_username_len = strlen(*username) + strlen(hostname_ptr) + 1 + 1;
  char *new_username = (char *)malloc(new_username_len);
  if (new_username == NULL) {
    return false;
  }
  strcpy(new_username, *username);
  // Eliminate \r\n
  new_username[strlen(*username) - 2] = '@';
  strcpy(new_username + strlen(*username) - 1, hostname_ptr);
  new_username[new_username_len - 1] = '\0';
  new_username[new_username_len - 2] = '\n';
  new_username[new_username_len - 3] = '\r';

  free(*username);
  *username = new_username;
  return true;
}

#endif