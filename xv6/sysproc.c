#include "types.h"
#include "x86.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "mmu.h"
#include "proc.h"
#ifdef PDX_XV6
#include "pdx-kernel.h"
#endif // PDX_XV6

#ifdef CS333_P2
#include "uproc.h"
#endif // CS333_P2


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
  return myproc()->pid;
}

int
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  addr = myproc()->sz;
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
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      return -1;
    }
    sleep(&ticks, (struct spinlock *)0);
  }
  return 0;
}

// return how many clock tick interrupts have occurred
// since start.
int
sys_uptime(void)
{
  uint xticks;
  xticks = ticks;
  return xticks;
}

#ifdef PDX_XV6
// Turn off the computer
int
sys_halt(void)
{
  cprintf("Shutting down ...\n");
  outw( 0x604, 0x0 | 0x2000);
  return 0;
}
#endif // PDX_XV6

#ifdef CS333_P1
int
sys_date(void)
{
  struct rtcdate * d;
  if(argptr(0, (void*)&d, sizeof(struct rtcdate)) < 0)
    return -1;
  else
  {
    cmostime(d);
    return 0;
  }
}
#endif //CS333_P2

#ifdef CS333_P2
uint
sys_getuid(void)
{
  if(myproc() != NULL)
    return getuid();
  else
    return -1; //error
}

uint
sys_getgid(void)
{
  if(myproc() != NULL)
    return getgid();
  else
    return -1; //error 
}

uint
sys_getppid(void)
{
  if(myproc()->parent == NULL)
    return getppid();
  else
    return getppid();
}

int 
sys_setuid(void)
{
  int temp = 0;
  if(argint(0, &temp) < 0)
    return -1;
  if(temp >= 0 && temp <= 32767)
    setuidlock(temp);   
  else
    return -1;
  return 0;
}

int 
sys_setgid(void)
{
  int temp = 0;
  if(argint(0, &temp) < 0)
    return -1;
  if(temp >= 0 && temp <= 32767)
    setgidlock(temp);
  else
    return -1;
  return 0;
}

int
sys_getprocs(void)
{
  int max = 0;
  struct uproc * table; 

  if(argint(0,&max) < 0 || argptr(1, (void*)&table, (max * sizeof(struct uproc))) < 0)
    return -1;
  int active_proc_count = getprocs(max, table);
  return active_proc_count;
}
#endif // CS333_P2

#ifdef CS333_P4
int
sys_setpriority(void)
{
  int pid = 0;
  int priority = 0;

  if((argint(0, &pid) < 0) || (argint(1, &priority) < 0))
    return -1;

  int rc = setpriority(pid, priority);
  
  if(rc < 0){
    if(rc == -1)
      cprintf("ERROR! Priority is out of bounds!\n");
    if(rc == -2)
      cprintf("ERROR! Process of that PID Does not exist!\n");
    if(rc == -3)
      cprintf("ERROR! That Process is not active or may not exist. The priority is also out of bounds!\n");
    return -1;
  }

  return 0;
}

int
sys_getpriority(void)
{
  int pid = 0;

  if(argint(0, &pid) < 0)
    return -1;

  int priority = getpriority(pid); 
  if(priority < 0)
    cprintf("ERROR! pid is not that of an Active Process or it DNE!\n");

  return priority;
}
#endif // CS333_P4
