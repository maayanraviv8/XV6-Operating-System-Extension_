#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define LCG_A 1664525u
#define LCG_B 1013904223u

static struct spinlock lcg_lock;
static uint lcg_state;

//Called once during boot (from main.c) to initialise the spinlock
//and pick a non-zero default state.
void
lcg_init(void)
{
  initlock(&lcg_lock, "lcg");
  lcg_state = 1;
}

//Seed the generator.
void
lcg_srand(uint seed)
{
  acquire(&lcg_lock);
  lcg_state = seed;
  release(&lcg_lock);
}

//Advance the generator one step and return the new state.
uint
lcg_rand(void)
{
  uint next;

  acquire(&lcg_lock);
  next = LCG_A * lcg_state + LCG_B;   //mod 2^32 by unsigned overflow
  lcg_state = next;
  release(&lcg_lock);
  return next;
}
