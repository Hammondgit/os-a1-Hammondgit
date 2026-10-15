
#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/uproc.h"
#include "user/user.h"

static char *
state_name(int state)
{
  switch (state) {
  case 1:
    return "used";
  case 2:
    return "sleeping";
  case 3:
    return "runnable";
  case 4:
    return "running";
  case 5:
    return "zombie";
  default:
    return "unknown";
  }
}

int
main(void)
{
  struct uproc processes[NPROC];
  int n;
  int i;

  n = getprocs(processes, NPROC);

  if (n < 0) {
    fprintf(2, "ps: getprocs failed\n");
    exit(1);
  }

  printf("PID\tPPID\tSTATE\tSIZE\tNAME\n");

  for (i = 0; i < n; i++) {
    printf("%d\t%d\t%s\t%lu\t%s\n",
           processes[i].pid,
           processes[i].ppid,
           state_name(processes[i].state),
           processes[i].sz,
           processes[i].name);
  }

  exit(0);
}
