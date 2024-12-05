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

#define PUERTO 17278
#define ADDRNOTFOUND 0xffffffff /* return address for unfound host */
#define BUFFERSIZE 1024         /* maximum size of packets to be received */
#define TAM_BUFFER 10
#define MAXHOST 128
#define LOG_FILENAME "server_log.txt"

extern int errno;

/*
 *	This routine aborts the child process attending the client.
 */
void errout(char *hostname)
{
  printf("Connection with %s aborted on error\n", hostname);
  exit(1);
}

/*

An RUIP MUST accept the entire Finger query specification.
   The Finger query specification is defined:
        {Q1}    ::= [{W}|{W}{S}{U}]{C}
        {Q2}    ::= [{W}{S}][{U}]{H}{C}
        {U}     ::= username
        {H}     ::= @hostname | @hostname{H}
        {W}     ::= /W
        {S}     ::= <SP> | <SP>{S}
        {C}     ::= <CRLF>
Zimmerman
*/

typedef enum
{
  HOSTNAME_REDIRECT,       // Found a hostname
  USERNAME,                // Just a username
  NO_USERNAME_NO_HOSTNAME, // No username or hostname
  ERROR
} parse_client_request_return;

static bool check_CRLF(char *ptr)
{
  return *ptr == '\r' && *(ptr + 1) == '\n';
}

static bool skip_spaces(char **ptr)
{
  bool found = false;
  while (**ptr == ' ')
  {
    found = true;
    (*ptr)++;
  }
  return found;
}

static bool check_end(char *ptr)
{
  return *ptr == '\0';
}

static bool check_barw(char *ptr)
{
  return *ptr == '/' && *(ptr + 1) == 'W';
}

static bool check_at_whole_str(char *ptr)
{
  while (*(ptr) != '\0' && *(ptr++) != '@')
    ;
  return *ptr == '\0';
}

// Expecting a username ex: i0960231
static int STATE_username(
    char *ptr,
    char **out_username,
    char **out_hostname,
    bool *username_set_out,
    bool *hostname_set_out,
    bool must_have_hostname)
{
  if (check_CRLF(ptr) || check_end(ptr))
  {
    return ERROR;
  }

  if (*ptr == '@')
  {
    return ERROR;
  }

  char *username_start = ptr;

  while (*ptr != ' ' && *ptr != '@' && !check_CRLF(ptr) && !check_end(ptr))
  {
    ptr++;
  }

  // Now ptr is pointing to a space, @, CRLF or end of string
  // Only CRLF or @ is valid

  // Check that we read something
  if (ptr == username_start)
  {
    return ERROR;
  }

  if (check_CRLF(ptr)) // Check CRLF
  {
    *out_username = username_start;
    *username_set_out = true;
    if (must_have_hostname)
    {
      return ERROR;
    }
    else
    {
      return USERNAME;
    }
  }
  else if (*ptr == '@') // Check @
  {
    *out_username = username_start;
    *username_set_out = true;
    ptr++;
    if (check_CRLF(ptr) || check_end(ptr) || skip_spaces(&ptr))
    {
      return ERROR;
    }
    else
    {
      // TODO check hostname
      exit(-1);
    }
  }
  else
  {
    return ERROR;
  }
}

int parse_client_request(char *in_buf, char *out_hostname, char *out_username)
{
  char *ptr = in_buf;
  bool found;
  bool username_set_out = false;
  bool hostname_set_out = false;
  bool must_have_hostname = false;

  if (ptr == NULL || check_end(ptr))
  {
    return ERROR;
  }

  if (check_at_whole_str(ptr))
  {
    must_have_hostname = true;
  }

  // It must start with anything but a space
  if (*ptr == ' ')
  {
    return ERROR;
  }

  // First thing can be /W, a @hostname or a username

  if (check_barw(ptr))
  {
    ptr += 2;
    // After /W, it can be a space or a CRLF
    if (skip_spaces(&ptr))
    {
      // after spaces, it can be username or hostname, no CRLF or \0
      if (check_CRLF(ptr) || check_end(ptr))
      {
        return ERROR;
      }
      else if ((*ptr) == '@')
      {
        // TODO STATE_hostname
        exit(-1);
      }
      else
      {
        return STATE_username(ptr, &out_username, &out_hostname, &username_set_out, &hostname_set_out, must_have_hostname);
      }
    }
  }
}

#endif