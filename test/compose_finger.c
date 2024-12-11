#include "../src/servidor/compose_finger.h"
#include "../src/common.h"

int main() {

  char *info = all_users_info();
  if (info) {
    printf("%s", info);
    if (check_crlf_format(info, strlen(info))) {
      printf("CRLF format is correct\n");
    } else {
      printf("CRLF format is incorrect\n");
    }
    free(info);
  }

  printf("FOR USER\n");

  info = just_one_user_info("i0960231");
  if (info) {
    printf("%s", info);
    if (check_crlf_format(info, strlen(info))) {
      printf("CRLF format is correct\n");
    } else {
      printf("CRLF format is incorrect\n");
    }
    free(info);
  } else {
    printf("No info\n");
  }

  info = just_one_user_info("juan");
  if (info) {
    printf("%s", info);
    if (check_crlf_format(info, strlen(info))) {
      printf("CRLF format is correct\n");
    } else {
      printf("CRLF format is incorrect\n");
    }
    free(info);
  } else {
    printf("No info\n");
  }

  info = just_one_user_info("root");
  if (info) {
    printf("%s", info);
    if (check_crlf_format(info, strlen(info))) {
      printf("CRLF format is correct\n");
    } else {
      printf("CRLF format is incorrect\n");
    }
    free(info);
  } else {
    printf("No info\n");
  }

  return 0;
}