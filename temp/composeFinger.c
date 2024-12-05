#include "../src/servidor/compose_finger.h"

int main()
{

  char *info = all_users_info();
  if (info)
  {
    printf("%s", info);
    free(info);
  }

  // Write to file finger.txt
  FILE *finger_file = fopen("finger.txt", "w");
  if (finger_file)
  {
    fprintf(finger_file, "%s", info);
    fclose(finger_file);
  }

  printf("FOR USER\n");

  info = user_info("i0960231", NULL);
  if (info)
  {
    printf("%s", info);
    free(info);
  }
  return 0;
}