#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0;  // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(killed(myproc())){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

uint64
sys_lcg_srand(void)
{
  int seed;

  argint(0, &seed);
  lcg_srand((uint)seed);
  return 0;
}

uint64
sys_lcg_rand(void)
{
  return (uint64)lcg_rand();
}

uint64
sys_setgid(void)
{
  int gid;

  argint(0, &gid);
  proc_setgid(gid);
  return 0;
}

uint64
sys_getgid(void)
{
  return (uint64)proc_getgid();
}

uint64
sys_israeli_create(void)
{
  int favoritism;

  argint(0, &favoritism);
  return (uint64)israeli_create(favoritism);
}

uint64
sys_israeli_acquire(void)
{
  int id;

  argint(0, &id);
  return (uint64)israeli_acquire(id);
}

uint64
sys_israeli_release(void)
{
  int id;

  argint(0, &id);
  return (uint64)israeli_release(id);
}

uint64
sys_israeli_destroy(void)
{
  int id;

  argint(0, &id);
  return (uint64)israeli_destroy(id);
}

uint64
sys_team_score_inc(void)
{
  int team;

  argint(0, &team);
  return (uint64)team_score_inc(team);
}

uint64
sys_team_score_get(void)
{
  int team;

  argint(0, &team);
  return (uint64)team_score_get(team);
}

uint64
sys_team_scores_reset(void)
{
  team_scores_reset();
  return 0;
}
