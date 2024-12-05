#ifndef COMPOSE_FINGER_H
#define COMPOSE_FINGER_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utmpx.h>
#include <pwd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdarg.h>
#include <stdbool.h>
#include <utmp.h>

#define MAX_LINE_LENGTH 516

// If ut == NULL, it will check if the user is logged in, opening utmpx
char *user_info(const char *username, struct utmpx *ut_in)
{
  char *lines = NULL;
  char *lines_ptr = NULL;
  int written_count = 0;

  struct passwd *pwd = getpwnam(username);
  if (!pwd)
  {
    char user_not_found[] = "User not found.";
    lines = malloc(strlen(user_not_found) + 1);
    if (!lines)
    {
      // Handle allocation error
      return NULL;
    }
    strcpy(lines, user_not_found);
    return lines;
  }

  char name[256] = "N/A";
  char *gecos = strdup(pwd->pw_gecos);
  if (gecos)
  {
    char *token = strtok(gecos, ",");
    if (token)
    {
      strncpy(name, token, sizeof(name) - 1);
      name[sizeof(name) - 1] = '\0';
    }
    free(gecos);
  }

  // First allocation
  lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
  if (!lines)
  {
    // Handle allocation error
    return NULL;
  }
  lines_ptr = lines + written_count;
  written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "Login: %s\t\t\tName: %s\r\n", pwd->pw_name, name);
  lines_ptr = lines + written_count;

  lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
  if (!lines)
  {
    // Handle allocation error
    return NULL;
  }
  lines_ptr = lines + written_count;
  written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "Directory: %s\tShell: %s\r\n", pwd->pw_dir, pwd->pw_shell);
  lines_ptr = lines + written_count;

  struct utmpx *ut = ut_in;
  bool logged_in = ut == NULL ? 0 : 1;
  if (!logged_in)
  {
    setutxent();
    while ((ut = getutxent()) != NULL)
    {
      if (ut->ut_type == USER_PROCESS && strncmp(ut->ut_user, username, UT_NAMESIZE) == 0)
      {
        logged_in = 1;
        break;
      }
    }
    endutxent();
  }

  if (!logged_in)
  {
    // Check wtmp for login history
    struct utmp wtmp_record;
    FILE *wtmp_file = fopen("/var/log/wtmp", "rb");
    bool has_logged_in = false;
    time_t last_logout_time = 0;
    char *last_logout_host = NULL;
    char *last_logout_line = NULL;

    if (wtmp_file)
    {
      while (fread(&wtmp_record, sizeof(struct utmp), 1, wtmp_file) == 1)
      {
        if (strncmp(wtmp_record.ut_user, username, UT_NAMESIZE) == 0)
        {
          has_logged_in = true;
          if (wtmp_record.ut_type == USER_PROCESS || wtmp_record.ut_type == LOGIN_PROCESS)
          {
            last_logout_time = wtmp_record.ut_time;
            last_logout_host = malloc(UT_HOSTSIZE + 1);
            if (last_logout_host)
            {
              strncpy(last_logout_host, wtmp_record.ut_host, UT_HOSTSIZE);
              last_logout_host[UT_HOSTSIZE] = '\0';
            }
            else
            {
              // Handle allocation error
              return NULL;
            }
            last_logout_line = malloc(UT_LINESIZE + 1);
            if (last_logout_line)
            {
              strncpy(last_logout_line, wtmp_record.ut_line, UT_LINESIZE);
              last_logout_line[UT_LINESIZE] = '\0';
            }
            else
            {
              // Handle allocation error
              return NULL;
            }
          }
        }
      }
      fclose(wtmp_file);
    }

    lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
    if (!lines)
    {
      // Handle allocation error
      return NULL;
    }
    lines_ptr = lines + written_count;

    if (has_logged_in)
    {
      char logout_time[64];
      strftime(logout_time, sizeof(logout_time), "%a %b %d %H:%M (%Z)", localtime(&last_logout_time));
      written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "Last login %s on %s from %s\r\n", logout_time, last_logout_line, last_logout_host);
    }
    else
    {
      written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "Never logged in.\r\n");
    }
    lines_ptr = lines + written_count;
  }
  else
  {
    char login_time[64];
    strftime(login_time, sizeof(login_time), "%a %b %d %H:%M (%Z)", localtime((time_t *)&ut->ut_tv.tv_sec));
    lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
    if (!lines)
    {
      // Handle allocation error
      return NULL;
    }
    lines_ptr = lines + written_count;
    written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "On since %s on %s from %s\r\n", login_time, ut->ut_line, ut->ut_host[0] != '\0' ? ut->ut_host : "local");
    lines_ptr = lines + written_count;
  }

  char mail_path[512];
  snprintf(mail_path, sizeof(mail_path), "/var/mail/%s", pwd->pw_name);
  struct stat mail_stat;
  if (stat(mail_path, &mail_stat) == 0 && mail_stat.st_size > 0)
  {
    lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
    if (!lines)
    {
      // Handle allocation error
      return NULL;
    }
    lines_ptr = lines + written_count;
    written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "You have mail.\r\n");
    lines_ptr = lines + written_count;
  }
  else
  {
    lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
    if (!lines)
    {
      // Handle allocation error
      return NULL;
    }
    lines_ptr = lines + written_count;
    written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "No mail.\r\n");
    lines_ptr = lines + written_count;
  }

  char plan_path[512];
  snprintf(plan_path, sizeof(plan_path), "%s/.plan", pwd->pw_dir);
  if (access(plan_path, F_OK) == 0)
  {
    lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
    if (!lines)
    {
      // Handle allocation error
      return NULL;
    }
    lines_ptr = lines + written_count;
    written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "You have Plan.\r\n");
    lines_ptr = lines + written_count;
  }
  else
  {
    lines = realloc(lines, written_count + MAX_LINE_LENGTH + 1);
    if (!lines)
    {
      // Handle allocation error
      return NULL;
    }
    lines_ptr = lines + written_count;
    written_count += snprintf(lines_ptr, MAX_LINE_LENGTH, "No Plan.\r\n");
    lines_ptr = lines + written_count;
  }

  return lines;
}

char *all_users_info()
{
  char *info = NULL;
  struct utmpx *ut;
  setutxent();
  int user_count = 0;
  bool first = true;

  while ((ut = getutxent()) != NULL)
  {
    if (ut->ut_type == USER_PROCESS)
    {
      user_count++;
      char *user_str = user_info(ut->ut_user, ut);
      if (user_str)
      {
        size_t current_len = info ? strlen(info) : 0;
        size_t user_len = strlen(user_str);
        char *new_info = realloc(info, current_len + user_len + 3); // +3 for \r\n\0
        if (!new_info)
        {
          free(info);
          free(user_str);
          return NULL;
        }
        info = new_info;
        strcpy(info + current_len, user_str);
        strcat(info, "\r\n");
        free(user_str);
      }
    }
  }
  endutxent();
  return info;
}

#endif