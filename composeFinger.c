#include <stdbool.h>
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <utmp.h>
#include <time.h>

/**
 * @brief Composes a finger request string for a given user.
 *
 * This function takes a username as input and returns a string
 * formatted for a finger request. If username == NULL, will compose finger of every user (-l). The returned string is dynamically
 * allocated and should be freed by the caller.
 *
 * @param username If NULL similar to -l, else will compose for username
 * @return A dynamically allocated string containing the finger request.
 */

char *composeFinger(char *username)
{
  struct passwd *pw;
  struct utmp *ut;
  char *result = NULL;
  size_t size = 0;
  FILE *memstream = open_memstream(&result, &size);

  if (!memstream)
  {
    return NULL;
  }

  setutent(); // Open utmp file

  if (username == NULL)
  {
    // List all users (-l option equivalent)
    fprintf(memstream, "Login      Name              TTY  Idle  Login Time\n");

    while ((pw = getpwent()) != NULL)
    {
      ut = getutent();

      // Format basic user info
      fprintf(memstream, "%-9s %-17s", pw->pw_name, pw->pw_gecos);

      // Add TTY and login time if logged in
      if (ut && strcmp(ut->ut_name, pw->pw_name) == 0)
      {
        time_t login_time = ut->ut_time;
        char *time_str = ctime(&login_time);
        time_str[strlen(time_str) - 1] = '\0'; // Remove newline

        fprintf(memstream, " %-4s        %s\n",
                ut->ut_line, time_str);
      }
      else
      {
        fprintf(memstream, " -           Not logged in\n");
      }
    }
  }
  else
  {
    // Show detailed info for specific user
    pw = getpwnam(username);
    if (!pw)
    {
      fprintf(memstream, "User %s not found.\n", username);
    }
    else
    {
      fprintf(memstream, "Login: %-16s\t\tName: %s\n",
              pw->pw_name, pw->pw_gecos);
      fprintf(memstream, "Directory: %-24s\tShell: %s\n",
              pw->pw_dir, pw->pw_shell);

      // Check if user is logged in
      while ((ut = getutent()) != NULL)
      {
        if (strcmp(ut->ut_name, username) == 0)
        {
          time_t login_time = ut->ut_time;
          char *time_str = ctime(&login_time);
          time_str[strlen(time_str) - 1] = '\0';

          fprintf(memstream, "On since %s on %s\n",
                  time_str, ut->ut_line);
          break;
        }
      }

      if (!ut)
      {
        fprintf(memstream, "Never logged in.\n");
      }
    }
  }

  endutent(); // Close utmp file
  endpwent(); // Close passwd file
  fclose(memstream);

  return result;
}

int main()
{
  // Test composeFinger

  // Test for all users
  char *allUsers = composeFinger(NULL);
  printf("All users:\n%s\n", allUsers);
  free(allUsers);

  // Test for specific user
  char *specificUser = composeFinger("juancalzadabernal");
  printf("Specific user:\n%s\n", specificUser);
  free(specificUser);
}