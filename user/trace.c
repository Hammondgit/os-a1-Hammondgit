#include "kernel/types.h"
#include "user/user.h"

static int
parse_mask(char *s, int *mask)
{
  int value = 0;
  int i;

  if (s[0] == '\0')
    return -1;

  for (i = 0; s[i] != '\0'; i++) {
    int digit;

    if (s[i] < '0' || s[i] > '9')
      return -1;

    digit = s[i] - '0';

    if (value > (2147483647 - digit) / 10)
      return -1;

    value = value * 10 + digit;
  }

  *mask = value;
  return 0;
}

int
main(int argc, char *argv[])
{
  int mask;

  if (argc < 3 || parse_mask(argv[1], &mask) < 0) {
    fprintf(2, "usage: trace <mask> <cmd> [args...]\n");
    exit(1);
  }

  if (trace(mask) < 0) {
    fprintf(2, "trace: system call failed\n");
    exit(1);
  }

  exec(argv[2], &argv[2]);

  fprintf(2, "trace: exec %s failed\n", argv[2]);
  exit(1);
}
