#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <utmpx.h>
#include <time.h>

char *fingerForUser(const char *user)
{
  static char result[1024];
  struct passwd *pwd_entry = NULL;

  // Consultar /etc/passwd para obtener información básica del usuario
  pwd_entry = getpwnam(user);
  if (pwd_entry == NULL)
  {
    snprintf(result, sizeof(result), "Error: Usuario '%s' no encontrado.\n", user);
    return result;
  }

  // Información básica
  snprintf(result, sizeof(result),
           "Login: %s\nName: %s\nDirectory: %s\nShell: %s\n",
           pwd_entry->pw_name, pwd_entry->pw_gecos,
           pwd_entry->pw_dir, pwd_entry->pw_shell);

  // Consultar información de inicio de sesión actual usando /var/run/utmpx
  struct utmpx *ut;
  setutxent(); // Abrir utmpx

  while ((ut = getutxent()) != NULL)
  {
    if (ut->ut_type == USER_PROCESS && strcmp(ut->ut_user, user) == 0)
    {
      char login_time[32];
      time_t login_t = ut->ut_tv.tv_sec;
      strftime(login_time, sizeof(login_time), "%c", localtime(&login_t));
      snprintf(result + strlen(result), sizeof(result) - strlen(result),
               "On since: %s on %s\n", login_time, ut->ut_line);
    }
  }

  endutxent(); // Cerrar utmpx

  return result;
}

int main()
{
  char user[256];
  printf("Introduce el nombre del usuario: ");
  scanf("%255s", user);

  char *finger_info = fingerForUser(user);
  printf("%s", finger_info);

  return 0;
}