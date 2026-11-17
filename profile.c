// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "profile.h"

#ifdef PROFILE

#define PROFILE_BUG(X)                                             \
  if (X)                                                           \
  {                                                                \
    fprintf(stderr, "%s::%ld:%s\n", __FILE__, (long)__LINE__, #X); \
    exit(EXIT_FAILURE);                                            \
  }

#define local static
#define FALSE 0
#define TRUE 1
#define or ||

#define NAME_MAX 32
#define BLOCK_MAX 200
#define STACK_MAX 100

#define TICKS(TV) (TV)
#define SECS(X) ((double)(X) / (double)frequency)
#define PERC(X) ((X) / time_self_total * 100)

#define PL profile_local[pid]

// call stack
typedef struct
{
  int stack_id;

  double stack_time_self;
  double stack_time_total;

  counter_t stack_counter_begin;
  counter_t stack_counter_end;
} stack_t;

typedef struct
{
  int child_active;
  long long child_ncalls;
  double child_time_total;
} child_t;

typedef struct
{
  int parent_active;
  long long parent_ncalls;
} parent_t;

typedef struct
{
  char block_name[NAME_MAX];
  int block_invocation;

  // needed for end_block
  int *block_invocation_pointer;

  long long block_ncalls;

  double block_time_self_total;
  double block_time_total;

  long long block_nchild_calls;
  double block_child_time_total;

  parent_t block_parents[BLOCK_MAX];
  child_t block_children[BLOCK_MAX];

  double block_time_recursive_total;
  long long block_ntotal_recursive_calls;
} block_t;

typedef struct
{
  int nstack_entries;
  stack_t stack_entries[STACK_MAX];

  int nblocks;
  block_t blocks[BLOCK_MAX];

  counter_t counter_overhead_begin;
  counter_t counter_overhead_end;
  double time_total;
} profile_local_t;

profile_global_t profile_global[THREAD_MAX];

local profile_mutex_t profile_mutex;

// tid = thread id
// pid = logical thread id

local int tids[THREAD_MAX];
local profile_local_t profile_local[THREAD_MAX];

local long long frequency;
local counter_t counter_dummy;

#define NEXCEPTIONS_MAX 1024

local long long counter_mean;
local long long counter_sigma;
local long long ncounter_largest;
local long long counter_largest;

int return_pid(int tid)
{
  int result;

  PROFILE_MUTEX_LOCK(profile_mutex)

  for (result = 0; result < THREAD_MAX; result++)
    if (tids[result] == tid)
      break;

  if (result >= THREAD_MAX)
  {
    for (result = 0; result < THREAD_MAX; result++)
      if (tids[result] == PROFILE_INVALID)
        break;

    PROFILE_BUG(result >= THREAD_MAX)

    tids[result] = tid;
  }

  PROFILE_MUTEX_UNLOCK(profile_mutex)

  return (result);
}

void init_block(int block_id[RECURSE_MAX])
{
  for (int iblock = 0; iblock < RECURSE_MAX; iblock++)
    block_id[iblock] = PROFILE_INVALID;
}

local void clear_block(block_t *with_block)
{
  with_block->block_ncalls = 0;

  with_block->block_time_self_total = 0.0;
  with_block->block_time_total = 0.0;

  for (int iblock = 0; iblock < BLOCK_MAX; iblock++)
  {
    child_t *with_child = with_block->block_children + iblock;

    with_child->child_active = FALSE;
    with_child->child_ncalls = 0;
    with_child->child_time_total = 0.0;

    parent_t *with_parent = with_block->block_parents + iblock;

    with_parent->parent_active = FALSE;
    with_parent->parent_ncalls = 0;
  }
}

#define MANGLE_MAX 256

local void mangle(char *dest, const char *source)
{
  char m[MANGLE_MAX];

  if (strlen(source) < NAME_MAX)
    strncpy(dest, source, NAME_MAX);
  else
  {
    PROFILE_BUG(strlen(source) >= MANGLE_MAX)

    strncpy(m, source, MANGLE_MAX);

    // remove vowels from the right

    int n = strlen(m);

    while (n >= NAME_MAX)
    {
      // search for a vowel or underscore from the right

      while (n >= 0)
      {
        // first vowels

        if ((m[n] == 'a') || (m[n] == 'o') or (m[n] == 'u') or (m[n] == 'i') or
            (m[n] == 'e'))
          break;

        // then underscores

        if (m[n] == '_')
          break;

        n--;
      }
      PROFILE_BUG(n < 0)

      // remove vowel

      while ((m[n] = m[n + 1]) != '\0')
        n++;
    }
    PROFILE_BUG(strlen(m) >= NAME_MAX)

    strncpy(dest, m, NAME_MAX - 1);
  }
}

int new_block(int pid, const char *name, int *invocation_pointer)
{
  if (PL.nblocks >= BLOCK_MAX)
  {
    for (int iblock = 0; iblock < PL.nblocks; iblock++)
    {
      block_t *with_block = PL.blocks + iblock;

      fprintf(stderr,
              "iblock=%d block_name=%s\n",
              iblock,
              with_block->block_name);
    }

    PROFILE_BUG(PL.nblocks >= BLOCK_MAX)
  }

  int block_id = PL.nblocks;

  block_t *with_block = PL.blocks + block_id;

  mangle(with_block->block_name, name);

  with_block->block_name[NAME_MAX - 1] = '\0';

  with_block->block_invocation = *invocation_pointer;

  with_block->block_invocation_pointer = invocation_pointer;

  clear_block(with_block);

  PL.nblocks++;

  return (block_id);
}

local void update_mean_sigma(long long n, long long x, double *mn, double *sn)
{
  double mnm1 = *mn;
  double snm1 = *sn;

  *mn = mnm1 + (x - mnm1) / n;

  *sn = snm1 + (x - mnm1) * (x - *mn);
}

#define NCALIBRATION 2

local long long counter_correction(int pid, long long counter_delta)
{
  PG.counter_pointer = &counter_dummy;

  double mn = 0.0;
  double sn = 0.0;

  for (long long n = 1; n <= NCALIBRATION; ++n)
  {
    counter_t counter_stamp;

    GET_COUNTER_BEGIN(&counter_stamp)
    GET_COUNTER_END(PG.counter_pointer)

    update_mean_sigma(n, TICKS(counter_dummy) - TICKS(counter_stamp), &mn, &sn);
  }

  long long result = round(mn);

  result = counter_delta - result;

  if (result < 0)
    result = 0;

  return (result);
}

void begin_block(int pid, int block_id)
{
  if (PL.nstack_entries > 0)
  {
    stack_t *with_previous = PL.stack_entries + PL.nstack_entries - 1;

    with_previous->stack_counter_end = PG.counter_stamp;

    long long counter_delta = TICKS(with_previous->stack_counter_end) -
                              TICKS(with_previous->stack_counter_begin);

    with_previous->stack_time_self +=
      SECS(counter_correction(pid, counter_delta));
  }
  else
  {
    GET_COUNTER_BEGIN(&(PL.counter_overhead_begin))
  }
  PROFILE_BUG(PL.nstack_entries >= STACK_MAX)

  stack_t *with_current = PL.stack_entries + PL.nstack_entries;

  with_current->stack_id = block_id;

  with_current->stack_time_self = 0.0;

  with_current->stack_time_total = 0.0;

  PG.counter_pointer = &(with_current->stack_counter_begin);

  PL.nstack_entries++;
}

void end_block(int pid)
{
  PL.nstack_entries--;

  PROFILE_BUG(PL.nstack_entries < 0)

  stack_t *with_current = PL.stack_entries + PL.nstack_entries;

  with_current->stack_counter_end = PG.counter_stamp;

  long long counter_delta = TICKS(with_current->stack_counter_end) -
                            TICKS(with_current->stack_counter_begin);

  with_current->stack_time_self += SECS(counter_correction(pid, counter_delta));

  with_current->stack_time_total += with_current->stack_time_self;

  block_t *with_block = PL.blocks + with_current->stack_id;

  with_block->block_ncalls++;

  with_block->block_time_self_total += with_current->stack_time_self;

  with_block->block_time_total += with_current->stack_time_total;

  (*with_block->block_invocation_pointer)--;

  PROFILE_BUG(*with_block->block_invocation_pointer < 0);

  PG.counter_pointer = &counter_dummy;

  if (PL.nstack_entries > 0)
  {
    stack_t *with_previous = PL.stack_entries + (PL.nstack_entries - 1);

    with_previous->stack_time_total += with_current->stack_time_total;

    PG.counter_pointer = &(with_previous->stack_counter_begin);

    // update parent in child

    parent_t *with_parent =
      PL.blocks[with_current->stack_id].block_parents + with_previous->stack_id;

    with_parent->parent_active = TRUE;

    with_parent->parent_ncalls++;

    // update child in parent

    child_t *with_child = PL.blocks[with_previous->stack_id].block_children +
                          with_current->stack_id;

    with_child->child_active = TRUE;

    with_child->child_ncalls++;

    with_child->child_time_total += with_current->stack_time_total;
  }
  else
  {
    GET_COUNTER_END(&(PL.counter_overhead_end))

    counter_delta =
      TICKS(PL.counter_overhead_end) - TICKS(PL.counter_overhead_begin);

    PL.time_total += SECS(counter_delta);
  }
}

#define NVALIDATE 100000LL

local void validate_counter_correction(void)
{
  BEGIN_BLOCK("main")

  for (long long n = 1; n <= NVALIDATE; ++n)
  {
    BEGIN_BLOCK("profile-0-0")
    END_BLOCK

    BEGIN_BLOCK("profile-1-1")
    BEGIN_BLOCK("profile-1-1-0")
    END_BLOCK
    END_BLOCK

    BEGIN_BLOCK("profile-2-2")
    BEGIN_BLOCK("profile-2-2-1-0")
    END_BLOCK
    BEGIN_BLOCK("profile-2-2-2-0")
    END_BLOCK
    END_BLOCK

    BEGIN_BLOCK("profile-3-1")
    BEGIN_BLOCK("profile-3-3-1-3")
    BEGIN_BLOCK("profile-3-3-1-3-1-0")
    END_BLOCK
    BEGIN_BLOCK("profile-3-3-1-3-2-0")
    END_BLOCK
    BEGIN_BLOCK("profile-3-3-1-3-3-0")
    END_BLOCK
    END_BLOCK
    END_BLOCK
  }
  END_BLOCK
  DUMP_PROFILE(1)
  exit(0);
}

#define NCALL 1000000LL

void init_profile(void)
{
  PROFILE_MUTEX_INIT(profile_mutex)

  for (int ithread = 0; ithread < THREAD_MAX; ithread++)
  {
    tids[ithread] = PROFILE_INVALID;

    profile_local_t *with_profile_local = profile_local + ithread;

    with_profile_local->nblocks = 0;

    with_profile_local->nstack_entries = 0;

    with_profile_local->time_total = 0.0;
  }

  (void)remove("profile.txt");

  for (int ithread = 0; ithread < THREAD_MAX; ithread++)
  {
    char name[NAME_MAX];

    snprintf(name, NAME_MAX, "profile-%d.txt", ithread);
    (void)remove(name);
  }

  // dynamically determine frequency

  counter_t counter_begin;
  counter_t counter_end;

  GET_COUNTER_BEGIN(&counter_begin)
  PROFILE_SLEEP(1)
  GET_COUNTER_END(&counter_end)

  frequency = TICKS(counter_end) - TICKS(counter_begin);

  int pid = PID;

  PG.counter_pointer = &counter_dummy;

  double mn = 0.0;
  double sn = 0.0;

  for (long long n = 1; n <= NCALL; ++n)
  {
    counter_t counter_stamp;

    GET_COUNTER_BEGIN(&counter_stamp)
    GET_COUNTER_END(PG.counter_pointer)

    update_mean_sigma(n, TICKS(counter_dummy) - TICKS(counter_stamp), &mn, &sn);
  }
  counter_mean = round(mn);
  counter_sigma = round(mn / 3.0);

  ncounter_largest = 0;
  counter_largest = 0;

  for (long long n = 1; n <= NCALL; ++n)
  {
    counter_t counter_stamp;

    GET_COUNTER_BEGIN(&counter_stamp)
    GET_COUNTER_END(PG.counter_pointer)

    long long delta = TICKS(counter_dummy) - TICKS(counter_stamp);

    if (delta > (counter_mean + 3 * counter_sigma))
    {
      ++ncounter_largest;
      counter_largest = delta;
    }
  }
  // validate_counter_correction();
}

void dump_profile(int pid, int verbose)
{
  char name[NAME_MAX];
  FILE *f;

  if (pid == 0)
    strncpy(name, "profile.txt", NAME_MAX);
  else
    snprintf(name, NAME_MAX, "profile-%d.txt", pid - 1);

  PROFILE_BUG((f = fopen(name, "w")) == NULL)

  {
    char stamp[NAME_MAX];
    time_t t = time(NULL);

    (void)strftime(stamp, NAME_MAX, "%H:%M:%S-%d/%m/%Y", localtime(&t));

    fprintf(f, "# Profile dumped at %s\n", stamp);
  }

  fprintf(f,
          "# The frequency is %lld ticks, or %.10f secs/tick.\n",
          frequency,
          1.0 / frequency);
  fprintf(f,
          "# The intrinsic profile overhead is %lld ticks on average.\n",
          counter_mean);
  fprintf(f,
          "# %lld out of %lld samples of the intrinsic profile overhead\n"
          "# ..are larger than twice the mean, the largest value is %lld.\n",
          ncounter_largest,
          NCALL,
          counter_largest);

  fprintf(f, "# The total number of blocks is %d.\n", PL.nblocks);

  if (PL.nstack_entries > 0)
  {
    fprintf(f,
            "# The following blocks are not properly terminated by an "
            "END_BLOCK!\n");

    for (int istack = 0; istack < PL.nstack_entries; istack++)
    {
      block_t *with_block = PL.blocks + PL.stack_entries[istack].stack_id;

      fprintf(f,
              "%s (invocation %d)\n",
              with_block->block_name,
              with_block->block_invocation);
    }
    fprintf(f, "\n");
  }

  // total time

  double time_self_total = 0.0;

  block_t *with_main = NULL;
  block_t *with_main_thread = NULL;

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
  {
    block_t *with_block = PL.blocks + iblock;

    if (strcmp(with_block->block_name, "main") == 0)
      with_main = with_block;

    if (strcmp(with_block->block_name, "main-thread") == 0)
      with_main_thread = with_block;

    with_block->block_nchild_calls = 0;

    with_block->block_child_time_total = 0.0;

    for (int jblock = 0; jblock < BLOCK_MAX; jblock++)
    {
      child_t *with_child = with_block->block_children + jblock;

      if (!with_child->child_active)
        continue;

      with_block->block_nchild_calls += with_child->child_ncalls;

      with_block->block_child_time_total += with_child->child_time_total;
    }

    time_self_total += with_block->block_time_self_total;
  }

  // main-thread takes precedence

  if (with_main_thread != NULL)
    with_main = with_main_thread;

  if (with_main == NULL)
  {
    fprintf(stderr, "block main or main-thread not found\n");
    exit(EXIT_FAILURE);
  }

  fprintf(f, "# The total run time was %.10f secs.\n", PL.time_total);

  fprintf(f, "# The total self time was %.10f secs.\n", time_self_total);

  fprintf(f,
          "# The total profile overhead was %.10f secs.\n",
          PL.time_total - time_self_total);

  fprintf(f, "\n");

  // sort by straight insertion

  int sort[BLOCK_MAX];

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
    sort[iblock] = iblock;

  for (int iblock = 0; iblock < PL.nblocks - 1; iblock++)
  {
    int kblock = iblock;

    for (int jblock = iblock + 1; jblock < PL.nblocks; jblock++)
    {
      if (PL.blocks[sort[jblock]].block_time_total >
          PL.blocks[sort[kblock]].block_time_total)
        kblock = jblock;
    }

    int t = sort[iblock];

    sort[iblock] = sort[kblock];
    sort[kblock] = t;
  }

  fprintf(f, "# Blocks sorted by total time spent in block and children.\n");

  fprintf(f, "# The sum of total times (or the sum of the percentages)\n");

  fprintf(
    f,
    "# does not have any meaning, since children will be double counted.\n");

  fprintf(f,
          "%-32s %-10s %6s %16s %10s\n",
          "name",
          "invocation",
          "perc",
          "total time",
          "calls");

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
  {
    block_t *with_block = PL.blocks + sort[iblock];

    fprintf(f,
            "%-32s %-10d %6.2f %16.10f %10lld\n",
            with_block->block_name,
            with_block->block_invocation,
            PERC(with_block->block_time_total),
            with_block->block_time_total,
            with_block->block_ncalls);
  }
  fprintf(f, "\n");

  // sort by straight insertion

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
    sort[iblock] = iblock;

  for (int iblock = 0; iblock < PL.nblocks - 1; iblock++)
  {
    int kblock = iblock;

    for (int jblock = iblock + 1; jblock < PL.nblocks; jblock++)
    {
      if (PL.blocks[sort[jblock]].block_time_self_total >
          PL.blocks[sort[kblock]].block_time_self_total)
        kblock = jblock;
    }

    int t = sort[iblock];

    sort[iblock] = sort[kblock];
    sort[kblock] = t;
  }

  fprintf(f, "# Blocks sorted by total time spent in own code.\n");

  fprintf(f, "# The sum of the self times is equal to the total self time.\n");

  fprintf(f,
          "%-32s %-10s %6s %16s %10s\n",
          "name",
          "invocation",
          "perc",
          "self time",
          "calls");

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
  {
    block_t *with_block = PL.blocks + sort[iblock];

    fprintf(f,
            "%-32s %-10d %6.2f %16.10f %10lld\n",
            with_block->block_name,
            with_block->block_invocation,
            PERC(with_block->block_time_self_total),
            with_block->block_time_self_total,
            with_block->block_ncalls);
  }
  fprintf(f, "\n");

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
  {
    PL.blocks[iblock].block_time_recursive_total = 0.0;

    PL.blocks[iblock].block_ntotal_recursive_calls = 0;

    if (PL.blocks[iblock].block_invocation == 1)
    {
      PL.blocks[iblock].block_time_recursive_total =
        PL.blocks[iblock].block_time_self_total;

      PL.blocks[iblock].block_ntotal_recursive_calls =
        PL.blocks[iblock].block_ncalls;

      for (int jblock = 0; jblock < PL.nblocks; jblock++)
      {
        if (PL.blocks[jblock].block_invocation == 1)
          continue;

        if (strcmp(PL.blocks[iblock].block_name,
                   PL.blocks[jblock].block_name) == 0)
        {
          PL.blocks[iblock].block_time_recursive_total +=
            PL.blocks[jblock].block_time_self_total;

          PL.blocks[iblock].block_ntotal_recursive_calls +=
            PL.blocks[jblock].block_ncalls;
        }
      }
    }
  }

  // sort by straight insertion

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
    sort[iblock] = iblock;

  for (int iblock = 0; iblock < PL.nblocks - 1; iblock++)
  {
    int kblock = iblock;

    for (int jblock = iblock + 1; jblock < PL.nblocks; jblock++)
    {
      if (PL.blocks[sort[jblock]].block_time_recursive_total >
          PL.blocks[sort[kblock]].block_time_recursive_total)
        kblock = jblock;
    }

    int t = sort[iblock];

    sort[iblock] = sort[kblock];
    sort[kblock] = t;
  }

  fprintf(f,
          "# Blocks sorted by self times summed over recursive invocations.\n");

  fprintf(f,
          "%-32s %6s %6s %16s %10s %16s %10s\n",
          "name",
          "perc",
          "%main",
          "self time",
          "calls",
          "self time/call",
          "ticks/call");

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
  {
    int jblock = sort[iblock];

    if (PL.blocks[jblock].block_invocation == 1)
    {
      double self_time_per_call =
        PL.blocks[jblock].block_time_recursive_total /
        PL.blocks[jblock].block_ntotal_recursive_calls;

      long long ticks_per_call = -1;

      if (self_time_per_call < 1.0)
        ticks_per_call = round(self_time_per_call * frequency);

      fprintf(f,
              "%-32s %6.2f %6.2f %16.10f %10lld %16.10f %10lld\n",
              PL.blocks[jblock].block_name,
              PERC(PL.blocks[jblock].block_time_recursive_total),
              PL.blocks[jblock].block_time_recursive_total /
                with_main->block_time_total * 100,
              PL.blocks[jblock].block_time_recursive_total,
              PL.blocks[jblock].block_ntotal_recursive_calls,
              self_time_per_call,
              ticks_per_call);
    }
  }
  fprintf(f, "\n");

  if (verbose == 0)
    goto label_return;

  for (int iblock = 0; iblock < PL.nblocks; iblock++)
  {
    block_t *with_block = PL.blocks + sort[iblock];

    fprintf(f,
            "# Summary for block %s, invocation %d.\n",
            with_block->block_name,
            with_block->block_invocation);

    fprintf(f,
            "Spends %.10f secs in %lld call(s), or %.2f%% of total execution "
            "time.\n",
            with_block->block_time_total,
            with_block->block_ncalls,
            PERC(with_block->block_time_total));

    fprintf(f,
            "Spends %.10f secs (%.2f%%) in own code, %.10f secs (%.2f%%) in "
            "children.\n",
            with_block->block_time_self_total,
            PERC(with_block->block_time_self_total),
            with_block->block_child_time_total,
            PERC(with_block->block_child_time_total));
    fprintf(f, "\n");

    int found = FALSE;

    for (int jblock = 0; jblock < BLOCK_MAX; jblock++)
    {
      child_t *with_child = with_block->block_children + jblock;

      if (!with_child->child_active)
        continue;

      found = TRUE;

      fprintf(f,
              "Spends %.10f secs in %lld call(s) to %s, invocation %d.\n",
              with_child->child_time_total,
              with_child->child_ncalls,
              PL.blocks[jblock].block_name,
              PL.blocks[jblock].block_invocation);
    }

    if (!found)
      fprintf(f, "No children were found.\n");

    found = FALSE;

    for (int jblock = 0; jblock < BLOCK_MAX; jblock++)
    {
      parent_t *with_parent = with_block->block_parents + jblock;

      if (!with_parent->parent_active)
        continue;

      found = TRUE;

      fprintf(f,
              "Is called %lld time(s) from %s, invocation %d.\n",
              with_parent->parent_ncalls,
              PL.blocks[jblock].block_name,
              PL.blocks[jblock].block_invocation);
    }

    if (!found)
      fprintf(f, "No parents were found\n");

    fprintf(f, "\n");
  }

label_return:

  fprintf(f, "# End of profile.\n");

  fclose(f);
}

#endif
