#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

#define DEFAULT_FAVORITISM 50
#define DEFAULT_TEAMS      3
#define DEFAULT_RUNNERS    5
#define DEFAULT_TARGET     30

//A runner's main loop.  Repeatedly takes the baton, increments the
//team score, prints, releases, and rests.
static void
runner(int team, int lock_id, int teams, int target)
{
  setgid(team);
  for(;;) {
    if(israeli_acquire(lock_id) < 0) {
      printf("runner pid=%d team=%d: acquire failed\n", getpid(), team);
      exit(1);
    }

    //Check end-of-race condition while holding the baton.
    int race_over = 0;
    for(int t = 0; t < teams; t++) {
      if(team_score_get(t) >= target) {
        race_over = 1;
        break;
      }
    }

    if(!race_over) {
      int score = team_score_inc(team);
      printf("Runner %d (Team %d) acquired the baton\n", getpid(), team);
      printf("Team %d score = %d\n", team, score);
    }

    if(israeli_release(lock_id) < 0) {
      printf("runner pid=%d team=%d: release failed\n", getpid(), team);
      exit(1);
    }

    if(race_over)
      break;
    sleep(1);  //Sleep briefly
  }
  exit(0);
}

int
main(int argc, char *argv[])
{
  int favoritism = (argc > 1) ? atoi(argv[1]) : DEFAULT_FAVORITISM;
  int teams      = (argc > 2) ? atoi(argv[2]) : DEFAULT_TEAMS;
  int runners    = (argc > 3) ? atoi(argv[3]) : DEFAULT_RUNNERS;
  int target     = (argc > 4) ? atoi(argv[4]) : DEFAULT_TARGET;

  if(favoritism < 0 || favoritism > 100 || teams <= 0 ||
     runners <= 0 || target <= 0) {
    printf("usage: relay_race [favoritism 0..100] [teams>0] "
           "[runners>0] [target>0]\n");
    exit(1);
  }

  printf("=== Relay Race ===\n");
  printf("favoritism=%d teams=%d runners/team=%d target=%d\n",
         favoritism, teams, runners, target);

  int lock_id = israeli_create(favoritism);
  if(lock_id < 0) {
    printf("israeli_create failed\n");
    exit(1);
  }

  team_scores_reset();
  lcg_srand(getpid());

  int total = teams * runners;
  for(int t = 0; t < teams; t++) {
    for(int r = 0; r < runners; r++) {
      int pid = fork();
      if(pid < 0) {
        printf("fork failed\n");
        exit(1);
      }
      if(pid == 0)
        runner(t, lock_id, teams, target);   //never returns
    }
  }

  for(int i = 0; i < total; i++)
    wait(0);

  //Race summary
  int winner = -1, winner_score = -1;
  printf("\n=== Final scores ===\n");
  for(int t = 0; t < teams; t++) {
    int s = team_score_get(t);
    printf("Team %d: %d\n", t, s);
    if(s > winner_score) {
      winner_score = s;
      winner = t;
    }
  }
  printf("Winner: Team %d (score %d)\n", winner, winner_score);

  if(israeli_destroy(lock_id) < 0)
    printf("israeli_destroy failed\n");

  exit(0);
}
