#include "types.h"
#include "user.h"
#include "uproc.h"

#define BAD -1
#define NULL 0

int
main(int argc, char* argv[]) 
{
  int ret;
  if ((ret = getprocs(BAD, NULL)) >= 0) {
    printf(1, "getprocs() returned successfully? Return value: %d\n", ret);
  } else {
    printf(1, "getprocs failed\n");
  }

  exit();
}
