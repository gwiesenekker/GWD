/* The PV representation needs no board or search state. */
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
  pv_t child, best, previous;
  clear_pv(&child);
  assert(pv_length(&child) == 0);
  prepend_pv(&child, 127, &child);
  prepend_pv(&child, 0, &child);
  prepend_pv(&best, 12, &child);
  assert(best.PV_moves[0] == 12 && best.PV_moves[1] == 0);
  assert(best.PV_moves[2] == 127 && best.PV_moves[3] == INVALID);
  assert(child.PV_moves[0] == 0 && pv_length(&child) == 2);
  memset(&previous, 0x5a, sizeof(previous));
  copy_pv(&previous, &best);
  assert(previous.PV_moves[4] == 0x5a5a);
  clear_pv(&child);
  prepend_pv(&best, 7, &child);
  assert(pv_length(&best) == 1 && best.PV_moves[1] == INVALID);
  assert(pv_length(&previous) == 3 && previous.PV_moves[0] == 12);
  copy_pv(&previous, &previous);
  copy_pv(&best, &child);
  assert(pv_length(&best) == 0);
  for (int i = 0; i < PV_MAX_MOVES; ++i)
    prepend_pv(&child, i % NMOVES_MAX, &child);
  assert(pv_length(&child) == PV_MAX_MOVES);
  prepend_pv(&best, 13, &child);
  assert(pv_length(&best) == PV_MAX_MOVES);
  assert(best.PV_moves[0] == 13);
  for (int i = 1; i < PV_MAX_MOVES; ++i)
    assert(best.PV_moves[i] == child.PV_moves[i - 1]);
  prepend_pv(&child, 13, &child);
  assert(memcmp(best.PV_moves, child.PV_moves, sizeof(best.PV_moves)) == 0);
  const i16_t *tail = best.PV_moves;
  int consumed = 0;
  while (*tail++ != INVALID)
    ++consumed;
  assert(consumed == PV_MAX_MOVES);
  puts("PV tests passed: self-contained lists, prepend, snapshots, termination and capacity.");
  return 0;
}
