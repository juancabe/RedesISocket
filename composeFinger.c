#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <utmpx.h>
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>

#define MAX_FINGER_LINE_SIZE 516
#define MAX_USER_FINGER_SIZE MAX_FINGER_LINE_SIZE * 5 // 5 lineax maximo

typedef struct
{
  char user[32];
  char line[32];
  time_t login_time;
} ActiveUser;

static void freeActiveUsers(ActiveUser **active_users, int count)
{
  for (int i = 0; i < count; i++)
  {
    free(active_users[i]);
  }
}

static int loadActiveUsers(ActiveUser **active_users, int max_users)
{
  struct utmpx *ut;
  int count = 0;
  setutxent();

  while ((ut = getutxent()) != NULL && count < max_users)
  {
    if (ut->ut_type == USER_PROCESS)
    {
      active_users[count] = malloc(sizeof(ActiveUser));
      if (!active_users[count])
      {
        perror("malloc failed");
        // Free previously allocated memory
        freeActiveUsers(active_users, count);
        endutxent();
        return -1;
      }
      strncpy(active_users[count]->user, ut->ut_user, sizeof(active_users[count]->user) - 1);
      active_users[count]->user[sizeof(active_users[count]->user) - 1] = '\0';

      strncpy(active_users[count]->line, ut->ut_line, sizeof(active_users[count]->line) - 1);
      active_users[count]->line[sizeof(active_users[count]->line) - 1] = '\0';

      active_users[count]->login_time = ut->ut_tv.tv_sec;
      count++;
    }
  }

  endutxent();
  return count;
}

static char *fingerForUser(struct passwd *pwd_entry, ActiveUser **active_userss, int active_count)
{
  char *result = malloc(MAX_USER_FINGER_SIZE);
  if (!result)
  {
    perror("malloc failed");
    return NULL;
  }

  char name[256] = "N/A";
  char *gecos = strdup(pwd_entry->pw_gecos);
  if (!gecos)
  {
    perror("strdup failed");
    free(result);
    return NULL;
  }
  char *token = strtok(gecos, ",");
  if (token)
    strncpy(name, token, sizeof(name) - 1);
  name[sizeof(name) - 1] = '\0';

  snprintf(result, MAX_FINGER_LINE_SIZE, // 1 line
           "Login: %s\t\t\tName: %s\r\n",
           pwd_entry->pw_name, name);

  snprintf(result + strlen(result), MAX_FINGER_LINE_SIZE, // 2 lines
           "Directory: %s\tShell: %s\r\n",
           pwd_entry->pw_dir, pwd_entry->pw_shell);

  for (int i = 0; i < active_count; i++)
  {
    if (strcmp(active_userss[i]->user, pwd_entry->pw_name) == 0)
    {
      char login_time[32];
      strftime(login_time, sizeof(login_time), "%c",
               localtime(&active_userss[i]->login_time));
      snprintf(result + strlen(result), MAX_FINGER_LINE_SIZE, // 3 lines
               "On since %s on %s\n", login_time, active_userss[i]->line);
    }
  }

  char mail_path[MAX_USER_FINGER_SIZE / 4];
  snprintf(mail_path, sizeof(mail_path), "/var/mail/%s", pwd_entry->pw_name);
  struct stat mail_stat;
  if (stat(mail_path, &mail_stat) == 0 && mail_stat.st_size > 0)
  {
    strncat(result, "You have mail.\r\n", MAX_FINGER_LINE_SIZE); // 4 lines
  }
  else
  {
    strncat(result, "No Mail.\r\n", MAX_FINGER_LINE_SIZE);
  }

  char plan_path[MAX_USER_FINGER_SIZE / 4];
  snprintf(plan_path, sizeof(plan_path), "%s/.plan", pwd_entry->pw_dir);
  FILE *plan_file = fopen(plan_path, "r");
  if (plan_file)
  {
    strncat(result, "You have Plan.\r\n", MAX_FINGER_LINE_SIZE); // 5 lines
    fclose(plan_file);
  }
  else
  {
    strncat(result, "No Plan.\r\n", MAX_FINGER_LINE_SIZE);
  }

  free(gecos);
  return result;
}

static int count_users_pw()
{
  struct passwd *pwd_entry;
  int user_count = 0;
  setpwent();
  while ((pwd_entry = getpwent()) != NULL)
  {
    if (pwd_entry->pw_uid >= 1000 || pwd_entry->pw_uid == 0)
    {
      user_count++;
    }
  }
  endpwent();
  return user_count;
}

static int count_active_users()
{
  struct utmpx *ut;
  int count = 0;
  setutxent();

  while ((ut = getutxent()) != NULL)
  {
    if (ut->ut_type == USER_PROCESS)
    {
      count++;
    }
  }

  endutxent();
  return count;
}

static char **allFinger(int *allFingerCount)
{
  int user_count = count_users_pw();
  int active_user_count = count_active_users();
  ActiveUser **active_users = malloc(active_user_count * sizeof(ActiveUser *));
  if (!active_users)
  {
    perror("malloc failed");
    return NULL;
  }
  char **all_fingers = malloc(user_count * sizeof(char *));
  if (!all_fingers)
  {
    perror("malloc failed");
    free(active_users);
    return NULL;
  }

  active_user_count = loadActiveUsers(active_users, active_user_count);
  if (active_user_count == -1)
  {
    free(active_users);
    free(all_fingers);
    return NULL;
  }

  setpwent();
  struct passwd *pwd_entry;
  while ((pwd_entry = getpwent()) != NULL)
  {
    if (pwd_entry->pw_uid >= 1000 || pwd_entry->pw_uid == 0)
    {
      char *finger_info = fingerForUser(pwd_entry, active_users, active_user_count);
      if (!finger_info)
      {
        // Skip this user if finger information could not be retrieved
        continue;
      }
      all_fingers[(*allFingerCount)] = malloc(strlen(finger_info) + 1);
      if (!all_fingers[(*allFingerCount)])
      {
        perror("malloc failed");
        free(finger_info);
        continue;
      }
      strcpy(all_fingers[(*allFingerCount)], finger_info);
      (*allFingerCount)++;
      free(finger_info);
    }
  }
  endpwent();

  freeActiveUsers(active_users, active_user_count);
  free(active_users);

  return all_fingers;
}

char **composeFinger(char *user, int *ret_array_size)
{
  if (user != NULL)
  {
    struct passwd *pwd_entry = getpwnam(user);
    if (pwd_entry == NULL)
    {
      return NULL;
    }
    int active_count = count_active_users();
    ActiveUser **active_users = malloc(active_count * sizeof(ActiveUser *));
    if (!active_users)
    {
      perror("malloc failed");
      return NULL;
    }
    active_count = loadActiveUsers(active_users, active_count);
    if (active_count == -1)
    {
      free(active_users);
      return NULL;
    }
    char *ffu = fingerForUser(pwd_entry, active_users, active_count);
    freeActiveUsers(active_users, active_count);
    free(active_users);

    if (!ffu)
    {
      return NULL;
    }

    char **result = malloc(sizeof(char *));
    if (!result)
    {
      perror("malloc failed");
      free(ffu);
      return NULL;
    }
    result[0] = ffu;
    *ret_array_size = 1;
    return result;
  }
  else
  {
    int allFingerCount = 0;
    char **result = allFinger(&allFingerCount);
    if (!result)
    {
      return NULL;
    }
    *ret_array_size = allFingerCount;
    return result;
  }
}

void stats_finger(char **finger_result, int arr_size)
{
  printf("-- STATS --\n");
  printf("Raw output size: %d", arr_size * MAX_USER_FINGER_SIZE);

  char *concat_message = malloc(arr_size * MAX_USER_FINGER_SIZE);
  if (!concat_message)
  {
    perror("malloc failed");
    return;
  }
  char *concat_ptr = concat_message;
  int concat_size = 0;
  for (int i = 0; i < arr_size; i++)
  {
    strcat(concat_ptr, finger_result[i]);
    concat_ptr += strlen(finger_result[i]);
    concat_size += strlen(finger_result[i]);
  }
  printf("Concatenated output size: %d", concat_size);
  free(concat_message);
}

int main(int argc, char *argv[])
{
  int array_size = 0;
  char **finger_result = composeFinger(argc == 2 ? argv[1] : NULL, &array_size);

  if (finger_result == NULL)
  {
    printf("User not found or an error occurred\n");
    return 1;
  }

  stats_finger(finger_result, array_size);

  return 0;

  FILE *finger_file = fopen("finger.txt", "w");
  if (finger_file)
  {
    for (int i = 0; i < array_size; i++)
    {
      fprintf(finger_file, "%s\n", finger_result[i]);
      free(finger_result[i]);
    }
    free(finger_result);
    fclose(finger_file);
  }
  else
  {
    perror("Error opening file");
    for (int i = 0; i < array_size; i++)
    {
      free(finger_result[i]);
    }
    free(finger_result);
    return 1;
  }

  return 0;
}