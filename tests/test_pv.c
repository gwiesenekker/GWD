/* Exercise the actual PV hash-table helpers; discard unrelated engine code. */
#include "../search.c"
#include <assert.h>

int zzzzzz_invocation;
_Noreturn void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

static search_t search;

static void clear_test_pv(pv_entries_t *arg_pv)
{
  /* Same empty-slot representation as init_search(). */
  for (int ientry = 0; ientry < NPV_ENTRIES; ientry++)
    for (int islot = 0; islot < NPV_SLOTS; islot++)
      arg_pv->PVE_entries[ientry].PVE_slots[islot] = PV_SLOT_MOVES_MASK;
}

static int lookup(pv_entries_t *arg_pv, ui64_t arg_key, int arg_colour)
{
  search.S_board.B_key = arg_key;
  search.S_board.B_colour2move = arg_colour;
  return probe_pv(arg_pv, &search);
}

static void store(pv_entries_t *arg_pv, ui64_t arg_key, int arg_colour, int arg_move)
{
  search.S_board.B_key = arg_key;
  search.S_board.B_colour2move = arg_colour;
  update_pv(arg_pv, &search, arg_move);
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  pv_entries_t pv;
  clear_test_pv(&pv);

  /* Include a zero tag, move zero, and the largest legal move index. */
  for (int icolour = 0; icolour < 2; icolour++)
  {
    clear_test_pv(&pv);
    assert(lookup(&pv, 0, icolour) == INVALID);
    assert(lookup(&pv, 0, 1 - icolour) == INVALID);
    store(&pv, 0, icolour, 0);
    assert(lookup(&pv, 0, icolour) == 0);
    assert(lookup(&pv, 0, 1 - icolour) == INVALID);
    store(&pv, 0, 1 - icolour, NMOVES_MAX - 1);
    assert(lookup(&pv, 0, icolour) == 0);
    assert(lookup(&pv, 0, 1 - icolour) == NMOVES_MAX - 1);
    store(&pv, 0, icolour, 12);
    assert(lookup(&pv, 0, icolour) == 12);
    assert(lookup(&pv, 0, 1 - icolour) == NMOVES_MAX - 1);
  }

  /* Find distinct tags hashing into the same bucket, plus an unrelated key. */
  ui64_t keys[NPV_SLOTS + 1];
  ui64_t other_key = 0;
  int nkeys = 0;
  for (ui64_t ikey = 1; nkeys < NPV_SLOTS + 1 || other_key == 0; ikey++)
  {
    ui64_t key = ikey << 16;
    int ientry = (ui32_t)return_fmix64(key) & (NPV_ENTRIES - 1);
    if (ientry == 0 && nkeys < NPV_SLOTS + 1)
      keys[nkeys++] = key;
    else if (ientry != 0 && other_key == 0)
      other_key = key;
    assert(ikey < 1000000);
  }

  for (int icolour = 0; icolour < 2; icolour++)
  {
    clear_test_pv(&pv);
    store(&pv, other_key, WHITE_ENUM, 21);
    for (int ikey = 0; ikey < NPV_SLOTS; ikey++)
    {
      assert(lookup(&pv, keys[ikey], icolour) == INVALID);
      store(&pv, keys[ikey], icolour, ikey);
      assert(lookup(&pv, keys[ikey], 1 - icolour) == INVALID);
      store(&pv, keys[ikey], 1 - icolour, ikey + 10);
    }
    for (int ikey = 0; ikey < NPV_SLOTS; ikey++)
    {
      assert(lookup(&pv, keys[ikey], icolour) == ikey);
      assert(lookup(&pv, keys[ikey], 1 - icolour) == ikey + 10);
    }

    /* Search snapshots are value copies; updates must not alter the original. */
    pv_entries_t snapshot = pv;
    store(&pv, keys[NPV_SLOTS], icolour, 31);
    assert(lookup(&pv, keys[NPV_SLOTS], icolour) == 31);
    assert(lookup(&pv, keys[NPV_SLOTS], 1 - icolour) == INVALID);
    assert(lookup(&snapshot, keys[NPV_SLOTS], icolour) == INVALID);
    assert(lookup(&pv, other_key, WHITE_ENUM) == 21);

    int nmissing = 0;
    for (int ikey = 0; ikey < NPV_SLOTS; ikey++)
    {
      int move = lookup(&pv, keys[ikey], icolour);
      if (move == INVALID)
      {
        ++nmissing;
        assert(lookup(&pv, keys[ikey], 1 - icolour) == INVALID);
      }
      else
      {
        assert(move == ikey);
        assert(lookup(&pv, keys[ikey], 1 - icolour) == ikey + 10);
      }
      assert(lookup(&snapshot, keys[ikey], icolour) == ikey);
      assert(lookup(&snapshot, keys[ikey], 1 - icolour) == ikey + 10);
    }
    assert(nmissing == 1);
  }

  puts("PV tests passed: misses, per-colour updates, snapshots, collisions and replacement.");
  return 0;
}
