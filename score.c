// SCU REVISION 8.1270 wo 11 nov 2026  9:25:12 CET
#include "globals.h"
#include <stdatomic.h>

typedef struct
{
  ui64_t SCS_key;
  union
  {
    ui64_t SCS_data;
    struct
    {
      score_t SCS_score;
      ui32_t SCS_crc32;
    };
  };
} score_cache_slot_t;

_Static_assert(sizeof(score_cache_slot_t) == 16 &&
                 offsetof(score_cache_slot_t, SCS_data) == 8 &&
                 offsetof(score_cache_slot_t, SCS_score) == 8 &&
                 offsetof(score_cache_slot_t, SCS_crc32) == 12,
               "score-cache slot must retain its 16-byte layout");

typedef struct
{
  _Atomic(uint64_t) SCS_key;
  _Atomic(uint64_t) SCS_data;
} score_cache_shared_slot_t;

_Static_assert(sizeof(score_cache_shared_slot_t) == sizeof(score_cache_slot_t),
               "atomic score-cache storage must preserve cache capacity");

/* Each word is atomic, not the complete record. Keep checksum validation. */
local inline void load_score_cache_slot(score_cache_slot_t *to,
                                        const score_cache_shared_slot_t *from)
{
  to->SCS_key = atomic_load_explicit(&from->SCS_key, memory_order_relaxed);
  to->SCS_data = atomic_load_explicit(&from->SCS_data, memory_order_relaxed);
}

local inline void store_score_cache_slot(score_cache_shared_slot_t *to,
                                         const score_cache_slot_t *from)
{
  atomic_store_explicit(&to->SCS_key, from->SCS_key, memory_order_relaxed);
  atomic_store_explicit(&to->SCS_data, from->SCS_data, memory_order_relaxed);
}

local inline void init_score_cache_slot(score_cache_shared_slot_t *to,
                                        const score_cache_slot_t *from)
{
  atomic_init(&to->SCS_key, from->SCS_key);
  atomic_init(&to->SCS_data, from->SCS_data);
}

local i64_t nscore_cache_entries;
local score_cache_slot_t score_cache_slot_default;
local score_cache_shared_slot_t *score_cache[NCOLOUR_ENUM];

local int probe_score_cache(search_t *arg_with_search,
                            score_cache_shared_slot_t *arg_score_cache,
                            score_cache_slot_t *arg_score_cache_slot)
{
  int result = FALSE;

  load_score_cache_slot(
    arg_score_cache_slot,
    &arg_score_cache[arg_with_search->S_board.B_key % nscore_cache_entries]);

  ui32_t crc32 = 0xFFFFFFFF;
  crc32 = HW_CRC32_U64(crc32, arg_score_cache_slot->SCS_key);
  crc32 = HW_CRC32_U32(crc32, arg_score_cache_slot->SCS_score);
  crc32 = ~crc32;

  if (crc32 != arg_score_cache_slot->SCS_crc32)
  {
    arg_with_search->S_total_score_cache_crc_errors++;

    *arg_score_cache_slot = score_cache_slot_default;
  }
  else if (arg_score_cache_slot->SCS_key == arg_with_search->S_board.B_key)
  {
    result = TRUE;
  }

  return (result);
}

local void update_score_cache(search_t *arg_with_search,
                              score_cache_shared_slot_t *arg_score_cache,
                              score_cache_slot_t *arg_score_cache_slot)
{
  store_score_cache_slot(
    &arg_score_cache[arg_with_search->S_board.B_key % nscore_cache_entries],
    arg_score_cache_slot);
}

#undef GEN_CSV

static inline uint64_t mirror_bb(uint64_t bb)
{
  ui64_t result;

  result = (bb >> 6);

  result = BIT_REVERSE(result) >> (64 - 54);

  result = (result << 6);

#ifdef DEBUG
  ui64_t debug = 0;

  while (bb)
  {
    int ifield = BIT_CTZ(bb);

    bb &= ~BITULL(ifield);

    debug |= 1ULL << (65 - ifield);
  }

  HARDBUG(debug != result)
#endif

  return (result);
}

score_t return_score_from_board(board_t *arg_with_board,
                                double_t *arg_network_score_scaled)
{
  score_t result = SCORE(0);

  int mirror = FALSE;

  if (IS_BLACK(arg_with_board->B_colour2move))
  {
    ui64_t white_man_bb = mirror_bb(arg_with_board->B_black_man_bb);
    ui64_t black_man_bb = mirror_bb(arg_with_board->B_white_man_bb);
    ui64_t white_king_bb = mirror_bb(arg_with_board->B_black_king_bb);
    ui64_t black_king_bb = mirror_bb(arg_with_board->B_white_king_bb);

    arg_with_board->B_white_man_bb = white_man_bb;
    arg_with_board->B_black_man_bb = black_man_bb;
    arg_with_board->B_white_king_bb = white_king_bb;
    arg_with_board->B_black_king_bb = black_king_bb;

    arg_with_board->B_colour2move = WHITE_ENUM;
    mirror = TRUE;
  }

  network_t *with_network = networks + arg_with_board->B_network_id;

  network_state_t *network_state = &(arg_with_board->B_network_state);

  for (int input = 0;
       input < with_network->N_pattern_dense_network_shared.DNS_ninputs;
       input++)
    network_state->NS_pattern_dense_network_state.DNS_inputs[input] = 0;

  layer_shared_t *with_current =
    with_network->N_pattern_dense_network_shared.DNS_layers;

  layer_thread_t *with_current_thread =
    network_state->NS_pattern_dense_network_state.DNS_layers;

  for (int ioutput = 0; ioutput < with_current->LS_noutputs; ioutput++)
    with_current_thread->LT_dot[ioutput] = 0;

  int nwhite_man = BIT_COUNT(arg_with_board->B_white_man_bb);
  int nblack_man = BIT_COUNT(arg_with_board->B_black_man_bb);
  int nwhite_king = BIT_COUNT(arg_with_board->B_white_king_bb);
  int nblack_king = BIT_COUNT(arg_with_board->B_black_king_bb);

#ifdef USE_AVX2_INTRINSICS
  __m256 sum0;
  __m256 sum1;
  __m256 sum64[8];

  if (with_network->N_pattern_dense_network_64x16_kernel)
    for (int i = 0; i < 8; i++)
      sum64[i] = _mm256_setzero_ps();

  if (with_network->N_pattern_dense_network_16x8_kernel)
  {
    sum0 = _mm256_setzero_ps();
    sum1 = _mm256_setzero_ps();
  }
#endif

  for (int ipattern = 0;
       ipattern < with_network->N_patterns_shared.PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      with_network->N_patterns_shared.PSS_patterns + ipattern;

    int istate = INVALID;

    if (with_pattern_shared->PS_id == PATTERN_BASE3_INDEX_ID)
    {
      istate = base3_index_v4(arg_with_board, with_pattern_shared->PS_bb);
    }

    HARDBUG(istate == INVALID)

    HARDBUG(istate < 0)

    if (with_network->N_architecture_patterns == NETWORK_ARCHITECTURE_ADD)
    {
#ifdef USE_AVX2_INTRINSICS
      if (with_network->N_pattern_dense_network_16x8_kernel)
      {
        const float *weights = with_pattern_shared->PS_weights_nstatesxnembed +
                               (size_t)istate * with_pattern_shared->PS_nembed;

        // MPI shared allocations need not be 32-byte aligned.
        sum0 = _mm256_add_ps(sum0, _mm256_loadu_ps(weights));
        sum1 = _mm256_add_ps(sum1, _mm256_loadu_ps(weights + 8));
      }
      else if (with_network->N_pattern_dense_network_64x16_kernel)
      {
        const float *weights = with_pattern_shared->PS_weights_nstatesxnembed +
                               (size_t)istate * with_pattern_shared->PS_nembed;

        // Preserve pattern order and allow unaligned shared-memory rows.
        for (int i = 0; i < 8; i++)
          sum64[i] = _mm256_add_ps(sum64[i], _mm256_loadu_ps(weights + 8 * i));
      }
      else
#endif
        vadd_ab2a(with_pattern_shared->PS_nembed,
                  network_state->NS_pattern_dense_network_state.DNS_inputs +
                    with_pattern_shared->PS_offset,
                  with_pattern_shared->PS_weights_nstatesxnembed +
                    (size_t)istate * with_pattern_shared->PS_nembed);
    }
    else
    {
      if (arg_network_score_scaled != NULL)
        vcopy_a2b(with_pattern_shared->PS_nembed,
                  with_pattern_shared->PS_weights_nstatesxnembed +
                    (size_t)istate * with_pattern_shared->PS_nembed,
                  network_state->NS_pattern_dense_network_state.DNS_inputs +
                    with_pattern_shared->PS_offset);

      vadd_ab2a(with_current->LS_noutputs,
                with_current_thread->LT_dot,
                with_pattern_shared->PS_sum_nstatesxnoutputs[istate]);
    }
  }

#ifdef USE_AVX2_INTRINSICS
  if (with_network->N_pattern_dense_network_16x8_kernel)
  {
    _mm256_store_ps(network_state->NS_pattern_dense_network_state.DNS_inputs,
                    sum0);
    _mm256_store_ps(network_state->NS_pattern_dense_network_state.DNS_inputs +
                      8,
                    sum1);
  }
  else if (with_network->N_pattern_dense_network_64x16_kernel)
  {
    for (int i = 0; i < 8; i++)
      _mm256_store_ps(network_state->NS_pattern_dense_network_state.DNS_inputs +
                        8 * i,
                      sum64[i]);
  }
#endif

  double network_score_scaled = return_dense_network_score_scaled(
    &(with_network->N_pattern_dense_network_shared),
    &(arg_with_board->B_network_state.NS_pattern_dense_network_state));

  if (with_network->N_architecture_material != NETWORK_ARCHITECTURE_NULL)
    evaluate_material_network(with_network,
                              &network_state->NS_material_dense_network_state,
                              nwhite_man,
                              nblack_man,
                              nwhite_king,
                              nblack_king,
                              arg_network_score_scaled != NULL);

  // prepare inputs

  dense_network_shared_t *with_pattern_shared =
    &(with_network->N_pattern_dense_network_shared);

  dense_network_state_t *with_pattern_thread =
    &(arg_with_board->B_network_state.NS_pattern_dense_network_state);

  vcopy_a2b(
    with_pattern_shared->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
      .LS_noutputs,
    with_pattern_thread->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
      .LT_outputs,
    network_state->NS_dense_network_state.DNS_inputs);

  if (with_network->N_architecture_material != NETWORK_ARCHITECTURE_NULL)
  {
    dense_network_shared_t *with_material_shared =
      &(with_network->N_material_dense_network_shared);

    dense_network_state_t *with_material_thread =
      &(arg_with_board->B_network_state.NS_material_dense_network_state);

    vcopy_a2b(
      with_material_shared->DNS_layers[with_material_shared->DNS_nlayers - 1]
        .LS_noutputs,
      with_material_thread->DNS_layers[with_material_shared->DNS_nlayers - 1]
        .LT_outputs,
      network_state->NS_dense_network_state.DNS_inputs +
        with_pattern_shared->DNS_layers[with_pattern_shared->DNS_nlayers - 1]
          .LS_noutputs);
  }

  network_score_scaled = return_dense_network_score_scaled(
    &(with_network->N_dense_network_shared),
    &(arg_with_board->B_network_state.NS_dense_network_state));

  if (arg_network_score_scaled != NULL)
    *arg_network_score_scaled = network_score_scaled;

#ifdef DEBUG
  static int n = 0;

  double network_score_double = return_dense_network_score_double(
    &(with_network->N_dense_network_shared),
    &(arg_with_board->B_network_state.NS_dense_network_state));

  n++;

  if (n < 99)
    PRINTF("network_score_scaled=%.6f network_score_double=%.6f\n",
           network_score_scaled,
           network_score_double);

  if (fabs(network_score_scaled - network_score_double) > 0.001)
  {
    my_printf(arg_with_board->B_my_printf,
              "WARNING network_score_scaled=%.6f network_score_double=%.6f\n",
              network_score_scaled,
              network_score_double);
  }
#endif

#if SCORE_T == SCORE_T_INT
  result = round(network_score_scaled *
                 (double)with_network->N_network2material_score);
#elif SCORE_T == SCORE_T_FLOAT
  result = round(network_score_scaled * SCORE(1)) *
           with_network->N_network2material_score;
#else
#error unknown SCORE_T
#endif

  if (mirror)
  {
    ui64_t white_man_bb = mirror_bb(arg_with_board->B_black_man_bb);
    ui64_t black_man_bb = mirror_bb(arg_with_board->B_white_man_bb);
    ui64_t white_king_bb = mirror_bb(arg_with_board->B_black_king_bb);
    ui64_t black_king_bb = mirror_bb(arg_with_board->B_white_king_bb);

    arg_with_board->B_white_man_bb = white_man_bb;
    arg_with_board->B_black_man_bb = black_man_bb;
    arg_with_board->B_white_king_bb = white_king_bb;
    arg_with_board->B_black_king_bb = black_king_bb;

    arg_with_board->B_colour2move = BLACK_ENUM;
  }

  /*
    if (IS_BLACK(arg_with_board->B_colour2move))
      result = -result;
  */

#ifdef GEN_CSV
  if ((BIT_COUNT(arg_with_board->B_black_king_bb) == 0) and
      (BIT_COUNT(arg_with_board->B_white_king_bb) == 0))
  {
    static int ncsv = 0;

    PRINTF("CSV %d,%d\n", material_score, result);
    ncsv++;
    HARDBUG(ncsv > 1000000)
  }
#endif

  if (IS_SCORE_LOST(result) or IS_SCORE_WON(result))
    result = return_material_score(arg_with_board);

  return (result);
}

score_t return_score(search_t *arg_with_search)
{
  colour_enum my_colour;

  if (IS_WHITE(arg_with_search->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  score_t result = SCORE(0);

  score_cache_slot_t score_cache_slot;

  if (options.score_cache_size > 0)
  {
    if (probe_score_cache(arg_with_search,
                          score_cache[my_colour],
                          &score_cache_slot))
    {
      ++(arg_with_search->S_total_score_cache_hits);

      result = score_cache_slot.SCS_score;

      return (result);
    }
  }

  board_t *with = &(arg_with_search->S_board);

  score_t material_score = return_material_score(with);

  if (options.material_only)
  {
    ++(arg_with_search->S_total_material_only_evaluations);

    result = material_score;
  }
  else
  {
    ++(arg_with_search->S_total_network_evaluations);

    result = return_score_from_board(with, NULL);

    if (options.score_cache_size > 0)
    {
      score_cache_slot.SCS_key = arg_with_search->S_board.B_key;
      score_cache_slot.SCS_score = result;

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = HW_CRC32_U64(crc32, score_cache_slot.SCS_key);
      crc32 = HW_CRC32_U32(crc32, score_cache_slot.SCS_score);
      score_cache_slot.SCS_crc32 = ~crc32;

      update_score_cache(arg_with_search,
                         score_cache[my_colour],
                         &score_cache_slot);
    }
  }

  HARDBUG(IS_SCORE_LOST(result))
  HARDBUG(IS_SCORE_WON(result))

  return (result);
}

int return_npieces(board_t *arg_with_board)
{
  return (BIT_COUNT(arg_with_board->B_white_man_bb) +
          BIT_COUNT(arg_with_board->B_white_king_bb) +
          BIT_COUNT(arg_with_board->B_black_man_bb) +
          BIT_COUNT(arg_with_board->B_black_king_bb));
}

score_t return_material_score(board_t *arg_with_board)
{
  int nwhite_man = BIT_COUNT(arg_with_board->B_white_man_bb);
  int nblack_man = BIT_COUNT(arg_with_board->B_black_man_bb);
  int nwhite_king = BIT_COUNT(arg_with_board->B_white_king_bb);
  int nblack_king = BIT_COUNT(arg_with_board->B_black_king_bb);

  score_t material_score = SCORE((nwhite_man - nblack_man) * SCORE_MAN +
                                 (nwhite_king - nblack_king) * SCORE_KING);
  if (IS_WHITE(arg_with_board->B_colour2move))
    return (material_score);
  else
    return (-material_score);
}

local void initialize_score_caches(void)
{
  PRINTF("clear_score_caches..\n");

  for (i64_t ientry = 0; ientry < nscore_cache_entries; ientry++)
    init_score_cache_slot(&score_cache[WHITE_ENUM][ientry],
                          &score_cache_slot_default);

  for (i64_t ientry = 0; ientry < nscore_cache_entries; ientry++)
    init_score_cache_slot(&score_cache[BLACK_ENUM][ientry],
                          &score_cache_slot_default);

  for (int colour = 0; colour < NCOLOUR_ENUM; colour++)
  {
    HARDBUG(!atomic_is_lock_free(&score_cache[colour][0].SCS_key))
    HARDBUG(!atomic_is_lock_free(&score_cache[colour][0].SCS_data))
  }

  PRINTF("..done\n");
}

void init_score(void)
{
  PRINTF("sizeof(score_cache_slot_t)=%lld\n",
         (i64_t)sizeof(score_cache_slot_t));

  score_cache[WHITE_ENUM] = NULL;
  score_cache[BLACK_ENUM] = NULL;

  nscore_cache_entries = 0;

  if (options.score_cache_size > 0)
  {
    i64_t score_cache_size = options.score_cache_size * MBYTE / 2;

    nscore_cache_entries =
      first_prime_below(roundf(score_cache_size / sizeof(score_cache_slot_t)));

    HARDBUG(nscore_cache_entries < 3)

    PRINTF("nscore_cache_entries=%lld\n", nscore_cache_entries);

    score_cache_slot_default.SCS_key = 0;
    score_cache_slot_default.SCS_score = SCORE_MINUS_INFINITY;

    ui32_t crc32 = 0xFFFFFFFF;
    crc32 = HW_CRC32_U64(crc32, score_cache_slot_default.SCS_key);
    crc32 = HW_CRC32_U32(crc32, score_cache_slot_default.SCS_score);
    score_cache_slot_default.SCS_crc32 = ~crc32;

    MY_MALLOC_BY_TYPE(score_cache[WHITE_ENUM],
                      score_cache_shared_slot_t,
                      nscore_cache_entries)

    MY_MALLOC_BY_TYPE(score_cache[BLACK_ENUM],
                      score_cache_shared_slot_t,
                      nscore_cache_entries)

    initialize_score_caches();
  }
}
