#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#include "uproc.h"

int
sys_fork(void)
{
  return fork();
}

int
sys_exit(void)
{
  exit();
  return 0;  // not reached
}

int
sys_wait(void)
{
  return wait();
}

int
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
  return kill(pid);
}

int
sys_getpid(void)
{
  return proc->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = proc->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

int
sys_sleep(void)
{
  int n;
  uint ticks0;
  
  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(proc->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  
  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

//Turn of the computer
int 
sys_halt(void){
  cprintf("Shutting down ...\n");
  outw (0xB004, 0x0 | 0x2000);
  return 0;
}

// get the current date
int
sys_date(void)
{
  struct rtcdate *d;

  if (argptr(0, (void*)&d, sizeof(*d)) < 0) {
    return -1;
  }
  cmostime(d);
  return 0;
}

// Below is project 3...

// get the current process' uid
int
sys_getuid(void)
{
  return proc->uid;
}

// get the current process' gid
int
sys_getgid(void)
{
  return proc->gid;
}

// get the current process' parent's pid
int
sys_getppid(void)
{
  return proc->parent->pid;
}

// set the current process' uid
int
sys_setuid(void) 
{
  int uid;
  if (argint(0, &uid) < 0) {
    return -1;
  }
  proc->uid = uid;
  return 0;
}

// set the current process' gid
int
sys_setgid(void)
{
  int gid;
  if (argint(0, &gid) < 0) {
    return -1;
  }
  proc->gid = gid;
  return 0;
}

// fill in a table of uproc structs
int
sys_getprocs(void)
{
  int max;
  struct uproc * table;
  if (argint(0, &max) < 0) {
    return -1;
  }
  if (argptr(1, (void*)&table, sizeof(*table)) < 0) {
    return -1;
  }
  return getprocs(max, table);
}

int
sys_setpriority(void)
{
  int pid, priority;
  if(argint(0, &pid) < 0) {
    return -1;
  }
  if(argint(1, &priority) < 0) {
    return -1;
  }
  return setpriority(pid, priority);
}
