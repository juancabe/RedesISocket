#ifndef PARSE_CLIENT_REQUEST_H
#define PARSE_CLIENT_REQUEST_H

#include <stdbool.h>
#include <stdlib.h>

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
    *out_username = malloc(ptr - username_start + 1);
    if (*out_username == NULL)
      return ERROR;
    strncpy(*out_username, username_start, ptr - username_start);
    (*out_username)[ptr - username_start] = '\0';

    *username_set_out = true;
    ptr++;
    if (check_CRLF(ptr) || check_end(ptr) || skip_spaces(&ptr))
    {
      return ERROR;
    }
    else
    {
      return STATE_hostname(ptr, out_username, out_hostname, username_set_out, hostname_set_out, must_have_hostname);
    }
  }
  else
  {
    return ERROR;
  }
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

  if (check_CRLF(ptr)) // Check CRLF
  {
    *out_hostname = malloc(ptr - hostname_start + 1);
    if (*out_hostname == NULL)
      return ERROR;
    strncpy(*out_hostname, hostname_start, ptr - hostname_start);
    (*out_hostname)[ptr - hostname_start] = '\0';

    *hostname_set_out = true;
    if (must_have_hostname)
    {
      return HOSTNAME_REDIRECT;
    }
    else
    {
      return ERROR;
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
        return STATE_hostname(ptr, &out_username, &out_hostname, &username_set_out, &hostname_set_out, must_have_hostname);
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