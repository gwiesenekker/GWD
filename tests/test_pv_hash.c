/* Test the actual hash-PV helpers; unused engine functions are discarded
 * with -ffunction-sections -fdata-sections -Wl,--gc-sections. */
#include "../search.c"
#include <assert.h>

int zzzzzz_invocation;
void zzzzzz(const char *arg_file,
            const char *arg_func,
            long arg_line,
            const char *arg_message,
            int arg_code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", arg_file, arg_line, arg_func, arg_message);
  exit(arg_code);
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;

  static search_t search;
  pv_entries_t pv;
  ui64_t keys[NPV_SLOTS + 1];
  int nkeys = 0;
  /* Distinct key signatures deliberately mapped to the same bucket. */
  for (ui64_t key = 0x1234567800000000ULL; nkeys <= NPV_SLOTS;
       key += 0x10000ULL)
    if ((return_fmix64(key) & (NPV_ENTRIES - 1)) == 0)
      keys[nkeys++] = key;

  for (int first_colour = 0; first_colour < 2; ++first_colour)
  {
    for (int ientry = 0; ientry < NPV_ENTRIES; ++ientry)
      for (int jslot = 0; jslot < NPV_SLOTS; ++jslot)
        pv.PVE_entries[ientry].PVE_slots[jslot] = PV_SLOT_MOVES_MASK;

    for (int islot = 0; islot < NPV_SLOTS; ++islot)
    {
      search.S_board.B_key = keys[islot];
      search.S_board.B_colour2move = first_colour;
      update_pv(&pv, &search, 7 + islot);

      assert(probe_pv(&pv, &search) == 7 + islot);

      search.S_board.B_colour2move = 1 - first_colour;

      assert(probe_pv(&pv, &search) == INVALID);

      update_pv(&pv, &search, 20 + islot);

      assert(probe_pv(&pv, &search) == 20 + islot);

      search.S_board.B_colour2move = first_colour;

      assert(probe_pv(&pv, &search) == 7 + islot);
    }
    /* A matching-key update must not reset the other colour's move. */
    search.S_board.B_key = keys[0];
    update_pv(&pv, &search, 42);

    assert(probe_pv(&pv, &search) == 42);

    search.S_board.B_colour2move = 1 - first_colour;

    assert(probe_pv(&pv, &search) == 20);

    /* Replacement must initialize the key and clear the evicted moves. */
    int victim =
      ((ui32_t)return_fmix64(keys[NPV_SLOTS]) >> 30) & (NPV_SLOTS - 1);

    ui64_t evicted = keys[victim];

    search.S_board.B_key = keys[NPV_SLOTS];
    update_pv(&pv, &search, 63);

    assert(probe_pv(&pv, &search) == 63);

    search.S_board.B_colour2move = first_colour;

    assert(probe_pv(&pv, &search) == INVALID);

    search.S_board.B_key = evicted;

    assert(probe_pv(&pv, &search) == INVALID);
  }
  puts("Hash-PV tests passed: empty slots, matching keys, both colours and "
       "replacement.");

  return (0);
}
