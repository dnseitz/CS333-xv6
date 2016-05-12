#include "types.h"
#include "user.h"
#include "uproc.h"

#define MAX 64

int 
main(int argc, char * argv[]) 
{
  struct uproc procs[MAX];
  int i, filled, pid, children = 0, is_child = 0;

  for (i = 0; i < 7; ++i) {
    pid = fork();
    if (pid > 0) {
      ++children;
    }
    else if (pid == 0) {
      is_child = 1;
    }
    else {
      break;
    }
  }
  if (is_child) {
    for (i = 0; i < children; ++i) {
      wait();
    }
    exit();
  }

  // Code below here is essentially ps.c verbatim, but I can't exec over this 
  // process without losing track of the children... think of the children!
  if ((filled = getprocs(MAX, procs)) < 0) {
    printf(2, "getprocs failed");
    exit();
  }
  
  printf(1, "pid uid gid ppid state size name priority\n");
  for (i = 0; i < filled; ++i) {
    printf(1, "%d  %d  %d  %d  %s  %d  %s  %d\n", procs[i].pid,
                                      procs[i].uid,
                                      procs[i].gid,
                                      procs[i].ppid,
                                      procs[i].state,
                                      procs[i].sz,
                                      procs[i].name,
                                      procs[i].priority);
  }

  // Now reap the child processes
  for (i = 0; i < children; ++i) {
    wait();
  } 

  exit();
}
