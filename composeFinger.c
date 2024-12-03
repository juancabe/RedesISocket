#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <utmpx.h>
#include <time.h>
#include <sys/stat.h>
#include <stdbool.h>

// #define TODO_BUFFER_SIZE 1165536
// #define TODO_MAX_ACTIVE_USERS 128
#define MAX_USER_FINGER_SIZE 4096

// Estructura para cargar conexiones activas
typedef struct
{
  char user[32];
  char line[32];
  time_t login_time;
} ActiveUser;

static void freeActiveUsers(ActiveUser *active_users, int count)
{
  for (int i = 0; i < count; i++)
  {
    free(&active_users[i]);
  }
}
// Cargar información de /var/run/utmpx en memoria
static int loadActiveUsers(ActiveUser **active_users, int max_users)
{
  struct utmpx *ut;
  int count = 0;
  setutxent();

  while ((ut = getutxent()) != NULL && count < max_users)
  {
    /*
    when ActiveUser * active_users
      strncpy(active_users[count].user, ut->ut_user, sizeof(active_users[count].user) - 1);
      strncpy(active_users[count].line, ut->ut_line, sizeof(active_users[count].line) - 1);
      active_users[count].login_time = ut->ut_tv.tv_sec;
      count++;
    */
    if (ut->ut_type == USER_PROCESS && count < max_users)
    {
      // when ActiveUser ** active_users

      active_users[count] = malloc(sizeof(ActiveUser));
      // Copy username
      strncpy(active_users[count]->user,              // to
              ut->ut_user,                            // from
              sizeof(active_users[count]->user) - 1); // size
      // Copy line
      strncpy(active_users[count]->line,              // to
              ut->ut_line,                            // from
              sizeof(active_users[count]->line) - 1); // size
      // Copy login time
      active_users[count]->login_time = ut->ut_tv.tv_sec;
    }
  }

  endutxent();
  return count;
}

// Generar finger para un usuario
static char *fingerForUser(struct passwd *pwd_entry, ActiveUser **active_userss, int active_count)
{
  char *result = malloc(MAX_USER_FINGER_SIZE);

  // Parsear el campo gecos
  char name[256] = "N/A";
  char *gecos = strdup(pwd_entry->pw_gecos);
  char *token = strtok(gecos, ",");
  if (token)
    strncpy(name, token, sizeof(name) - 1);

  // Información básica
  snprintf(result, sizeof(result),
           "Login: %s\t\t\tName: %s\nDirectory: %s\tShell: %s\n",
           pwd_entry->pw_name, name, pwd_entry->pw_dir, pwd_entry->pw_shell);

  // Información de sesiones activas
  for (int i = 0; i < active_count; i++)
  {
    if (strcmp(active_userss[i]->user, pwd_entry->pw_name) == 0)
    {
      char login_time[32];
      strftime(login_time, sizeof(login_time), "%c",
               localtime(&active_userss[i]->login_time));
      snprintf(result + strlen(result), sizeof(result) - strlen(result),
               "On since %s on %s\n", login_time, active_userss[i]->line);
    }
  }
  // Comprobar correo
  char mail_path[MAX_USER_FINGER_SIZE / 4];
  snprintf(mail_path, sizeof(mail_path), "/var/mail/%s", pwd_entry->pw_name);
  struct stat mail_stat;
  if (stat(mail_path, &mail_stat) == 0 && mail_stat.st_size > 0)
  {
    strncat(result, "You have mail.\n", sizeof(result) - strlen(result) - 1);
  }
  else
  {
    strncat(result, "No Mail.\n", sizeof(result) - strlen(result) - 1);
  }
  // Comprobar plan
  char plan_path[MAX_USER_FINGER_SIZE / 4];
  snprintf(plan_path, sizeof(plan_path), "%s/.plan", pwd_entry->pw_dir);
  FILE *plan_file = fopen(plan_path, "r");
  if (plan_file)
  {
    strncat(result, "Plan:\n", sizeof(result) - strlen(result) - 1);
    char line[256];
    while (fgets(line, sizeof(line), plan_file))
    {
      strncat(result, line, sizeof(result) - strlen(result) - 1);
    }
    fclose(plan_file);
  }
  else
  {
    strncat(result, "No Plan.\n", sizeof(result) - strlen(result) - 1);
  }

  free(gecos);
  return result;
}

static int count_users_pw()
{
  struct passwd *pwd_entry;
  int user_count = 0;
  setpwent(); // Start reading /etc/passwd
  // First pass to count users
  while ((pwd_entry = getpwent()) != NULL)
  {
    if (pwd_entry->pw_uid >= 1000 || pwd_entry->pw_uid == 0)
    {
      user_count++;
    }
  }
  endpwent(); // Finish reading /etc/passwd

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

// Generar finger para todos los usuarios
static char **allFinger(int *activeCountRef, int *allFingerCount)
{
  int user_count = count_users_pw();
  int active_user_count = count_active_users();
  ActiveUser **active_users = malloc(active_user_count * sizeof(ActiveUser *)); // alloc x
  char **all_fingers = malloc(user_count * sizeof(char *));                     // alloc y

  active_user_count = loadActiveUsers(active_users, active_user_count); // alloc z

  setpwent(); // Iniciar lectura de /etc/passwd
  // Iterar sobre todos los usuarios
  struct passwd *pwd_entry;
  while ((pwd_entry = getpwent()) != NULL)
  {
    if (pwd_entry->pw_uid >= 1000 || pwd_entry->pw_uid == 0)
    { // Usuarios regulares y root
      char *finger_info = fingerForUser(pwd_entry, active_users, active_user_count);
      all_fingers[(*allFingerCount)] = malloc(strlen(finger_info) + 1);
      strcpy(all_fingers[(*allFingerCount)], finger_info);
      (*allFingerCount)++;
    }
  }
  endpwent(); // Finalizar lectura

  freeActiveUsers(*active_users, active_user_count); // free z
  free(active_users);                                // free x

  return all_fingers; // return y
}
// If user != NULL, will just look for user's finger
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
    ActiveUser **active_users = malloc(active_count * sizeof(ActiveUser *)); // alloc x
    active_count = loadActiveUsers(active_users, count_active_users());      // alloc y
    char *ffu = fingerForUser(pwd_entry, active_users, active_count);        // alloc z
    freeActiveUsers(*active_users, active_count);                            // free y
    free(active_users);                                                      // free x

    // Needed for returning a pointer to an array
    char **result = malloc(sizeof(char *));
    (*result) = ffu;
    *ret_array_size = 1;
    return result;
  }
  else
  {
    int activeCount = 0, allFingerCount = 0;
    char **result = allFinger(&activeCount, &allFingerCount);
    *ret_array_size = allFingerCount;
    return result;
  }
}
// if argc == 2, will just look for user's finger
int main(int argc, char *argv[])
{
  int array_size;
  char **finger_result = composeFinger(argc == 2 ? argv[1] : NULL, &array_size);

  if (finger_result == NULL)
  {
    printf("User not found\n");
    return 1;
  }

  FILE *finger_file = fopen("finger.txt", "w");
  if (finger_file)
  {
    for (int i = 0; i < array_size; i++)
    {
      fprintf(finger_file, "%s\n", *finger_result[i]);
      free(finger_result[i]);
    }
    fclose(finger_file);
  }
  else
  {
    perror("Error opening file");
    return 1;
  }

  return 0;
}