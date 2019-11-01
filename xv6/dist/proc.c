#include "types.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "x86.h"
#include "proc.h"
#include "spinlock.h"
#ifdef CS333_P2
#include "uproc.h"
#endif //CS333_P2

#ifdef CS333_P3
#define statecount NELEM(states)
#endif //CS333_P3

#ifdef CS333_P4
//#define MAXPRIO 6
//#define TICKS_TO_PROMOTE 3000
#endif //CS333_P4

static char *states[] = {
  [UNUSED]    "unused",
  [EMBRYO]    "embryo",
  [SLEEPING]  "sleep ",
  [RUNNABLE]  "runble",
  [RUNNING]   "run",
  [ZOMBIE]    "zombie"
};

#ifdef CS333_P3
struct ptrs {
  struct proc * head;       //Points to process at start of list
  struct proc * tail;       //Points to process at end of list
};
#endif //CS333_P3

static struct {
  struct spinlock lock;
  struct proc proc[NPROC];
#ifdef CS333_P3
  struct ptrs list[statecount];
#endif //CS333_P3
#ifdef CS333_P4
  struct ptrs ready[MAXPRIO+1];
  uint PromoteAtTime;
#endif //CS333_P4
} ptable;

static struct proc *initproc;

uint nextpid = 1;
extern void forkret(void);
extern void trapret(void);
static void wakeup1(void* chan);


#ifdef CS333_P3
static void initProcessLists(void);
static void initFreeList(void);
static void stateListAdd(struct ptrs*, struct proc*);
static int stateListRemove(struct ptrs*, struct proc* p);
static void assertState(struct proc * p, enum procstate state);
#endif //CS333_P3

#ifdef CS333_P4
void periodicPriorityAdjustment(void);
#endif //CS333_P4

#ifdef CS333_P3
  static void
stateListAdd(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL){
    (*list).head = p;
    (*list).tail = p;
    p->next = NULL;
  } else{
    ((*list).tail)->next = p;
    (*list).tail = ((*list).tail)->next;
    ((*list).tail)->next = NULL;
  }
}

  static int
stateListRemove(struct ptrs* list, struct proc* p)
{
  if((*list).head == NULL || (*list).tail == NULL || p == NULL){
    return -1;
  }
  struct proc* current = (*list).head;
  struct proc* previous = 0;
  if(current == p){
    (*list).head = ((*list).head)->next;
    // prevent tail remaining assigned when we've removed the only item
    // on the list
    if((*list).tail == p){
      (*list).tail = NULL;
    }
    return 0;
  }
  while(current){
    if(current == p){
      break;
    }
    previous = current;
    current = current->next;
  }
  // Process not found, hit eject.
  if(current == NULL){
    return -1;
  }
  // Process found. Set the appropriate next pointer.
  if(current == (*list).tail){
    (*list).tail = previous;
    ((*list).tail)->next = NULL;
  } else{
    previous->next = current->next;
  }
  // Make sure p->next doesn't point into the list.
  p->next = NULL;
  return 0;
}
  static void 
assertState(struct proc * p, enum procstate state)
{
  if(p->state != state){
    cprintf("process state is: %s\nProcess state was thought to be: %s\n", states[p->state], states[state]);
    panic("Unexpected process removed from ptable.list"); 
  }
}

  static void
initProcessLists()
{
  int i;
  for (i = UNUSED; i <= ZOMBIE; i++) {
    ptable.list[i].head = NULL;
    ptable.list[i].tail = NULL;
  }
#ifdef CS333_P4
  for (i = 0; i <= MAXPRIO; i++) {
    ptable.ready[i].head = NULL;
    ptable.ready[i].tail = NULL;
  }
#endif //CS333_P4
}

  static void
initFreeList(void)
{
  struct proc* p;
  for(p = ptable.proc; p < ptable.proc + NPROC; ++p){
    p->state = UNUSED;
    stateListAdd(&ptable.list[UNUSED], p);
  }
}
#endif //CS333_P3

  void
pinit(void)
{
  initlock(&ptable.lock, "ptable");
}

// Must be called with interrupts disabled
int
cpuid() {
  return mycpu()-cpus;
}

// Must be called with interrupts disabled to avoid the caller being
// rescheduled between reading lapicid and running through the loop.
  struct cpu*
mycpu(void)
{
  int apicid, i;

  if(readeflags()&FL_IF)
    panic("mycpu called with interrupts enabled\n");

  apicid = lapicid();
  // APIC IDs are not guaranteed to be contiguous. Maybe we should have
  // a reverse map, or reserve a register to store &cpus[i].
  for (i = 0; i < ncpu; ++i) {
    if (cpus[i].apicid == apicid) {
      return &cpus[i];
    }
  }
  panic("unknown apicid\n");
}

// Disable interrupts so that we are not rescheduled
// while reading proc from the cpu structure
struct proc*
myproc(void) {
  struct cpu *c;
  struct proc *p;
  pushcli();
  c = mycpu();
  p = c->proc;
  popcli();
  return p;
}

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
#ifdef CS333_P3
  p = ptable.list[UNUSED].head;
  if(!p){
    release(&ptable.lock);
    return 0;
  }
  stateListRemove(&ptable.list[p->state], p);
  assertState(p, UNUSED);
  p->state = EMBRYO;
  stateListAdd(&ptable.list[p->state], p);
#else
  int found = 0;
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == UNUSED) {
      found = 1;
      break;
    }
  if (!found) {
    release(&ptable.lock);
    return 0;
  }
  p->state = EMBRYO;
#endif //CS333_P3

  p->pid = nextpid++;
  release(&ptable.lock);

  // Allocate kernel stack.
  if((p->kstack = kalloc()) == 0){
    p->state = UNUSED;
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

#ifdef CS333_P1
  p->start_ticks = ticks;
#endif //CS333_P1
#ifdef CS333_P1
  p->cpu_ticks_total = 0;
  p->cpu_ticks_in = 0;
#endif //CS333_P2

  return p;
}

// Set up first user process.
  void
userinit(void)
{
  struct proc *p;
  extern char _binary_initcode_start[], _binary_initcode_size[];

#ifdef CS333_P3
  acquire(&ptable.lock);
  initProcessLists();
  initFreeList();
  release(&ptable.lock);
#endif //CS333_P3

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

  // this assignment to p->state lets other cores
  // run this process. the acquire forces the above
  // writes to be visible, and the lock is also needed
  // because the assignment might not be atomic.
  acquire(&ptable.lock);
  #ifdef CS333_P4
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
  if(stateListRemove(&ptable.list[p->state], p) < 0)
    panic("stateListRemove FAILED in userinit\n");
  assertState(p, EMBRYO);
  p->priority = MAXPRIO;
  p->budget = BUDGET;
  p->state = RUNNABLE;
  stateListAdd(&ptable.ready[p->priority], p);
  #elif CS333_P3
  stateListRemove(&ptable.list[p->state], p);
  assertState(p, EMBRYO);
  p->state = RUNNABLE;
  stateListAdd(&ptable.list[p->state], p);
  #else
  p->state = RUNNABLE;
  #endif //CS333_P4

#ifdef CS333_P2
  p->uid = INIT_UID;
  p->gid = INIT_GID;
#endif //CS333_P2
  release(&ptable.lock);
}

// Grow current process's memory by n bytes.
// Return 0 on success, -1 on failure.
  int
growproc(int n)
{
  uint sz;
  struct proc *curproc = myproc();

  sz = curproc->sz;
  if(n > 0){
    if((sz = allocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  } else if(n < 0){
    if((sz = deallocuvm(curproc->pgdir, sz, sz + n)) == 0)
      return -1;
  }
  curproc->sz = sz;
  switchuvm(curproc);
  return 0;
}

// Create a new process copying p as the parent.
// Sets up stack to return as if from system call.
// Caller must set state of returned proc to RUNNABLE.
  int
fork(void)
{
  int i;
  uint pid;

  struct proc *np;
  struct proc *curproc = myproc();

  // Allocate process.
  if((np = allocproc()) == 0){
    return -1;
  }

  // Copy process state from proc.
  if((np->pgdir = copyuvm(curproc->pgdir, curproc->sz)) == 0){
    kfree(np->kstack);
    np->kstack = 0;
#ifdef CS333_P3
    acquire(&ptable.lock);
    np = ptable.list[EMBRYO].head;
    stateListRemove(&ptable.list[np->state], np);
    assertState(np, EMBRYO);
    np->state = UNUSED;
    stateListAdd(&ptable.list[np->state], np); 
    release(&ptable.lock);
#else
    np->state = UNUSED;
#endif //CS333_P3
    return -1;
  }
  np->sz = curproc->sz;
  np->parent = curproc;
  *np->tf = *curproc->tf;

  // Clear %eax so that fork returns 0 in the child.
  np->tf->eax = 0;

  for(i = 0; i < NOFILE; i++)
    if(curproc->ofile[i])
      np->ofile[i] = filedup(curproc->ofile[i]);
  np->cwd = idup(curproc->cwd);

  safestrcpy(np->name, curproc->name, sizeof(curproc->name));

  pid = np->pid;
#ifdef CS333_P2
  np->uid = curproc->uid;
  np->gid = curproc->gid; 
#endif //CS333_P2

  acquire(&ptable.lock);
  #ifdef CS333_P4
  //np->priority = MAXPRIO;
  //np->budget = BUDGET;
  if(stateListRemove(&ptable.list[np->state], np) < 0)
    panic("stateListRemove FAILED in fork\n");
  assertState(np, EMBRYO);
  np->priority = MAXPRIO;
  np->budget = BUDGET;
  np->state = RUNNABLE;
  stateListAdd(&ptable.ready[np->priority], np);
  #elif CS333_P3
  np = ptable.list[EMBRYO].head;
  stateListRemove(&ptable.list[np->state], np);
  assertState(np, EMBRYO);
  np->state = RUNNABLE;
  stateListAdd(&ptable.list[np->state], np);
  #else
  np->state = RUNNABLE;
  #endif //CS333_P4
  release(&ptable.lock);
  return pid;
}

// Exit the current process.  Does not return.
// An exited process remains in the zombie state
// until its parent calls wait() to find out it exited.
#ifdef CS333_P3
void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;

  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock); // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(int i = 1; i < 6; i++){ // iterates through table index
    #ifdef CS333_P4
    if(i == RUNNABLE)
      continue;
    #endif //CS333_P4
    p = ptable.list[i].head;
    while(p){           //iterates through process list
      if(p->parent == curproc){
        p->parent = initproc;
        if(p->state == ZOMBIE)
          wakeup1(initproc);
      }
      p = p->next;
    }
  } 

  #ifdef CS333_P4
  for(int j = 0; j <= MAXPRIO; j++){
    p = ptable.ready[j].head;
    while(p){
      if(p->parent == curproc){
        p->parent = initproc;
        } //EOF if
      p = p->next;
    } //EOF while
  } //EOF for
  #endif //CS333_P4

  // Jump into the scheduler, never to return.
  stateListRemove(&ptable.list[curproc->state], curproc);
  assertState(curproc, RUNNING);
  curproc->state = ZOMBIE;
  stateListAdd(&ptable.list[curproc->state], curproc);
  sched();
  panic("zombie exit");

}
#else
  void
exit(void)
{
  struct proc *curproc = myproc();
  struct proc *p;
  int fd;

  if(curproc == initproc)
    panic("init exiting");

  // Close all open files.
  for(fd = 0; fd < NOFILE; fd++){
    if(curproc->ofile[fd]){
      fileclose(curproc->ofile[fd]);
      curproc->ofile[fd] = 0;
    }
  }

  begin_op();
  iput(curproc->cwd);
  end_op();
  curproc->cwd = 0;

  acquire(&ptable.lock);

  // Parent might be sleeping in wait().
  wakeup1(curproc->parent);

  // Pass abandoned children to init.
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->parent == curproc){
      p->parent = initproc;
      if(p->state == ZOMBIE)
        wakeup1(initproc);
    }
  }

  // Jump into the scheduler, never to return.
  curproc->state = ZOMBIE;
  sched();
  panic("zombie exit");
}
#endif //CS333_P3

// Wait for a child process to exit and return its pid.
// Return -1 if this process has no children.
#ifdef CS333_P3
  int
wait(void)
{
  struct proc *p;
  int havekids;
  uint pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    p = ptable.list[ZOMBIE].head;
    // Scan through table looking for exited children.
    havekids = 0;
    while(p){
      if(p->parent != curproc){
        p = p->next;
        continue;
      }
      if(p->state == ZOMBIE){
        havekids = 1;
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        stateListRemove(&ptable.list[p->state], p);
        assertState(p, ZOMBIE);
        p->state = UNUSED;
        stateListAdd(&ptable.list[p->state], p);
        release(&ptable.lock);
        return pid;
      }
    }

    //sets havekids to 1 for EMBRYO processes whos parent is the current process
    p = ptable.list[EMBRYO].head;
    while(p){
      if(p->parent == curproc)
        havekids = 1;
      p = p->next;
    }

    //sets havekids to 1 for RUNNABLE processes whos parent is the current process
    #ifdef CS333_P4
    for(int i = MAXPRIO; i >= 0; i--){
      p = ptable.ready[i].head;
      while(p){
        if(p->parent == curproc)
          havekids = 1;
        p = p->next; 
      }
    }
    #else
    p = ptable.list[RUNNABLE].head;
    while(p){
      if(p->parent == curproc)
        havekids = 1;
      p = p->next;
    }
    #endif //CS333_P4
    
    //sets havekids to 1 for RUNNING processes whos parent is the current process
    p = ptable.list[RUNNING].head;
    while(p){
      if(p->parent == curproc)
        havekids = 1;
      p = p->next;
    }

    //sets havekids to 1 for SLEEPING processes whos parent is the current process
    p = ptable.list[SLEEPING].head;
    while(p){
      if(p->parent == curproc)
        havekids = 1;
      p = p->next;
    }
    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }
    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
#else
  int
wait(void)
{
  struct proc *p;
  int havekids;
  uint pid;
  struct proc *curproc = myproc();

  acquire(&ptable.lock);
  for(;;){
    // Scan through table looking for exited children.
    havekids = 0;
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->parent != curproc)
        continue;
      havekids = 1;
      if(p->state == ZOMBIE){
        // Found one.
        pid = p->pid;
        kfree(p->kstack);
        p->kstack = 0;
        freevm(p->pgdir);
        p->pid = 0;
        p->parent = 0;
        p->name[0] = 0;
        p->killed = 0;
        p->state = UNUSED;
        release(&ptable.lock);
        return pid;
      }
    }

    // No point waiting if we don't have any children.
    if(!havekids || curproc->killed){
      release(&ptable.lock);
      return -1;
    }

    // Wait for children to exit.  (See wakeup1 call in proc_exit.)
    sleep(curproc, &ptable.lock);  //DOC: wait-sleep
  }
}
#endif //CS333_P3

// Per-CPU process scheduler.
// Each CPU calls scheduler() after setting itself up.
// Scheduler never returns.  It loops, doing:
//  - choose a process to run
//  - swtch to start running that process
//  - eventually that process transfers control
//      via swtch back to the scheduler.
#ifdef CS333_P3
  void
scheduler(void)
{
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
    sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    #ifdef CS333_P4
    for(int i = MAXPRIO; i >= 0; i--){
      struct proc * p = ptable.ready[i].head;
    #else
    struct proc *p = ptable.list[RUNNABLE].head;
    #endif //CS333_P4

      if(p){
        // Switch to chosen process.  It is the process's job
        // to release ptable.lock and then reacquire it
        // before jumping back to us.
#ifdef PDX_XV6
        idle = 0;  // not idle this timeslice
#endif // PDX_XV6
        c->proc = p;
        switchuvm(p);

#ifdef CS333_P4
        if(ticks >= ptable.PromoteAtTime)
          periodicPriorityAdjustment();
        if(stateListRemove(&ptable.ready[p->priority], p) < 0){
          cprintf("index is %d, prio is %d\n", i, p->priority);
          panic("stateListRemove FAILED In scheduler\n");
        }
        assertState(p, RUNNABLE);
        p->state = RUNNING;
        stateListAdd(&ptable.list[p->state], p); 
#else
        stateListRemove(&ptable.list[p->state], p);
        assertState(p, RUNNABLE);
        p->state = RUNNING;
        stateListAdd(&ptable.list[p->state], p);
#endif //CS333_P4

#ifdef CS333_P2
        p->cpu_ticks_in = ticks; 
#endif //CS333_P2
        swtch(&(c->scheduler), p->context);
        switchkvm();

        // Process is done running for now.
        // It should have changed its p->state before coming back.
        c->proc = 0;
        break;
      }
    }
    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }

#endif // PDX_XV6
  }
}
#else
  void
scheduler(void)
{
  struct proc *p;
  struct cpu *c = mycpu();
  c->proc = 0;
#ifdef PDX_XV6
  int idle;  // for checking if processor is idle
#endif // PDX_XV6

  for(;;){
    // Enable interrupts on this processor.
    sti();

#ifdef PDX_XV6
    idle = 1;  // assume idle unless we schedule a process
#endif // PDX_XV6
    // Loop over process table looking for process to run.
    acquire(&ptable.lock);
    for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
      if(p->state != RUNNABLE)
        continue;

      // Switch to chosen process.  It is the process's job
      // to release ptable.lock and then reacquire it
      // before jumping back to us.
#ifdef PDX_XV6
      idle = 0;  // not idle this timeslice
#endif // PDX_XV6
      c->proc = p;
      switchuvm(p);
      p->state = RUNNING;
#ifdef CS333_P2
      p->cpu_ticks_in = ticks; 
#endif //CS333_P2
      swtch(&(c->scheduler), p->context);
      switchkvm();

      // Process is done running for now.
      // It should have changed its p->state before coming back.
      c->proc = 0;
    }
    release(&ptable.lock);
#ifdef PDX_XV6
    // if idle, wait for next interrupt
    if (idle) {
      sti();
      hlt();
    }
#endif // PDX_XV6
  }
}
#endif // CS333_P3

// Enter scheduler.  Must hold only ptable.lock
// and have changed proc->state. Saves and restores
// intena because intena is a property of this
// kernel thread, not this CPU. It should
// be proc->intena and proc->ncli, but that would
// break in the few places where a lock is held but
// there's no process.
  void
sched(void)
{
  int intena;
  struct proc *p = myproc();

  if(!holding(&ptable.lock))
    panic("sched ptable.lock");
  if(mycpu()->ncli != 1)
    panic("sched locks");
  if(p->state == RUNNING)
    panic("sched running");
  if(readeflags()&FL_IF)
    panic("sched interruptible");
  intena = mycpu()->intena;
#ifdef CS333_P2
  p->cpu_ticks_total += ticks - p->cpu_ticks_in;
#endif //CS333_P2
  swtch(&p->context, mycpu()->scheduler);
  mycpu()->intena = intena;
}

// Give up the CPU for one scheduling round.
#ifdef CS333_P3
  void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock
#ifdef CS333_P4
  if(updateBudget(curproc) <= 0)
    demote(curproc);
  if(stateListRemove(&ptable.list[curproc->state], curproc) < 0)
    panic("stateListRemove FAILED in yield\n");
  assertState(curproc, RUNNING);
  curproc->state = RUNNABLE;
  stateListAdd(&ptable.ready[curproc->priority], curproc);
#else
  stateListRemove(&ptable.list[curproc->state], curproc);
  assertState(curproc, RUNNING);
  curproc->state = RUNNABLE;
  stateListAdd(&ptable.list[curproc->state], curproc);
#endif //CS333_P4
  sched();
  release(&ptable.lock);
}
#else
  void
yield(void)
{
  struct proc *curproc = myproc();

  acquire(&ptable.lock);  //DOC: yieldlock
  curproc->state = RUNNABLE;
  sched();
  release(&ptable.lock);
}
#endif //CS333_P3

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

#ifdef CS333_P3
  void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    if (lk) release(lk);
  }
  // Go to sleep.
  p->chan = chan;

  stateListRemove(&ptable.list[p->state], p);
  assertState(p, RUNNING);
  p->state = SLEEPING;
  stateListAdd(&ptable.list[p->state], p);

#ifdef CS333_P4
  if(updateBudget(p) <= 0)
    demote(p);
#endif //CS333_P4

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#else
  void
sleep(void *chan, struct spinlock *lk)
{
  struct proc *p = myproc();

  if(p == 0)
    panic("sleep");

  // Must acquire ptable.lock in order to
  // change p->state and then call sched.
  // Once we hold ptable.lock, we can be
  // guaranteed that we won't miss any wakeup
  // (wakeup runs with ptable.lock locked),
  // so it's okay to release lk.
  if(lk != &ptable.lock){  //DOC: sleeplock0
    acquire(&ptable.lock);  //DOC: sleeplock1
    if (lk) release(lk);
  }
  // Go to sleep.
  p->chan = chan;
  p->state = SLEEPING;

  sched();

  // Tidy up.
  p->chan = 0;

  // Reacquire original lock.
  if(lk != &ptable.lock){  //DOC: sleeplock2
    release(&ptable.lock);
    if (lk) acquire(lk);
  }
}
#endif //CS333_P3

// Wake up all processes sleeping on chan.
// The ptable lock must be held.
#ifdef CS333_P3
  static void
wakeup1(void *chan)
{
  struct proc *p = ptable.list[SLEEPING].head;
  struct proc *temp;
  while(p){
    temp = p->next;
    if(p->state == SLEEPING && p->chan == chan){
#ifdef CS333_P4
      if(stateListRemove(&ptable.list[p->state], p) < 0)
        panic("stateListRemove FAILED in wakeup1\n");
      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.ready[p->priority], p);
#else
      stateListRemove(&ptable.list[p->state], p);
      assertState(p, SLEEPING);
      p->state = RUNNABLE;
      stateListAdd(&ptable.list[p->state], p);
#endif //CS333_P4
    }
    p = temp;
  }
}
#else
  static void
wakeup1(void *chan)
{
  struct proc *p;

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++)
    if(p->state == SLEEPING && p->chan == chan)
      p->state = RUNNABLE;
}
#endif //CS333_P3

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
#ifdef CS333_P3
  int
kill(int pid)
{
  struct proc *p;
  acquire(&ptable.lock);
  p = ptable.list[SLEEPING].head;

  for(int i = 0; i < 6; i++){
  #ifdef CS333_P4
    if(i == RUNNABLE){
      for(int j = 0; j <= MAXPRIO; j++){
        p = ptable.ready[j].head;
        while(p){
          if(p->pid == pid){
            p->killed = 1;
            release(&ptable.lock);
            return 0;
          }
          p = p->next;
        }
      }
      continue;
    }
    #endif //CS333_P4

    p = ptable.list[i].head;
    while(p){
      if(p->pid == pid){
        p->killed = 1;
        // Wake process from sleep if necessary.
        if(p->state == SLEEPING){
#ifdef CS333_P4 
          //if(updateBudget(p) <= 0)
            //demote(p);
          if(stateListRemove(&ptable.list[p->state], p) < 0)
            panic("stateListRemove FAILED in kill\n");
          assertState(p, SLEEPING);
          p->state = RUNNABLE;
          p->priority = MAXPRIO;
          stateListAdd(&ptable.ready[MAXPRIO], p);
#else
          stateListRemove(&ptable.list[p->state], p);
          assertState(p, SLEEPING);
          p->state = RUNNABLE;
          stateListAdd(&ptable.list[p->state], p);
#endif //CS333_P4
        }
        release(&ptable.lock);
        return 0;
      }
      p = p->next;
    }
  }
/*
#ifdef CS333_P4
  for(int j = 0; j < MAXPRIO+1; j++){
    p = ptable.ready[j].head;
    while(p){
      if(p->pid == pid){
        p->killed = 1;
        release(&ptable.lock);
        return 0;
      }
    }
  }
#endif //CS333_P4
*/
  /*
     while(p){
     if(p->pid == pid){
     p->killed = 1;
  // Wake process from sleep if necessary.
  if(p->state == SLEEPING){
  stateListRemove(&ptable.list[p->state], p);
  assertState(p, SLEEPING);
  p->state = RUNNABLE;
  stateListAdd(&ptable.list[p->state], p);
  }
  release(&ptable.lock);
  return 0;
  }
  }
  */
  release(&ptable.lock);
  return -1;
}
#else
  int
kill(int pid)
{
  struct proc *p;

  acquire(&ptable.lock);
  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->pid == pid){
      p->killed = 1;
      // Wake process from sleep if necessary.
      if(p->state == SLEEPING)
        p->state = RUNNABLE;
      release(&ptable.lock);
      return 0;
    }
  }
  release(&ptable.lock);
  return -1;
}
#endif //CS333_P3

// Print a process listing to console.  For debugging.
// Runs when user types ^P on console.
// No lock to avoid wedging a stuck machine further.
  void
procdumpP2(struct proc * p, char * state)
{
  uint correctPID;
  if(p->parent == NULL)
    correctPID = p->pid;
  else
    correctPID = p->parent->pid;

  uint elapsedTicks = ticks - p->start_ticks;

  cprintf("%d\t%s\t%d\t%d\t%d\t%d", p->pid, p->name, p->uid, p->gid, correctPID, elapsedTicks/1000);

  if(elapsedTicks%1000 < 10)
    cprintf(".00%d\t", elapsedTicks%1000);
  if(elapsedTicks%1000 < 100 && elapsedTicks%1000 < 10)
    cprintf(".0%d\t", elapsedTicks%1000);
  else
    cprintf(".%d\t", elapsedTicks%1000);

  cprintf("%d", p->cpu_ticks_total/1000);
  if(p->cpu_ticks_total%1000 < 10)
    cprintf(".00%d\t", p->cpu_ticks_total%1000);
  if(p->cpu_ticks_total%1000 < 100 && p->cpu_ticks_total%1000 < 10)
    cprintf(".0%d\t", p->cpu_ticks_total%1000);
  else
    cprintf(".%d\t", p->cpu_ticks_total%1000);

  cprintf("%s\t\t%d", state, p->sz);
}

  void
procdumpP4(struct proc * p, char * state)
{
  uint correctPID;
  if(p->parent == NULL)
    correctPID = p->pid;
  else
    correctPID = p->parent->pid;

  uint elapsedTicks = ticks - p->start_ticks;

  cprintf("%d\t%s\t\t%d\t%d\t%d\t%d\t%d", p->pid, p->name, p->uid, p->gid, correctPID, p->priority, elapsedTicks/1000);

  if(elapsedTicks%1000 < 10)
    cprintf(".00%d\t", elapsedTicks%1000);
  if(elapsedTicks%1000 < 100 && elapsedTicks%1000 < 10)
    cprintf(".0%d\t", elapsedTicks%1000);
  else
    cprintf(".%d\t", elapsedTicks%1000);

  cprintf("%d", p->cpu_ticks_total/1000);
  if(p->cpu_ticks_total%1000 < 10)
    cprintf(".00%d\t", p->cpu_ticks_total%1000);
  if(p->cpu_ticks_total%1000 < 100 && p->cpu_ticks_total%1000 < 10)
    cprintf(".0%d\t", p->cpu_ticks_total%1000);
  else
    cprintf(".%d\t", p->cpu_ticks_total%1000);

  cprintf("%s\t%d", state, p->sz);
}

  void
procdump(void)
{
  int i;
  struct proc *p;
  char * state;
  uint pc[10];

  //#if defined(CS333_P1)
  //#define HEADER "\nPID      Name    Elapsed     State      Size     PCs\n"
#if defined(CS333_P4)
#define HEADER "pid\tname\t\tuid\tgid\tppid\tPrio\telapsed\tCPU\tstate\tsize\tPCs\n"
#elif defined(CS333_P2)
#define HEADER "pid\tname\tuid\tgid\tppid\telapsed\tCPU\tstate\tsize\tPCs\n"
#endif //CS333_P1

  cprintf(HEADER);

  for(p = ptable.proc; p < &ptable.proc[NPROC]; p++){
    if(p->state == UNUSED)
      continue;
    if(p->state >= 0 && p->state < NELEM(states) && states[p->state])
      state = states[p->state];
    else
      state = "???";
    /*
#ifdef CS333_P1
uint seconds = (ticks - p->start_ticks) / 1000;
uint milliseconds = (ticks - p->start_ticks) % 1000;
cprintf("%d\t %s\t %d.%d\t     %s\t%d   ", p->pid, p->name, seconds, milliseconds, state, p->sz);
#endif
*/

#ifdef CS333_P4
    procdumpP4(p, state);
#elif CS333_P2
    procdumpP2(p, state);
#else
    cprintf("%d\t%s\t%s\t", p->pid, p->name, state);
#endif //CS333_P1

    if(p->state == SLEEPING){
      getcallerpcs((uint*)p->context->ebp+2, pc);
      for(i=0; i<10 && pc[i] != 0; i++)
        cprintf(" %p", pc[i]);
    }
    cprintf("\n");
  }
}

//#ifdef CS333_P2
#ifdef CS333_P4
  uint
getuid(void)
{
  int uid;
  acquire(&ptable.lock);
  uid = myproc()->uid; 
  release(&ptable.lock);
  return uid;
}

  uint
getgid(void)
{
  int gid;
  acquire(&ptable.lock);
  gid = myproc()->gid;
  release(&ptable.lock);
  return gid;
}

  uint
getppid(void)
{
  int ppid;
  acquire(&ptable.lock);
  if(myproc()->parent == NULL)
    ppid = myproc()->pid;
  else
    ppid = myproc()->parent->pid;
  release(&ptable.lock);
  return ppid;
}

  void 
setuidlock(int temp)
{
  acquire(&ptable.lock);
  myproc()->uid = temp;
  release(&ptable.lock);
}

  void
setgidlock(int temp)
{
  acquire(&ptable.lock);
  myproc()->gid = temp;
  release(&ptable.lock);
}

  int
getprocs(uint max, struct uproc * table)
{
  acquire(&ptable.lock);
  int counter = 0;
  int upper_bound = 0;

  if(max >= NPROC)
    upper_bound = NPROC;
  else
    upper_bound = max;

  for(int i = 0; i < upper_bound; ++i){
    if(ptable.proc[i].state == UNUSED || ptable.proc[i].state == EMBRYO) 
      continue;
    else
    {
      table[counter].pid = ptable.proc[i].pid;
      table[counter].uid = ptable.proc[i].uid;
      table[counter].gid = ptable.proc[i].gid;

      if(ptable.proc[i].parent == NULL)
        table[counter].ppid = ptable.proc[i].pid;
      else
        table[counter].ppid = ptable.proc[i].parent->pid;

      #ifdef CS333_P4
      table[counter].priority = ptable.proc[i].priority;
      #endif //CS333_P4

      table[counter].elapsed_ticks = (ticks - ptable.proc[i].start_ticks);
      table[counter].CPU_total_ticks = ptable.proc[i].cpu_ticks_total;

      if(ptable.proc[i].state == 3)
        safestrcpy(table[counter].state, "RUNNABLE", strlen("RUNNABLE") + 1);
      if(ptable.proc[i].state == 2)
        safestrcpy(table[counter].state, "SLEEPING", strlen("SLEEPING") + 1);
      if(ptable.proc[i].state == 4)
        safestrcpy(table[counter].state, "RUNNING", strlen("RUNNING") + 1);
      if(ptable.proc[i].state == 5)
        safestrcpy(table[counter].state, "ZOMBIE", strlen("ZOMBIE") + 1);

      table[counter].size = ptable.proc[i].sz;
      safestrcpy(table[counter].name, ptable.proc[i].name, strlen(ptable.proc[i].name) + 1);

      counter++;
    }
  }
  release(&ptable.lock);
  return counter; 
}
#endif //CS333_P2

#ifdef CS333_P3
  void
control_r(void)
{
  struct proc * p;

  acquire(&ptable.lock);
  #if defined(CS333_P4)
  cprintf("\n");
  for(int i = MAXPRIO; i >= 0; i--){
    p = ptable.ready[i].head;
    if(!p)
      cprintf("NONE in priority level %d\n\n", i);
    while(p){
      if(p == ptable.ready[i].head){
        cprintf("Priority Level %d: ", i);
        //if(i < MAXPRIO){
          //cprintf("%d:", i-MAXPRIO); 
        //}
      }
      //if(i != MAXPRIO)
        //cprintf("-%d", i-MAXPRIO);
      cprintf("(%d, %d)", p->pid, p->budget);
      if(p->next != NULL)
        cprintf(" -> ");
      else
        cprintf("\n\n");
      p = p->next;
    } //EOF while
  } //EOF for
  #else
  p = ptable.list[RUNNABLE].head;
  cprintf("Ready List Processes:\n");
  if(!p)
    cprintf("NONE\n");
  while(p){
    cprintf("%d", p->pid);
    if(p->next != NULL)
      cprintf("->");
    p = p->next;
  }
  cprintf("\n");
  #endif //CS333_P4
  release(&ptable.lock);
}

  void
control_f(void)
{
  struct proc * p;
  int counter = 0;

  acquire(&ptable.lock);
  p = ptable.list[UNUSED].head;
  if(!p)
    cprintf("Free List Size: 0\n");
  while(p){
    ++counter;
    p = p->next;
  }
  cprintf("Free List Size: %d\n", counter);
  release(&ptable.lock);
}

  void
control_s(void)
{
  struct proc * p;
  acquire(&ptable.lock);
  p = ptable.list[SLEEPING].head;
  cprintf("Sleep List Processes:\n");
  if(!p)
    cprintf("NONE\n");
  while(p){
    cprintf("%d", p->pid);
    if(p->next != NULL)
      cprintf("->");
    p = p->next;
  }
  cprintf("\n");
  release(&ptable.lock);
}

  void
control_z(void)
{
  struct proc * p;
  acquire(&ptable.lock);
  p = ptable.list[ZOMBIE].head;
  cprintf("Zombie List Processes:\n");
  if(!p)
    cprintf("NONE\n");
  while(p){
    if(p->parent)
      cprintf("(%d, %d)", p->pid, p->parent->pid);
    else
      cprintf("(%d, %d)", p->pid, p->pid);
    if(p->next != NULL)
      cprintf("->");
    p = p->next;
  }
  cprintf("\n");
  release(&ptable.lock);
}

  void
printListStats(void)
{
  int i, count, total = 0;
  struct proc *p;

  acquire(&ptable.lock);
  for (i=UNUSED; i<=ZOMBIE; i++) {
    count = 0;
    p = ptable.list[i].head;
    if (p != NULL)
      while (p != NULL) {
        count++;
        p = p->next;
      }
    cprintf("\n%s list has ", states[i]);
    if (count < 10) cprintf(" ");  // line up columns
    cprintf("%d processes", count);
    total += count;
  }
  release(&ptable.lock);
  cprintf("\nTotal on lists is: %d. NPROC = %d. %s",
      total, NPROC, (total == NPROC) ? "Congratulations!" : "Bummer");
  cprintf("\n$ ");  // simulate shell prompt
  return;
}
#endif //CS333_P3

#ifdef CS333_P4
//Promotion
  void
periodicPriorityAdjustment()
{ 
  for(int i = MAXPRIO-1; i >= 0; i--){
    struct proc * p = ptable.ready[i].head;
    while(p){
      if(stateListRemove(&ptable.ready[i], p) < 0)
        panic("stateListRemove FAILED in periodicPriorityAdjustment\n");
      p->priority = i+1;
      p->budget = BUDGET;
      //test
      if(p->next){
        struct proc * temp = p->next;
        stateListAdd(&ptable.ready[i+1], p);  
        //updateBudget(p);
        p = temp;
      }
      else{
        stateListAdd(&ptable.ready[i+1], p);
        //updateBudget(p);
        p = p->next;
      }
      //p = p->next;
      //test
      //p = temp;
    }
  }

  for(int i = SLEEPING; i <= RUNNING; i++){
    if(i == RUNNABLE)
      continue;
    struct proc * p = ptable.list[i].head;
    while(p){
      if(p->priority == MAXPRIO){
        p = p->next;
        continue;
      }
      p->priority++;
      //updateBudget(p);
      p = p->next;
    } 
  }
  ptable.PromoteAtTime = ticks + TICKS_TO_PROMOTE;
}

//Demotion
  void
demote(struct proc * p)
{
  p->budget = BUDGET; //Set budget to default

  if(p->priority == 0){
    return;
  }
  else
    p->priority = p->priority - 1;

}

  struct proc*
matchThenRetrieveActiveProcess(int pid)
{
  for(int i = SLEEPING; i < ZOMBIE; i++){
    if(i != RUNNABLE){
      struct proc * p = ptable.list[i].head;
      while(p){
        if(p->pid == pid){
          return p;
        }
        p = p->next;
      }
    }
    else{
      for(int j = 0; j <= MAXPRIO; j++){
        struct proc * p = ptable.ready[j].head;
        while(p){
          if(p->pid == pid){
            return p;
          }
          p = p->next;
        }//EOF while
      } //EOF j for
    }//EOF else
  } //EOF i for
  return NULL;
}

  int
updateBudget(struct proc * p)
{
  p->budget = p->budget - (ticks - p->cpu_ticks_in);
  return p->budget;
}

  int
updatePriority(struct proc * p, int setPriority)
{
  if(p->state == RUNNING || p->state == SLEEPING){
    if(p->priority < 0)
      return -1;
    else if(p->priority == setPriority)
      return 0;
    p->priority = setPriority;
    p->budget = BUDGET;
    //updateBudget(p);
  }
  else{
    if(p->priority < 0)
      return -1;
    else if(p->priority == setPriority)
      return 0;
    else{
      if(stateListRemove(&ptable.ready[p->priority], p) < 0)
        panic("stateListRemove FAILED in updatePriority\n");
      p->priority = setPriority;
      stateListAdd(&ptable.ready[setPriority], p);
      p->budget = BUDGET;
      //updateBudget(p);
    } //EOF 2nd else
  } //EOF 1st else
  return 1;
}

  int
setpriority(int pid, int priority)
{
  struct proc * p;

  acquire(&ptable.lock);
  if(priority < 0 || priority > MAXPRIO){
    p = matchThenRetrieveActiveProcess(pid);
    if(!p){
      release(&ptable.lock);
      return -3;
    }
    release(&ptable.lock);
    return -1;
  }

  p = matchThenRetrieveActiveProcess(pid);
  if(!p){
    release(&ptable.lock);
    return -2;
  }

  int rc = updatePriority(p, priority);
  if(rc < 0)  
    cprintf("ERROR! priority is out of bounds\n");
  if(rc == 0)  
    cprintf("Current priority level is equal to the priority level you entered. Priority will remain the same and the budget will not be updated.\n");

  release(&ptable.lock);
  return rc;
}

  int
getpriority(int pid)
{
  acquire(&ptable.lock);
  struct proc * p = matchThenRetrieveActiveProcess(pid);
  if(!p){
    release(&ptable.lock);
    return -1;
  }
  cprintf("Priority level of process with PID = %d is %d\n", pid, p->priority);
  release(&ptable.lock);
  return p->priority;
}
#endif //CS333_P4
