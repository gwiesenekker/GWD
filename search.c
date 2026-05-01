// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST
#include "globals.h"

#define ABCS_AGE_MAX 4

#define ABCS_SCORE_GE_BETA_BIT BIT(2)
#define ABCS_SCORE_LE_ALPHA_BIT BIT(3)
#define ABCS_SCORE_TRUE_SCORE_BIT BIT(4)

#define FAST_MODULO64(H, N) ((ui64_t)(((__uint128_t)(H) * (N)) >> 64));

#define TWEAK_SEARCH_EXTENDED_BIT BIT(0)
#define TWEAK_MOVE_NULL_BIT BIT(1)
#define TWEAK_MOVE_REDUCED_BIT BIT(2)
#define TWEAK_MOVE_EXTENDED_BIT BIT(3)
#define TWEAK_SINGULAR_PROBE_BIT BIT(4)

#define STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH BIT(0)
#define STAGE_SEARCH_MOVE_AT_FULL_DEPTH BIT(1)
#define STAGE_SEARCHED_MOVE_AT_REDUCED_DEPTH BIT(2)
#define STAGE_SEARCHED_MOVE_AT_FULL_DEPTH BIT(3)

#undef TRACE_AB

#ifdef TRACE_AB

#define TRACE_AB_ENTRY(arg_with_search, \
                       arg_my_alpha,    \
                       arg_my_beta,     \
                       arg_node_type,   \
                       arg_my_depth)    \
  trace_ab_entry(arg_with_search,       \
                 __FILE__,              \
                 __FUNC__,              \
                 __LINE__,              \
                 arg_my_alpha,          \
                 arg_my_beta,           \
                 arg_node_type,         \
                 arg_my_depth);

#define TRACE_AB_CACHE(arg_with_search, arg_info, arg_alpha_beta_cache_slot) \
  trace_ab_cache(arg_with_search,                                            \
                 __FILE__,                                                   \
                 __FUNC__,                                                   \
                 __LINE__,                                                   \
                 arg_info,                                                   \
                 arg_alpha_beta_cache_slot);

#define TRACE_KEY(arg_with_search) \
  ui64_t trace_key = arg_with_search->S_board.B_key;

#define TRACE_AB_WINDOW(arg_trace_key,  \
                        arg_info,       \
                        arg_alpha,      \
                        arg_beta,       \
                        arg_moves_list, \
                        arg_jmove)      \
  trace_ab_window(arg_trace_key,        \
                  __FILE__,             \
                  __FUNC__,             \
                  __LINE__,             \
                  arg_info,             \
                  arg_alpha,            \
                  arg_beta,             \
                  arg_moves_list,       \
                  arg_jmove);

#define TRACE_INDENT(arg_moves_list, arg_jmove)     \
  int trace_lpv_string = blength(trace_bpv_string); \
  trace_indent(arg_moves_list, arg_jmove);

#define TRACE_DEDENT                                              \
  HARDBUG(btrunc(trace_bpv_string, trace_lpv_string) == BSTR_ERR) \
  trace_dedent();

#define TRACE_AB_RESULT(arg_trace_key,  \
                        arg_info,       \
                        arg_alpha,      \
                        arg_beta,       \
                        arg_moves_list, \
                        arg_jmove,      \
                        arg_result)     \
  trace_ab_result(arg_trace_key,        \
                  __FILE__,             \
                  __FUNC__,             \
                  __LINE__,             \
                  arg_info,             \
                  arg_alpha,            \
                  arg_beta,             \
                  arg_moves_list,       \
                  arg_jmove,            \
                  arg_result);

#define TRACE_AB_EXIT(arg_with_search, \
                      arg_my_alpha,    \
                      arg_my_beta,     \
                      arg_node_type,   \
                      arg_my_depth,    \
                      arg_best_result) \
  trace_ab_exit(arg_with_search,       \
                __FILE__,              \
                __FUNC__,              \
                __LINE__,              \
                arg_my_alpha,          \
                arg_my_beta,           \
                arg_node_type,         \
                arg_my_depth,          \
                arg_best_result);

#else

#define TRACE_AB_ENTRY(arg_with_search, \
                       arg_my_alpha,    \
                       arg_my_beta,     \
                       arg_node_type,   \
                       arg_my_depth)
#define TRACE_AB_CACHE(arg_with_search, arg_info, arg_alpha_beta_cache_slot)
#define TRACE_KEY(arg_with_search)
#define TRACE_AB_WINDOW(arg_trace_key,  \
                        arg_info,       \
                        arg_alpha,      \
                        arg_beta,       \
                        arg_moves_list, \
                        arg_jmove)
#define TRACE_INDENT(arg_moves_list, arg_jmove)
#define TRACE_DEDENT
#define TRACE_AB_RESULT(arg_trace_key,  \
                        arg_info,       \
                        arg_alpha,      \
                        arg_beta,       \
                        arg_moves_list, \
                        arg_jmove,      \
                        arg_result)
#define TRACE_AB_EXIT(arg_with_search, \
                      arg_my_alpha,    \
                      arg_my_beta,     \
                      arg_node_type,   \
                      arg_my_depth,    \
                      arg_best_result)

#endif

#undef CHECK_TIME_LIMIT_IN_QUIESCENCE

#define UPDATE_NODES 1000

#define NABCS_MOVES 4
#define NABCS_SLOTS 4

typedef struct
{
  ui64_t ABCS_key;
  union
  {
    ui64_t ABCS_data;
    struct
    {
      i16_t ABCS_depth;
      ui16_t ABCS_flags;
      i8_t ABCS_moves[NABCS_MOVES];
    };
  };
  union
  {
    ui64_t ABCS_scores;
    struct
    {
      score_t ABCS_raw_score;
      score_t ABCS_score;
    };
  };
  ui32_t ABCS_crc32;
  ui32_t ABCS_age;
#ifdef DEBUG
  ui64_t ABCS_debug;
#endif
} alpha_beta_cache_slot_t;

typedef struct
{
  alpha_beta_cache_slot_t ABCE_slots[NABCS_SLOTS];
} alpha_beta_cache_entry_t;

local i64_t nalpha_beta_cache_entries;
local alpha_beta_cache_slot_t alpha_beta_cache_slot_default;
local alpha_beta_cache_entry_t alpha_beta_cache_entry_default;
local alpha_beta_cache_entry_t *alpha_beta_cache[NETWORKS_MAX][NCOLOUR_ENUM];

local pv_entry_t pv_entry_default;
local pv_entries_t pv_entries_default;

local i64_t global_nodes;

local bucket_t bucket_depth;

#undef LAZY_STATS
#ifdef LAZY_STATS
local stats_t lazy_stats[2 * NPIECES_MAX + 1][2 * NPIECES_MAX + 1];
#endif

local int trace_depth = 0;
local long trace_line = 0;

bstring trace_bpv_string = NULL;

local void binary(bstring arg_bstring, uint64_t arg_uint64, int arg_nbits)
{
  HARDBUG((arg_nbits <= 0) or (arg_nbits > 64))

  btrunc(arg_bstring, 0);

  for (int ibit = arg_nbits - 1; ibit >= 0; --ibit)
  {
    char cbit = (arg_uint64 & (1ULL << ibit)) ? '1' : '0';

    HARDBUG(bconchar(arg_bstring, cbit) == BSTR_ERR)
  }
}

int draw_by_repetition(board_t *arg_with_board, int arg_strict)
{
  int result = FALSE;

  int npieces = return_npieces(arg_with_board);

  int n = 1;

  for (int istate = arg_with_board->B_inode - 2; istate >= 0; istate -= 2)
  {
    if (arg_with_board->B_states[istate + 1].BS_npieces != npieces)
      goto label_return;

    if (arg_with_board->B_states[istate].BS_npieces != npieces)
      goto label_return;

    if (HASH_KEY_EQ(arg_with_board->B_states[istate].BS_key,
                    arg_with_board->B_key))
    {
      ++n;

      if (!arg_strict or (n >= 3))
      {
        result = TRUE;

        goto label_return;
      }
    }
  }

label_return:

  return (result);
}

int move_repetition(board_t *arg_with_board)
{
  if ((arg_with_board->B_inode - 2) < 0)
    return (FALSE);

  int npieces;

  if (IS_WHITE(arg_with_board->B_colour2move))
  {
    npieces = BIT_COUNT(arg_with_board->B_white_man_bb |
                        arg_with_board->B_white_king_bb);
  }
  else
  {
    npieces = BIT_COUNT(arg_with_board->B_black_man_bb |
                        arg_with_board->B_black_king_bb);
  }

  if (npieces > 1)
    return (FALSE);

  // 50-45
  //
  // 45-50
  //
  // 50-45
  //
  // 45-50
  //
  // 50-45
  //
  // 45-50
  //
  // so now black will play 50-45 again
  // we look two inodes back for 45-40, and then two more inodes back
  // as Zobrist hashing will return the same key for 50-45 and 45-50

  int n = 0;

  for (int jnode = arg_with_board->B_inode - 4; jnode >= 0; jnode -= 2)
  {
    if (HASH_KEY_EQ(
          arg_with_board->B_nodes[arg_with_board->B_inode - 2].node_move_key,
          arg_with_board->B_nodes[jnode].node_move_key))
    {
      ++n;

      if (n >= 3)
      {
        return (TRUE);
      }
    }
    else
    {
      return (FALSE);
    }
  }

  return (FALSE);
}

local inline ui32_t
abcs2crc32(ui64_t abcs_key, ui64_t abcs_data, ui64_t abcs_scores)
{
  ui32_t crc32 = 0xFFFFFFFF;

  crc32 = HW_CRC32_U64(crc32, abcs_key);
  crc32 = HW_CRC32_U64(crc32, abcs_data);
  crc32 = HW_CRC32_U64(crc32, abcs_scores);

  return (~crc32);
}

local inline ui64_t
abcs2crc64(ui64_t abcs_key, ui64_t abcs_data, ui64_t abcs_scores)
{
  ui64_t crc64;

  ui32_t crc32 = 0xFFFFFFFF;
  crc32 = HW_CRC32_U64(crc32, abcs_key);
  crc32 = HW_CRC32_U64(crc32, abcs_data);
  crc32 = HW_CRC32_U64(crc32, abcs_scores);
  crc64 = ~crc32;

  crc32 = 0x0;
  crc32 = HW_CRC32_U64(crc32, abcs_scores);
  crc32 = HW_CRC32_U64(crc32, abcs_data);
  crc32 = HW_CRC32_U64(crc32, abcs_key);
  crc64 = (crc64 << 32) | ~crc32;

  return (crc64);
}

local int return_combination_length(ui64_t arg_history)
{
  ui64_t history = arg_history;

  // a combination should end with a regular move by me

  if ((arg_history & 0xF) != 0)
    return (0);

  arg_history >>= 4;

  // and the combination should end with a capture by you

  int ncaptx_you = arg_history & 0xF;

  if ((ncaptx_you == 0) or (ncaptx_you == 0xF))
    return (0);

  arg_history >>= 4;

  int balance = ncaptx_you;

  int result = 1;

  while (TRUE)
  {
    int ncaptx = arg_history & 0xF;

    if (ncaptx == 0xF)
      break;

    int ncaptx_me = ncaptx;

    arg_history >>= 4;

    ncaptx = arg_history & 0xF;

    if (ncaptx == 0xF)
      break;

    ncaptx_you = ncaptx;

    // you should not have captured during the combination?

    if (ncaptx_you > 0)
      return (0);

    arg_history >>= 4;

    balance -= ncaptx_me;

    // if the balance becomes <= 0 you got nowhere

    if (balance <= 0)
      return (0);

    if ((ncaptx_me > 0) and (ncaptx_you == 0))
      ++result;

    HARDBUG(result > 7)
  }

  // a combination should start with a regular move by you

  if (ncaptx_you != 0)
    return (0);

  HARDBUG(balance <= 0)

  if (FALSE and (result >= 3))
  {
    for (int i = 15; i >= 0; i--)
    {
      int j = (history >> (4 * i)) & 0xF;
      if ((j != 15) and ((i % 2) == 0))
        j = -j;
      PRINTF(" (%d)", j);
    }
    PRINTF(" balance=%d result=%d\n", balance, result);
    // FATAL("eh?", 1)
  }

  return (result);
}

void clear_totals(search_t *arg_with_search)
{
  arg_with_search->S_total_move_repetitions = 0;

  arg_with_search->S_total_quiescence_nodes = 0;
  arg_with_search->S_total_quiescence_1move = 0;
  arg_with_search->S_total_quiescence_all_moves_captures_only = 0;
  arg_with_search->S_total_quiescence_all_moves_le2_moves = 0;
  arg_with_search->S_total_quiescence_all_moves_combination = 0;
  arg_with_search->S_total_quiescence_all_moves_extended = 0;

  arg_with_search->S_total_nodes = 0;
  arg_with_search->S_total_alpha_beta_nodes = 0;
  arg_with_search->S_total_alpha_beta_1move = 0;
  arg_with_search->S_total_capture_nodes = 0;
  arg_with_search->S_total_minimal_window_nodes = 0;
  arg_with_search->S_total_pv_nodes = 0;

  arg_with_search->S_total_quiescence_extension_searches_combination = 0;

  arg_with_search->S_total_pv_extension_searches_combination = 0;

  arg_with_search->S_total_reductions_combinations = 0;
  arg_with_search->S_total_reductions = 0;
  arg_with_search->S_total_reductions_lost = 0;
  arg_with_search->S_total_reductions_le_alpha = 0;
  arg_with_search->S_total_reductions_ge_beta = 0;

  arg_with_search->S_total_singular_extension_probes = 0;
  arg_with_search->S_total_singular_extension_probes_le_alpha = 0;
  arg_with_search->S_total_singular_extension_probes_ge_beta = 0;
  arg_with_search->S_total_singular_extensions = 0;
  arg_with_search->S_total_singular_reductions = 0;

  arg_with_search->S_total_evaluations = 0;
  arg_with_search->S_total_lazy_alpha_evaluations = 0;
  arg_with_search->S_total_lazy_beta_evaluations = 0;
  arg_with_search->S_total_material_only_evaluations = 0;
  arg_with_search->S_total_network_evaluations = 0;

  arg_with_search->S_total_alpha_beta_cache_updates = 0;
  arg_with_search->S_total_alpha_beta_cache_aged = 0;
  arg_with_search->S_total_alpha_beta_cache_hits = 0;
  arg_with_search->S_total_alpha_beta_cache_depth_hits = 0;
  arg_with_search->S_total_alpha_beta_cache_le_alpha_hits = 0;
  arg_with_search->S_total_alpha_beta_cache_le_alpha_cutoffs = 0;
  arg_with_search->S_total_alpha_beta_cache_ge_beta_hits = 0;
  arg_with_search->S_total_alpha_beta_cache_ge_beta_cutoffs = 0;
  arg_with_search->S_total_alpha_beta_cache_true_score_hits = 0;
  arg_with_search->S_total_alpha_beta_cache_le_alpha_stored = 0;
  arg_with_search->S_total_alpha_beta_cache_ge_beta_stored = 0;
  arg_with_search->S_total_alpha_beta_cache_true_score_stored = 0;
  arg_with_search->S_total_alpha_beta_cache_nmoves_errors = 0;
  arg_with_search->S_total_alpha_beta_cache_nprobes = 0;
  arg_with_search->S_total_alpha_beta_cache_memory_hits = 0;
  arg_with_search->S_total_alpha_beta_cache_local_hits = 0;
  arg_with_search->S_total_alpha_beta_cache_crc_errors = 0;

  arg_with_search->S_total_score_cache_hits = 0;
  arg_with_search->S_total_score_cache_crc_errors = 0;
}

#define PRINTF_TOTAL(X) \
  my_printf(arg_with_search->S_my_printf, #X "=%lld\n", arg_with_search->X);

void print_totals(search_t *arg_with_search){
  // clang-format off

  PRINTF_TOTAL(S_total_move_repetitions)

  PRINTF_TOTAL(S_total_quiescence_nodes)
  PRINTF_TOTAL(S_total_quiescence_1move)
  PRINTF_TOTAL(S_total_quiescence_all_moves_captures_only)
  PRINTF_TOTAL(S_total_quiescence_all_moves_le2_moves)
  PRINTF_TOTAL(S_total_quiescence_all_moves_combination)
  PRINTF_TOTAL(S_total_quiescence_all_moves_extended)

  PRINTF_TOTAL(S_total_nodes)
  PRINTF_TOTAL(S_total_alpha_beta_nodes)
  PRINTF_TOTAL(S_total_alpha_beta_1move)
  PRINTF_TOTAL(S_total_capture_nodes)
  PRINTF_TOTAL(S_total_minimal_window_nodes)
  PRINTF_TOTAL(S_total_pv_nodes)

  PRINTF_TOTAL(S_total_quiescence_extension_searches_combination)

  PRINTF_TOTAL(S_total_pv_extension_searches_combination)

  PRINTF_TOTAL(S_total_reductions_combinations)
  PRINTF_TOTAL(S_total_reductions)
  PRINTF_TOTAL(S_total_reductions_lost)
  PRINTF_TOTAL(S_total_reductions_le_alpha)
  PRINTF_TOTAL(S_total_reductions_ge_beta)

  PRINTF_TOTAL(S_total_singular_extension_probes)
  PRINTF_TOTAL(S_total_singular_extension_probes_le_alpha)
  PRINTF_TOTAL(S_total_singular_extension_probes_ge_beta)
  PRINTF_TOTAL(S_total_singular_extensions)
  PRINTF_TOTAL(S_total_singular_reductions)

  PRINTF_TOTAL(S_total_evaluations)
  PRINTF_TOTAL(S_total_lazy_alpha_evaluations)
  PRINTF_TOTAL(S_total_lazy_beta_evaluations)
  PRINTF_TOTAL(S_total_material_only_evaluations)
  PRINTF_TOTAL(S_total_network_evaluations)

  PRINTF_TOTAL(S_total_alpha_beta_cache_updates)
  PRINTF_TOTAL(S_total_alpha_beta_cache_aged)
  PRINTF_TOTAL(S_total_alpha_beta_cache_hits)
  PRINTF_TOTAL(S_total_alpha_beta_cache_depth_hits)
  PRINTF_TOTAL(S_total_alpha_beta_cache_le_alpha_hits)
  PRINTF_TOTAL(S_total_alpha_beta_cache_le_alpha_cutoffs)
  PRINTF_TOTAL(S_total_alpha_beta_cache_ge_beta_hits)
  PRINTF_TOTAL(S_total_alpha_beta_cache_ge_beta_cutoffs)
  PRINTF_TOTAL(S_total_alpha_beta_cache_true_score_hits)
  PRINTF_TOTAL(S_total_alpha_beta_cache_le_alpha_stored)
  PRINTF_TOTAL(S_total_alpha_beta_cache_ge_beta_stored)
  PRINTF_TOTAL(S_total_alpha_beta_cache_true_score_stored)
  PRINTF_TOTAL(S_total_alpha_beta_cache_nmoves_errors)
  PRINTF_TOTAL(S_total_alpha_beta_cache_nprobes)
  PRINTF_TOTAL(S_total_alpha_beta_cache_memory_hits)
  PRINTF_TOTAL(S_total_alpha_beta_cache_local_hits)
  PRINTF_TOTAL(S_total_alpha_beta_cache_crc_errors)

  PRINTF_TOTAL(S_total_score_cache_hits)
  PRINTF_TOTAL(S_total_score_cache_crc_errors)

  // clang-format on
}

local
  int probe_alpha_beta_cache(search_t *arg_with_search,
                             alpha_beta_cache_entry_t *arg_alpha_beta_cache,
                             alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot)
{
  BEGIN_BLOCK(__func__)

  arg_with_search->S_total_alpha_beta_cache_nprobes++;

  int result = FALSE;

  alpha_beta_cache_entry_t *with_alpha_beta_cache_entry =
    arg_alpha_beta_cache +
    FAST_MODULO64(arg_with_search->S_board.B_key, nalpha_beta_cache_entries);

  for (int islot = 0; islot < NABCS_SLOTS; islot++)
  {
    if (with_alpha_beta_cache_entry->ABCE_slots[islot].ABCS_key !=
        arg_with_search->S_board.B_key)
      continue;

    *arg_alpha_beta_cache_slot = with_alpha_beta_cache_entry->ABCE_slots[islot];

    ui32_t abcs_crc32 = arg_alpha_beta_cache_slot->ABCS_crc32;

    ui64_t abcs_data = arg_alpha_beta_cache_slot->ABCS_data;

    ui64_t abcs_scores = arg_alpha_beta_cache_slot->ABCS_scores;

    if (abcs2crc32(arg_with_search->S_board.B_key, abcs_data, abcs_scores) ==
        abcs_crc32)
    {
      HARDBUG(arg_alpha_beta_cache_slot->ABCS_key !=
              arg_with_search->S_board.B_key)

#ifdef DEBUG
      HARDBUG(with_alpha_beta_cache_entry->ABCE_slots[islot].ABCS_debug !=
              arg_with_search->S_board.B_key)
#endif

      result = TRUE;

      break;
    }
    else
    {
      arg_with_search->S_total_alpha_beta_cache_crc_errors++;
    }
  }

  END_BLOCK

  return (result);
}

local void
update_alpha_beta_cache(search_t *arg_with_search,
                        alpha_beta_cache_entry_t *arg_alpha_beta_cache,
                        alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot)
{
  BEGIN_BLOCK(__func__)

  HARDBUG(arg_alpha_beta_cache_slot->ABCS_key != arg_with_search->S_board.B_key)

  arg_alpha_beta_cache_slot->ABCS_age = ABCS_AGE_MAX;

  alpha_beta_cache_entry_t *with_alpha_beta_cache_entry =
    arg_alpha_beta_cache +
    FAST_MODULO64(arg_with_search->S_board.B_key, nalpha_beta_cache_entries);

  int islot = INVALID;

  for (islot = 0; islot < NABCS_SLOTS; islot++)
  {
    if (with_alpha_beta_cache_entry->ABCE_slots[islot].ABCS_key !=
        arg_with_search->S_board.B_key)
      continue;

    alpha_beta_cache_slot_t abcs_slot =
      with_alpha_beta_cache_entry->ABCE_slots[islot];

    ui32_t abcs_crc32 = abcs_slot.ABCS_crc32;

    ui64_t abcs_data = abcs_slot.ABCS_data;

    ui64_t abcs_scores = abcs_slot.ABCS_scores;

    if (abcs2crc32(arg_with_search->S_board.B_key, abcs_data, abcs_scores) ==
        abcs_crc32)
    {
      HARDBUG(abcs_slot.ABCS_key != arg_with_search->S_board.B_key)

#ifdef DEBUG
      HARDBUG(with_alpha_beta_cache_entry->ABCE_slots[islot].ABCS_debug !=
              arg_with_search->S_board.B_key)
#endif

      break;
    }
    else
    {
      arg_with_search->S_total_alpha_beta_cache_crc_errors++;
    }
  }

#undef EVICT_BY_RANK
#ifdef EVICT_BY_RANK
  if (islot == NABCS_SLOTS)
  {
    arg_with_search->S_total_alpha_beta_cache_updates++;

    ui32_t rank[NABCS_SLOTS];

    for (int jslot = 0; jslot < NABCS_SLOTS; jslot++)
    {
      // make local copies as depth and age might have been corrupted

      i32_t depth = with_alpha_beta_cache_entry->ABCE_slots[jslot].ABCS_depth;
      ui32_t age = with_alpha_beta_cache_entry->ABCE_slots[jslot].ABCS_age;

      if ((age > ABCS_AGE_MAX) or (depth > DEPTH_MAX) or (depth < 0))
      {
        arg_with_search->S_total_alpha_beta_cache_aged++;

        with_alpha_beta_cache_entry->ABCE_slots[jslot] =
          alpha_beta_cache_slot_default;

        rank[jslot] = 0;
      }
      else
      {
        --age;

        rank[jslot] = ((ui32_t)depth << 16) | age;

        with_alpha_beta_cache_entry->ABCE_slots[jslot].ABCS_age = age;
      }
    }

    // replace smallest depth and oldest entry

    islot = 0;

    for (int jslot = 1; jslot < NABCS_SLOTS; jslot++)
      if (rank[jslot] < rank[islot])
        islot = jslot;
  }
#else
  if (islot == NABCS_SLOTS)
    islot = NABCS_SLOTS - 1;

  while (islot > 0)
  {
    with_alpha_beta_cache_entry->ABCE_slots[islot] =
      with_alpha_beta_cache_entry->ABCE_slots[islot - 1];

    --islot;
  }
#endif

  ui64_t abcs_crc32 = abcs2crc32(arg_alpha_beta_cache_slot->ABCS_key,
                                 arg_alpha_beta_cache_slot->ABCS_data,
                                 arg_alpha_beta_cache_slot->ABCS_scores);

  arg_alpha_beta_cache_slot->ABCS_crc32 = abcs_crc32;

  with_alpha_beta_cache_entry->ABCE_slots[islot] = *arg_alpha_beta_cache_slot;

#ifdef DEBUG
  with_alpha_beta_cache_entry->ABCE_slots[islot].ABCS_debug =
    arg_with_search->S_board.B_key;
#endif

  END_BLOCK
}

local inline ui64_t return_fmix64(ui64_t arg_key)
{
  arg_key ^= arg_key >> 33;
  arg_key *= 0xff51afd7ed558ccdULL;
  arg_key ^= arg_key >> 33;
  arg_key *= 0xc4ceb9fe1a85ec53ULL;
  arg_key ^= arg_key >> 33;

  return (arg_key);
}

local int probe_pv(pv_entries_t *arg_pv, search_t *arg_with_search)
{
  int result = INVALID;

  ui64_t key = arg_with_search->S_board.B_key;

  ui32_t fmix32 = (ui32_t)return_fmix64(key);

  int ientry = fmix32 & (NPV_ENTRIES - 1);

  ui64_t *pve_slots = arg_pv->PVE_entries[ientry].PVE_slots;

  for (int islot = 0; islot < NPV_SLOTS; islot++)
  {
    if ((pve_slots[islot] & ~PV_SLOT_MOVES_MASK) == (key & ~PV_SLOT_MOVES_MASK))
    {
      if (IS_WHITE(arg_with_search->S_board.B_colour2move))
        result = pve_slots[islot] & PV_SLOT_WHITE_MOVE_MASK;
      else
        result = (pve_slots[islot] & PV_SLOT_BLACK_MOVE_MASK) >> 8;

      if (result == PV_SLOT_WHITE_MOVE_MASK)
        result = INVALID;

      break;
    }
  }
  return (result);
}

local void
update_pv(pv_entries_t *arg_pv, search_t *arg_with_search, int arg_imove)
{
  ui64_t key = arg_with_search->S_board.B_key;

  ui32_t fmix32 = (ui32_t)return_fmix64(key);

  int ientry = fmix32 & (NPV_ENTRIES - 1);

  ui64_t *pve_slots = arg_pv->PVE_entries[ientry].PVE_slots;

  int islot;

  for (islot = 0; islot < NPV_SLOTS; islot++)
    if ((pve_slots[islot] & ~PV_SLOT_MOVES_MASK) == (key & ~PV_SLOT_MOVES_MASK))
      break;

  if (islot == NPV_SLOTS)
  {
    for (islot = 0; islot < NPV_SLOTS; islot++)
      if (pve_slots[islot] == PV_SLOT_MOVES_MASK)
        break;
  }

  if (islot == NPV_SLOTS)
  {
    islot = (fmix32 >> 30) & (NPV_SLOTS - 1);

    pve_slots[islot] = key | PV_SLOT_MOVES_MASK;
  }

  if (IS_WHITE(arg_with_search->S_board.B_colour2move))
    pve_slots[islot] =
      (pve_slots[islot] & ~PV_SLOT_WHITE_MOVE_MASK) | (ui64_t)arg_imove;
  else
    pve_slots[islot] =
      (pve_slots[islot] & ~PV_SLOT_BLACK_MOVE_MASK) | ((ui64_t)arg_imove) << 8;
}

local void
endgame_pv(search_t *arg_with_search, int arg_mate, bstring arg_bpv_string)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(arg_with_search->S_board), &moves_list);

  if (moves_list.ML_nmoves == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      HARDBUG(bcatcstr(arg_bpv_string, " #") == BSTR_ERR)
    }

    return;
  }
  if (arg_mate == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      HARDBUG(bcatcstr(arg_bpv_string, " mate==0?") == BSTR_ERR)
    }

    return;
  }

  int ibest_move = INVALID;
  int best_mate = ENDGAME_UNKNOWN;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    do_move(&(arg_with_search->S_board), imove, &moves_list);

    int your_mate = read_endgame(arg_with_search, NULL);

    undo_move(&(arg_with_search->S_board), imove, &moves_list);

    // may be this endgame is not loaded
    if (your_mate == ENDGAME_UNKNOWN)
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
      }
      else
      {
        HARDBUG(bcatcstr(arg_bpv_string, " not loaded") == BSTR_ERR)
      }

      return;
    }

    if (your_mate == INVALID)
    {
      if (best_mate == ENDGAME_UNKNOWN)
      {
        best_mate = INVALID;
        ibest_move = imove;
      }
      else if (best_mate <= 0)
      {
        best_mate = INVALID;
        ibest_move = imove;
      }
    }
    else //(your_mate != INVALID)
    {
      int temp_mate;

      if (your_mate > 0)
        temp_mate = -your_mate - 1;
      else
        temp_mate = -your_mate + 1;
      if (best_mate == ENDGAME_UNKNOWN)
      {
        best_mate = temp_mate;
        ibest_move = imove;
      }
      else if (best_mate == INVALID)
      {
        if (temp_mate > 0)
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
      }
      else if (best_mate > 0)
      {
        if ((temp_mate > 0) and (temp_mate < best_mate))
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
      }
      else //(best_mate <= 0)
      {
        if (temp_mate > 0)
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
        else if (temp_mate < best_mate)
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
      }
    }
  }
  HARDBUG(ibest_move == INVALID)

  HARDBUG(best_mate == ENDGAME_UNKNOWN)

  if (best_mate != arg_mate)
  {
    print_board(&(arg_with_search->S_board));

    my_printf(arg_with_search->S_my_printf,
              "best_mate=%d mate=%d\n",
              best_mate,
              arg_mate);

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      do_move(&(arg_with_search->S_board), imove, &moves_list);

      int your_mate = read_endgame(arg_with_search, NULL);

      undo_move(&(arg_with_search->S_board), imove, &moves_list);

      BSTRING(bmove_string)

      move2bstring(&moves_list, imove, bmove_string);

      my_printf(arg_with_search->S_my_printf,
                "move=%s your_mate=%d\n",
                bdata(bmove_string),
                your_mate);

      BDESTROY(bmove_string)
    }
    my_printf(arg_with_search->S_my_printf, "$");

    FATAL("best_mate != mate", EXIT_FAILURE)
  }

  HARDBUG(bcatcstr(arg_bpv_string, " ") == BSTR_ERR)

  BSTRING(bmove_string)

  move2bstring(&moves_list, ibest_move, bmove_string);

  bconcat(arg_bpv_string, bmove_string);

  BDESTROY(bmove_string)

  do_move(&(arg_with_search->S_board), ibest_move, &moves_list);

  int temp_mate;

  if (arg_mate > 0)
    temp_mate = -arg_mate + 1;
  else
    temp_mate = -arg_mate - 1;

  endgame_pv(arg_with_search, temp_mate, arg_bpv_string);

  undo_move(&(arg_with_search->S_board), ibest_move, &moves_list);
}

local void inline flip_score(score_t *arg_score)
{
  if (!IS_SCORE_NAN(*arg_score))
    *arg_score = -*arg_score;
}

local void quiescence(search_t *arg_with_search,
                      score_t arg_my_alpha,
                      score_t arg_my_beta,
                      ui32_t arg_node_type,
                      moves_list_t *arg_moves_list,
                      int arg_all_moves,
                      ui64_t arg_history,
                      score_t *arg_best_score,
                      pv_entries_t *arg_best_pv)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and
          ((arg_my_beta - arg_my_alpha) != 1))

  if ((arg_with_search->S_board.B_inode -
       arg_with_search->S_board.B_root_inode) >= 128)
  {
    my_printf(arg_with_search->S_my_printf,
              "quiescence inode=%d\n",
              arg_with_search->S_board.B_inode -
                arg_with_search->S_board.B_root_inode);
    print_board(&(arg_with_search->S_board));
    my_printf(arg_with_search->S_my_printf,
              "my_alpha=" PRINTF_SCORE_T " my_beta=" PRINTF_SCORE_T
              " node_type=%d\n",
              arg_my_alpha,
              arg_my_beta,
              arg_node_type);
  }

  *arg_best_score = SCORE(SCORE_MINUS_INFINITY);

  ++(arg_with_search->S_total_nodes);

  ++(arg_with_search->S_total_quiescence_nodes);

  if ((arg_with_search->S_total_quiescence_nodes % UPDATE_NODES) == 0)
  {
    // only thread[0] should check the time limit

    if (arg_with_search->S_thread == thread_alpha_beta_master)
    {
      // Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      // struct timespec tv;

      // HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      // check for input

      if (poll_queue(return_thread_queue(arg_with_search->S_thread)) != INVALID)
      {
        my_printf(arg_with_search->S_my_printf, "got message\n");

        *arg_best_score = SCORE_NOT_A_NUMBER;

        goto label_return;
      }

      if (return_my_timer(&(arg_with_search->S_timer), FALSE) >=
          options.time_limit)
      {
        if ((arg_with_search->S_best_score < 0) and
            ((arg_with_search->S_best_score - arg_with_search->S_root_score) <
             SCORE(-SCORE_MAN / 4)))
        {
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(arg_with_search->S_my_printf,
                        "problems.. itrouble=%d time_trouble=%.2f\n",
                        itrouble,
                        options.time_trouble[itrouble]);

              options.time_limit += options.time_trouble[itrouble];

              options.time_trouble[itrouble] = 0.0;

              break;
            }
          }
        }
      }
      // check time_limit again
      if (return_my_timer(&(arg_with_search->S_timer), FALSE) >=
          options.time_limit)
      {
        *arg_best_score = SCORE_NOT_A_NUMBER;

        goto label_return;
      }
    }
    else
    {
      HARDBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(arg_with_search->S_thread)) != INVALID)
      {
        *arg_best_score = SCORE_NOT_A_NUMBER;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(arg_with_search->S_board), FALSE))
  {
    *arg_best_score = SCORE(0);

    goto label_return;
  }

  // check endgame

  score_t endgame_score = return_endgame_score(arg_with_search, TRUE);

  if (!IS_SCORE_NAN(endgame_score))
  {
    *arg_best_score = endgame_score;

    goto label_return;
  }

  moves_list_t moves_list_temp;

  if (arg_moves_list == NULL)
  {
    construct_moves_list(&moves_list_temp);

    gen_moves(&(arg_with_search->S_board), &moves_list_temp);

    arg_moves_list = &moves_list_temp;
  }

  if (arg_moves_list->ML_nmoves == 0)
  {
    *arg_best_score =
      SCORE(SCORE_LOST_ABSOLUTE + arg_with_search->S_board.B_inode);

    goto label_return;
  }

  arg_history = (arg_history << 4) | (arg_moves_list->ML_ncaptx & 0xF) |
                0xF000000000000000ULL;

  if ((arg_moves_list->ML_nmoves == 1) and
      !move_repetition(&(arg_with_search->S_board)))
  {
    ++(arg_with_search->S_total_quiescence_1move);

    do_move(&(arg_with_search->S_board), 0, arg_moves_list);

    quiescence(arg_with_search,
               -arg_my_beta,
               -arg_my_alpha,
               arg_node_type,
               NULL,
               FALSE,
               arg_history,
               arg_best_score,
               arg_best_pv);

    flip_score(arg_best_score);

    undo_move(&(arg_with_search->S_board), 0, arg_moves_list);

    goto label_return;
  }

  int nextend = arg_moves_list->ML_nmoves;

  if (!arg_all_moves)
  {
    if (arg_moves_list->ML_ncaptx > 0)
    {
      ++(arg_with_search->S_total_quiescence_all_moves_captures_only);

      arg_all_moves = TRUE;
    }
    else if ((arg_moves_list->ML_nmoves <= options.singular_extension_moves) and
             !move_repetition(&(arg_with_search->S_board)))
    {
      ++(arg_with_search->S_total_quiescence_all_moves_le2_moves);

      arg_all_moves = TRUE;
    }
    else if ((options.quiescence_combination_length > 0) and
             (return_combination_length(arg_history) >=
              options.quiescence_combination_length))
    {
      ++(arg_with_search->S_total_quiescence_all_moves_combination);

      arg_all_moves = TRUE;
    }
    else
    {
      nextend = 0;

      for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
      {
        if (MOVE_EXTEND_IN_QUIESCENCE(arg_moves_list->ML_move_flags[imove]))
          nextend++;
      }

      if (nextend == arg_moves_list->ML_nmoves)
      {
        ++(arg_with_search->S_total_quiescence_all_moves_extended);

        arg_all_moves = TRUE;
      }
    }
  }

  if (!arg_all_moves and IS_PV(arg_node_type) and !IS_SCORE_LOST(arg_my_alpha))
  {
    if ((options.quiescence_combination_length > 0) and
        (return_combination_length(arg_history) >=
         options.quiescence_combination_length))
    {
      arg_with_search->S_total_quiescence_extension_searches_combination++;

      nextend = arg_moves_list->ML_nmoves;

      arg_all_moves = TRUE;
    }
  }

  if ((nextend == 0) or
      (!arg_all_moves and (options.quiescence_evaluation_policy == 0)))
  {
    ++(arg_with_search->S_total_evaluations);

    *arg_best_score = return_score(arg_with_search);

    if ((nextend == 0) or (*arg_best_score >= arg_my_beta))
      goto label_return;
  }

  HARDBUG(nextend == 0)

  int moves_weight[NMOVES_MAX];

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    moves_weight[imove] = arg_moves_list->ML_move_weights[imove];

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = 0;

    for (int kmove = 1; kmove < arg_moves_list->ML_nmoves; kmove++)
    {
      if (moves_weight[kmove] == L_MIN)
        continue;

      if (moves_weight[kmove] > moves_weight[jmove])
        jmove = kmove;
    }

    HARDBUG(moves_weight[jmove] == L_MIN)

    moves_weight[jmove] = L_MIN;

    if (!arg_all_moves)
    {
      if (!MOVE_EXTEND_IN_QUIESCENCE(arg_moves_list->ML_move_flags[jmove]))
        continue;
    }

    score_t temp_alpha;

    if (*arg_best_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = *arg_best_score;

    score_t temp_score = SCORE(SCORE_MINUS_INFINITY);

    pv_entries_t temp_pv = *arg_best_pv;

    if (IS_MINIMAL_WINDOW(arg_node_type))
    {
      do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

      quiescence(arg_with_search,
                 -arg_my_beta,
                 -temp_alpha,
                 arg_node_type,
                 NULL,
                 FALSE,
                 arg_history,
                 &temp_score,
                 &temp_pv);

      flip_score(&temp_score);

      undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);
    }
    else
    {
      if (imove == 0)
      {
        do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

        quiescence(arg_with_search,
                   -arg_my_beta,
                   -temp_alpha,
                   arg_node_type,
                   NULL,
                   FALSE,
                   arg_history,
                   &temp_score,
                   &temp_pv);

        flip_score(&temp_score);

        undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);
      }
      else
      {
        score_t temp_beta = temp_alpha + SCORE(1);

        do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

        quiescence(arg_with_search,
                   -temp_beta,
                   -temp_alpha,
                   MINIMAL_WINDOW_BIT,
                   NULL,
                   FALSE,
                   arg_history,
                   &temp_score,
                   &temp_pv);

        flip_score(&temp_score);

        if (!IS_SCORE_NAN(temp_score) and (temp_score >= temp_beta) and
            (temp_score < arg_my_beta))
        {
          quiescence(arg_with_search,
                     -arg_my_beta,
                     -temp_alpha,
                     arg_node_type,
                     NULL,
                     FALSE,
                     arg_history,
                     &temp_score,
                     &temp_pv);

          flip_score(&temp_score);
        }

        undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);
      }
    }

    if (IS_SCORE_NAN(temp_score))
    {
      *arg_best_score = SCORE_NOT_A_NUMBER;

      goto label_return;
    }

    HARDBUG(temp_score == SCORE(SCORE_MINUS_INFINITY))

    if (temp_score > *arg_best_score)
    {
      *arg_best_score = temp_score;

      update_pv(&temp_pv, arg_with_search, jmove);

      *arg_best_pv = temp_pv;
    }

    if (*arg_best_score >= arg_my_beta)
      break;
  } // imove

  HARDBUG(*arg_best_score == SCORE(SCORE_MINUS_INFINITY))

  if ((options.quiescence_evaluation_policy > 0) and
      (*arg_best_score < arg_my_beta))
  {
    ++(arg_with_search->S_total_evaluations);

    score_t temp_score = return_score(arg_with_search);

    if (temp_score > *arg_best_score)
      *arg_best_score = temp_score;
  }

label_return:

  if (!IS_SCORE_NAN(*arg_best_score) and (*arg_best_score >= arg_my_beta))
    *arg_best_score = arg_my_beta;

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void construct_search_result(search_result_t *arg_with_search_result,
                                   score_t arg_raw_score,
                                   score_t arg_score,
                                   ui32_t arg_flags)
{
  arg_with_search_result->SR_raw_score = arg_raw_score;
  arg_with_search_result->SR_score = arg_score;
  arg_with_search_result->SR_flags = arg_flags;
  arg_with_search_result->SR_depth = INVALID;
}

local void flip_search_result(search_result_t *arg_search_result)
{
  if (!IS_SCORE_NAN(arg_search_result->SR_score))
  {
    arg_search_result->SR_raw_score = -arg_search_result->SR_raw_score;
    arg_search_result->SR_score = -arg_search_result->SR_score;

    if (IS_SR_GE_BETA(arg_search_result->SR_flags))
    {
      arg_search_result->SR_flags &= ~SR_GE_BETA_BIT;
      arg_search_result->SR_flags |= SR_LE_ALPHA_BIT;
    }
    else if (IS_SR_LE_ALPHA(arg_search_result->SR_flags))
    {
      arg_search_result->SR_flags &= ~SR_LE_ALPHA_BIT;
      arg_search_result->SR_flags |= SR_GE_BETA_BIT;
    }
    else
    {
      HARDBUG(!IS_SR_TRUE_SCORE(arg_search_result->SR_flags))
    }

    if (IS_SR_WON(arg_search_result->SR_flags))
    {
      arg_search_result->SR_flags &= ~SR_WON_BIT;
      arg_search_result->SR_flags |= SR_LOST_BIT;
    }
    else if (IS_SR_LOST(arg_search_result->SR_flags))
    {
      arg_search_result->SR_flags &= ~SR_LOST_BIT;
      arg_search_result->SR_flags |= SR_WON_BIT;
    }
  }
}

local int compare_search_results(search_result_t *a,
                                 search_result_operator_enum op,
                                 search_result_t *b)
{
  int result = INVALID;

  if (op == SR_OPERATOR_LT)
    result = a->SR_score < b->SR_score;
  else if (op == SR_OPERATOR_LE)
    result = a->SR_score <= b->SR_score;
  else if (op == SR_OPERATOR_EQ)
    result = a->SR_score == b->SR_score;
  else if (op == SR_OPERATOR_GE)
    result = a->SR_score >= b->SR_score;
  else if (op == SR_OPERATOR_GT)
    result = a->SR_score > b->SR_score;
  else
    FATAL("unknown op", EXIT_FAILURE)

  return (result);
}

local void assign_search_result_b2a(search_result_t *a, search_result_t *b)
{
  a->SR_raw_score = b->SR_raw_score;
  a->SR_score = b->SR_score;
  a->SR_flags = b->SR_flags;

  // HARDBUG((a->SR_score != 0) and IS_SR_DRAW_BY_REP(a->SR_flags))
}

local void trace_ab_entry(search_t *arg_with_search,
                          char *file,
                          const char *func,
                          int line,
                          score_t arg_my_alpha,
                          score_t arg_my_beta,
                          ui32_t arg_node_type,
                          int arg_my_depth)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  char *colour2move;

  if (IS_WHITE(arg_with_search->S_board.B_colour2move))
    colour2move = "WTM";
  else
    colour2move = "BTM";

  PRINTF("(%ld,%s %s)ab_entry:::%s::%s:%d"
         " key=%llx"
         " arg_my_alpha=" PRINTF_SCORE_T " arg_my_beta=" PRINTF_SCORE_T
         " arg_node_type=%d"
         " arg_my_depth=%d\n",
         ++trace_line,
         bdata(trace_bpv_string),
         colour2move,
         file,
         func,
         line,
         arg_with_search->S_board.B_key,
         arg_my_alpha,
         arg_my_beta,
         arg_node_type,
         arg_my_depth);
}

local void trace_ab_cache(search_t *arg_with_search,
                          char *file,
                          const char *func,
                          int line,
                          char *arg_info,
                          alpha_beta_cache_slot_t *arg_cache_slot)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  moves_list_t moves_list;

  gen_moves(&(arg_with_search->S_board), &moves_list);

  BSTRING(bmove_string)

  score_t ABCS_raw_score = arg_cache_slot->ABCS_raw_score;
  score_t ABCS_score = arg_cache_slot->ABCS_score;
  int ABCS_depth = arg_cache_slot->ABCS_depth;
  ui32_t ABCS_flags = arg_cache_slot->ABCS_flags;
  int ABCS_move = arg_cache_slot->ABCS_moves[0];

  if (ABCS_move == INVALID)
    HARDBUG(bassigncstr(bmove_string, "NULL") == BSTR_ERR)
  else
  {
    HARDBUG(ABCS_move >= moves_list.ML_nmoves)

    move2bstring(&moves_list, ABCS_move, bmove_string);
  }

  BSTRING(bABCS_flags)

  binary(bABCS_flags, ABCS_flags, 8);

  PRINTF("(%ld,%s)ab_cache:::%s::%s:%d"
         " key=%llx arg_info=%s"
         " ABCS_raw_score=" PRINTF_SCORE_T " ABCS_score=" PRINTF_SCORE_T
         " ABCS_depth=%d"
         " ABCS_flags=%s"
         " ABCS_move=%s\n",
         ++trace_line,
         bdata(trace_bpv_string),
         file,
         func,
         line,
         arg_with_search->S_board.B_key,
         arg_info,
         ABCS_raw_score,
         ABCS_score,
         ABCS_depth,
         bdata(bABCS_flags),
         bdata(bmove_string));

  BDESTROY(bABCS_flags)

  BDESTROY(bmove_string)
}

local void trace_ab_window(ui64_t arg_trace_key,
                           char *file,
                           const char *func,
                           int line,
                           char *arg_info,
                           int arg_alpha,
                           int arg_beta,
                           moves_list_t *arg_moves_list,
                           int arg_jmove)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  BSTRING(bmove_string)

  if (arg_jmove == INVALID)
    bassigncstr(bmove_string, "NULL");
  else
  {
    HARDBUG(arg_jmove >= arg_moves_list->ML_nmoves)

    move2bstring(arg_moves_list, arg_jmove, bmove_string);
  }

  PRINTF("(%ld,%s)ab_window:::%s::%s:%d"
         " arg_trace_key=%llx arg_info=%s arg_alpha=%d arg_beta=%d"
         " arg_jmove=%s\n",
         ++trace_line,
         bdata(trace_bpv_string),
         file,
         func,
         line,
         arg_trace_key,
         arg_info,
         arg_alpha,
         arg_beta,
         bdata(bmove_string));

  BDESTROY(bmove_string)
}

local void trace_indent(moves_list_t *arg_moves_list, int arg_jmove)
{
  if (trace_bpv_string == NULL)
  {
    HARDBUG((trace_bpv_string = bfromcstr("")) == NULL)
  }

  BSTRING(bmove_string)

  HARDBUG(arg_jmove == INVALID)
  HARDBUG(arg_jmove >= arg_moves_list->ML_nmoves)

  move2bstring(arg_moves_list, arg_jmove, bmove_string);

  bformata(trace_bpv_string, " %s", bdata(bmove_string));

  BDESTROY(bmove_string)

  ++trace_depth;
}

local void trace_dedent(void)
{
  --trace_depth;

  HARDBUG(trace_depth < 0)
}

local void trace_ab_result(ui64_t arg_trace_key,
                           char *file,
                           const char *func,
                           int line,
                           char *arg_info,
                           score_t arg_alpha,
                           score_t arg_beta,
                           moves_list_t *arg_moves_list,
                           int arg_jmove,
                           search_result_t *arg_result)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  BSTRING(bmove_string)

  if (arg_jmove == INVALID)
    bassigncstr(bmove_string, "NULL");
  else
  {
    HARDBUG(arg_jmove >= arg_moves_list->ML_nmoves)

    move2bstring(arg_moves_list, arg_jmove, bmove_string);
  }

  BSTRING(bSR_flags)

  binary(bSR_flags, arg_result->SR_flags, 8);

  PRINTF(
    "(%ld,%s)ab_window:::%s::%s:%d"
    " arg_trace_key=%llx"
    " arg_info=%s"
    " arg_alpha=" PRINTF_SCORE_T " arg_beta=" PRINTF_SCORE_T " arg_jmove=%s"
    " arg_result->SR_raw_score=" PRINTF_SCORE_T
    " arg_search_result->SR_score=" PRINTF_SCORE_T " arg_result->SR_flags=%s"
    " arg_result->SR_depth=%d\n",
    ++trace_line,
    bdata(trace_bpv_string),
    file,
    func,
    line,
    arg_trace_key,
    arg_info,
    arg_alpha,
    arg_beta,
    bdata(bmove_string),
    arg_result->SR_raw_score,
    arg_result->SR_score,
    bdata(bSR_flags),
    arg_result->SR_depth);

  BDESTROY(bSR_flags)

  BDESTROY(bmove_string)
}

local void trace_ab_exit(search_t *arg_with_search,
                         char *file,
                         const char *func,
                         int line,
                         score_t arg_my_alpha,
                         score_t arg_my_beta,
                         ui32_t arg_node_type,
                         int arg_my_depth,
                         search_result_t *arg_best_result)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  char *colour2move;

  if (IS_WHITE(arg_with_search->S_board.B_colour2move))
    colour2move = "WTM";
  else
    colour2move = "BTM";

  moves_list_t moves_list;

  gen_moves(&(arg_with_search->S_board), &moves_list);

  BSTRING(bmove_string)

  int best_move = probe_pv(&(arg_best_result->SR_pv), arg_with_search);

  if (best_move == INVALID)
    HARDBUG(bassigncstr(bmove_string, "NULL") == BSTR_ERR)
  else
  {
    HARDBUG(best_move >= moves_list.ML_nmoves)

    move2bstring(&moves_list, best_move, bmove_string);
  }

  BSTRING(bSR_flags)

  binary(bSR_flags, arg_best_result->SR_flags, 8);

  PRINTF("(%ld,%s %s %s)ab_exit:::%s::%s:%d"
         " key=%llx"
         " arg_my_alpha=" PRINTF_SCORE_T " arg_my_beta=" PRINTF_SCORE_T
         " arg_node_type=%d arg_my_depth=%d"
         " arg_best_result->SR_raw_score=" PRINTF_SCORE_T
         " arg_best_result->SR_score=" PRINTF_SCORE_T
         " arg_best_result->SR_flags=%s"
         " arg_best_result->SR_pv[0]=%s"
         " arg_best_result->SR_depth=%d\n",
         ++trace_line,
         bdata(trace_bpv_string),
         colour2move,
         bdata(bmove_string),
         file,
         func,
         line,
         arg_with_search->S_board.B_key,
         arg_my_alpha,
         arg_my_beta,
         arg_node_type,
         arg_my_depth,
         arg_best_result->SR_raw_score,
         arg_best_result->SR_score,
         bdata(bSR_flags),
         bdata(bmove_string),
         arg_best_result->SR_depth);

  BDESTROY(bSR_flags)

  BDESTROY(bmove_string)
}

local void inline fail_hard(score_t arg_my_alpha,
                            score_t arg_my_beta,
                            search_result_t *arg_search_result)
{
  if (IS_SCORE_NAN(arg_search_result->SR_score))
    return;

  if (arg_search_result->SR_score >= arg_my_beta)
  {
    arg_search_result->SR_score = arg_my_beta;

    arg_search_result->SR_flags |= SR_GE_BETA_BIT;
  }
  else if (arg_search_result->SR_score <= arg_my_alpha)
  {
    arg_search_result->SR_flags |= SR_LE_ALPHA_BIT;
  }
  else
  {
    arg_search_result->SR_flags |= SR_TRUE_SCORE_BIT;
  }
}

local void inline check_search_result(score_t arg_my_alpha,
                                      score_t arg_my_beta,
                                      search_result_t *arg_search_result)
{
  int err = 0;

  if (IS_SR_GE_BETA(arg_search_result->SR_flags))
  {
    if (arg_search_result->SR_score < arg_my_beta)
      err = 1;
    else if (arg_search_result->SR_raw_score < arg_search_result->SR_score)
      err = 2;
  }

  // not else if because arg_search_result can be NOT_A_NUMBER

  if (IS_SR_LE_ALPHA(arg_search_result->SR_flags))
  {
    if (arg_search_result->SR_score > arg_my_alpha)
      err = 3;
    else if (arg_search_result->SR_raw_score > arg_search_result->SR_score)
      err = 4;
  }

  if (IS_SR_TRUE_SCORE(arg_search_result->SR_flags))
  {
    if ((arg_search_result->SR_score <= arg_my_alpha) or
        (arg_search_result->SR_score >= arg_my_beta))
      err = 5;
    else if (arg_search_result->SR_raw_score != arg_search_result->SR_score)
      err = 6;
  }

  if (err > 0)
  {
    BSTRING(bSR_flags)

    binary(bSR_flags, arg_search_result->SR_flags, 8);

    PRINTF("err=%d"
           " arg_my_alpha=" PRINTF_SCORE_T " arg_my_beta=" PRINTF_SCORE_T
           " arg_search_result->SR_raw_score=" PRINTF_SCORE_T
           " arg_search_result->SR_score=" PRINTF_SCORE_T
           " arg_search_result->SR_flags=%s\n",
           err,
           arg_my_alpha,
           arg_my_beta,
           arg_search_result->SR_raw_score,
           arg_search_result->SR_score,
           bdata(bSR_flags));

    FATAL("eh?", EXIT_FAILURE)
  }
}

local void alpha_beta(search_t *,
                      score_t,
                      score_t,
                      int,
                      ui32_t,
                      int,
                      ui32_t,
                      moves_list_t *,
                      int[NMOVES_MAX],
                      int,
                      int,
                      ui64_t,
                      search_result_t *);

local void alpha_beta_captures(search_t *arg_with_search,
                               score_t arg_my_alpha,
                               score_t arg_my_beta,
                               int arg_my_depth,
                               ui32_t arg_node_type,
                               int arg_reduction_depth_root,
                               ui32_t arg_my_tweaks,
                               moves_list_t *arg_moves_list,
                               int arg_sort[NMOVES_MAX],
                               int arg_nsingular_moves,
                               int arg_singular_extension,
                               ui64_t arg_history,
                               search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(arg_with_search,
                 arg_my_alpha,
                 arg_my_beta,
                 arg_node_type,
                 arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 0)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and
          ((arg_my_beta - arg_my_alpha) != 1))

  HARDBUG((arg_nsingular_moves == INVALID) and
          (arg_singular_extension != INVALID))

  HARDBUG(arg_history == 0)

  ++(arg_with_search->S_total_nodes);

  ++(arg_with_search->S_total_capture_nodes);

  HARDBUG(arg_moves_list->ML_nmoves == 0)

  construct_search_result(arg_search_result,
                          SCORE(SCORE_MINUS_INFINITY),
                          SCORE(SCORE_MINUS_INFINITY),
                          SR_NULL);

  ui32_t your_tweaks = 0;

  int nproven = 0;

  search_result_t temp_result;

  temp_result.SR_pv = arg_search_result->SR_pv;

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = arg_sort[imove];

    int singular_extension = 0;

    if (imove < arg_nsingular_moves)
    {
      if (arg_singular_extension == INVALID)
        continue;

      singular_extension = arg_singular_extension;

      if (singular_extension > 0)
        ++(arg_with_search->S_total_singular_extensions);
    }

    score_t temp_alpha;

    if (arg_search_result->SR_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = arg_search_result->SR_score;

    score_t temp_beta = arg_my_beta;

    ui32_t node_type = arg_node_type;

    if (IS_PV(arg_node_type) and (imove > 0))
    {
      temp_beta = temp_alpha + SCORE(1);

      node_type = MINIMAL_WINDOW_BIT;
    }

    construct_search_result(&temp_result,
                            SCORE(SCORE_MINUS_INFINITY),
                            SCORE(SCORE_MINUS_INFINITY),
                            SR_NULL);

    TRACE_KEY(arg_with_search)

    do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    for (int ipass = 1; ipass <= 2; ++ipass)
    {
      TRACE_AB_WINDOW(trace_key,
                      "window search",
                      temp_alpha,
                      temp_beta,
                      arg_moves_list,
                      jmove)

      TRACE_INDENT(arg_moves_list, jmove)

      alpha_beta(arg_with_search,
                 -temp_beta,
                 -temp_alpha,
                 arg_my_depth + singular_extension,
                 node_type,
                 arg_reduction_depth_root,
                 your_tweaks |
                   (singular_extension ? TWEAK_MOVE_EXTENDED_BIT : 0U),
                 NULL,
                 NULL,
                 INVALID,
                 INVALID,
                 arg_history,
                 &temp_result);

      TRACE_DEDENT

      flip_search_result(&temp_result);

      TRACE_AB_RESULT(trace_key,
                      "result window search",
                      temp_alpha,
                      temp_beta,
                      arg_moves_list,
                      jmove,
                      &temp_result);

      if (IS_SCORE_NAN(temp_result.SR_score))
      {
        undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

        construct_search_result(arg_search_result,
                                SCORE_NOT_A_NUMBER,
                                SCORE_NOT_A_NUMBER,
                                SR_NULL);

        goto label_return;
      }

      if (ipass == 2)
        break;

      if (IS_MINIMAL_WINDOW(arg_node_type) or
          (temp_result.SR_score < temp_beta) or
          (temp_result.SR_score >= arg_my_beta))
        break;

      temp_beta = arg_my_beta;

      node_type = PV_BIT;
    }

    undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    HARDBUG(temp_result.SR_score == SCORE(SCORE_MINUS_INFINITY))

    if (compare_search_results(&temp_result, SR_OPERATOR_GT, arg_search_result))
    {
      assign_search_result_b2a(arg_search_result, &temp_result);

      nproven = 1;

      update_pv(&(temp_result.SR_pv), arg_with_search, jmove);

      arg_search_result->SR_pv = temp_result.SR_pv;

      if ((temp_result.SR_depth == DEPTH_MAX) or (arg_my_depth == DEPTH_MAX))
        arg_search_result->SR_depth = temp_result.SR_depth;
      else
        arg_search_result->SR_depth = arg_my_depth + singular_extension;

      for (int kmove = imove; kmove > 0; kmove--)
        arg_sort[kmove] = arg_sort[kmove - 1];

      arg_sort[0] = jmove;

      TRACE_AB_RESULT(trace_key,
                      "result best move",
                      arg_my_alpha,
                      arg_my_beta,
                      arg_moves_list,
                      jmove,
                      arg_search_result);

      if (arg_search_result->SR_score >= arg_my_beta)
        break;
    }
    else if ((temp_result.SR_depth == DEPTH_MAX) and
             compare_search_results(&temp_result,
                                    SR_OPERATOR_EQ,
                                    arg_search_result))
    {
      ++nproven;
    }
  }

  HARDBUG(arg_search_result->SR_depth == INVALID)

  if ((nproven < arg_moves_list->ML_nmoves) and
      (arg_search_result->SR_depth == DEPTH_MAX) and
      !IS_SR_LOST(arg_search_result->SR_flags) and
      !IS_SR_WON(arg_search_result->SR_flags))
  {
    // print_board(&(arg_with_search->S_board));
    // PRINTF("NOT PROVEN!\n");

    arg_search_result->SR_depth = arg_my_depth;
  }

label_return:

  if (FALSE and (arg_search_result->SR_depth == DEPTH_MAX))
  {
    HARDBUG(!IS_SCORE_WON(arg_search_result->SR_score) and
            !IS_SCORE_LOST(arg_search_result->SR_score) and
            (arg_search_result->SR_score != 0))
  }

  if (!IS_SCORE_NAN(arg_search_result->SR_score))
  {
    HARDBUG(arg_search_result->SR_depth < arg_my_depth)
  }

  fail_hard(arg_my_alpha, arg_my_beta, arg_search_result);

  TRACE_AB_EXIT(arg_with_search,
                arg_my_alpha,
                arg_my_beta,
                arg_node_type,
                arg_my_depth,
                arg_search_result);

  check_search_result(arg_my_alpha, arg_my_beta, arg_search_result);

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void alpha_beta_minimal(search_t *arg_with_search,
                              score_t arg_my_alpha,
                              score_t arg_my_beta,
                              int arg_my_depth,
                              ui32_t arg_node_type,
                              int arg_reduction_depth_root,
                              ui32_t arg_my_tweaks,
                              moves_list_t *arg_moves_list,
                              int arg_sort[NMOVES_MAX],
                              int arg_nsingular_moves,
                              int arg_singular_extension,
                              ui64_t arg_history,
                              search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(arg_with_search,
                 arg_my_alpha,
                 arg_my_beta,
                 arg_node_type,
                 arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 1)

  HARDBUG(arg_node_type == 0)

  HARDBUG(!IS_MINIMAL_WINDOW(arg_node_type))

  HARDBUG((arg_my_beta - arg_my_alpha) != 1)

  HARDBUG(arg_moves_list->ML_nmoves == 0)

  HARDBUG(arg_moves_list->ML_ncaptx > 0)

  HARDBUG((arg_nsingular_moves == INVALID) and
          (arg_singular_extension != INVALID))

  HARDBUG(arg_history == 0)

  construct_search_result(arg_search_result,
                          SCORE(SCORE_MINUS_INFINITY),
                          SCORE(SCORE_MINUS_INFINITY),
                          SR_NULL);

  ++(arg_with_search->S_total_nodes);

  ++(arg_with_search->S_total_minimal_window_nodes);

  int your_depth = arg_my_depth - 1;

  HARDBUG(your_depth < 0)

  int allow_reductions =
    options.use_reductions and (arg_singular_extension != 1) and
    !(arg_my_tweaks & TWEAK_SEARCH_EXTENDED_BIT) and
    !(arg_my_tweaks & TWEAK_SINGULAR_PROBE_BIT) and
    !(arg_my_tweaks & TWEAK_MOVE_NULL_BIT) and
    !(arg_my_tweaks & TWEAK_MOVE_REDUCED_BIT) and
    !(arg_my_tweaks & TWEAK_MOVE_EXTENDED_BIT) and
    (arg_moves_list->ML_ncaptx == 0) and
    (arg_moves_list->ML_nmoves > options.singular_extension_moves) and
    (your_depth >= options.reduction_depth_leaf) and
    (arg_reduction_depth_root == 0) and !IS_SCORE_LOST(arg_my_alpha) and
    !IS_SCORE_WON(arg_my_beta);

  if (allow_reductions and (options.reduction_combination_length > 0) and
      (return_combination_length(arg_history) >=
       options.reduction_combination_length))
  {
    arg_with_search->S_total_reductions_combinations++;

    allow_reductions = FALSE;
  }

  ui32_t your_tweaks = 0;

  int nmoves_searched_at_full_depth = 0;

  int reduced_depth = INVALID;

  if (allow_reductions)
  {
    reduced_depth = (your_depth * options.reduction_percentage) / 100;

    if (reduced_depth < 1)
      reduced_depth = 1;

    if (reduced_depth >= your_depth)
      allow_reductions = FALSE;
  }

  HARDBUG(allow_reductions and (reduced_depth == INVALID))

  int nproven = 0;

  search_result_t temp_result;

  temp_result.SR_pv = arg_search_result->SR_pv;

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = arg_sort[imove];

    int singular_extension = 0;

    if (imove < arg_nsingular_moves)
    {
      if (arg_singular_extension == INVALID)
        continue;

      singular_extension = arg_singular_extension;

      if (singular_extension > 0)
        ++(arg_with_search->S_total_singular_extensions);
    }

    construct_search_result(&temp_result,
                            SCORE(SCORE_MINUS_INFINITY),
                            SCORE(SCORE_MINUS_INFINITY),
                            SR_NULL);

    TRACE_KEY(arg_with_search)

    do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    int stage = STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH;

    if (!allow_reductions or
        (nmoves_searched_at_full_depth < options.reduction_full_min) or
        MOVE_DO_NOT_REDUCE(arg_moves_list->ML_move_flags[jmove]))
    {
      stage = STAGE_SEARCH_MOVE_AT_FULL_DEPTH;
    }

    if (stage == STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH)
    {
      ++(arg_with_search->S_total_reductions);

      HARDBUG(reduced_depth == INVALID)
      HARDBUG(reduced_depth < 1)
      HARDBUG(reduced_depth >= your_depth)

      score_t reduced_alpha =
        arg_search_result->SR_score - options.reduction_margin;

      score_t reduced_beta = reduced_alpha + SCORE(1);

      TRACE_AB_WINDOW(trace_key,
                      "reduced depth search",
                      reduced_alpha,
                      reduced_beta,
                      arg_moves_list,
                      jmove)

      TRACE_INDENT(arg_moves_list, jmove)

      alpha_beta(arg_with_search,
                 -reduced_beta,
                 -reduced_alpha,
                 reduced_depth,
                 arg_node_type,
                 arg_reduction_depth_root,
                 your_tweaks | TWEAK_MOVE_REDUCED_BIT,
                 NULL,
                 NULL,
                 INVALID,
                 INVALID,
                 arg_history,
                 &temp_result);

      TRACE_DEDENT

      flip_search_result(&temp_result);

      TRACE_AB_RESULT(trace_key,
                      "result reduced depth search",
                      reduced_alpha,
                      reduced_beta,
                      arg_moves_list,
                      jmove,
                      &temp_result);

      if (IS_SCORE_NAN(temp_result.SR_score))
      {
        undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

        construct_search_result(arg_search_result,
                                SCORE_NOT_A_NUMBER,
                                SCORE_NOT_A_NUMBER,
                                SR_NULL);

        goto label_return;
      }

      if (temp_result.SR_score <= reduced_alpha)
      {
        ++(arg_with_search->S_total_reductions_le_alpha);

        stage = STAGE_SEARCHED_MOVE_AT_REDUCED_DEPTH;
      }
      else
      {
        ++(arg_with_search->S_total_reductions_ge_beta);

        stage = STAGE_SEARCH_MOVE_AT_FULL_DEPTH;
      }
    } // if (stage

    if (stage == STAGE_SEARCH_MOVE_AT_FULL_DEPTH)
    {
      TRACE_AB_WINDOW(trace_key,
                      "full depth search",
                      arg_my_alpha,
                      arg_my_beta,
                      arg_moves_list,
                      jmove)

      TRACE_INDENT(arg_moves_list, jmove)

      alpha_beta(arg_with_search,
                 -arg_my_beta,
                 -arg_my_alpha,
                 your_depth + singular_extension,
                 arg_node_type,
                 arg_reduction_depth_root,
                 your_tweaks |
                   (singular_extension ? TWEAK_MOVE_EXTENDED_BIT : 0U),
                 NULL,
                 NULL,
                 INVALID,
                 INVALID,
                 arg_history,
                 &temp_result);

      TRACE_DEDENT

      flip_search_result(&temp_result);

      TRACE_AB_RESULT(trace_key,
                      "result full depth search",
                      arg_my_alpha,
                      arg_my_beta,
                      arg_moves_list,
                      jmove,
                      &temp_result);

      stage = STAGE_SEARCHED_MOVE_AT_FULL_DEPTH;

      nmoves_searched_at_full_depth++;
    }

    undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    if (IS_SCORE_NAN(temp_result.SR_score))
    {
      construct_search_result(arg_search_result,
                              SCORE_NOT_A_NUMBER,
                              SCORE_NOT_A_NUMBER,
                              SR_NULL);

      goto label_return;
    }

    HARDBUG(temp_result.SR_score == SCORE(SCORE_MINUS_INFINITY))

    if (compare_search_results(&temp_result, SR_OPERATOR_GT, arg_search_result))
    {
      HARDBUG(stage != STAGE_SEARCHED_MOVE_AT_FULL_DEPTH)

      assign_search_result_b2a(arg_search_result, &temp_result);

      nproven = 1;

      update_pv(&(temp_result.SR_pv), arg_with_search, jmove);

      arg_search_result->SR_pv = temp_result.SR_pv;

      if ((temp_result.SR_depth == DEPTH_MAX) or (arg_my_depth == DEPTH_MAX))
        arg_search_result->SR_depth = temp_result.SR_depth;
      else
        arg_search_result->SR_depth = arg_my_depth + singular_extension;

      for (int kmove = imove; kmove > 0; kmove--)
        arg_sort[kmove] = arg_sort[kmove - 1];

      arg_sort[0] = jmove;

      TRACE_AB_RESULT(trace_key,
                      "result best move",
                      arg_my_alpha,
                      arg_my_beta,
                      arg_moves_list,
                      jmove,
                      arg_search_result);

      if (arg_search_result->SR_score >= arg_my_beta)
        break;
    }
    else if ((temp_result.SR_depth == DEPTH_MAX) and
             compare_search_results(&temp_result,
                                    SR_OPERATOR_EQ,
                                    arg_search_result))
    {
      ++nproven;
    }
  } // for (int imove

  HARDBUG(arg_search_result->SR_depth == INVALID)

  if ((nproven < arg_moves_list->ML_nmoves) and
      (arg_search_result->SR_depth == DEPTH_MAX) and
      !IS_SR_LOST(arg_search_result->SR_flags) and
      !IS_SR_WON(arg_search_result->SR_flags))
  {
    // print_board(&(arg_with_search->S_board));
    // PRINTF("NOT PROVEN!\n");

    arg_search_result->SR_depth = arg_my_depth;
  }

  HARDBUG(nmoves_searched_at_full_depth == 0)

label_return:

  if (FALSE and (arg_search_result->SR_depth == DEPTH_MAX))
  {
    HARDBUG(!IS_SCORE_WON(arg_search_result->SR_score) and
            !IS_SCORE_LOST(arg_search_result->SR_score) and
            (arg_search_result->SR_score != 0))
  }

  if (!IS_SCORE_NAN(arg_search_result->SR_score))
    HARDBUG(arg_search_result->SR_depth < arg_my_depth)

  fail_hard(arg_my_alpha, arg_my_beta, arg_search_result);

  TRACE_AB_EXIT(arg_with_search,
                arg_my_alpha,
                arg_my_beta,
                arg_node_type,
                arg_my_depth,
                arg_search_result);

  check_search_result(arg_my_alpha, arg_my_beta, arg_search_result);

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void alpha_beta_pv(search_t *arg_with_search,
                         score_t arg_my_alpha,
                         score_t arg_my_beta,
                         int arg_my_depth,
                         ui32_t arg_node_type,
                         int arg_reduction_depth_root,
                         ui32_t arg_my_tweaks,
                         moves_list_t *arg_moves_list,
                         int arg_sort[NMOVES_MAX],
                         int arg_nsingular_moves,
                         int arg_singular_extension,
                         ui64_t arg_history,
                         search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(arg_with_search,
                 arg_my_alpha,
                 arg_my_beta,
                 arg_node_type,
                 arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 1)

  HARDBUG(arg_node_type == 0)

  HARDBUG((arg_nsingular_moves == INVALID) and
          (arg_singular_extension != INVALID))

  HARDBUG(!IS_PV(arg_node_type))

  HARDBUG(arg_history == 0)

  ++(arg_with_search->S_total_nodes);

  ++(arg_with_search->S_total_pv_nodes);

  construct_search_result(arg_search_result,
                          SCORE(SCORE_MINUS_INFINITY),
                          SCORE(SCORE_MINUS_INFINITY),
                          SR_NULL);

  ui32_t your_tweaks = 0;

  int your_depth = arg_my_depth - 1;

  HARDBUG(your_depth < 0)

  search_result_t temp_result;

  temp_result.SR_pv = arg_search_result->SR_pv;

  if (!(arg_my_tweaks & TWEAK_SEARCH_EXTENDED_BIT) and
      !(arg_my_tweaks & TWEAK_MOVE_EXTENDED_BIT) and
      !IS_SCORE_LOST(arg_my_alpha))
  {
    if ((options.pv_combination_length > 0) and
        (return_combination_length(arg_history) >=
         options.pv_combination_length))
    {
      arg_with_search->S_total_pv_extension_searches_combination++;

      your_depth++;

      your_tweaks |= TWEAK_MOVE_EXTENDED_BIT;
    }
  }

  int nproven = INVALID;

  nproven = 0;

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = arg_sort[imove];

    int singular_extension = 0;

    if (imove < arg_nsingular_moves)
    {
      if (arg_singular_extension == INVALID)
        continue;

      singular_extension = arg_singular_extension;

      if (singular_extension > 0)
        ++(arg_with_search->S_total_singular_extensions);
    }

    score_t temp_alpha;

    if (arg_search_result->SR_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = arg_search_result->SR_score;

    score_t temp_beta = arg_my_beta;

    ui32_t node_type = arg_node_type;

    if (IS_PV(arg_node_type) and (imove > 0))
    {
      temp_beta = temp_alpha + SCORE(1);

      node_type = MINIMAL_WINDOW_BIT;
    }

    construct_search_result(&temp_result,
                            SCORE(SCORE_MINUS_INFINITY),
                            SCORE(SCORE_MINUS_INFINITY),
                            SR_NULL);

    TRACE_KEY(arg_with_search)

    do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    for (int ipass = 1; ipass <= 2; ++ipass)
    {
      TRACE_AB_WINDOW(trace_key,
                      "window search",
                      temp_alpha,
                      temp_beta,
                      arg_moves_list,
                      jmove)

      TRACE_INDENT(arg_moves_list, jmove)

      alpha_beta(arg_with_search,
                 -temp_beta,
                 -temp_alpha,
                 your_depth + singular_extension,
                 node_type,
                 arg_reduction_depth_root,
                 your_tweaks |
                   (singular_extension ? TWEAK_MOVE_EXTENDED_BIT : 0U),
                 NULL,
                 NULL,
                 INVALID,
                 INVALID,
                 arg_history,
                 &temp_result);

      TRACE_DEDENT

      flip_search_result(&temp_result);

      TRACE_AB_RESULT(trace_key,
                      "window search",
                      temp_alpha,
                      temp_beta,
                      arg_moves_list,
                      jmove,
                      &temp_result);

      if (IS_SCORE_NAN(temp_result.SR_score))
      {
        undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

        construct_search_result(arg_search_result,
                                SCORE_NOT_A_NUMBER,
                                SCORE_NOT_A_NUMBER,
                                SR_NULL);

        goto label_return;
      }

      if (ipass == 2)
        break;

      if (IS_MINIMAL_WINDOW(arg_node_type) or
          (temp_result.SR_score < temp_beta) or
          (temp_result.SR_score >= arg_my_beta))
        break;

      temp_beta = arg_my_beta;

      node_type = PV_BIT;
    }

    undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    HARDBUG(temp_result.SR_score == SCORE(SCORE_MINUS_INFINITY))

    if (compare_search_results(&temp_result, SR_OPERATOR_GT, arg_search_result))
    {
      assign_search_result_b2a(arg_search_result, &temp_result);

      nproven = 1;

      update_pv(&(temp_result.SR_pv), arg_with_search, jmove);

      arg_search_result->SR_pv = temp_result.SR_pv;

      if ((temp_result.SR_depth == DEPTH_MAX) or (arg_my_depth == DEPTH_MAX))
        arg_search_result->SR_depth = temp_result.SR_depth;
      else
        arg_search_result->SR_depth = arg_my_depth + singular_extension;

      for (int kmove = imove; kmove > 0; kmove--)
        arg_sort[kmove] = arg_sort[kmove - 1];

      arg_sort[0] = jmove;

      TRACE_AB_RESULT(trace_key,
                      "result best move",
                      arg_my_alpha,
                      arg_my_beta,
                      arg_moves_list,
                      jmove,
                      arg_search_result);

      if (arg_search_result->SR_score >= arg_my_beta)
        goto label_break;
    }
    else if ((temp_result.SR_depth == DEPTH_MAX) and
             compare_search_results(&temp_result,
                                    SR_OPERATOR_EQ,
                                    arg_search_result))
    {
      ++nproven;
    }
  } // imove

label_break:

  HARDBUG(arg_search_result->SR_depth == INVALID)

  if ((nproven < arg_moves_list->ML_nmoves) and
      (arg_search_result->SR_depth == DEPTH_MAX) and
      !IS_SR_LOST(arg_search_result->SR_flags) and
      !IS_SR_WON(arg_search_result->SR_flags))
  {
    // print_board(&(arg_with_search->S_board));
    // PRINTF("NOT PROVEN!\n");

    arg_search_result->SR_depth = arg_my_depth;
  }

label_return:

  if (FALSE and (arg_search_result->SR_depth == DEPTH_MAX))
  {
    HARDBUG(!IS_SCORE_WON(arg_search_result->SR_score) and
            !IS_SCORE_LOST(arg_search_result->SR_score) and
            (arg_search_result->SR_score != 0))
  }

  if (!IS_SCORE_NAN(arg_search_result->SR_score))
    HARDBUG(arg_search_result->SR_depth < arg_my_depth)

  fail_hard(arg_my_alpha, arg_my_beta, arg_search_result);

  TRACE_AB_EXIT(arg_with_search,
                arg_my_alpha,
                arg_my_beta,
                arg_node_type,
                arg_my_depth,
                arg_search_result);

  check_search_result(arg_my_alpha, arg_my_beta, arg_search_result);

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void inline sort_moves_weight(int arg_nmoves,
                                    int arg_moves_weight[NMOVES_MAX],
                                    int arg_sort[NMOVES_MAX])
{
  for (int imove = 0; imove < arg_nmoves; imove++)
    arg_sort[imove] = imove;

  for (int imove = 1; imove < arg_nmoves; imove++)
  {
    int key = arg_sort[imove];

    int moves_weight = arg_moves_weight[key];

    int jmove = imove - 1;

    while ((jmove >= 0) and arg_moves_weight[arg_sort[jmove]] < moves_weight)
    {
      arg_sort[jmove + 1] = arg_sort[jmove];

      --jmove;
    }

    arg_sort[jmove + 1] = key;
  }

#ifdef DEBUG
  int frequency[NMOVES_MAX] = {0};

  for (int imove = 0; imove < arg_nmoves; imove++)
  {
    int key = arg_sort[imove];

    HARDBUG((key < 0) or (key >= arg_nmoves))

    frequency[key]++;
  }

  for (int imove = 0; imove < arg_nmoves; imove++)
    HARDBUG(frequency[arg_sort[imove]] != 1)

  for (int imove = 1; imove < arg_nmoves; imove++)
    HARDBUG(arg_moves_weight[arg_sort[imove - 1]] <
            arg_moves_weight[arg_sort[imove]])
#endif
}

local void alpha_beta(search_t *arg_with_search,
                      score_t arg_my_alpha,
                      score_t arg_my_beta,
                      int arg_my_depth,
                      ui32_t arg_node_type,
                      int arg_reduction_depth_root,
                      ui32_t arg_my_tweaks,
                      moves_list_t *arg_moves_list,
                      int arg_sort[NMOVES_MAX],
                      int arg_nsingular_moves,
                      int arg_singular_extension,
                      ui64_t arg_history,
                      search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(arg_with_search,
                 arg_my_alpha,
                 arg_my_beta,
                 arg_node_type,
                 arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 0)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and
          ((arg_my_beta - arg_my_alpha) != 1))

  if (arg_my_tweaks & TWEAK_SINGULAR_PROBE_BIT)
  {
    HARDBUG(arg_moves_list == NULL)
    HARDBUG(arg_sort == NULL)
    HARDBUG(arg_nsingular_moves == INVALID)
    HARDBUG(arg_singular_extension != INVALID)
  }
  else
  {
    HARDBUG((arg_nsingular_moves == INVALID) and
            (arg_singular_extension != INVALID))
  }

#ifdef DEBUG
  if (arg_sort != NULL)
  {
    HARDBUG(arg_moves_list == NULL)

    int frequency[NMOVES_MAX] = {0};

    for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    {
      int key = arg_sort[imove];

      HARDBUG((key < 0) or (key >= arg_moves_list->ML_nmoves))

      frequency[key]++;
    }

    for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
      HARDBUG(frequency[arg_sort[imove]] != 1)
  }
#endif

  HARDBUG(arg_history == 0)

  if (RUNNING_ON_VALGRIND)
  {
    VALGRIND_CHECK_MEM_IS_DEFINED(&(arg_search_result->SR_pv),
                                  sizeof(pv_entries_t));
  }

  colour_enum my_colour;

  if (IS_WHITE(arg_with_search->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  if (arg_with_search->S_thread == thread_alpha_beta_master)
  {
    update_bucket(&bucket_depth, arg_my_depth);
  }

  if ((arg_with_search->S_board.B_inode -
       arg_with_search->S_board.B_root_inode) >= 128)
  {
    my_printf(arg_with_search->S_my_printf,
              "alpha_beta inode=%d\n",
              arg_with_search->S_board.B_inode -
                arg_with_search->S_board.B_root_inode);
    print_board(&(arg_with_search->S_board));
    my_printf(arg_with_search->S_my_printf,
              "my_alpha=" PRINTF_SCORE_T " my_beta=" PRINTF_SCORE_T
              " my_depth=%d"
              " node_type=%d\n",
              arg_my_alpha,
              arg_my_beta,
              arg_my_depth,
              arg_node_type);
  }

  construct_search_result(arg_search_result,
                          SCORE_NOT_A_NUMBER,
                          SCORE_NOT_A_NUMBER,
                          SR_NULL);

  ++(arg_with_search->S_total_nodes);

  ++(arg_with_search->S_total_alpha_beta_nodes);

  if ((arg_with_search->S_total_alpha_beta_nodes % UPDATE_NODES) == 0)
  {
    // only thread[0] should check the time limit

    if (arg_with_search->S_thread == thread_alpha_beta_master)
    {
      // Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      // struct timespec tv;

      // HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      // check for input

      if (poll_queue(return_thread_queue(arg_with_search->S_thread)) != INVALID)
      {
        my_printf(arg_with_search->S_my_printf, "got message\n");

        construct_search_result(arg_search_result,
                                SCORE_NOT_A_NUMBER,
                                SCORE_NOT_A_NUMBER,
                                SR_NULL);

        goto label_return;
      }

      if (return_my_timer(&(arg_with_search->S_timer), FALSE) >=
          options.time_limit)
      {
        if ((arg_with_search->S_best_score < 0) and
            ((arg_with_search->S_best_score - arg_with_search->S_root_score) <
             SCORE(-SCORE_MAN / 4)))
        {
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(arg_with_search->S_my_printf,
                        "problems.. itrouble=%d time_trouble=%.2f\n",
                        itrouble,
                        options.time_trouble[itrouble]);

              options.time_limit += options.time_trouble[itrouble];

              options.time_trouble[itrouble] = 0.0;

              break;
            }
          }
        }
      }
      // check time_limit again
      if (return_my_timer(&(arg_with_search->S_timer), FALSE) >=
          options.time_limit)
      {
        construct_search_result(arg_search_result,
                                SCORE_NOT_A_NUMBER,
                                SCORE_NOT_A_NUMBER,
                                SR_NULL);

        goto label_return;
      }
    }
    else
    {
      HARDBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(arg_with_search->S_thread)) != INVALID)
      {
        construct_search_result(arg_search_result,
                                SCORE_NOT_A_NUMBER,
                                SCORE_NOT_A_NUMBER,
                                SR_NULL);

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(arg_with_search->S_board), FALSE))
  {
    construct_search_result(arg_search_result,
                            SCORE(0),
                            SCORE(0),
                            SR_DRAW_BY_REP_BIT);

    arg_search_result->SR_depth = arg_my_depth;

    goto label_return;
  }

  // check endgame

  score_t endgame_score = return_endgame_score(arg_with_search, TRUE);

  if (!IS_SCORE_NAN(endgame_score))
  {
    if (IS_SCORE_WON(endgame_score))
    {
      construct_search_result(arg_search_result,
                              endgame_score,
                              endgame_score,
                              SR_WON_BIT);
    }
    else if (IS_SCORE_LOST(endgame_score))
    {
      construct_search_result(arg_search_result,
                              endgame_score,
                              endgame_score,
                              SR_LOST_BIT);
    }
    else
    {
      construct_search_result(arg_search_result,
                              endgame_score,
                              endgame_score,
                              SR_EGTB_DRAW_BIT);
    }

    arg_search_result->SR_depth = DEPTH_MAX;

    goto label_return;
  }

  moves_list_t *moves_list = arg_moves_list;

  moves_list_t moves_list_local;

  if (moves_list == NULL)
  {
    moves_list = &moves_list_local;

    construct_moves_list(moves_list);

    gen_moves(&(arg_with_search->S_board), moves_list);
  }

  if (moves_list->ML_nmoves == 0)
  {
    construct_search_result(
      arg_search_result,
      SCORE(SCORE_LOST_ABSOLUTE + arg_with_search->S_board.B_inode),
      SCORE(SCORE_LOST_ABSOLUTE + arg_with_search->S_board.B_inode),
      SR_LOST_BIT);

    arg_search_result->SR_depth = DEPTH_MAX;

    goto label_return;
  }

  arg_history =
    (arg_history << 4) | (moves_list->ML_ncaptx & 0xF) | 0xF000000000000000ULL;

  if ((moves_list->ML_nmoves == 1) and
      !move_repetition(&(arg_with_search->S_board)))
  {
    ++(arg_with_search->S_total_alpha_beta_1move);

    do_move(&(arg_with_search->S_board), 0, moves_list);

    alpha_beta(arg_with_search,
               -arg_my_beta,
               -arg_my_alpha,
               arg_my_depth,
               arg_node_type,
               arg_reduction_depth_root,
               0,
               NULL,
               NULL,
               INVALID,
               INVALID,
               arg_history,
               arg_search_result);

    flip_search_result(arg_search_result);

    undo_move(&(arg_with_search->S_board), 0, moves_list);

    goto label_return;
  }

  int alpha_beta_cache_hit = FALSE;

  alpha_beta_cache_slot_t alpha_beta_cache_slot;

  if ((options.alpha_beta_cache_size > 0) and
      !(arg_my_tweaks & TWEAK_SINGULAR_PROBE_BIT))
  {
    alpha_beta_cache_hit = probe_alpha_beta_cache(
      arg_with_search,
      alpha_beta_cache[arg_with_search->S_network_id][my_colour],
      &alpha_beta_cache_slot);

    if (alpha_beta_cache_hit)
    {
      ++(arg_with_search->S_total_alpha_beta_cache_hits);

      if (!IS_PV(arg_node_type) and
          (alpha_beta_cache_slot.ABCS_depth >= arg_my_depth))
      {
        ++(arg_with_search->S_total_alpha_beta_cache_depth_hits);

        if (alpha_beta_cache_slot.ABCS_flags & ABCS_SCORE_LE_ALPHA_BIT)
        {
          ++(arg_with_search->S_total_alpha_beta_cache_le_alpha_hits);

          if (alpha_beta_cache_slot.ABCS_score <= arg_my_alpha)
          {
            TRACE_AB_CACHE(arg_with_search,
                           "LE ALPHA HIT",
                           &alpha_beta_cache_slot)

            ++(arg_with_search->S_total_alpha_beta_cache_le_alpha_cutoffs);

            construct_search_result(arg_search_result,
                                    alpha_beta_cache_slot.ABCS_raw_score,
                                    alpha_beta_cache_slot.ABCS_score,
                                    SR_NULL);

            update_pv(&(arg_search_result->SR_pv),
                      arg_with_search,
                      alpha_beta_cache_slot.ABCS_moves[0]);

            arg_search_result->SR_depth = alpha_beta_cache_slot.ABCS_depth;

            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot.ABCS_flags & ABCS_SCORE_GE_BETA_BIT)
        {
          ++(arg_with_search->S_total_alpha_beta_cache_ge_beta_hits);

          if (alpha_beta_cache_slot.ABCS_score >= arg_my_beta)
          {
            TRACE_AB_CACHE(arg_with_search,
                           "GE BETA HIT",
                           &alpha_beta_cache_slot)

            ++(arg_with_search->S_total_alpha_beta_cache_ge_beta_cutoffs);

            construct_search_result(arg_search_result,
                                    alpha_beta_cache_slot.ABCS_raw_score,
                                    alpha_beta_cache_slot.ABCS_score,
                                    SR_NULL);

            update_pv(&(arg_search_result->SR_pv),
                      arg_with_search,
                      alpha_beta_cache_slot.ABCS_moves[0]);

            arg_search_result->SR_depth = alpha_beta_cache_slot.ABCS_depth;

            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot.ABCS_flags & ABCS_SCORE_TRUE_SCORE_BIT)
        {
          TRACE_AB_CACHE(arg_with_search,
                         "TRUE SCORE HIT",
                         &alpha_beta_cache_slot)

          ++(arg_with_search->S_total_alpha_beta_cache_true_score_hits);

          construct_search_result(arg_search_result,
                                  alpha_beta_cache_slot.ABCS_raw_score,
                                  alpha_beta_cache_slot.ABCS_score,
                                  SR_NULL);

          update_pv(&(arg_search_result->SR_pv),
                    arg_with_search,
                    alpha_beta_cache_slot.ABCS_moves[0]);

          arg_search_result->SR_depth = alpha_beta_cache_slot.ABCS_depth;

          goto label_return;
        }
      }
    }
  }

  int moves_weight[NMOVES_MAX];

  for (int imove = 0; imove < moves_list->ML_nmoves; imove++)
    moves_weight[imove] = moves_list->ML_move_weights[imove];

  if (alpha_beta_cache_hit)
  {
    for (int imove = 0; imove < NABCS_MOVES; imove++)
    {
      int jmove = alpha_beta_cache_slot.ABCS_moves[imove];

      if (jmove == INVALID)
        break;

      if (jmove >= moves_list->ML_nmoves)
        arg_with_search->S_total_alpha_beta_cache_nmoves_errors++;
      else
        moves_weight[jmove] = L_MAX - imove;
    }
  }

  int sort_local[NMOVES_MAX];

  int *sort = arg_sort;

  if (sort == NULL)
  {
    sort = sort_local;

    sort_moves_weight(moves_list->ML_nmoves, moves_weight, sort);
  }

  int nsingular_moves = INVALID;

  int singular_extension = INVALID;

  int singular_probe_score = SCORE_NOT_A_NUMBER;

  if ((options.singular_probe_depth > 0) and
      !(arg_my_tweaks & TWEAK_MOVE_REDUCED_BIT) and
      !(arg_my_tweaks & TWEAK_SINGULAR_PROBE_BIT) and
      (arg_my_depth >= options.singular_probe_depth))
  {
    if (alpha_beta_cache_hit and
        (moves_list->ML_nmoves > options.singular_extension_moves) and
        (alpha_beta_cache_slot.ABCS_depth >= arg_my_depth) and
        !(alpha_beta_cache_slot.ABCS_flags & ABCS_SCORE_LE_ALPHA_BIT) and
        !IS_SCORE_WON(alpha_beta_cache_slot.ABCS_raw_score) and
        !IS_SCORE_LOST(alpha_beta_cache_slot.ABCS_raw_score))
    {
      nsingular_moves = options.singular_extension_moves;

      singular_probe_score = alpha_beta_cache_slot.ABCS_raw_score;
    }
  }

  if (singular_probe_score != SCORE_NOT_A_NUMBER)
  {
    ++(arg_with_search->S_total_singular_extension_probes);

    score_t singular_probe_beta =
      singular_probe_score - SCORE(options.singular_probe_margin);

    score_t singular_probe_alpha = singular_probe_beta - SCORE(1);

    int singular_probe_depth =
      (arg_my_depth * options.singular_probe_percentage) / 100;

    int singular_sort[NMOVES_MAX];

    for (int imove = 0; imove < moves_list->ML_nmoves; imove++)
      singular_sort[imove] = sort[imove];

    search_result_t singular_probe_result;

    singular_probe_result.SR_pv = arg_search_result->SR_pv;

    alpha_beta(arg_with_search,
               singular_probe_alpha,
               singular_probe_beta,
               singular_probe_depth,
               MINIMAL_WINDOW_BIT,
               arg_reduction_depth_root,
               arg_my_tweaks | TWEAK_SINGULAR_PROBE_BIT,
               moves_list,
               singular_sort,
               nsingular_moves,
               INVALID,
               arg_history,
               &singular_probe_result);

    if (IS_SCORE_NAN(singular_probe_result.SR_score))
      goto label_return;

    if (singular_probe_result.SR_score <= singular_probe_alpha)
    {
      ++(arg_with_search->S_total_singular_extension_probes_le_alpha);

      singular_extension = 1;
    }
    else
    {
      ++(arg_with_search->S_total_singular_extension_probes_ge_beta);

      singular_extension = 0;
    }
  }

  construct_search_result(arg_search_result,
                          SCORE(SCORE_MINUS_INFINITY),
                          SCORE(SCORE_MINUS_INFINITY),
                          SR_NULL);

  HARDBUG(arg_search_result->SR_depth != INVALID)

  if (((moves_list->ML_nmoves <= options.singular_extension_moves) and
       !move_repetition(&(arg_with_search->S_board))) or
      (moves_list->ML_ncaptx > 0))
  {
    alpha_beta_captures(arg_with_search,
                        arg_my_alpha,
                        arg_my_beta,
                        arg_my_depth,
                        arg_node_type,
                        arg_reduction_depth_root,
                        arg_my_tweaks,
                        moves_list,
                        sort,
                        nsingular_moves,
                        singular_extension,
                        arg_history,
                        arg_search_result);

    if (IS_SCORE_NAN(arg_search_result->SR_score))
      goto label_return;

    goto label_store;
  }

  if (arg_reduction_depth_root > 0)
    arg_reduction_depth_root--;

  if (arg_my_depth == 0)
  {
    score_t quiescence_score;

    quiescence(arg_with_search,
               arg_my_alpha,
               arg_my_beta,
               arg_node_type,
               moves_list,
               FALSE,
               arg_history,
               &quiescence_score,
               &(arg_search_result->SR_pv));

    construct_search_result(arg_search_result,
                            quiescence_score,
                            quiescence_score,
                            SR_NULL);

    arg_search_result->SR_depth = 0;

    goto label_return;
  }

  if (IS_MINIMAL_WINDOW(arg_node_type))
  {
    alpha_beta_minimal(arg_with_search,
                       arg_my_alpha,
                       arg_my_beta,
                       arg_my_depth,
                       arg_node_type,
                       arg_reduction_depth_root,
                       arg_my_tweaks,
                       moves_list,
                       sort,
                       nsingular_moves,
                       singular_extension,
                       arg_history,
                       arg_search_result);

    if (IS_SCORE_NAN(arg_search_result->SR_score))
      goto label_return;
  }
  else
  {
    alpha_beta_pv(arg_with_search,
                  arg_my_alpha,
                  arg_my_beta,
                  arg_my_depth,
                  arg_node_type,
                  arg_reduction_depth_root,
                  arg_my_tweaks,
                  moves_list,
                  sort,
                  nsingular_moves,
                  singular_extension,
                  arg_history,
                  arg_search_result);

    if (IS_SCORE_NAN(arg_search_result->SR_score))
      goto label_return;
  }

label_store:

  // we always searched move 0

  HARDBUG(arg_search_result->SR_score == SCORE(SCORE_MINUS_INFINITY))

  HARDBUG(arg_search_result->SR_depth == INVALID)

  HARDBUG(arg_search_result->SR_depth > DEPTH_MAX)

  // #ifdef DEBUG
  int frequency[NMOVES_MAX] = {0};

  for (int imove = 0; imove < moves_list->ML_nmoves; imove++)
  {
    int key = sort[imove];

    HARDBUG((key < 0) or (key >= moves_list->ML_nmoves))

    frequency[key]++;
  }

  for (int imove = 0; imove < moves_list->ML_nmoves; imove++)
    HARDBUG(frequency[sort[imove]] != 1)

  // #endif

  if (FALSE and (options.alpha_beta_cache_size > 0) and
      IS_SR_DRAW_BY_REP(arg_search_result->SR_flags))
  {
    // HARDBUG(arg_search_result->SR_score != 0)

    my_printf(arg_with_search->S_my_printf, "NOT STORING DRAW BY REP!\n");
  }

  if ((options.alpha_beta_cache_size > 0) and
      !(arg_my_tweaks & TWEAK_SINGULAR_PROBE_BIT) and
      !IS_SR_DRAW_BY_REP(arg_search_result->SR_flags) and
      (options.cache_draw_results or (arg_search_result->SR_score != 0)))
  {
    HARDBUG(arg_search_result->SR_score > SCORE_PLUS_INFINITY)
    HARDBUG(arg_search_result->SR_score < SCORE_MINUS_INFINITY)

    alpha_beta_cache_slot.ABCS_key = arg_with_search->S_board.B_key;

    alpha_beta_cache_slot.ABCS_raw_score = arg_search_result->SR_raw_score;
    alpha_beta_cache_slot.ABCS_score = arg_search_result->SR_score;

    alpha_beta_cache_slot.ABCS_depth = arg_search_result->SR_depth;

    for (int imove = 0; imove < NABCS_MOVES; imove++)
      alpha_beta_cache_slot.ABCS_moves[imove] = INVALID;

    int nmoves = NABCS_MOVES;
    if (moves_list->ML_nmoves < nmoves)
      nmoves = moves_list->ML_nmoves;

    for (int imove = 0; imove < nmoves; imove++)
      alpha_beta_cache_slot.ABCS_moves[imove] = sort[imove];

    if (arg_search_result->SR_score <= arg_my_alpha)
    {
      HARDBUG(IS_SR_GE_BETA(arg_search_result->SR_flags))

      ++(arg_with_search->S_total_alpha_beta_cache_le_alpha_stored);

      alpha_beta_cache_slot.ABCS_flags = ABCS_SCORE_LE_ALPHA_BIT;

      TRACE_AB_CACHE(arg_with_search, "STORE LE ALPHA", &alpha_beta_cache_slot)
    }
    else if (arg_search_result->SR_score >= arg_my_beta)
    {
      HARDBUG(IS_SR_LE_ALPHA(arg_search_result->SR_flags))

      ++(arg_with_search->S_total_alpha_beta_cache_ge_beta_stored);

      // alpha_beta_cache_slot.ABCS_raw_score = arg_my_beta;
      alpha_beta_cache_slot.ABCS_score = arg_my_beta;

      alpha_beta_cache_slot.ABCS_flags = ABCS_SCORE_GE_BETA_BIT;

      TRACE_AB_CACHE(arg_with_search, "STORE GE BETA", &alpha_beta_cache_slot)
    }
    else
    {
      ++(arg_with_search->S_total_alpha_beta_cache_true_score_stored);

      alpha_beta_cache_slot.ABCS_flags = ABCS_SCORE_TRUE_SCORE_BIT;

      TRACE_AB_CACHE(arg_with_search,
                     "STORE TRUE SCORE",
                     &alpha_beta_cache_slot)
    }

    update_alpha_beta_cache(
      arg_with_search,
      alpha_beta_cache[arg_with_search->S_network_id][my_colour],
      &alpha_beta_cache_slot);
#ifdef DEBUG
    HARDBUG(!probe_alpha_beta_cache(
      arg_with_search,
      alpha_beta_cache[arg_with_search->S_network_id][my_colour],
      &alpha_beta_cache_slot))
#endif
  }

label_return:

  if (FALSE and (arg_search_result->SR_depth == DEPTH_MAX))
  {
    HARDBUG(!IS_SCORE_WON(arg_search_result->SR_score) and
            !IS_SCORE_LOST(arg_search_result->SR_score) and
            (arg_search_result->SR_score != 0))
  }

  if (!IS_SCORE_NAN(arg_search_result->SR_score) and
      (arg_search_result->SR_depth < arg_my_depth))
  {
    PRINTF("arg_search_result->SR_depth=%d arg_my_depth=%d\n",
           arg_search_result->SR_depth,
           arg_my_depth);

    FATAL("eh?", EXIT_FAILURE)
  }

  fail_hard(arg_my_alpha, arg_my_beta, arg_search_result);

  TRACE_AB_EXIT(arg_with_search,
                arg_my_alpha,
                arg_my_beta,
                arg_node_type,
                arg_my_depth,
                arg_search_result);

  check_search_result(arg_my_alpha, arg_my_beta, arg_search_result);

  END_BLOCK

  POP_NAME(__FUNC__)
}

void do_pv(search_t *arg_with_search,
           pv_entries_t *arg_best_pv,
           score_t arg_pv_score,
           bstring arg_bpv_string)
{
  if (draw_by_repetition(&(arg_with_search->S_board), FALSE))
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      HARDBUG(bcatcstr(arg_bpv_string, " rep") == BSTR_ERR)
    }

    return;
  }

  int wdl;

  int temp_mate = read_endgame(arg_with_search, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    temp_mate = read_endgame(arg_with_search, NULL);

    if (temp_mate == INVALID)
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
      }
      else
      {
        HARDBUG(bcatcstr(arg_bpv_string, " draw") == BSTR_ERR)
      }
    }
    else if (temp_mate > 0)
    {
      if (!options.hub)
      {
        HARDBUG(bformata(arg_bpv_string, " won in %d", temp_mate) == BSTR_ERR)
      }
      endgame_pv(arg_with_search, temp_mate, arg_bpv_string);
    }
    else
    {
      if (!options.hub)
      {
        HARDBUG(bformata(arg_bpv_string, " lost in %d", -temp_mate) == BSTR_ERR)
      }
      endgame_pv(arg_with_search, temp_mate, arg_bpv_string);
    }
    return;
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(arg_with_search->S_board), &moves_list);

  if (moves_list.ML_nmoves == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      HARDBUG(bcatcstr(arg_bpv_string, " #") == BSTR_ERR)
    }

    return;
  }

  int jmove = 0;

  if (moves_list.ML_nmoves > 1)
    jmove = probe_pv(arg_best_pv, arg_with_search);

  if (jmove == INVALID)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      score_t temp_score = return_score(arg_with_search);

      HARDBUG(bformata(arg_bpv_string, " " PRINTF_SCORE_T, temp_score) ==
              BSTR_ERR)

      if (temp_score != arg_pv_score)
      {
        HARDBUG(bcatcstr(arg_bpv_string, " PV ERROR") == BSTR_ERR)
      }
    }
  }
  else
  {
    BSTRING(bmove_string)

    move2bstring(&moves_list, jmove, bmove_string);

    HARDBUG(bcatcstr(arg_bpv_string, " ") == BSTR_ERR)

    HARDBUG(bconcat(arg_bpv_string, bmove_string) == BSTR_ERR)

    BDESTROY(bmove_string)

    do_move(&(arg_with_search->S_board), jmove, &moves_list);

    do_pv(arg_with_search, arg_best_pv, -arg_pv_score, arg_bpv_string);

    undo_move(&(arg_with_search->S_board), jmove, &moves_list);
  }
}

local void print_pv(search_t *arg_with_search,
                    int arg_my_depth,
                    int arg_imove,
                    int arg_jmove,
                    moves_list_t *arg_moves_list,
                    score_t arg_move_score,
                    pv_entries_t *arg_best_pv,
                    char *arg_move_status)
{
  BSTRING(bmove_string)

  move2bstring(arg_moves_list, arg_jmove, bmove_string);

  BSTRING(bpv_string)

  if (options.hub)
  {
    if (options.nthreads == INVALID)
      global_nodes = arg_with_search->S_total_nodes;
    else
    {
      global_nodes = 0;

      for (int ithread = 0; ithread < options.nthreads; ithread++)
      {
        thread_t *with_thread = threads + ithread;

        global_nodes += with_thread->T_search.S_total_nodes;
      }
    }
    bformata(bpv_string,
             "depth=%d score=%.3f time=%.2f nps=%.2f pv=\"%s",
             arg_my_depth,
             (float)arg_move_score / (float)SCORE_MAN,
             return_my_timer(&(arg_with_search->S_timer), FALSE),
             global_nodes / return_my_timer(&(arg_with_search->S_timer), TRUE) /
               1000000.0,
             bdata(bmove_string));
  }
  else
  {
    bformata(bpv_string,
             "d=%d i=%d j=%d m=%s s=" PRINTF_SCORE_T " (%.2f) r=" PRINTF_SCORE_T
             " t=%.2f n=%lld/%lld/%lld "
             "x=%s pv=%s",
             arg_my_depth,
             arg_imove,
             arg_jmove,
             bdata(bmove_string),
             arg_move_score,
             (float)arg_move_score / (float)SCORE_MAN,
             arg_with_search->S_root_score,
             return_my_timer(&(arg_with_search->S_timer), FALSE),
             arg_with_search->S_total_nodes,
             arg_with_search->S_total_alpha_beta_nodes,
             arg_with_search->S_total_quiescence_nodes,
             arg_move_status,
             bdata(bmove_string));
  }

  do_move(&(arg_with_search->S_board), arg_jmove, arg_moves_list);

  do_pv(arg_with_search, arg_best_pv, -arg_move_score, bpv_string);

  undo_move(&(arg_with_search->S_board), arg_jmove, arg_moves_list);

  my_printf(arg_with_search->S_my_printf, "%s\n", bdata(bpv_string));

  // flush

  if (arg_with_search->S_thread == NULL)
    my_printf(arg_with_search->S_my_printf, "$");

  if (options.mode == GAME_MODE)
  {
    if ((arg_with_search->S_thread != NULL) and
        (arg_with_search->S_thread == thread_alpha_beta_master))
      enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string));
  }

  BDESTROY(bpv_string)

  BDESTROY(bmove_string)
}

void do_search(search_t *arg_with_search,
               moves_list_t *arg_moves_list,
               int arg_depth_min,
               int arg_depth_max,
               score_t arg_root_score,
               my_random_t *arg_shuffle)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  my_printf(arg_with_search->S_my_printf,
            "time_limit=%.2f\n",
            options.time_limit);

  my_printf(arg_with_search->S_my_printf,
            "time_ntrouble=%d\n",
            options.time_ntrouble);

  for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
    my_printf(arg_with_search->S_my_printf,
              "itrouble=%d time_trouble=%.2f\n",
              itrouble,
              options.time_trouble[itrouble]);

  if (arg_with_search->S_thread == thread_alpha_beta_master)
  {
    clear_bucket(&bucket_depth);
  }

  int my_depth = INVALID;

  HARDBUG(arg_moves_list->ML_nmoves == 0)

  if (arg_depth_min == INVALID)
    arg_depth_min = 1;
  if (arg_depth_max == INVALID)
    arg_depth_max = options.depth_limit;

  arg_with_search->S_board.B_root_inode = arg_with_search->S_board.B_inode;

  clear_totals(arg_with_search);

  reset_my_timer(&(arg_with_search->S_timer));

  int sort[NMOVES_MAX];

  shuffle(sort, arg_moves_list->ML_nmoves, arg_shuffle);

  // check for known endgame

  int egtb_mate = read_endgame(arg_with_search, NULL);

  if (egtb_mate != ENDGAME_UNKNOWN)
  {
    print_board(&(arg_with_search->S_board));

    my_printf(arg_with_search->S_my_printf,
              "known endgame egtb_mate=%d\n",
              egtb_mate);

    arg_with_search->S_best_score = SCORE(SCORE_MINUS_INFINITY);

    int your_mate = INVALID;

    for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    {
      int jmove = sort[imove];

      do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

      ++(arg_with_search->S_total_nodes);

      int temp_mate = read_endgame(arg_with_search, NULL);

      HARDBUG(temp_mate == ENDGAME_UNKNOWN)

      score_t temp_score;

      if (temp_mate == INVALID)
        temp_score = SCORE(0);
      else if (temp_mate > 0)
        temp_score =
          SCORE(SCORE_WON - arg_with_search->S_board.B_inode - temp_mate);
      else
        temp_score =
          SCORE(SCORE_LOST + arg_with_search->S_board.B_inode - temp_mate);

      undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

      score_t move_score = -temp_score;

      BSTRING(bmove_string)

      move2bstring(arg_moves_list, imove, bmove_string);

      my_printf(arg_with_search->S_my_printf,
                "%s " PRINTF_SCORE_T "\n",
                bdata(bmove_string),
                move_score);

      BDESTROY(bmove_string)

      if (move_score > arg_with_search->S_best_score)
      {
        for (int kmove = imove; kmove > 0; kmove--)
          sort[kmove] = sort[kmove - 1];

        sort[0] = jmove;

        arg_with_search->S_best_score = move_score;

        arg_with_search->S_best_move = jmove;

        your_mate = temp_mate;
      }
#ifdef DEBUG
      for (int idebug = 0; idebug < arg_moves_list->ML_nmoves; idebug++)
      {
        int ndebug = 0;
        for (int jdebug = 0; jdebug < arg_moves_list->ML_nmoves; jdebug++)
          if (sort[jdebug] == idebug)
            ++ndebug;
        HARDBUG(ndebug != 1)
      }
#endif
    }

    HARDBUG(arg_with_search->S_best_score == SCORE(SCORE_MINUS_INFINITY))

    arg_with_search->S_best_score_kind = SEARCH_BEST_SCORE_EGTB;

    arg_with_search->S_best_depth = 0;

    BSTRING(bmove_string)

    move2bstring(arg_moves_list, sort[0], bmove_string);

    BSTRING(bpv_string)

    if (options.hub)
    {
      HARDBUG(bformata(bpv_string,
                       "depth=127 score=" PRINTF_SCORE_T " nps=0.00 pv=\"%s",
                       arg_with_search->S_best_score,
                       bdata(bmove_string)) == BSTR_ERR)
    }
    else
    {
      HARDBUG(bformata(bpv_string,
                       "s=" PRINTF_SCORE_T " (%d) x=egtb pv=%s ",
                       arg_with_search->S_best_score,
                       egtb_mate,
                       bdata(bmove_string)) == BSTR_ERR)
    }

    if (your_mate != INVALID)
    {
      do_move(&(arg_with_search->S_board), sort[0], arg_moves_list);

      endgame_pv(arg_with_search, your_mate, bpv_string);

      undo_move(&(arg_with_search->S_board), sort[0], arg_moves_list);
    }
    else
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(bpv_string, "\"") == BSTR_ERR)
      }
    }

    my_printf(arg_with_search->S_my_printf, "%s\n", bdata(bpv_string));

    if (options.mode == GAME_MODE)
    {
      if ((arg_with_search->S_thread != NULL) and
          (arg_with_search->S_thread == thread_alpha_beta_master))
        enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string));
    }

    BDESTROY(bpv_string)

    BDESTROY(bmove_string)

    goto label_limit;
  }

  ui32_t your_tweaks = 0;

  arg_with_search->S_root_simple_score = return_score(arg_with_search);

  search_result_t temp_result;

  temp_result.SR_pv = pv_entries_default;

  if (FALSE and (arg_moves_list->ML_ncaptx == 0) and
      (arg_moves_list->ML_nmoves > 2))
  {
    do_move(&(arg_with_search->S_board), INVALID, NULL);

    alpha_beta(arg_with_search,
               SCORE(SCORE_MINUS_INFINITY),
               SCORE(SCORE_PLUS_INFINITY),
               1,
               PV_BIT,
               options.reduction_depth_root,
               TWEAK_MOVE_NULL_BIT,
               NULL,
               NULL,
               INVALID,
               INVALID,
               0xFULL,
               &temp_result);

    arg_with_search->S_root_score = -temp_result.SR_score;

    undo_move(&(arg_with_search->S_board), INVALID, NULL);
  }
  else
  {
    alpha_beta(arg_with_search,
               SCORE(SCORE_MINUS_INFINITY),
               SCORE(SCORE_PLUS_INFINITY),
               1,
               PV_BIT,
               options.reduction_depth_root,
               0,
               NULL,
               NULL,
               INVALID,
               INVALID,
               0xFULL,
               &temp_result);

    arg_with_search->S_root_score = temp_result.SR_score;
  }

  if (IS_SCORE_NAN(temp_result.SR_score))
  {
    arg_with_search->S_root_score = return_score(arg_with_search);

    arg_with_search->S_best_score_kind = SEARCH_BEST_SCORE_AB;

    arg_with_search->S_best_move = 0;

    arg_with_search->S_best_depth = INVALID;

    goto label_limit;
  }

  my_printf(arg_with_search->S_my_printf,
            "root_simple_score=" PRINTF_SCORE_T " root_score=" PRINTF_SCORE_T
            "\n",
            arg_with_search->S_root_simple_score,
            arg_with_search->S_root_score);

  arg_with_search->S_best_score_kind = SEARCH_BEST_SCORE_AB;

  arg_with_search->S_best_move = 0;

  arg_with_search->S_best_depth = 0;

  if (arg_root_score == SCORE(SCORE_MINUS_INFINITY))
    arg_with_search->S_best_score = arg_with_search->S_root_score;
  else
    arg_with_search->S_best_score = arg_root_score;

  // publish work

  if ((arg_with_search->S_thread != NULL) and
      (arg_with_search->S_thread == thread_alpha_beta_master))
  {
    BSTRING(bmove_string)

    move2bstring(arg_moves_list, 0, bmove_string);

    BSTRING(bmessage)

    HARDBUG(bformata(bmessage,
                     "%s %d %d " PRINTF_SCORE_T " %d",
                     bdata(bmove_string),
                     arg_depth_min,
                     arg_depth_max,
                     arg_with_search->S_best_score,
                     FALSE) == BSTR_ERR)

    if (options.lazy_smp_policy == 0)
    {
      for (int ithread = 1; ithread < options.nthreads_alpha_beta; ithread++)
      {
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_SEARCH_FIRST,
                bdata(bmessage));
      }
    }
    else
    {
      for (int ithread = 1; ithread < options.nthreads_alpha_beta; ithread++)
      {
        if ((ithread % 3) == 1)
        {
          enqueue(return_thread_queue(threads + ithread),
                  MESSAGE_SEARCH_FIRST,
                  bdata(bmessage));
        }
        else if ((ithread % 3) == 2)
        {
          enqueue(return_thread_queue(threads + ithread),
                  MESSAGE_SEARCH_AHEAD,
                  bdata(bmessage));
        }
        else
        {
          enqueue(return_thread_queue(threads + ithread),
                  MESSAGE_SEARCH_SECOND,
                  bdata(bmessage));
        }
      }
    }
    BDESTROY(bmessage)

    BDESTROY(bmove_string)
  }

  for (my_depth = arg_depth_min; my_depth <= arg_depth_max; ++my_depth)
  {
#ifdef TRACE_AB
    PRINTF("my_depth=%d\n", my_depth);
#endif

    int your_depth = my_depth - 1;

    int nbest_moves = 0;

    // if node_type is MINIMAL_WINDOW_BIT
    // the first move will be also be searched with a minimal window
    // if a move fails high we want to have a true score
    // so we research the move with a PV (non-minimal) window
    // so we will always get a true score
    //(a score that does not fail low or fail high)
    // if all moves fail low we do not have a true score
    // in that case we double best_score_margin
    // and force the first move to be searched with a PV window

    ui32_t node_type = PV_BIT;

    // best_score_margin determines if the current search
    // should be aborted if the score of the current move drops too low
    // if so, we should try the next move
    // with a window around the current best score
    // not around the score of the current move

    score_t best_score_margin = SCORE(options.best_score_margin);

    // all moves will fail low if all moves are bad in the above sense
    // so we will not have a true score
    // in that case we double best_score_margin
    // and force the first move to be searched with a PV window

    // if best_score_margin is large (say SCORE_PLUS_INFINITY)
    // the score will never fail low against best_score_margin
    // so we will get a true score

    while (TRUE)
    {
      for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
      {
        int jmove = sort[imove];

        score_t window_alpha;
        score_t window_beta;

        // initial search window

        if (IS_PV(node_type))
        {
          window_alpha = SCORE(options.aspiration_margin);
          window_beta = SCORE(options.aspiration_margin);
        }
        else
        {
          window_alpha = SCORE(0);
          window_beta = SCORE(1);
        }

        score_t move_score = arg_with_search->S_best_score;

        // results of searches are speculative:
        // if a search fails low or high the research
        // could fail high or low for example

        // if a minimal-window search fails low we are done
        // but this is already an approximation as
        // the fail-low could also be speculative
        // but researching fail-lows will cost too much time
        // if a move fails high we want to have a true score
        // so we research the move with a PV (non-minimal) window

        int iteration = 0;

        while (TRUE)
        {
          score_t my_alpha = move_score - window_alpha;
          score_t my_beta = move_score + window_beta;

          if (my_alpha < SCORE_MINUS_INFINITY)
            my_alpha = SCORE(SCORE_MINUS_INFINITY);
          if (my_beta > SCORE_PLUS_INFINITY)
            my_beta = SCORE(SCORE_PLUS_INFINITY);

          TRACE_KEY(arg_with_search)

          do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

          TRACE_AB_WINDOW(trace_key,
                          "search",
                          my_alpha,
                          my_beta,
                          arg_moves_list,
                          jmove)

          TRACE_INDENT(arg_moves_list, jmove)

          alpha_beta(arg_with_search,
                     -my_beta,
                     -my_alpha,
                     your_depth,
                     node_type,
                     options.reduction_depth_root,
                     your_tweaks,
                     NULL,
                     NULL,
                     INVALID,
                     INVALID,
                     0xFULL,
                     &temp_result);

          TRACE_DEDENT

          TRACE_AB_RESULT(trace_key,
                          "result search",
                          my_alpha,
                          my_beta,
                          arg_moves_list,
                          jmove,
                          &temp_result);

          flip_search_result(&temp_result);

          check_search_result(my_alpha, my_beta, &temp_result);

          move_score = temp_result.SR_score;

          undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

          if (IS_SCORE_NAN(temp_result.SR_score))
            goto label_limit;

          if (iteration == 0)
          {
            window_alpha = SCORE(options.research_margin);
            window_beta = SCORE(options.research_margin);
          }

#if SCORE_T == SCORE_T_INT
          if (move_score <= my_alpha)
#elif SCORE_T == SCORE_T_FLOAT
          // move_score should have been <= my_alpha
          // move_score=1.963999987 my_alpha=1.962000012 my_beta=2.961999893

          if ((move_score <= my_alpha) or
              ((move_score < my_beta) and IS_MINIMAL_WINDOW(node_type)))
#else
#error unknown SCORE_T
#endif
          {
            // if a minimal-window search fails low we are done

            if (IS_MINIMAL_WINDOW(node_type))
            {
              HARDBUG(iteration != 0)

              goto label_next_move;
            }

            // if we already have a true score and the current move fails low
            // we are done with this move and continue with the next move

            if ((nbest_moves > 0) and
                (my_alpha <= arg_with_search->S_best_score))
            {
              node_type = MINIMAL_WINDOW_BIT;

              goto label_next_move;
            }

            // three other cases:

            // suppose ((nbest_moves > 0) and
            //          (my_alpha > arg_with_search->S_best_score))
            // this can happen if the move failed high with a margin of say +100
            // we research the move with margins (-25, +25)
            // suppose the move now fails low with a margin of -50
            // the move seems to be a fail-high with respect to
            // the best score but it could still be a fail low
            // so we have to research

            // suppose ((nbest_moves == 0) and
            //          (my_alpha > arg_with_search->S_best_score))
            // we have to research

            // suppose ((nbest_moves == 0) and
            //          (my_alpha <= arg_with_search->S_best_score))
            // this could be a really bad move

            if ((nbest_moves == 0) and (best_score_margin > 0) and
                ((my_alpha + best_score_margin) <=
                 arg_with_search->S_best_score))
            {
              // abandon this move and search the next move with a PV window

              HARDBUG(!IS_PV(node_type))

              goto label_next_move;
            }

            print_pv(arg_with_search,
                     my_depth,
                     0,
                     jmove,
                     arg_moves_list,
                     move_score,
                     &(temp_result.SR_pv),
                     "<=");

            if (iteration > 0)
              window_alpha *= 2;
          }
          else if (move_score >= my_beta)
          {
            // this looks like a promosing move
            // we research it with a PV window

            print_pv(arg_with_search,
                     my_depth,
                     0,
                     jmove,
                     arg_moves_list,
                     move_score,
                     &(temp_result.SR_pv),
                     ">=");

            if (iteration > 0)
              window_beta *= 2;

            node_type = PV_BIT;
          }
          else
          {
            // we have a true score

            HARDBUG(!IS_PV(node_type))

            if ((best_score_margin > 0) and (move_score + best_score_margin) <=
                                              arg_with_search->S_best_score)
              goto label_next_move;

            print_pv(arg_with_search,
                     my_depth,
                     0,
                     jmove,
                     arg_moves_list,
                     move_score,
                     &(temp_result.SR_pv),
                     "==");
            break;
          }

          iteration++;
        }

        // if we do not have a best score yet we always accept the score
        // otherwise only if the best score improves

        if ((nbest_moves == 0) or (move_score > arg_with_search->S_best_score))
        {
          HARDBUG(!IS_PV(node_type))

          if (sort[0] != jmove)
          {
            for (int kmove = imove; kmove > 0; kmove--)
              sort[kmove] = sort[kmove - 1];

            sort[0] = jmove;

            if ((arg_with_search->S_thread != NULL) and
                (arg_with_search->S_thread == thread_alpha_beta_master) and
                (options.lazy_smp_policy > 0))
            {
              BSTRING(bmove_string)

              move2bstring(arg_moves_list, jmove, bmove_string);

              BSTRING(bmessage)

              HARDBUG(bformata(bmessage,
                               "%s %d %d " PRINTF_SCORE_T " %d",
                               bdata(bmove_string),
                               arg_depth_min,
                               arg_depth_max,
                               arg_with_search->S_best_score,
                               FALSE) == BSTR_ERR)

              for (int ithread = 1; ithread < options.nthreads_alpha_beta;
                   ithread++)
              {
                if ((ithread % 3) == 2)
                {
                  enqueue(return_thread_queue(threads + ithread),
                          MESSAGE_SEARCH_AHEAD,
                          bdata(bmessage));
                }
                else if ((ithread % 3) == 0)
                {
                  enqueue(return_thread_queue(threads + ithread),
                          MESSAGE_SEARCH_SECOND,
                          bdata(bmessage));
                }
              }

              BDESTROY(bmessage)

              BDESTROY(bmove_string)
            }
          }

          arg_with_search->S_best_score = move_score;

          arg_with_search->S_best_move = jmove;

          arg_with_search->S_best_depth = my_depth;

          print_pv(arg_with_search,
                   my_depth,
                   imove,
                   jmove,
                   arg_moves_list,
                   move_score,
                   &(temp_result.SR_pv),
                   "===");

          nbest_moves++;

          // now that we have a best move with a true score
          // the next should be searched with a minimal window

          node_type = MINIMAL_WINDOW_BIT;
        }

        for (int idebug = 0; idebug < arg_moves_list->ML_nmoves; idebug++)
        {
          int ndebug = 0;
          for (int jdebug = 0; jdebug < arg_moves_list->ML_nmoves; jdebug++)
            if (sort[jdebug] == idebug)
              ++ndebug;
          HARDBUG(ndebug != 1)
        }

      label_next_move:;
      } // for (int imove)

      if (IS_SCORE_WON(arg_with_search->S_best_score))
        goto label_limit;
      if (IS_SCORE_LOST(arg_with_search->S_best_score))
        goto label_limit;

      if (nbest_moves > 0)
        break;

      // all moves seem to be bad
      // the next move should return a true score

      node_type = PV_BIT;

      best_score_margin *= 2;
    } // while(TRUE)
  }
label_limit:

  if (arg_with_search->S_thread == thread_alpha_beta_master)
  {
    if (FALSE and (arg_with_search->S_best_score < -SCORE(SCORE_MAN / 4)))
    {
      BSTRING(bfen)

      board2fen(&(arg_with_search->S_board), bfen, TRUE);

      BSTRING(bmove_string)

      move2bstring(arg_moves_list, arg_with_search->S_best_move, bmove_string);

      my_printf(arg_with_search->S_my_printf,
                "%s bs=" PRINTF_SCORE_T " bm=%s md=%d bd=%d\n",
                bdata(bfen),
                arg_with_search->S_best_score,
                bdata(bmove_string),
                my_depth,
                arg_with_search->S_best_depth);

      char stamp[MY_LINE_MAX];

      time_t t = time(NULL);

      HARDBUG(strftime(stamp, MY_LINE_MAX, "%d%b%Y-%H%M%S", localtime(&t)) == 0)

      FILE *fbugs;

      HARDBUG((fbugs = fopen("bugs.txt", "a")) == NULL)

      fprintf(fbugs,
              "%s %s %s bs=" PRINTF_SCORE_T " bm=%s md=%d bd=%d\n",
              bdata(bfen),
              stamp,
              REVISION,
              arg_with_search->S_best_score,
              bdata(bmove_string),
              my_depth,
              arg_with_search->S_best_depth);

      FCLOSE(fbugs)

      BDESTROY(bmove_string)

      BDESTROY(bfen)
    }
  }

  stop_my_timer(&(arg_with_search->S_timer));

  BSTRING(bmove_string)

  move2bstring(arg_moves_list, arg_with_search->S_best_move, bmove_string),

    my_printf(arg_with_search->S_my_printf,
              "best_move=%s best_score=" PRINTF_SCORE_T "\n",
              bdata(bmove_string),
              arg_with_search->S_best_score);

  BDESTROY(bmove_string)

  my_printf(arg_with_search->S_my_printf,
            "%lld nodes in %.2f seconds\n"
            "%.0f nodes/second\n",
            arg_with_search->S_total_nodes,
            return_my_timer(&(arg_with_search->S_timer), FALSE),
            arg_with_search->S_total_nodes /
              return_my_timer(&(arg_with_search->S_timer), FALSE));

  if (options.verbose > 0)
    print_totals(arg_with_search);

  if ((arg_with_search->S_thread != NULL) and
      (arg_with_search->S_thread == thread_alpha_beta_master))
  {
    global_nodes = 0;

    for (int ithread = 0; ithread < options.nthreads; ithread++)
    {
      thread_t *with_thread = threads + ithread;

      global_nodes += with_thread->T_search.S_total_nodes;
    }

    my_printf(arg_with_search->S_my_printf,
              "%lld global_nodes in %.2f seconds\n"
              "%.0f global_nodes/second\n",
              global_nodes,
              return_my_timer(&(arg_with_search->S_timer), FALSE),
              global_nodes /
                return_my_timer(&(arg_with_search->S_timer), FALSE));
  }

  if (arg_with_search->S_thread == thread_alpha_beta_master)
  {
    printf_bucket(&bucket_depth);
  }

#ifdef LAZY_STATS

  for (int ndelta_man = -NPIECES_MAX; ndelta_man <= NPIECES_MAX; ++ndelta_man)
  {
    for (int ndelta_king = -NPIECES_MAX; ndelta_king <= NPIECES_MAX;
         ++ndelta_king)
    {
      mean_sigma(
        &(lazy_stats[ndelta_man + NPIECES_MAX][ndelta_king + NPIECES_MAX]));

      if (lazy_stats[ndelta_man + NPIECES_MAX][ndelta_king + NPIECES_MAX].S_n >
          0)
      {
        PRINTF("ndelta_man=%d ndelta_king=%d\n", ndelta_man, ndelta_king);

        printf_stats(
          &(lazy_stats[ndelta_man + NPIECES_MAX][ndelta_king + NPIECES_MAX]));
      }
    }
  }
#endif
  END_BLOCK

  POP_NAME(__FUNC__)
}

local void
clear_alpha_beta_cache(alpha_beta_cache_entry_t *arg_alpha_beta_cache)
{
  PRINTF("clear_alpha_beta_cache..\n");

  for (i64_t ientry = 0; ientry < nalpha_beta_cache_entries; ientry++)
    arg_alpha_beta_cache[ientry] = alpha_beta_cache_entry_default;

  PRINTF("..done\n");
}

void construct_search(search_t *arg_with_search,
                      my_printf_t *arg_my_printf,
                      void *arg_thread,
                      int arg_network_id)
{
  HARDBUG((arg_network_id < 0) or (arg_network_id >= NETWORKS_MAX))

  arg_with_search->S_my_printf = arg_my_printf;

  arg_with_search->S_thread = arg_thread;

  arg_with_search->S_network_id = arg_network_id;

  construct_board(&(arg_with_search->S_board),
                  arg_my_printf,
                  arg_with_search->S_network_id);

  construct_my_timer(&(arg_with_search->S_timer),
                     "board",
                     arg_with_search->S_my_printf,
                     FALSE);

  if ((arg_thread == NULL) and my_mpi_globals.MMG_is_single)
    construct_my_random(&(arg_with_search->S_random), NULL);
  else
  {
    ui64_t seed;

    construct_my_random(&(arg_with_search->S_random), &seed);

    PRINTF("construct_search seed=%llX\n", seed);
  }

  entry_i16_t entry_i16_default = {MATE_DRAW, MATE_DRAW};

  i64_t key_default = -1;

  i64_t endgame_entry_cache_size = options.egtb_entry_cache_size * MBYTE;

  BSTRING(bname)

  HARDBUG(bformata(bname, "S_%p_entry_cache\n", arg_with_search) == BSTR_ERR)

  construct_cache(&(arg_with_search->S_endgame_entry_cache),
                  bdata(bname),
                  endgame_entry_cache_size,
                  CACHE_ENTRY_KEY_TYPE_I64_T,
                  &key_default,
                  sizeof(entry_i16_t),
                  &entry_i16_default);

  HARDBUG(bassigncstr(bname, "") == BSTR_ERR)

  HARDBUG(bformata(bname, "S_%p_wdl_entry_cache\n", arg_with_search) ==
          BSTR_ERR)

  construct_cache(&(arg_with_search->S_endgame_wdl_entry_cache),
                  bdata(bname),
                  endgame_entry_cache_size,
                  CACHE_ENTRY_KEY_TYPE_I64_T,
                  &key_default,
                  sizeof(entry_i16_t),
                  &entry_i16_default);

  BDESTROY(bname)
}

void init_search(void)
{
  PRINTF("sizeof(alpha_beta_cache_slot_t)=%lld\n",
         (i64_t)sizeof(alpha_beta_cache_slot_t));
  PRINTF("sizeof(alpha_beta_cache_entry_t)=%lld\n",
         (i64_t)sizeof(alpha_beta_cache_entry_t));

  global_nodes = 0;

  // alpha_beta cache

  alpha_beta_cache[NETWORK0_ID][WHITE_ENUM] = NULL;
  alpha_beta_cache[NETWORK0_ID][BLACK_ENUM] = NULL;

  alpha_beta_cache[NETWORK1_ID][WHITE_ENUM] = NULL;
  alpha_beta_cache[NETWORK1_ID][BLACK_ENUM] = NULL;

  nalpha_beta_cache_entries = 0;

  if (options.alpha_beta_cache_size > 0)
  {
    float alpha_beta_cache_size = options.alpha_beta_cache_size * MBYTE / 2;

    nalpha_beta_cache_entries = first_prime_below(
      roundf(alpha_beta_cache_size / sizeof(alpha_beta_cache_entry_t)));

    HARDBUG(nalpha_beta_cache_entries < 3)

    PRINTF("nalpha_beta_cache_entries=%lld\n", nalpha_beta_cache_entries);

    alpha_beta_cache_slot_default.ABCS_key = 0;
    alpha_beta_cache_slot_default.ABCS_depth = 0;
    alpha_beta_cache_slot_default.ABCS_flags = 0;
    for (int imove = 0; imove < NABCS_MOVES; imove++)
      alpha_beta_cache_slot_default.ABCS_moves[imove] = INVALID;
    alpha_beta_cache_slot_default.ABCS_raw_score = SCORE(SCORE_MINUS_INFINITY);
    alpha_beta_cache_slot_default.ABCS_score = SCORE(SCORE_MINUS_INFINITY);

    ui32_t abcs_crc32 = abcs2crc32(alpha_beta_cache_slot_default.ABCS_key,
                                   alpha_beta_cache_slot_default.ABCS_data,
                                   alpha_beta_cache_slot_default.ABCS_scores);

    alpha_beta_cache_slot_default.ABCS_crc32 = abcs_crc32;
    alpha_beta_cache_slot_default.ABCS_age = ABCS_AGE_MAX;

    for (int islot = 0; islot < NABCS_SLOTS; islot++)
      alpha_beta_cache_entry_default.ABCE_slots[islot] =
        alpha_beta_cache_slot_default;

    HARDBUG(networks[NETWORK0_ID].N_network_id == INVALID)

    MY_MALLOC_BY_TYPE(alpha_beta_cache[NETWORK0_ID][WHITE_ENUM],
                      alpha_beta_cache_entry_t,
                      nalpha_beta_cache_entries)

    clear_alpha_beta_cache(alpha_beta_cache[NETWORK0_ID][WHITE_ENUM]);

    MY_MALLOC_BY_TYPE(alpha_beta_cache[NETWORK0_ID][BLACK_ENUM],
                      alpha_beta_cache_entry_t,
                      nalpha_beta_cache_entries)

    clear_alpha_beta_cache(alpha_beta_cache[NETWORK0_ID][BLACK_ENUM]);

    if (networks[NETWORK1_ID].N_network_id != INVALID)
    {
      MY_MALLOC_BY_TYPE(alpha_beta_cache[NETWORK1_ID][WHITE_ENUM],
                        alpha_beta_cache_entry_t,
                        nalpha_beta_cache_entries)

      clear_alpha_beta_cache(alpha_beta_cache[NETWORK1_ID][WHITE_ENUM]);

      MY_MALLOC_BY_TYPE(alpha_beta_cache[NETWORK1_ID][BLACK_ENUM],
                        alpha_beta_cache_entry_t,
                        nalpha_beta_cache_entries)

      clear_alpha_beta_cache(alpha_beta_cache[NETWORK1_ID][BLACK_ENUM]);
    }
  }

  for (int islot = 0; islot < NPV_SLOTS; islot++)
    pv_entry_default.PVE_slots[islot] = PV_SLOT_MOVES_MASK;

  for (int ientry = 0; ientry < NPV_ENTRIES; ientry++)
    pv_entries_default.PVE_entries[ientry] = pv_entry_default;

  construct_bucket(&bucket_depth,
                   "bucket_depth",
                   1,
                   0,
                   DEPTH_MAX,
                   BUCKET_LINEAR);

#ifdef LAZY_STATS
  for (int ndelta_man = -NPIECES_MAX; ndelta_man <= NPIECES_MAX; ++ndelta_man)
  {
    for (int ndelta_king = -NPIECES_MAX; ndelta_king <= NPIECES_MAX;
         ++ndelta_king)
    {
      construct_stats(
        &(lazy_stats[ndelta_man + NPIECES_MAX][ndelta_king + NPIECES_MAX]),
        "lazy_stats");
    }
  }
#endif
}

void fin_search(void) {}
