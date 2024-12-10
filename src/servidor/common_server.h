#ifndef COMMON_SERVER_H
#define COMMON_SERVER_H

#include "../common.h"

extern int errno;

#include <time.h>

// Add these helper functions before serverUDP
void log_event(const char *event_type, const struct sockaddr_in *client_addr, const char *additional_info, const char *protocol) {
  FILE *log_file = fopen("peticiones.log", "a");
  if (log_file == NULL) {
    perror("Error opening log file");
    return;
  }

  time_t now;
  time(&now);
  char *date = ctime(&now);
  date[strlen(date) - 1] = '\0'; // Remove newline

  char host[NI_MAXHOST];
  char service[NI_MAXSERV];
  getnameinfo((struct sockaddr *)client_addr, sizeof(*client_addr), host, NI_MAXHOST, service, NI_MAXSERV, NI_NUMERICSERV);

  fprintf(log_file, "%s - %s: %s, IP: %s, Protocol: %s, Port: %s\n%s\n", date, event_type, host, inet_ntoa(client_addr->sin_addr), protocol, service, additional_info ? additional_info : "");

  fclose(log_file);
}

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