// SCU REVISION 8.1273 wo 11 nov 2026 19:28:10 CET
#ifndef SortH
#define SortH

#define HISTORY_NFIELDS 64
// History scores saturate at +/- HISTORY_MAX.
#define HISTORY_MAX 16384
// Cap each depth-based bonus/malus: at most 1/16 towards either limit.
#define HISTORY_BONUS_MAX 1024

typedef struct
{
  int S_array[NMOVES_MAX];
  int S_narray;
  int S_ninitialized;
  int S_next;
  int S_isort0;
  int S_randomize;
  my_random_t S_random;
  // History ordering: when S_history is set, the not yet tried moves are
  // returned in order of decreasing history score (ties in random order).
  const moves_list_t *S_moves_list;
  const int (*S_history)[HISTORY_NFIELDS];
  int S_scored;
  int S_scores[NMOVES_MAX];
} sort_t;

static inline void construct_sort(sort_t *arg_sort,
                                  int arg_narray,
                                  my_random_t *arg_my_random,
                                  int arg_isort0)
{
  HARDBUG((arg_narray < 0) or (arg_narray > NMOVES_MAX))
  HARDBUG((arg_isort0 != INVALID) and
          ((arg_isort0 < 0) or (arg_isort0 >= arg_narray)))

  arg_sort->S_narray = arg_narray;
  arg_sort->S_ninitialized = INVALID;
  arg_sort->S_next = 0;
  arg_sort->S_isort0 = arg_isort0;
  arg_sort->S_randomize = arg_my_random != NULL;
  arg_sort->S_moves_list = NULL;
  arg_sort->S_history = NULL;
  arg_sort->S_scored = FALSE;
  if (arg_my_random != NULL)
  {
    seed_my_random(&arg_sort->S_random, return_my_random(arg_my_random));
  }
}

static inline void initialize_sort(sort_t *arg_sort)
{
  if (arg_sort->S_ninitialized != INVALID)
    return;

  for (int isort = 0; isort < arg_sort->S_narray; isort++)
    arg_sort->S_array[isort] = isort;

  arg_sort->S_ninitialized = 0;

  if (arg_sort->S_isort0 != INVALID)
  {
    arg_sort->S_array[arg_sort->S_isort0] = 0;
    arg_sort->S_array[0] = arg_sort->S_isort0;
    arg_sort->S_ninitialized = 1;
    arg_sort->S_isort0 = INVALID;
  }
}

// Snapshot history once so copied probes and later passes use the same scores.
static inline void score_sort(sort_t *arg_sort)
{
  if ((arg_sort->S_history == NULL) or arg_sort->S_scored)
    return;

  for (int isort = 0; isort < arg_sort->S_narray; isort++)
  {
    const move_t *move = arg_sort->S_moves_list->ML_moves + isort;

    arg_sort->S_scores[isort] =
      arg_sort->S_history[move->M_move_from][move->M_move_to];
  }
  arg_sort->S_scored = TRUE;
}

static inline void update_sort(sort_t *arg_sort)
{
  initialize_sort(arg_sort);

  int isort = arg_sort->S_ninitialized;

  if (isort == arg_sort->S_narray)
    return;

  if (arg_sort->S_randomize and (isort + 1 < arg_sort->S_narray))
  {
    int jsort =
      isort + (int)compat_mulhi_u64(return_my_random(&arg_sort->S_random),
                                    (ui64_t)(arg_sort->S_narray - isort));

    if (arg_sort->S_history != NULL)
    {
      score_sort(arg_sort);

      // Scan cyclically from the random position so that ties are broken
      // (nearly) at random.

      int best_score = arg_sort->S_scores[arg_sort->S_array[jsort]];
      int ksort = jsort;

      for (int nsort = isort + 1; nsort < arg_sort->S_narray; nsort++)
      {
        if (++ksort == arg_sort->S_narray)
          ksort = isort;

        int score = arg_sort->S_scores[arg_sort->S_array[ksort]];

        if (score > best_score)
        {
          best_score = score;
          jsort = ksort;
        }
      }
    }

    int value = arg_sort->S_array[jsort];

    arg_sort->S_array[jsort] = arg_sort->S_array[isort];
    arg_sort->S_array[isort] = value;
  }

  ++(arg_sort->S_ninitialized);
}

static inline int return_next_sort(sort_t *arg_sort)
{
  initialize_sort(arg_sort);

  if (arg_sort->S_next == arg_sort->S_narray)
    return (INVALID);

  if (arg_sort->S_next == arg_sort->S_ninitialized)
    update_sort(arg_sort);

  return (arg_sort->S_array[arg_sort->S_next++]);
}

static inline void reset_sort(sort_t *arg_sort, int arg_first)
{
  HARDBUG((arg_first < 0) or (arg_first > arg_sort->S_narray))

  initialize_sort(arg_sort);
  while (arg_sort->S_ninitialized < arg_first)
    update_sort(arg_sort);
  arg_sort->S_next = arg_first;
}

static inline void copy_sort(sort_t *arg_to, sort_t *arg_from)
{
  score_sort(arg_from);

  arg_to->S_narray = arg_from->S_narray;
  arg_to->S_ninitialized = arg_from->S_ninitialized;
  arg_to->S_next = arg_from->S_next;
  arg_to->S_isort0 = arg_from->S_isort0;
  arg_to->S_randomize = arg_from->S_randomize;
  if (arg_from->S_randomize)
    copy_my_random(&arg_to->S_random, &arg_from->S_random);
  arg_to->S_moves_list = arg_from->S_moves_list;
  arg_to->S_history = arg_from->S_history;
  arg_to->S_scored = arg_from->S_scored;
  if (arg_from->S_scored)
    memcpy(arg_to->S_scores,
           arg_from->S_scores,
           arg_from->S_narray * sizeof(arg_from->S_scores[0]));
  if (arg_from->S_ninitialized != INVALID)
    memcpy(arg_to->S_array,
           arg_from->S_array,
           arg_from->S_narray * sizeof(arg_from->S_array[0]));
}

#endif
