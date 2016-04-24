#include "types.h"
#include "user.h"
#include "param.h"
#include "mmu.h"
#include "proc.h"

int
main(int argc, char *argv[]) 
{
  struct uproc procs[5];
  int i, filled;
  if ((filled = getprocs(5, procs)) < 0) 
  {
    printf(2, "getprocs failed");
    exit();
  }
  
  printf(1, "pid uid gid ppid state size name\n");
  for (i = 0; i < filled; ++i)
  {
    printf(1, "%d %d %d %d %s %d %s\n", procs[i].pid,
                                      procs[i].uid,
                                      procs[i].gid,
                                      procs[i].ppid,
                                      procs[i].state,
                                      procs[i].sz,
                                      procs[i].name);
  }

  exit();
}
