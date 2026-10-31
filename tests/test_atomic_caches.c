/* Include implementations so this test exercises the actual cache helpers and
 * probe/update paths. Unused engine code is discarded by the test linker. */
#include "../search.c"
#include "../score.c"
#include <assert.h>

int zzzzzz_invocation;
_Noreturn void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

static alpha_beta_cache_entry_t tt;
static score_cache_shared_slot_t sc;
static thread_t publication_threads[4];

static score_cache_slot_t make_score(uint64_t key, score_t score)
{
  score_cache_slot_t value = {.SCS_key = key, .SCS_score = score, .SCS_crc32 = 0};
  uint32_t crc = HW_CRC32_U64(UINT32_MAX, key);
  value.SCS_crc32 = ~HW_CRC32_U32(crc, score);
  return value;
}

static search_t *new_search(void)
{
  search_t *search = compat_aligned_alloc(64, sizeof(*search));
  assert(search);
  memset(search, 0, sizeof(*search));
  return search;
}

static void *worker(void *argument)
{
  uint64_t key = *(unsigned *)argument;
  search_t *search = new_search();
  search->S_thread = &publication_threads[key - 1];
  search->S_board.B_key = key;
  score_cache_slot_t score = make_score(key, (score_t)(key * 100));
  for (unsigned i = 0; i < 50000; ++i) {
    search->S_total_nodes = i + 1;
    if (i % 1000 == 0)
      publish_nodes(search);
    alpha_beta_cache_slot_t value = {0}, found;
    value.ABCS_key = key;
    value.ABCS_score = (score_t)(key * 100);
    value.ABCS_depth = 12;
    value.ABCS_flags = ABCS_SCORE_TRUE_SCORE_BIT;
    value.ABCS_moves[0] = 3;
    value.ABCS_moves[1] = 7;
    update_alpha_beta_cache(search, &tt, &value);
    if (probe_alpha_beta_cache(search, &tt, &found, i % 17 == 0)) {
      assert(found.ABCS_key == key);
      assert(found.ABCS_score == (score_t)(key * 100));
      assert(found.ABCS_depth == 12 && found.ABCS_moves[1] == 7);
    }
    update_score_cache(search, &sc, &score);
    score_cache_slot_t readback;
    if (probe_score_cache(search, &sc, &readback))
      assert(readback.SCS_key == key && readback.SCS_score == score.SCS_score);
  }
  publish_nodes(search);
  compat_aligned_free(search);
  return NULL;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  nalpha_beta_cache_entries = nscore_cache_entries = 1;
  alpha_beta_cache_slot_default.ABCS_key = abcs2crc64(0, 0);
  for (int i = 0; i < NABCS_SLOTS; ++i)
    init_alpha_beta_cache_slot(&tt.ABCE_slots[i], &alpha_beta_cache_slot_default);
  score_cache_slot_default = make_score(0, 0);
  init_score_cache_slot(&sc, &score_cache_slot_default);
  assert(atomic_is_lock_free(&tt.ABCE_slots[0].ABCS_key));
  assert(atomic_is_lock_free(&tt.ABCE_slots[0].ABCS_data));
  assert(atomic_is_lock_free(&sc.SCS_key));
  assert(atomic_is_lock_free(&sc.SCS_data));

  search_t *search = new_search();
  /* All keys collide: verify slot shifting and payload bits exactly. */
  for (unsigned key = 1; key <= 6; ++key) {
    alpha_beta_cache_slot_t value = {0};
    search->S_board.B_key = value.ABCS_key = key;
    value.ABCS_score = -(score_t)key;
    value.ABCS_depth = 23;
    value.ABCS_moves[0] = -1;
    value.ABCS_moves[1] = 11;
    update_alpha_beta_cache(search, &tt, &value);
    alpha_beta_cache_slot_t readback;
    load_alpha_beta_cache_slot(&readback, &tt.ABCE_slots[0]);
    assert(readback.ABCS_key == value.ABCS_key && readback.ABCS_data == value.ABCS_data);
  }
  for (unsigned key = 1; key <= 6; ++key) {
    alpha_beta_cache_slot_t found;
    search->S_board.B_key = key;
    int hit = probe_alpha_beta_cache(search, &tt, &found, 0);
    assert(hit == (key >= 3));
    if (hit) assert(found.ABCS_score == -(score_t)key && found.ABCS_moves[0] == -1);
  }
  search->S_board.B_key = 6;
  alpha_beta_cache_slot_t found;
  assert(probe_alpha_beta_cache(search, &tt, &found, 1));
  assert(!probe_alpha_beta_cache(search, &tt, &found, 0));

  score_cache_slot_t score = make_score(UINT64_MAX, -1234567), readback;
  store_score_cache_slot(&sc, &score);
  load_score_cache_slot(&readback, &sc);
  assert(memcmp(&readback, &score, sizeof(score)) == 0);
  compat_aligned_free(search);

  /* Clear using the concurrent-safe stores, then stress all writers/readers
   * on the same entry. Per-worker search counters are never shared. */
  for (int i = 0; i < NABCS_SLOTS; ++i)
    store_alpha_beta_cache_slot(&tt.ABCE_slots[i], &alpha_beta_cache_slot_default);
  store_score_cache_slot(&sc, &score_cache_slot_default);
  my_thread_t threads[4];
  unsigned keys[4] = {1, 2, 3, 4};
  for (int i = 0; i < 4; ++i)
    atomic_init(&publication_threads[i].T_published_nodes, 0);
  for (int i = 0; i < 4; ++i)
    assert(compat_thread_create(&threads[i], worker, &keys[i]) == 0);
  for (int sample = 0; sample < 50000; ++sample)
    for (int i = 0; i < 4; ++i) {
      i64_t nodes = atomic_load_explicit(&publication_threads[i].T_published_nodes,
                                         memory_order_relaxed);
      assert(nodes >= 0 && nodes <= 50000);
    }
  for (int i = 0; i < 4; ++i)
    assert(compat_thread_join(threads[i]) == 0);
  for (int i = 0; i < 4; ++i)
    assert(atomic_load_explicit(&publication_threads[i].T_published_nodes,
                               memory_order_relaxed) == 50000);
  puts("atomic cache tests passed");
  return 0;
}
