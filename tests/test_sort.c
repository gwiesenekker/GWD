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
  puts("Sort tests passed: preferred move, permutations, exhaustion, replay and independent probes.");
  return 0;
}
