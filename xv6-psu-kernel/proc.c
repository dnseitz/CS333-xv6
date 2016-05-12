#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#include "uproc.h"

struct {
  struct spinlock lock;
  struct proc proc[NPROC];
  // Added in project 4
  struct proc *pready[3]; // Set up as circular LL
  struct proc *pfree;     // Set up as circular LL
  uint resetcounter;      // Number of times a scheduler is allowed to run before
                          // resetting priorities
} ptable;

static struct proc *initproc;

int nextpid = 1;
extern void forkret(void);
extern void trapret(void);

static void wakeup1(void *chan);

void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// push a process onto the back of a process list
// ptable lock should be held by caller
void
queue(struct proc** list, struct proc* p) 
{
  if(!(*list)) {
    (*list) = p;
    p->next = p;
  } else {
    p->next = (*list)->next;
    (*list)->next = p;
    (*list) = p;
  }
}

// pop a process off of the front of a process list
// ptable lock should be held by caller
struct proc*
dequeue(struct proc** list) 
{
  struct proc *p;
  if(!(*list)) {
    return NULL;
  }

  p = (*list)->next;
  if(p == (*list)) {
    (*list) = NULL;
  } else {
    (*list)->next = p->next;
  }
  return p;
}

struct proc*
removeproc(struct proc** list, struct proc* p) 
{
  struct proc *curr, *prev;
  if(!(*list)) {
    return NULL;
  }

  curr = (*list)->next;
  prev = (*list);
  do {
    if(curr == p) {
      if(curr == prev) {
        (*list) = NULL;
        return curr;
      }
      if(curr == (*list)) {
        (*list) = curr->next;
      }
      prev->next = curr->next;
      return curr;
    }
    prev = curr;
    curr = curr->next;
  } while(curr != (*list)->next);
  return NULL;
}

void
printlist(struct proc* list) 
{
  struct proc *p;
  if(list == NULL) {
    return;
  }
  p = list->next;
  do {
    cprintf("pid: %d  ->  ", p->pid);
    p = p->next;
  } while(p != list->next);
  cprintf("\n");
}

//PAGEBREAK: 32
// Look in the process table for an UNUSED proc.
// If found, change state to EMBRYO and initialize
// state required to run in the kernel.
// Otherwise return 0.
static struct proc*
allocproc(void)
{
  struct proc *p;
  char *sp;

  acquire(&ptable.lock);
  if(!(p = dequeue(&ptable.pfree))) {
    release(&ptable.lock);
    return 0;
  }
  if(p->state != UNUSED) {
    panic("allocproc: used process in free list");
  }

  p->state = EMBRYO;
  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
    acquire(&ptable.lock);
    queue(&ptable.pfree, p);
    release(&ptable.lock);
    return 0;
  }
  sp = p->kstack + KSTACKSIZE;
  
  // Leave room for trap frame.
  sp -= sizeof *p->tf;
  p->tf = (struct trapframe*)sp;
  
  // Set up new context to start executing at forkret,
  // which returns to trapret.
  sp -= 4;
  *(uint*)sp = (uint)trapret;

  sp -= sizeof *p->context;
  p->context = (struct context*)sp;
  memset(p->context, 0, sizeof *p->context);
  p->context->eip = (uint)forkret;

  return p;
}

//PAGEBREAK: 32
// Set up first user process.
void
userinit(void)
{
  struct proc *p;
  int i;
  extern char _binary_initcode_start[], _binary_initcode_size[];

  // Initialize ready and free lists, no need to lock ptable since
  // we should be the only thing running
  ptable.resetcounter = MAX_TIME_TO_RESET;
  ptable.pfree = NULL;
  for(i = HIGH; i <= LOW; ++i) {
    ptable.pready[i] = NULL;
  }
  // Setup the free list
  for(p = ptable.proc; p < &ptable.proc[NPROC]; ++p) {
    if (p->state != UNUSED) {
      panic("userinit: process allocated already?");
    }
    queue(&ptable.pfree, p);
  }
#ifdef DEBUGSCHED
  struct proc * _p = ptable.pfree;
  int _i = 0;
  do {
    cprintf("proc num in free list: %d\n", _i);
    ++_i;
    _p = _p->next;
  } while(_p != ptable.pfree);
  cprintf("userinit called\n"); //Checking to see how many times userinit is called
#endif
  p = allocproc();
  initproc = p;
  if((p->pgdir = setupkvm()) == 0)
    panic("userinit: out of memory?");
  inituvm(p->pgdir, _binary_initcode_start, (int)_binary_initcode_size);
  p->sz = PGSIZE;
  memset(p->tf, 0, sizeof(*p->tf));
  p->tf->cs = (SEG_UCODE << 3) | DPL_USER;
  p->tf->ds = (SEG_UDATA << 3) | DPL_USER;
  p->tf->es = p->tf->ds;
  p->tf->ss = p->tf->ds;
  p->tf->eflags = FL_IF;
  p->tf->esp = PGSIZE;
  p->tf->eip = 0;  // beginning of initcode.S

  safestrcpy(p->name, "initcode", sizeof(p->name));
  p->cwd = namei("/");
  // Added in project 3, initialize parent, uid and gid
  p->parent = p; // parent of the first process is itself
  p->gid = INIT_GID;
  p->uid = INIT_UID;

  p->state = RUNNABLE;
  // Project 4:
  // Put into high priority queue because this should be the only process running
  // there should be no need for a lock since only init should be running
  p->priority = DEFAULT;
  queue(&ptable.pready[HIGH], p); 
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
int
growproc(int n)
{
  uint sz;
  
  sz = proc->sz;
  if(n > 0){
    if((sz = allocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(proc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  proc->sz = sz;
  switchuvm(proc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
int
fork(void)
{
  int i, pid;
  struct proc *np;

  // Allocate process.
  if((np = allocproc()) == 0)
    return -1;

  // Copy process state from p.
  if((np->pgdir = copyuvm(proc->pgdir, proc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
    np->state = UNUSED;
    acquire(&ptable.lock);
    queue(&ptable.pfree, np); // Project 4
    acquire(&ptable.lock);
    return -1;
  }
  np->sz = proc->sz;
  np->parent = proc;
  *np->tf = *proc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(proc->ofile[i])
      np->ofile[i] = filedup(proc->ofile[i]);
  np->cwd = idup(proc->cwd);

  safestrcpy(np->name, proc->name, sizeof(proc->name));
 
  pid = np->pid;
  // Added in Project 3, copy over gid and uid from parent process
  np->gid = proc->gid;
  np->uid = proc->uid;

  // lock to force the compiler to emit the np->state write last.
  acquire(&ptable.lock);
  np->state = RUNNABLE;
  np->priority = DEFAULT;
  queue(&ptable.pready[DEFAULT], np); // Project 4: Add to default priority queue
  release(&ptable.lock);
  
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
void
exit(void)
{
  struct proc *p;
  int fd;

  if(proc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(proc->ofile[fd]){
      fileclose(proc->ofile[fd]);
      proc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(proc->cwd);
  end_op();
  proc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(proc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == proc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  proc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
int
wait(void)
{
  struct proc *p;
  int havekids, pid;

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for zombie children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != proc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->state = UNUSED;
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        queue(&ptable.pfree, p); // Project 4: add this process back to the free list
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || proc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(proc, &ptable.lock);  //DOC: wait-sleep
  }
}

//PAGEBREAK: 42
// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
void
scheduler(void)
{
  struct proc *p;
  int priority; // Priority of ready queue 0=high, 1=default, 2=low

  for(;;){
    // Enable interrupts on this processor.
    sti();

    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
#ifdef OLDSCHED
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
#else
    for(priority = HIGH; priority <= LOW; ++priority) {
      if(!(p = dequeue(&ptable.pready[priority]))) {
        continue;
      }
      if(p->state != RUNNABLE) {
        panic("scheduler: non-runnable process in ready list");
      }

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
      proc = p;
      switchuvm(p);
      p->state = RUNNING;
      swtch(&cpu->scheduler, proc->context);
      switchkvm();
      if (--ptable.resetcounter == 0) {
  #ifdef DEBUGSCHED
        cprintf("ptable.resetcounter hit 0\n");
  #endif
        for(p = ptable.proc; p < &ptable.proc[NPROC]; ++p) {
          if(p->state != RUNNABLE && p->state != SLEEPING && p->state != RUNNING) {
            continue;
          }
          p->priority = DEFAULT;
        }
        ptable.resetcounter = MAX_TIME_TO_RESET; 
      }

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      proc = 0;
    }
#endif
    release(&ptable.lock);

  }
}

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state.
void
sched(void)
{
  int intena;

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(cpu->ncli != 1)
    panic("sched locks");
  if(proc->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = cpu->intena;
  swtch(&proc->context, cpu->scheduler);
  cpu->intena = intena;
}

// Give up the CPU for one scheduling round.
void
yield(void)
{
  acquire(&ptable.lock);  //DOC: yieldlock
  proc->state = RUNNABLE;
  queue(&ptable.pready[proc->priority], proc);
  sched();
  release(&ptable.lock);
}

// A fork child's very first scheduling by scheduler()
// will swtch here.  "Return" to user space.
void
forkret(void)
{
  static int first = 1;
  // Still holding ptable.lock from scheduler.
  release(&ptable.lock);

  if (first) {
    // Some initialization functions must be run in the context
    // of a regular process (e.g., they call sleep), and thus cannot 
    // be run from main().
    first = 0;
    iinit(ROOTDEV);
    initlog(ROOTDEV);
  }
  
  // Return to "caller", actually trapret (see allocproc).
}

// Atomically release lock and sleep on chan.
// Reacquires lock when awakened.
void
sleep(void *chan, struct spinlock *lk)
{
  if(proc == 0)
    panic("sleep");

  if(lk == 0)
    panic("sleep without lk");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    release(lk);
  }

  // Go to sleep.
  proc->chan = chan;
  proc->state = SLEEPING;
  sched();

  // Tidy up.
  proc->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    acquire(lk);
  }
}

//PAGEBREAK!
// Wake up all processes sleeping on chan.
// The ptable lock must be held.
static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++) {
    if(p->state == SLEEPING && p->chan == chan) {
      p->state = RUNNABLE;
      // No lock here since we already have it
      queue(&ptable.pready[p->priority], p);
    }
  }
}

// Wake up all processes sleeping on chan.
void
wakeup(void *chan)
{
  acquire(&ptable.lock);
  wakeup1(chan);
  release(&ptable.lock);
}

// Kill the process with the given pid.
// Process won't exit until it returns
// to user space (see trap in trap.c).
int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING) {
        p->state = RUNNABLE;
        queue(&ptable.pready[p->priority], p);
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}

//PAGEBREAK: 36
// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
void
procdump(void)
{
  static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run   ",
  [ZOMBIE]    "zombie"
  };
  int i;
  struct proc *p;
  char *state;
  uint pc[10];
  
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    cprintf("%d %s %s %d %d %d", p->pid, state, p->name, p->priority, p->uid, p->gid);
    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

void
scheddump(void)
{
  static char *priorities[] = {
    [HIGH]    "High   ",
    [DEFAULT] "Default",
    [LOW]     "Low    "
  };
  int i;
  acquire(&ptable.lock);
  cprintf("Free list:  ");
  printlist(ptable.pfree);
  cprintf("\n");
  for(i = HIGH; i <= LOW; ++i) {
    cprintf("%s priority list: ", priorities[i]);
    printlist(ptable.pready[i]);
    cprintf("\n");
  }
  release(&ptable.lock);
}

// Fill in a table of uprocs with process information
// Returns the number of processes filled in on success
// ERR CODES: -1 = max value was negative
int
getprocs(int max, struct uproc * table)
{
  static char *states[] = {
  [UNUSED]    "UNUSED  ",
  [EMBRYO]    "EMBRYO  ",
  [SLEEPING]  "SLEEPING",
  [RUNNABLE]  "RUNNABLE",
  [RUNNING]   "RUNNING ",
  [ZOMBIE]    "ZOMBIE  "
  };
  if (max < 0) {
    return -1;
  }
  struct proc * p;
  int used = 0;
  acquire(&ptable.lock);
  for (p = ptable.proc; p != &ptable.proc[NPROC] && used < max; ++p) {
    if (p->state != UNUSED && p->state != EMBRYO) {
      table[used].pid = p->pid;
      table[used].uid = p->uid;
      table[used].gid = p->gid;
      table[used].ppid = p->parent->pid;
      safestrcpy(table[used].state, states[p->state], 9/*longest state name*/);
      table[used].sz = p->sz;
      safestrcpy(table[used].name, p->name, sizeof(p->name));
      table[used].priority = p->priority;
      ++used;
    }
  }
  release(&ptable.lock);
  return used;
}

// Set the priority of a process
// Added in Project 4
// Returns 0 on success
// ERR CODES: -1 = priority out of range
//            -2 = invalid pid
//            -3 = pid not found
int
setpriority(int pid, int priority) 
{
  struct proc *p;
  if(priority < HIGH || priority > LOW) {
    return -1;
  }
  if(pid <= 0) {
    return -2;
  }
  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; ++p) {
    if(p->pid == pid) {
      // If the process already has this priority, do nothing
      if(p->priority != priority) {
        if(p->state == RUNNABLE) {
          if(removeproc(&ptable.pready[p->priority], p) != p) {
            panic("setpriority: ready list messed up");
          }
          queue(&ptable.pready[priority], p);
        }
        p->priority = priority;
      }
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -3;
}
