#include "types.h"
#include "user.h"

#define MAX_CHILDREN 10

void
cleanup(int children[], int filled) {
  int i;
  for(i = 0; i < filled; ++i) {
    if(children[i] > 0) {
      wait();
    }
  }
}

int
main(int argc, char *argv[]) 
{
  int pid, priority, ret, i, children[MAX_CHILDREN], failed = 0;

  for(i = 0; i < MAX_CHILDREN; ++i) {
    pid = fork();
    if(pid > 0) {
      children[i] = pid;
      priority = (i%5)-1; // Range of [(-1)-3]
      ret = setpriority(pid, priority);
      if((priority > 2 || priority < 0) && ret != -1) {
        printf(2, "setpriority returned %d when it should have returned -1!\n", ret);
        ++failed;
      } else if(priority <= 2 && priority >= 0 && ret != 0) {
        printf(2, "setpriority returned %d when it should have returned 0!\n", ret);
        printf(2, "%d\n", i);
        ++failed;
      }
    } else if(pid == 0) {
      if((ret = setpriority(-i,0)) != -2 && pid != 0) {
        printf(2, "setpriority returned %d when it should have returned -3!\n", ret);
        ++failed;
      } else if(ret != -2) {
        printf(2, "setpriority returned %d when it should have returned -2!\n", ret);
        ++failed;
      }
      exit();
    } else {
      printf(2, "fork failed\n");
      children[i] = -1;
    }
  }

  if(!failed) {
    printf(1, "setpriority test passed!\n");
  }

  cleanup(children, MAX_CHILDREN);

  exit();
}
