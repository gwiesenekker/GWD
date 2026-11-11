#include "../globals.h"
#include <assert.h>

int zzzzzz_invocation;
void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  my_random_t random;
  construct_my_random(&random, NULL);

  my_random_t seeded;
  seed_my_random(&seeded, 0);
  assert(memcmp(&random, &seeded, sizeof(random)) == 0);

  // Seeding a sort consumes exactly one parent draw, including empty sorts.
  for (int narray = 0; narray <= NMOVES_MAX; narray++)
  {
    my_random_t expected_parent;
    copy_my_random(&expected_parent, &random);
    seed_my_random(&seeded, return_my_random(&expected_parent));

    sort_t sort;
    construct_sort(&sort, narray, &random, INVALID);
    assert(memcmp(&random, &expected_parent, sizeof(random)) == 0);
    for (int idraw = 0; idraw < 8; idraw++)
      assert(return_my_random(&sort.S_random) == return_my_random(&seeded));
  }

  for (int nmoves = 0; nmoves <= NMOVES_MAX; nmoves++)
  {
    for (int isort0 = 0; isort0 <= nmoves; isort0++)
    {
      int kmove = isort0 - 1;
      for (int irandom = 0; irandom < 2; irandom++)
      {
        sort_t sort;
        construct_sort(&sort, nmoves, irandom ? &random : NULL, kmove);
        int order[NMOVES_MAX];
        int found[NMOVES_MAX] = {0};
        int prefix = nmoves / 2;
        for (int imove = 0; imove < prefix; imove++)
          order[imove] = return_next_sort(&sort);

        sort_t probe;
        construct_sort(&probe, 0, NULL, INVALID);
        copy_sort(&probe, &sort);
        reset_sort(&probe, 0);

        for (int imove = 0; imove < nmoves; imove++)
        {
          int jmove = return_next_sort(&probe);
          assert(jmove >= 0 && jmove < nmoves);
          assert(!found[jmove]++);
          if (imove < prefix)
            assert(jmove == order[imove]);
          else
            order[imove] = jmove;
        }
        assert(return_next_sort(&probe) == INVALID);
        assert(return_next_sort(&probe) == INVALID);
        assert(sort.S_ninitialized == prefix ||
               (prefix == 0 && sort.S_ninitialized == INVALID));

        // Child searches cannot change the parent's or probe's private RNG.
        for (int idraw = 0; idraw < 10; idraw++)
          (void)return_my_random(&random);
        for (int imove = prefix; imove < nmoves; imove++)
          assert(return_next_sort(&sort) == order[imove]);
        assert(return_next_sort(&sort) == INVALID);
        if (kmove != INVALID)
          assert(order[0] == kmove);
        if (!irandom && kmove == INVALID)
          for (int imove = 0; imove < nmoves; imove++)
            assert(order[imove] == imove);

        reset_sort(&sort, nmoves ? 1 : 0);
        for (int imove = nmoves ? 1 : 0; imove < nmoves; imove++)
          assert(return_next_sort(&sort) == order[imove]);

        // A search promotes a consumed move, then starts a new pass.
        if (nmoves > 2)
        {
          reset_sort(&probe, 0);
          int best = probe.S_array[2];
          probe.S_array[2] = probe.S_array[1];
          probe.S_array[1] = probe.S_array[0];
          probe.S_array[0] = best;
          assert(return_next_sort(&probe) == best);
          reset_sort(&sort, 0);
          assert(return_next_sort(&sort) == order[0]);
        }
      }
    }
  }

  // Copy before the preferred move is consumed; skip it in a singular probe.
  sort_t parent;
  construct_sort(&parent, NMOVES_MAX, &random, 17);
  sort_t singular;
  construct_sort(&singular, 0, NULL, INVALID);
  copy_sort(&singular, &parent);
  reset_sort(&singular, 1);
  assert(return_next_sort(&parent) == 17);
  for (int imove = 1; imove < NMOVES_MAX; imove++)
    assert(return_next_sort(&singular) == return_next_sort(&parent));
  // A lazy history snapshot must survive copies made before any move is read.
  moves_list_t moves = {0};
  moves.ML_nmoves = 6;
  int history[HISTORY_NFIELDS][HISTORY_NFIELDS] = {0};
  for (int imove = 0; imove < moves.ML_nmoves; imove++)
  {
    moves.ML_moves[imove].M_move_from = 6;
    moves.ML_moves[imove].M_move_to = 11 + imove;
    history[6][11 + imove] = imove * 100;
  }
  construct_sort(&parent, moves.ML_nmoves, &random, 2);
  parent.S_moves_list = &moves;
  parent.S_history = (const int (*)[HISTORY_NFIELDS])history;
  construct_sort(&singular, 0, NULL, INVALID);
  copy_sort(&singular, &parent);
  assert(parent.S_scored && singular.S_scored);
  for (int imove = 0; imove < moves.ML_nmoves; imove++)
    history[6][11 + imove] = -imove * 100;
  reset_sort(&singular, 1);
  assert(return_next_sort(&parent) == 2);
  int expected[] = {5, 4, 3, 1, 0};
  for (int isort = 0; isort < 5; isort++)
  {
    assert(return_next_sort(&parent) == expected[isort]);
    assert(return_next_sort(&singular) == expected[isort]);
  }
  assert(return_next_sort(&parent) == INVALID);
  assert(return_next_sort(&singular) == INVALID);

  // Zero history preserves the original randomized permutation.
  memset(history, 0, sizeof(history));
  my_random_t same_random;
  copy_my_random(&same_random, &random);
  construct_sort(&parent, moves.ML_nmoves, &random, INVALID);
  construct_sort(&singular, moves.ML_nmoves, &same_random, INVALID);
  parent.S_moves_list = &moves;
  parent.S_history = (const int (*)[HISTORY_NFIELDS])history;
  for (int isort = 0; isort < moves.ML_nmoves; isort++)
    assert(return_next_sort(&parent) == return_next_sort(&singular));

  puts("Sort tests passed: preferred move, permutations, exhaustion, replay and independent probes.");
  return 0;
}
