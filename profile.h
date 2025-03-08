//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#ifndef ProfileH
#define ProfileH

#ifdef PROFILE

#define PROFILE_LINUX   0
#define PROFILE_WINDOWS 1

#define PROFILE_PLATFORM PROFILE_LINUX

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#if PROFILE_PLATFORM == PROFILE_LINUX

#include <unistd.h>
#include <threads.h>
#include <x86intrin.h>

#define PID return_pid(thrd_current())

typedef mtx_t profile_mutex_t;

#define PROFILE_MUTEX_INIT(M) PROFILE_BUG(mtx_init(&M, mtx_plain) != 0)
#define PROFILE_MUTEX_LOCK(M) mtx_lock(&M);
#define PROFILE_MUTEX_UNLOCK(M) mtx_unlock(&M);

#define PROFILE_SLEEP(X) sleep(X);

#else
#include <Windows.h>
#include <intrin.h>

#define PID return_pid(GetCurrentThreadId())

typedef SRWLOCK profile_mutex_t;

#define PROFILE_MUTEX_INIT(M) InitializeSRWLock(&M);
#define PROFILE_MUTEX_LOCK(M) AcquireSRWLockExclusive(&M);
#define PROFILE_MUTEX_UNLOCK(M) ReleaseSRWLockExclusive(&M);

#define PROFILE_SLEEP(X) Sleep(X * 1000);

#endif

#define PROFILE_INVALID (-1)
#define THREAD_MAX      64
#define RECURSE_MAX     100

#define PG profile_global[pid]
#define PS profile_static[pid]

//typedef struct timespec counter_t;
typedef long long counter_t;

typedef struct
{
  counter_t counter_stamp;
  counter_t *counter_pointer;
} profile_global_t;

typedef struct
{
  int block_id[RECURSE_MAX];
  int block_init;
  int block_invocation;
} profile_static_t;

extern profile_global_t profile_global[THREAD_MAX];

int return_pid(int);
void init_block(int [RECURSE_MAX]);
int new_block(int, const char *, int *);
void begin_block(int, int);
void end_block(int);
void init_profile(void);
void clear_profile(void);
void dump_profile(int, int);

#define GET_COUNTER_BEGIN(P)\
{\
  unsigned int dummy;\
  *P = __rdtscp(&dummy);\
}

#define GET_COUNTER_END(P)\
{\
  unsigned int dummy;\
  *P = __rdtscp(&dummy);\
}

#define BEGIN_BLOCK(X) \
  {\
    static profile_static_t profile_static[THREAD_MAX];\
    counter_t counter_stamp;\
    GET_COUNTER_END(&counter_stamp);\
    int pid = PID;\
    PG.counter_stamp = counter_stamp;\
    if (PS.block_init == 0) {init_block(PS.block_id); PS.block_init = 1;}\
    PS.block_invocation++;\
    if (PS.block_id[PS.block_invocation] == PROFILE_INVALID)\
      PS.block_id[PS.block_invocation] = new_block(pid, X, &(PS.block_invocation));\
    begin_block(pid, PS.block_id[PS.block_invocation]);\
    GET_COUNTER_BEGIN(PG.counter_pointer);\
  }
#define END_BLOCK \
  {\
    counter_t counter_stamp;\
    GET_COUNTER_END(&counter_stamp);\
    int pid = PID;\
    PG.counter_stamp = counter_stamp;\
    end_block(pid);\
    GET_COUNTER_BEGIN(PG.counter_pointer);\
  }

#define INIT_PROFILE init_profile();
#define DUMP_PROFILE(V) dump_profile(PID, V);

#else
#define BEGIN_BLOCK(X)
#define END_BLOCK
#define INIT_PROFILE
#define DUMP_PROFILE(V)
#endif

#endif

