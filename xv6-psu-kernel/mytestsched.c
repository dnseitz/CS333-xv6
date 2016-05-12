#include "types.h"
#include "user.h"
#include "date.h"

#define MAX 30

int
main(int argc, char *argv[]) 
{
  int i, pid, children[MAX];
  
  for(i = 0; i < MAX; ++i) {
    pid = fork();
    if(pid == 0) {
      for(;;);
    } else {
      if(pid > 0) {
        setpriority(pid, 2);
      }
      children[i] = pid;
    }
  }
  
  for(;;); // Spin here
  
  for(i = 0; i < MAX; ++i) {
    if(children[i] > 0) {
      kill(children[i]);
    }
  }
  exit();
}
