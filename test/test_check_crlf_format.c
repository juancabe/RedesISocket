/*
** Fichero: test_check_crlf_format.c
** Autores:
** Juan Calzada Bernal DNI 70919688Q
** Hugo Chalard Collado DNI 70964149H
*/

#include "../src/common.h"
#include <stdio.h>

int test_crlf_format(int num, char *buffer, bool expected) {
  bool result = check_crlf_format(buffer, strlen(buffer));
  if (result != expected) {
    fprintf(stderr, "%d Test FAILED  ⚠️   for buffer: %s\n\n\n", num, buffer);
    return 1;
  } else {
    fprintf(stderr, "%d Test passed for buffer: %s\n\n\n", num, buffer);
  }
  return 0;
}

int main() {
  // test check_crlf_format("hola\n", 5);
  test_crlf_format(1, "hola\n", false);
  test_crlf_format(2, "hola\r\n", true);
  test_crlf_format(3, "hola\r\n\r\n", true);
  test_crlf_format(4, "\r\n", true);
  test_crlf_format(5, "\n", false);
  test_crlf_format(6, "hola", false);
  test_crlf_format(7, "hola\r", false);
  test_crlf_format(8, "hola\n\r", false);
  test_crlf_format(9, "hola\n\r\n", false);
  test_crlf_format(10, "hola\r\n\n", false);
  test_crlf_format(12, "Response doesn't fit in UDP packet\r\n", true);
  return 0;
}