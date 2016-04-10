#include "types.h"
#include "user.h"
#include "date.h"

// Year and Month conversions aren't exact, but hopefully
// nothing runs for that long...
#define YSECONDS 31557600
#define MOSECONDS 2592000
#define DSECONDS 86400
#define HSECONDS 3600
#define MSECONDS 60

int
main(int argc, char *argv[]) 
{
  struct rtcdate start, end;
  char *filename, **args;
  int pid;
  unsigned long long int start_sec, end_sec, elapsed_sec;

  if (argc < 2)
  {
    printf(2, "Usage: %s command [args...]\n", argv[0]);
    exit();
  }

  filename = argv[1];
  args = argv+1;

  if (date(&start)) 
  {
    printf(2, "date failed\n");
    exit();
  }
  pid = fork();
  if (pid > 0 /*Parent*/) 
  {
    wait();
    if (date(&end))
    {
      printf(2, "date failed\n");
      exit();
    }
  } 
  else if (pid == 0 /*Child*/)
  {
    exec(filename, args);
    printf(2, "exec %s failed\n", filename);
    exit();
  }
  else /*Error*/
  {
    printf(2, "fork failed\n");
    exit();
  }

  start_sec = (start.year * YSECONDS) +
      (start.month * MOSECONDS) +
      (start.day * DSECONDS) +
      (start.hour * HSECONDS) +
      (start.minute * MSECONDS) +
      (start.second);
  end_sec = (end.year * YSECONDS) +
      (end.month * MOSECONDS) +
      (end.day * DSECONDS) +
      (end.hour * HSECONDS) +
      (end.minute * MSECONDS) +
      (end.second);
  elapsed_sec = end_sec - start_sec;

  printf(2, "%s ran in %d minutes and %d seconds\n",
            filename, ((int)elapsed_sec/60), ((int)elapsed_sec%60));
  exit();
}
