// SCU REVISION 8.1267 wo 11 nov 2026  3:10:47 CET
#ifndef SortH
#define SortH

typedef struct
{
  int S_array[NMOVES_MAX];
  int S_narray;
  int S_ninitialized;
  int S_next;
  int S_isort0;
  int S_randomize;
  my_random_t S_random;
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

static inline void copy_sort(sort_t *arg_to, const sort_t *arg_from)
{
  arg_to->S_narray = arg_from->S_narray;
  arg_to->S_ninitialized = arg_from->S_ninitialized;
  arg_to->S_next = arg_from->S_next;
  arg_to->S_isort0 = arg_from->S_isort0;
  arg_to->S_randomize = arg_from->S_randomize;
  if (arg_from->S_randomize)
    copy_my_random(&arg_to->S_random, &arg_from->S_random);
  if (arg_from->S_ninitialized != INVALID)
    memcpy(arg_to->S_array,
           arg_from->S_array,
           arg_from->S_narray * sizeof(arg_from->S_array[0]));
}

#endif
