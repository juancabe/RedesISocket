#ifndef PARSE_CLIENT_REQUEST_H
#define PARSE_CLIENT_REQUEST_H

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#ifdef DEBUG
#include <stdio.h>
#endif

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
  while (*(ptr) != '\0' && *(ptr) != '@')
  {
    ptr++;
  }
  return *ptr != '\0';
}

// Expecting a hostname ex: @hostname
static int STATE_hostname(
    char *ptr,
    char **out_username,
    char **out_hostname,
    bool *username_set_out,
    bool *hostname_set_out,
    bool must_have_hostname)
{
  if (*ptr != '@')
  {
    return ERROR;
  }

#ifdef DEBUG
  printf("[STATE_hostname] till here\n");
#endif

  ptr++;

  if (check_CRLF(ptr) || check_end(ptr))
  {
    return ERROR;
  }

  char *hostname_start = ptr;

  while (*ptr != ' ' && !check_CRLF(ptr) && !check_end(ptr))
  {
    ptr++;
  }

  // Now ptr is pointing to a space, CRLF or end of string
  // Only CRLF is valid

  // Check that we read something
  if (ptr == hostname_start)
  {
    return ERROR;
  }

#ifdef DEBUG
  printf("[STATE_hostname] hostname exists\n");
#endif

  if (check_CRLF(ptr)) // Check CRLF
  {
#ifdef DEBUG
    printf("[STATE_hostname] CRLF -> ");
#endif
    *out_hostname = malloc(ptr - hostname_start + 1);
    if (*out_hostname == NULL)
      return ERROR;
    strncpy(*out_hostname, hostname_start, ptr - hostname_start);
    (*out_hostname)[ptr - hostname_start] = '\0';

    *hostname_set_out = true;
    if (must_have_hostname)
    {
#ifdef DEBUG
      printf("HOSTNAME_REDIRECT\n");
#endif
      return HOSTNAME_REDIRECT;
    }
    else
    {
#ifdef DEBUG
      printf("ERROR\n");
#endif
      return ERROR;
    }
  }
  else
  {
#ifdef DEBUG
    printf("[STATE_hostname] CRLF not found\n");
#endif
    return ERROR;
  }
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

#ifdef DEBUG
  printf("[STATE_username] username exists\n");
#endif

  if (check_CRLF(ptr)) // Check CRLF
  {
#ifdef DEBUG
    printf("[STATE_username] CRLF -> ");
#endif
    *out_username = malloc(ptr - username_start + 1);
    if (*out_username == NULL)
      return ERROR;
    strncpy(*out_username, username_start, ptr - username_start);
    (*out_username)[ptr - username_start] = '\0';
    *username_set_out = true;
    if (must_have_hostname)
    {
#ifdef DEBUG
      printf("ERROR\n");
#endif
      return ERROR;
    }
    else
    {
#ifdef DEBUG
      printf("NO_USERNAME_NO_HOSTNAME\n");
#endif
      return USERNAME;
    }
  }
  else if (*ptr == '@') // Check @
  {
#ifdef DEBUG
    printf("[STATE_username] @ -> ");
#endif
    *out_username = malloc(ptr - username_start + 1);
    if (*out_username == NULL)
      return ERROR;
    strncpy(*out_username, username_start, ptr - username_start);
    (*out_username)[ptr - username_start] = '\0';

    *username_set_out = true;
    if (check_CRLF(ptr) || check_end(ptr) || skip_spaces(&ptr))
    {
#ifdef DEBUG
      printf("ERROR\n");
#endif
      return ERROR;
    }
    else
    {
#ifdef DEBUG
      printf("goto hostname\n");
#endif
      return STATE_hostname(ptr, out_username, out_hostname, username_set_out, hostname_set_out, must_have_hostname);
    }
  }
  else
  {
    return ERROR;
  }
}

static int username_or_hostname(char *ptr, char **out_username, char **out_hostname, bool *username_set_out, bool *hostname_set_out, bool must_have_hostname)
{
  if (check_CRLF(ptr) || check_end(ptr))
  {
    return ERROR;
  }

  if (*ptr == '@')
  {
#ifdef DEBUG
    printf("[username_or_hostname] Detected hostname\n");
#endif
    return STATE_hostname(ptr, out_username, out_hostname, username_set_out, hostname_set_out, must_have_hostname);
  }
  else
  {
#ifdef DEBUG
    printf("[username_or_hostname] Detected username\n");
#endif
    return STATE_username(ptr, out_username, out_hostname, username_set_out, hostname_set_out, must_have_hostname);
  }
}

parse_client_request_return parse_client_request(char *in_buf, char **out_hostname, char **out_username)
{
  char *ptr = in_buf;
  bool found;
  bool username_set_out = false;
  bool hostname_set_out = false;
  bool must_have_hostname = false;
  *out_hostname = NULL;
  *out_username = NULL;

  if (ptr == NULL || check_end(ptr))
  {
    return ERROR;
  }

  if (check_at_whole_str(ptr))
  {
    must_have_hostname = true;
  }

#ifdef DEBUG
  printf("The input %s\n", must_have_hostname ? "MUST have hostname" : "MUST NOT have hostname");
#endif

  // It must start with anything but a space
  if (*ptr == ' ')
  {
    return ERROR;
  }

  // First thing can be /W, a @hostname or a username

  if (check_barw(ptr))
  {
#ifdef DEBUG
    printf("ERROR 1\n");
#endif
    ptr += 2;
    // After /W, it can be a space or a CRLF
    if (skip_spaces(&ptr))
    {
      // after spaces, it can be username or hostname, no CRLF or \0
      return username_or_hostname(ptr, out_username, out_hostname, &username_set_out, &hostname_set_out, must_have_hostname);
    }
    else
    {
      // It can be just /W, so now it must be CRLF
      if (check_CRLF(ptr))
      {
        return NO_USERNAME_NO_HOSTNAME;
      }
      else
      {
        return ERROR;
      }
    }
  }
  else
  {
    // If no /W, it can be a username or hostname or CRLF
    if (check_CRLF(ptr))
    {
      return NO_USERNAME_NO_HOSTNAME;
    }
    else
    {
      return username_or_hostname(ptr, out_username, out_hostname, &username_set_out, &hostname_set_out, must_have_hostname);
    }
  }
}

#endif