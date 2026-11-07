#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "defs.h"

#define NTEAMS 16

static struct spinlock team_lock;
static int team_scores[NTEAMS];

void
team_init(void)
{
  initlock(&team_lock, "teams");
  for(int i = 0; i < NTEAMS; i++)
    team_scores[i] = 0;
}

int
team_score_inc(int team)
{
  if(team < 0 || team >= NTEAMS)
    return -1;

  int v;
  acquire(&team_lock);
  team_scores[team]++;
  v = team_scores[team];
  release(&team_lock);
  return v;
}

int
team_score_get(int team)
{
  if(team < 0 || team >= NTEAMS)
    return -1;

  int v;
  acquire(&team_lock);
  v = team_scores[team];
  release(&team_lock);
  return v;
}

void
team_scores_reset(void)
{
  acquire(&team_lock);
  for(int i = 0; i < NTEAMS; i++)
    team_scores[i] = 0;
  release(&team_lock);
}
