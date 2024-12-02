#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <utmpx.h>
#include <time.h>
#include <sys/stat.h>

#define BUFFER_SIZE 65536

// Estructura para cargar conexiones activas
typedef struct
{
  char user[32];
  char line[32];
  time_t login_time;
} ActiveUser;

// Cargar información de /var/run/utmpx en memoria
int loadActiveUsers(ActiveUser *active_users, int max_users)
{
  struct utmpx *ut;
  int count = 0;
  setutxent();

  while ((ut = getutxent()) != NULL)
  {
    if (ut->ut_type == USER_PROCESS && count < max_users)
    {
      strncpy(active_users[count].user, ut->ut_user, sizeof(active_users[count].user) - 1);
      strncpy(active_users[count].line, ut->ut_line, sizeof(active_users[count].line) - 1);
      active_users[count].login_time = ut->ut_tv.tv_sec;
      count++;
    }
  }

  endutxent();
  return count;
}

// Generar finger para un usuario
char *fingerForUser(const char *user, ActiveUser *active_users, int active_count)
{
  static char result[4096];
  struct passwd *pwd_entry = NULL;

  // Consultar /etc/passwd
  pwd_entry = getpwnam(user);
  if (pwd_entry == NULL)
  {
    snprintf(result, sizeof(result), "Error: Usuario '%s' no encontrado.\n", user);
    return result;
  }

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
    if (strcmp(active_users[i].user, user) == 0)
    {
      char login_time[32];
      strftime(login_time, sizeof(login_time), "%c", localtime(&active_users[i].login_time));
      snprintf(result + strlen(result), sizeof(result) - strlen(result),
               "On since %s on %s\n", login_time, active_users[i].line);
    }
  }

  // Comprobar correo
  char mail_path[512];
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
  char plan_path[512];
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

// Generar finger para todos los usuarios
char *allFinger()
{
  static char all_fingers[BUFFER_SIZE];
  all_fingers[0] = '\0';

  ActiveUser active_users[128];
  int active_count = loadActiveUsers(active_users, 128);

  struct passwd *pwd_entry;
  setpwent(); // Iniciar lectura de /etc/passwd

  // Iterar sobre todos los usuarios
  while ((pwd_entry = getpwent()) != NULL)
  {
    if (pwd_entry->pw_uid >= 1000 || pwd_entry->pw_uid == 0)
    { // Usuarios regulares y root
      char *finger_info = fingerForUser(pwd_entry->pw_name, active_users, active_count);
      strncat(all_fingers, finger_info, sizeof(all_fingers) - strlen(all_fingers) - 1);
      strncat(all_fingers, "\n\n", sizeof(all_fingers) - strlen(all_fingers) - 1);
    }
  }

  endpwent(); // Finalizar lectura de /etc/passwd
  return all_fingers;
}

int main()
{
  char *all_users_finger = allFinger();
  printf("%s", all_users_finger);
  return 0;
}