#include "../src/servidor/parse_client_request.h"
#include <stdio.h>
#include <string.h>

/*

  Test following inputs:

  i0919688
  i0919688@
  i0919688@localhost
  i0919688@localhost\r\n
  \r\n
  i0919688\r\n
  @localhost\r\n

  Remember the expected outputs:
  typedef enum
  {
    HOSTNAME_REDIRECT,       // Found a hostname
    USERNAME,                // Just a username
    NO_USERNAME_NO_HOSTNAME, // No username or hostname
    ERROR
  } parse_client_request_return;

*/

const char *test_failed = "FAIL⚠️";
const char *test_passed = "Test passed";

void test_parse_client_request(char *input, parse_client_request_return expected)
{
  char *username = NULL;
  char *hostname = NULL;
  parse_client_request_return ret = parse_client_request(input, &hostname, &username);
  printf("Test: %s -> %d\n", input, ret);
  printf("%s\n", ret == expected ? test_passed : test_failed);
  if (username != NULL)
  {
    printf("Username: %s\n", username);
    free(username);
  }
  if (hostname != NULL)
  {
    printf("Hostname: %s\n", hostname);
    free(hostname);
  }
  printf("\n");
}
int main()
{
  char *username = NULL;
  char *hostname = NULL;

  test_parse_client_request("i0919688", ERROR);

  test_parse_client_request("i0919688@", ERROR);

  test_parse_client_request("i0919688@localhost", ERROR);

  test_parse_client_request("i0919688@localhost\r\n", HOSTNAME_REDIRECT);

  test_parse_client_request("\r\n", NO_USERNAME_NO_HOSTNAME);

  test_parse_client_request("i0919688\r\n", USERNAME);

  test_parse_client_request("@localhost\r\n", HOSTNAME_REDIRECT);

  test_parse_client_request("/W i0919688", ERROR);

  test_parse_client_request("/W\r\n", NO_USERNAME_NO_HOSTNAME);

  test_parse_client_request("/W i0919688@localhost", ERROR);

  test_parse_client_request("/W i0919688@localhost\r\n", HOSTNAME_REDIRECT);

  test_parse_client_request("/W i0919688\r\n", USERNAME);

  return 0;
}