#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <utmpx.h>
#include <time.h>
#include <sys/stat.h>

char *fingerForUser(const char *user)
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

  // Información de inicio de sesión
  struct utmpx *ut;
  setutxent();
  while ((ut = getutxent()) != NULL)
  {
    if (ut->ut_type == USER_PROCESS && strcmp(ut->ut_user, user) == 0)
    {
      char login_time[32];
      time_t login_t = ut->ut_tv.tv_sec;
      strftime(login_time, sizeof(login_time), "%c", localtime(&login_t));
      snprintf(result + strlen(result), sizeof(result) - strlen(result),
               "On since %s on %s\n", login_time, ut->ut_line);
    }
  }
  endutxent();

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

char *allFinger()
{
  static char all_fingers[65536]; // Tamaño suficiente para manejar muchos usuarios
  all_fingers[0] = '\0';          // Inicializar como cadena vacía

  struct passwd *pwd_entry;
  setpwent(); // Iniciar lectura de /etc/passwd

  // Iterar sobre todos los usuarios
  while ((pwd_entry = getpwent()) != NULL)
  {
    if (pwd_entry->pw_uid >= 1000 || pwd_entry->pw_uid == 0)
    { // Usuarios regulares y root
      char *finger_info = fingerForUser(pwd_entry->pw_name);
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