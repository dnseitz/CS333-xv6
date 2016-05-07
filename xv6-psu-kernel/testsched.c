#include "types.h"
#include "user.h"
#include "date.h"

#define MAX 30

int
main(int argc, char *argv[]) 
{
  int i, pid, children[MAX], start_sec, end_sec;
  struct rtcdate start, end;

  if(date(&start) < 0) {
    printf(2, "date failed\n");
    exit();
  }
  
  for(i = 0; i < MAX; ++i) {
    pid = fork();
    if(pid == 0) {
      for(;;);
    } else {
      if(pid > 0) {
        setpriority(pid, 0);
      }
      children[i] = pid;
    }
  }
  if(date(&end) < 0) {
    printf(2, "date failed\n");
    for(i = 0; i < MAX; ++i) {
      if(children[i] > 0) {
        kill(children[i]);
      }
    }
    exit();
  }
  
  start_sec = start.minute * 60 + start.second;
  end_sec = end.minute * 60 + end.second;
  printf(1, "Elapsed time: %d\n", end_sec - start_sec);

  for(;;); // Spin here
  exit();
}
