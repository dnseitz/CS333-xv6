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

  setpriority(getpid(), 0); //Make this a high priority process

  if ((filled = getprocs(max, procs)) < 0) {
    printf(2, "getprocs failed");
    exit();
  }
  
  printf(1, "pid uid gid ppid state size name priority\n");
  for (i = 0; i < filled; ++i) {
    printf(1, "%d  %d  %d  %d  %s  %d  %s %d\n", procs[i].pid,
                                      procs[i].uid,
                                      procs[i].gid,
                                      procs[i].ppid,
                                      procs[i].state,
                                      procs[i].sz,
                                      procs[i].name,
                                      procs[i].priority);
  }
  exit();
}
