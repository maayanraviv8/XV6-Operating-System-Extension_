#define NISLOCK    16   //total number of available locks
#define NISWAITERS 16   //max simultaneous waiters per lock

struct israelilock {
  struct spinlock lk;                 //protects every field below
  int             active;             //1 iff this slot is a live lock
  int             held;               //1 while some process owns the lock
  int             favoritism;         //0..100, fixed at create time
  struct proc    *holder;             //current owner, 0 if !held
  int             nwaiters;           //current queue size
  struct proc    *queue[NISWAITERS];  //FIFO of waiting processes
};
