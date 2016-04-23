#include "types.h"
#include "user.h"

int 
main(int argc, char *argv[])
{
  int uid, gid, ppid;

  uid = getuid();
  printf(1, "Current UID is %d\n", uid);
  printf(1, "Setting UID to 100\n");
  setuid(100);

  uid = getuid();
  printf(1, "Current UID is %d\n", uid);

  gid = getgid();
  printf(1, "Current GID is %d\n", gid);
  printf(1, "Setting GID to 99\n");
  setgid(99);
  
  gid = getgid();
  printf(1, "Current GID is %d\n", gid);

  ppid = getppid();
  printf(1, "My parent process is: %d\n", ppid);
  printf(1, "Done!\n");

  exit();
}
