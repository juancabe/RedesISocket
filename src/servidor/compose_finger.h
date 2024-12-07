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
#define UT_USER_SIZE (sizeof(((struct utmpx *)0)->ut_user))

typedef struct
{
  char *username;
  struct utmpx *ut;
  int ut_count;
} UUTX_user_utmpxs;

typedef struct
{
  int count;
  UUTX_user_utmpxs *users;
} UUTX_array;

void UUTX_array_start(UUTX_array *array)
{
  array->count = 0;
  array->users = NULL;
}

static char *safe_strdup(const char *src, size_t len)
{
  char *dest = malloc(len + 1);
  if (dest)
  {
    strncpy(dest, src, len);
    dest[len] = '\0';
  }
  return dest;
}

static int UUTX_array_add(UUTX_array *array, struct utmpx *ut)
{
  if (array == NULL || ut == NULL)
  {
    return -1;
  }

  char ut_user[UT_USER_SIZE + 1];
  strncpy(ut_user, ut->ut_user, UT_USER_SIZE);
  ut_user[UT_USER_SIZE] = '\0';

  if (array->users == NULL)
  {
    if (array->count != 0)
    {
      return -2;
    }
    array->users = malloc(sizeof(UUTX_user_utmpxs));
    if (array->users == NULL)
    {
      return -3;
    }

    array->users[0].username = safe_strdup(ut_user, UT_USER_SIZE);
    array->users[0].ut = malloc(sizeof(struct utmpx));
    if (array->users[0].ut == NULL)
    {
      return -3;
    }
    *(array->users[0].ut) = *ut;
    array->users[0].ut_count = 1;
    array->count = 1;
    return 0;
  }
  else
  {
    for (int i = 0; i < array->count; i++)
    {
      if (strcmp(array->users[i].username, ut_user) == 0)
      {
        array->users[i].ut_count++;
        array->users[i].ut = realloc(array->users[i].ut, array->users[i].ut_count * sizeof(struct utmpx));
        if (array->users[i].ut == NULL)
        {
          return -3;
        }
        array->users[i].ut[array->users[i].ut_count - 1] = *ut;
        return 0;
      }
    }
    array->users = realloc(array->users, (array->count + 1) * sizeof(UUTX_user_utmpxs));
    if (array->users == NULL)
    {
      return -3;
    }
    array->users[array->count].username = safe_strdup(ut_user, UT_USER_SIZE);
    if (array->users[array->count].username == NULL)
    {
      return -3;
    }
    array->users[array->count].ut = malloc(sizeof(struct utmpx));
    if (array->users[array->count].ut == NULL)
    {
      return -3;
    }
    array->users[array->count].ut[0] = *ut;
    array->users[array->count].ut_count = 1;
    array->count++;
    return 0;
  }
}

static int UUTX_array_free(UUTX_array *array)
{
  if (array == NULL)
  {
    return -1;
  }
  if (array->users == NULL)
  {
    return 0;
  }
  for (int i = 0; i < array->count; i++)
  {
    free(array->users[i].username);
    free(array->users[i].ut);
  }
  free(array->users);
  return 0;
}

// If ut == NULL, user should not be logged in
static char *user_info(char *username, UUTX_user_utmpxs *ut_in)
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

  struct utmpx *ut = ut_in ? ut_in->ut : NULL;
  bool logged_in = ut == NULL ? 0 : 1;
  if (!logged_in)
  {
    setutxent();
    while ((ut = getutxent()) != NULL)
    {
      char ut_user[UT_USER_SIZE + 1];
      strncpy(ut_user, ut->ut_user, UT_USER_SIZE);
      ut_user[UT_USER_SIZE] = '\0';

      if (ut->ut_type == USER_PROCESS && strcmp(ut_user, username) == 0)
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
#ifdef __APPLE__
        if (strncmp(wtmp_record.ut_name, username, UT_NAMESIZE) == 0)
        {
          has_logged_in = true;
          if (wtmp_record.ut_line == USER_PROCESS || wtmp_record.ut_line == LOGIN_PROCESS)
#else
        if (strncmp(wtmp_record.ut_user, username, UT_NAMESIZE) == 0)
        {
          has_logged_in = true;
          if (wtmp_record.ut_type == USER_PROCESS || wtmp_record.ut_type == LOGIN_PROCESS)
#endif
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

    if (last_logout_host)
    {
      free(last_logout_host);
    }
    if (last_logout_line)
    {
      free(last_logout_line);
    }
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
    // Draw a line for each terminal active for the user

    for (int i = 0; i < ut_in->ut_count; i++)
    {
      lines_ptr = lines + written_count;
      written_count += snprintf(lines_ptr, MAX_LINE_LENGTH,
                                "On since %s on %s from %s\r\n",
                                login_time, ut_in->ut[i].ut_line,
                                ut_in->ut[i].ut_host[0] != '\0' ? ut_in->ut[i].ut_host : "local");
      lines_ptr = lines + written_count;
    }
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

  // Add null terminator
  lines = realloc(lines, written_count + 1);
  if (!lines)
  {
    // Handle allocation error
    return NULL;
  }
  lines[written_count] = '\0';

  return lines;
}

char *all_users_info()
{
  char *info = NULL;
  struct utmpx *ut;
  setutxent();
  // User array
  UUTX_array users_array;
  UUTX_array_start(&users_array);

  while ((ut = getutxent()) != NULL)
  {
    if (ut->ut_type == USER_PROCESS)
    {
      UUTX_array_add(&users_array, ut);
    }
  }
  endutxent();

  for (int i = 0; i < users_array.count; i++)
  {
    char *user_str = user_info(users_array.users[i].username, &(users_array.users[i]));
    if (user_str)
    {
      size_t current_len = info ? strlen(info) : 0;
      size_t user_len = strlen(user_str);
      char *new_info = realloc(info, current_len + user_len + 3); // +3 for \r\n\0
      if (!new_info)
      {
        free(info);
        free(user_str);
        UUTX_array_free(&users_array);
        return NULL;
      }
      info = new_info;
      strcpy(info + current_len, user_str);
      strcat(info, "\r\n");
      free(user_str);
    }
  }

  if (users_array.count == 0)
  {
    const char *no_users = "No one logged on.\r\n";
    info = malloc(strlen(no_users) + 1);
    if (!info)
    {
      UUTX_array_free(&users_array);
      return NULL;
    }
    strcpy(info, no_users);
    return info;
  }

  UUTX_array_free(&users_array);
  // Add null terminator
  if (info)
  {
    size_t len = strlen(info);
    char *new_info = realloc(info, len + 1);
    if (!new_info)
    {
      free(info);
      return NULL;
    }
    info = new_info;
    info[len] = '\0';
  }

  return info;
}

char *just_one_user_info(char *username)
{
  char *info = NULL;
  struct utmpx *ut;
  setutxent();
  // User array
  UUTX_array users_array;
  UUTX_array_start(&users_array);

  while ((ut = getutxent()) != NULL)
  {
    char ut_user[UT_USER_SIZE + 1];
    strncpy(ut_user, ut->ut_user, UT_USER_SIZE);
    ut_user[UT_USER_SIZE] = '\0';

    if (ut->ut_type == USER_PROCESS && strcmp(ut_user, username) == 0)
    {
      int ret = UUTX_array_add(&users_array, ut);
    }
  }
  endutxent();

  for (int i = 0; i < users_array.count; i++)
  {
    char *user_str = user_info(users_array.users[i].username, &(users_array.users[i]));
    if (user_str)
    {
      size_t current_len = info ? strlen(info) : 0;
      size_t user_len = strlen(user_str);
      char *new_info = realloc(info, current_len + user_len + 3); // +3 for \r\n\0
      if (!new_info)
      {
        free(info);
        free(user_str);
        UUTX_array_free(&users_array);
        return NULL;
      }
      info = new_info;
      strcpy(info + current_len, user_str);
      strcat(info, "\r\n");
      free(user_str);
    }
  }

  if (users_array.count == 0)
  {
    char *user_str = user_info(username, NULL);
    if (user_str)
    {
      size_t current_len = info ? strlen(info) : 0;
      size_t user_len = strlen(user_str);
      char *new_info = realloc(info, current_len + user_len + 3); // +3 for \r\n\0
      if (!new_info)
      {
        free(info);
        free(user_str);
        UUTX_array_free(&users_array);
        return NULL;
      }
      info = new_info;
      strcpy(info + current_len, user_str);
      strcat(info, "\r\n");
      free(user_str);
    }
  }

  UUTX_array_free(&users_array);
  // Add null terminator
  if (info)
  {
    size_t len = strlen(info);
    char *new_info = realloc(info, len + 1);
    if (!new_info)
    {
      free(info);
      return NULL;
    }
    info = new_info;
    info[len] = '\0';
  }

  return info;
}

#endif