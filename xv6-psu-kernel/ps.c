#include "types.h"
#include "user.h"
#include "uproc.h"

#define MAX 64

int
main(int argc, char *argv[]) 
{
  struct uproc procs[MAX];
  int i, filled, max = MAX;
  if (argc > 1) {
    max = atoi(argv[1]);
  }

  if ((filled = getprocs(max, procs)) < 0) {
    printf(2, "getprocs failed");
    exit();
  }
  
  printf(1, "pid uid gid ppid state size name\n");
  for (i = 0; i < filled; ++i) {
    printf(1, "%d  %d  %d  %d  %s  %d  %s\n", procs[i].pid,
                                      procs[i].uid,
                                      procs[i].gid,
                                      procs[i].ppid,
                                      procs[i].state,
                                      procs[i].sz,
                                      procs[i].name);
  }
  exit();
}
