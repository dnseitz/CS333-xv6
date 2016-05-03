#include "types.h"
#include "user.h"
#include "uproc.h"

#define MAX 64

int 
main(int argc, char * argv[]) 
{
  struct uproc procs[MAX];
  int i, filled, pid, children = 0, is_child = 0;

  for (i = 0; i < 7; ++i)
  {
    pid = fork();
    if (pid > 0)
    {
      ++children;
    }
    else if (pid == 0)
    {
      is_child = 1;
    }
    else 
    {
      break;
    }
  }
  if (is_child)
  {
    for (i = 0; i < children; ++i)
    {
      wait();
    }
    exit();
  }

  if ((filled = getprocs(MAX, procs)) < 0) 
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

  for (i = 0; i < children; ++i)
  {
    wait();
  } 

  exit();
}
