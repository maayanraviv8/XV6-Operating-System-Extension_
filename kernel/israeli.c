#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "israeli.h"
#include "defs.h"

static struct israelilock israeli_locks[NISLOCK];

//Called once during boot.  Initialises every lock slot's spinlock and
//marks every slot as free.
void
israeli_init(void)
{
  for(int i = 0; i < NISLOCK; i++){
    initlock(&israeli_locks[i].lk, "israeli");
    israeli_locks[i].active = 0;
    israeli_locks[i].held = 0;
    israeli_locks[i].favoritism = 0;
    israeli_locks[i].holder = 0;
    israeli_locks[i].nwaiters = 0;
  }
}

//Bounds-check a user-supplied lock id and return the slot, or 0.
static struct israelilock *
lock_by_id(int id)
{
  if(id < 0 || id >= NISLOCK)
    return 0;
  return &israeli_locks[id];
}

//Remove queue[idx], shifting the tail left.  L->lk must be held.
static void
queue_remove(struct israelilock *L, int idx)
{
  for(int i = idx; i < L->nwaiters - 1; i++)
    L->queue[i] = L->queue[i + 1];
  L->queue[L->nwaiters - 1] = 0;
  L->nwaiters--;
}

//Find the position of p in the queue, or -1.  L->lk must be held.
static int
queue_find(struct israelilock *L, struct proc *p)
{
  for(int i = 0; i < L->nwaiters; i++)
    if(L->queue[i] == p)
      return i;
  return -1;
}

//Pick the next owner from the queue per the Israeli policy.
//Returns the chosen index, or -1 if the queue is empty.
//L->lk must be held.
static int
choose_winner(struct israelilock *L, int releaser_gid)
{
  if(L->nwaiters == 0)
    return -1;

  //Search for the earliest waiter sharing the releaser's gid.
  int same_gid = -1;
  for(int i = 0; i < L->nwaiters; i++){
    if(L->queue[i]->gid == releaser_gid){
      same_gid = i;
      break;
    }
  }

  //With probability favoritism% favour the same-gid waiter.
  if(same_gid >= 0 && (lcg_rand() % 100) < (uint)L->favoritism)
    return same_gid;

  return 0;  //FIFO head
}

int
israeli_create(int favoritism)
{
  if(favoritism < 0 || favoritism > 100)
    return -1;

  for(int i = 0; i < NISLOCK; i++){
    struct israelilock *L = &israeli_locks[i];
    acquire(&L->lk);
    if(!L->active){
      L->active = 1;
      L->held = 0;
      L->favoritism = favoritism;
      L->holder = 0;
      L->nwaiters = 0;
      release(&L->lk);
      return i;
    }
    release(&L->lk);
  }
  return -1;  //every slot in use
}

int
israeli_acquire(int id)
{
  struct israelilock *L = lock_by_id(id);
  if(!L)
    return -1;

  struct proc *p = myproc();

  acquire(&L->lk);
  if(!L->active){
    release(&L->lk);
    return -1;
  }

  //lock is free and queue is empty -> take it directly.
  if(!L->held && L->nwaiters == 0){
    L->held = 1;
    L->holder = p;
    release(&L->lk);
    return 0;
  }

  //enqueue and sleep until someone hands us ownership.
  if(L->nwaiters >= NISWAITERS){
    release(&L->lk);
    return -1;
  }
  L->queue[L->nwaiters++] = p;

  for(;;){
    //sleep atomically releases L->lk; reacquires it on wakeup.
    sleep(p, &L->lk);

    if(!L->active){
      int idx = queue_find(L, p);
      if(idx >= 0)
        queue_remove(L, idx);
      release(&L->lk);
      return -1;
    }

    if(L->held && L->holder == p){
      //The previous releaser already transferred ownership to us.
      release(&L->lk);
      return 0;
    }

    if(killed(p)){
      int idx = queue_find(L, p);
      if(idx >= 0)
        queue_remove(L, idx);
      release(&L->lk);
      return -1;
    }
  }
}

int
israeli_release(int id)
{
  struct israelilock *L = lock_by_id(id);
  if(!L)
    return -1;

  struct proc *p = myproc();

  acquire(&L->lk);
  if(!L->active || !L->held || L->holder != p){
    release(&L->lk);
    return -1;
  }

  int winner_idx = choose_winner(L, p->gid);
  if(winner_idx < 0){
    //No waiters- simply release the lock.
    L->held = 0;
    L->holder = 0;
  } else {
    //Hand ownership to the chosen waiter atomically.
    struct proc *winner = L->queue[winner_idx];
    queue_remove(L, winner_idx);
    L->holder = winner;
    wakeup(winner);
  }
  release(&L->lk);
  return 0;
}

int
israeli_destroy(int id)
{
  struct israelilock *L = lock_by_id(id);
  if(!L)
    return -1;

  acquire(&L->lk);
  //Refuse to destroy a lock that is in use.
  if(!L->active || L->held || L->nwaiters > 0){
    release(&L->lk);
    return -1;
  }
  L->active = 0;
  release(&L->lk);
  return 0;
}
