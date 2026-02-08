// SCU REVISION 8.1178 zo  8 feb 2026  6:06:03 CET
#include "globals.h"

#define FAST_MODULO64(H, N) ((ui64_t)(((__uint128_t)(H) * (N)) >> 64));

#define TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT BIT(0)
#define TWEAK_PREVIOUS_MOVE_NULL_BIT BIT(1)
#define TWEAK_PREVIOUS_MOVE_REDUCED_BIT BIT(2)
#define TWEAK_PREVIOUS_MOVE_EXTENDED_BIT BIT(3)

#define STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH BIT(0)
#define STAGE_SEARCH_MOVE_AT_FULL_DEPTH BIT(1)
#define STAGE_SEARCHED_MOVE_AT_REDUCED_DEPTH BIT(2)
#define STAGE_SEARCHED_MOVE_AT_FULL_DEPTH BIT(3)

#undef TRACE_AB

#ifdef TRACE_AB

#define TRACE_AB_ENTRY(object,        \
                       arg_my_alpha,  \
                       arg_my_beta,   \
                       arg_node_type, \
                       arg_my_depth)  \
  trace_ab_entry(object,              \
                 __FILE__,            \
                 __FUNC__,            \
                 __LINE__,            \
                 arg_my_alpha,        \
                 arg_my_beta,         \
                 arg_node_type,       \
                 arg_my_depth);

#define TRACE_AB_CACHE(object, arg_info, arg_alpha_beta_cache_slot) \
  trace_ab_cache(object,                                            \
                 __FILE__,                                          \
                 __FUNC__,                                          \
                 __LINE__,                                          \
                 arg_info,                                          \
                 arg_alpha_beta_cache_slot);

#define TRACE_KEY(object) ui64_t trace_key = object->S_board.B_key;

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

#define TRACE_AB_EXIT(object,          \
                      arg_my_alpha,    \
                      arg_my_beta,     \
                      arg_node_type,   \
                      arg_my_depth,    \
                      arg_best_result) \
  trace_ab_exit(object,                \
                __FILE__,              \
                __FUNC__,              \
                __LINE__,              \
                arg_my_alpha,          \
                arg_my_beta,           \
                arg_node_type,         \
                arg_my_depth,          \
                arg_best_result);

#else

#define TRACE_AB_ENTRY(object,        \
                       arg_my_alpha,  \
                       arg_my_beta,   \
                       arg_node_type, \
                       arg_my_depth)
#define TRACE_AB_CACHE(object, arg_info, arg_alpha_beta_cache_slot)
#define TRACE_KEY(object)
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
#define TRACE_AB_EXIT(object,        \
                      arg_my_alpha,  \
                      arg_my_beta,   \
                      arg_node_type, \
                      arg_my_depth,  \
                      arg_best_result)

#endif

#undef CHECK_TIME_LIMIT_IN_QUIESCENCE

#define UPDATE_NODES 1000

#define NMOVES 4
#define NSLOTS 4

typedef struct
{
  ui64_t ABCS_key;
  union
  {
    ui64_t ABCS_data;
    struct
    {
      i16_t ABCS_score;
      i8_t ABCS_depth;
      i8_t ABCS_flags;
      i8_t ABCS_moves[NMOVES];
    };
  };
  ui64_t ABCS_pad;
  ui64_t ABCS_crc64;
} alpha_beta_cache_slot_t;

typedef struct
{
  alpha_beta_cache_slot_t ABCE_slots[NSLOTS];
} alpha_beta_cache_entry_t;

local i64_t nalpha_beta_pv_cache_entries;
local i64_t nalpha_beta_cache_entries;
local alpha_beta_cache_slot_t alpha_beta_cache_slot_default;
local alpha_beta_cache_entry_t alpha_beta_cache_entry_default;
local alpha_beta_cache_entry_t *alpha_beta_cache[NCOLOUR_ENUM];

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

int draw_by_repetition(board_t *object, int arg_strict)
{
  int result = FALSE;

  int npieces = return_npieces(object);

  int n = 1;

  for (int istate = object->B_inode - 2; istate >= 0; istate -= 2)
  {
    if (object->B_states[istate + 1].BS_npieces != npieces)
      goto label_return;

    if (object->B_states[istate].BS_npieces != npieces)
      goto label_return;

    if (HASH_KEY_EQ(object->B_states[istate].BS_key, object->B_key))
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

local inline ui64_t return_crc64_from_crc32(alpha_beta_cache_slot_t *arg_abcs)
{
  ui64_t crc64;

  ui32_t crc32 = 0xFFFFFFFF;
  crc32 = HW_CRC32_U64(crc32, arg_abcs->ABCS_key);
  crc32 = HW_CRC32_U64(crc32, arg_abcs->ABCS_data);
  crc64 = ~crc32;

  crc32 = 0x0;
  crc32 = HW_CRC32_U64(crc32, arg_abcs->ABCS_data);
  crc32 = HW_CRC32_U64(crc32, arg_abcs->ABCS_key);
  crc64 = (crc64 << 32) | ~crc32;

  return (crc64);
}

local int move_repetition(board_t *object)
{
  if ((object->B_inode - 2) < 0)
    return (FALSE);

  int npieces;

  if (IS_WHITE(object->B_colour2move))
  {
    npieces = BIT_COUNT(object->B_white_man_bb | object->B_white_king_bb);
  }
  else
  {
    npieces = BIT_COUNT(object->B_black_man_bb | object->B_black_king_bb);
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

  for (int jnode = object->B_inode - 4; jnode >= 0; jnode -= 2)
  {
    if (HASH_KEY_EQ(object->B_nodes[object->B_inode - 2].node_move_key,
                    object->B_nodes[jnode].node_move_key))
    {
      ++n;

      if (n >= 3)
      {
        //++(with->S_total_move_repetitions);

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

void clear_totals(search_t *object)
{
  object->S_total_move_repetitions = 0;

  object->S_total_quiescence_nodes = 0;
  object->S_total_quiescence_all_moves_captures_only = 0;
  object->S_total_quiescence_all_moves_le2_moves = 0;
  object->S_total_quiescence_all_moves_combination = 0;
  object->S_total_quiescence_all_moves_extended = 0;

  object->S_total_nodes = 0;
  object->S_total_alpha_beta_nodes = 0;
  object->S_total_capture_nodes = 0;
  object->S_total_minimal_window_nodes = 0;
  object->S_total_pv_nodes = 0;

  object->S_total_quiescence_extension_searches = 0;
  object->S_total_quiescence_extension_searches_combination = 0;
  object->S_total_quiescence_extension_searches_le_alpha = 0;
  object->S_total_quiescence_extension_searches_ge_beta = 0;
  object->S_total_quiescence_capture_extensions = 0;

  object->S_total_pv_extension_searches = 0;
  object->S_total_pv_extension_searches_combination = 0;
  object->S_total_pv_extension_searches_le_alpha = 0;
  object->S_total_pv_extension_searches_ge_beta = 0;

  object->S_total_reductions_combinations = 0;
  object->S_total_reductions = 0;
  object->S_total_reductions_lost = 0;
  object->S_total_reductions_le_alpha = 0;
  object->S_total_reductions_ge_beta = 0;

  object->S_total_single_reply_extensions = 0;

  object->S_total_evaluations = 0;
  object->S_total_lazy_alpha_evaluations = 0;
  object->S_total_lazy_beta_evaluations = 0;
  object->S_total_material_only_evaluations = 0;
  object->S_total_network_evaluations = 0;

  object->S_total_alpha_beta_cache_hits = 0;
  object->S_total_alpha_beta_cache_depth_hits = 0;
  object->S_total_alpha_beta_cache_le_alpha_hits = 0;
  object->S_total_alpha_beta_cache_le_alpha_cutoffs = 0;
  object->S_total_alpha_beta_cache_ge_beta_hits = 0;
  object->S_total_alpha_beta_cache_ge_beta_cutoffs = 0;
  object->S_total_alpha_beta_cache_true_score_hits = 0;
  object->S_total_alpha_beta_cache_le_alpha_stored = 0;
  object->S_total_alpha_beta_cache_ge_beta_stored = 0;
  object->S_total_alpha_beta_cache_true_score_stored = 0;
  object->S_total_alpha_beta_cache_nmoves_errors = 0;
  object->S_total_alpha_beta_cache_crc64_errors = 0;

  object->S_total_score_cache_hits = 0;
  object->S_total_score_cache_crc32_errors = 0;
}

#define PRINTF_TOTAL(X) my_printf(object->S_my_printf, #X "=%lld\n", object->X);

void print_totals(search_t *object){
  // clang-format off

  PRINTF_TOTAL(S_total_move_repetitions)

  PRINTF_TOTAL(S_total_quiescence_nodes)
  PRINTF_TOTAL(S_total_quiescence_all_moves_captures_only)
  PRINTF_TOTAL(S_total_quiescence_all_moves_le2_moves)
  PRINTF_TOTAL(S_total_quiescence_all_moves_combination)
  PRINTF_TOTAL(S_total_quiescence_all_moves_extended)

  PRINTF_TOTAL(S_total_nodes)
  PRINTF_TOTAL(S_total_alpha_beta_nodes)
  PRINTF_TOTAL(S_total_capture_nodes)
  PRINTF_TOTAL(S_total_minimal_window_nodes)
  PRINTF_TOTAL(S_total_pv_nodes)

  PRINTF_TOTAL(S_total_quiescence_extension_searches)
  PRINTF_TOTAL(S_total_quiescence_extension_searches_combination)
  PRINTF_TOTAL(S_total_quiescence_extension_searches_le_alpha)
  PRINTF_TOTAL(S_total_quiescence_extension_searches_ge_beta)
  PRINTF_TOTAL(S_total_quiescence_capture_extensions)

  PRINTF_TOTAL(S_total_pv_extension_searches)
  PRINTF_TOTAL(S_total_pv_extension_searches_combination)
  PRINTF_TOTAL(S_total_pv_extension_searches_le_alpha)
  PRINTF_TOTAL(S_total_pv_extension_searches_ge_beta)

  PRINTF_TOTAL(S_total_reductions_combinations)
  PRINTF_TOTAL(S_total_reductions)
  PRINTF_TOTAL(S_total_reductions_lost)
  PRINTF_TOTAL(S_total_reductions_le_alpha)
  PRINTF_TOTAL(S_total_reductions_ge_beta)

  PRINTF_TOTAL(S_total_single_reply_extensions)

  PRINTF_TOTAL(S_total_evaluations)
  PRINTF_TOTAL(S_total_lazy_alpha_evaluations)
  PRINTF_TOTAL(S_total_lazy_beta_evaluations)
  PRINTF_TOTAL(S_total_material_only_evaluations)
  PRINTF_TOTAL(S_total_network_evaluations)

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
  PRINTF_TOTAL(S_total_alpha_beta_cache_crc64_errors)

  PRINTF_TOTAL(S_total_score_cache_hits)
  PRINTF_TOTAL(S_total_score_cache_crc32_errors)

  // clang-format on
}

local
  int probe_alpha_beta_cache(search_t *object,
                             int arg_node_type,
                             int arg_pv_only,
                             alpha_beta_cache_entry_t *arg_alpha_beta_cache,
                             alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot)
{
  int result = FALSE;

  alpha_beta_cache_entry_t *with = NULL;

  if (IS_PV(arg_node_type))
  {
    with = arg_alpha_beta_cache +
           FAST_MODULO64(object->S_board.B_key, nalpha_beta_pv_cache_entries);

    int islot;

    for (islot = 0; islot < NSLOTS; islot++)
    {
      *arg_alpha_beta_cache_slot = with->ABCE_slots[islot];

      ui64_t crc64 = return_crc64_from_crc32(arg_alpha_beta_cache_slot);

      if (crc64 != arg_alpha_beta_cache_slot->ABCS_crc64)
      {
        object->S_total_alpha_beta_cache_crc64_errors++;

        with->ABCE_slots[islot] = alpha_beta_cache_slot_default;
      }
      else if (arg_alpha_beta_cache_slot->ABCS_key == object->S_board.B_key)
      {
        result = TRUE;

        break;
      }
    }
  }

  if (!result and !arg_pv_only)
  {
    with = arg_alpha_beta_cache + nalpha_beta_pv_cache_entries +
           FAST_MODULO64(object->S_board.B_key, nalpha_beta_cache_entries);

#ifdef DEBUG
    for (int idebug = 0; idebug < NSLOTS; idebug++)
    {
      if (with->ABCE_slots[idebug].ABCS_key == 0)
        continue;

      int ndebug = 0;

      for (int jdebug = 0; jdebug < NSLOTS; jdebug++)
      {
        if (with->ABCE_slots[idebug].ABCS_key ==
            with->ABCE_slots[jdebug].ABCS_key)
          ndebug++;
      }
      HARDBUG(ndebug != 1)
    }
#endif

    int islot;

    for (islot = 0; islot < NSLOTS; islot++)
    {
      *arg_alpha_beta_cache_slot = with->ABCE_slots[islot];

      ui64_t crc64 = return_crc64_from_crc32(arg_alpha_beta_cache_slot);

      if (crc64 != arg_alpha_beta_cache_slot->ABCS_crc64)
      {
        object->S_total_alpha_beta_cache_crc64_errors++;

        with->ABCE_slots[islot] = alpha_beta_cache_slot_default;
      }
      else if (arg_alpha_beta_cache_slot->ABCS_key == object->S_board.B_key)
      {
        result = TRUE;

        break;
      }
    }
  }

  if (!result)
    *arg_alpha_beta_cache_slot = alpha_beta_cache_slot_default;

  return (result);
}

local void
update_alpha_beta_cache(search_t *object,
                        int arg_node_type,
                        alpha_beta_cache_entry_t *arg_alpha_beta_cache,
                        alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot)
{
  alpha_beta_cache_entry_t *with = NULL;

  if (IS_PV(arg_node_type))
  {
    with = arg_alpha_beta_cache +
           FAST_MODULO64(object->S_board.B_key, nalpha_beta_pv_cache_entries);
  }
  else
  {
    with = arg_alpha_beta_cache + nalpha_beta_pv_cache_entries +
           FAST_MODULO64(object->S_board.B_key, nalpha_beta_cache_entries);
  }

  int islot = INVALID;

  for (islot = 0; islot < NSLOTS; islot++)
    if (with->ABCE_slots[islot].ABCS_key == object->S_board.B_key)
      break;

  if (islot == NSLOTS)
    islot = NSLOTS - 1;

  while (islot > 0)
  {
    with->ABCE_slots[islot] = with->ABCE_slots[islot - 1];

    --islot;
  }

  with->ABCE_slots[0] = *arg_alpha_beta_cache_slot;
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

local int probe_pv(pv_entries_t *arg_pv, ui64_t arg_key)
{
  int result = INVALID;

  ui32_t fmix32 = (ui32_t)return_fmix64(arg_key);

  int ientry = fmix32 & (NPV_ENTRIES - 1);

  ui64_t *pve_slots = arg_pv->PVE_entries[ientry].PVE_slots;

  for (int islot = 0; islot < NPV_SLOTS; islot++)
  {
    if ((pve_slots[islot] >> 8) == (arg_key >> 8))
    {
      result = pve_slots[islot] & PV_SLOT_IMOVE_MASK;
      break;
    }
  }
  return (result);
}

local void update_pv(pv_entries_t *arg_pv, ui64_t arg_key, int arg_imove)
{
  ui32_t fmix32 = (ui32_t)return_fmix64(arg_key);

  int ientry = fmix32 & (NPV_ENTRIES - 1);

  ui64_t *pve_slots = arg_pv->PVE_entries[ientry].PVE_slots;

  int islot;

  for (islot = 0; islot < NPV_SLOTS; islot++)
    if ((pve_slots[islot] >> 8) == (arg_key >> 8))
      break;

  if (islot == NPV_SLOTS)
  {
    for (islot = 0; islot < NPV_SLOTS; islot++)
      if (pve_slots[islot] == PV_SLOT_IMOVE_MASK)
        break;
  }

  if (islot == NPV_SLOTS)
    islot = (fmix32 >> 30) & (NPV_SLOTS - 1);

  pve_slots[islot] = (arg_key & ~PV_SLOT_IMOVE_MASK) | (ui64_t) arg_imove;
}

local void endgame_pv(search_t *object, int arg_mate, bstring arg_bpv_string)
{
  colour_enum your_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    your_colour = BLACK_ENUM;
  else
    your_colour = WHITE_ENUM;

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(object->S_board), &moves_list);

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
    do_move(&(object->S_board), imove, &moves_list, FALSE);

    int your_mate = read_endgame(object, your_colour, NULL);

    undo_move(&(object->S_board), imove, &moves_list, FALSE);

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
    print_board(&(object->S_board));

    my_printf(object->S_my_printf,
              "best_mate=%d mate=%d\n",
              best_mate,
              arg_mate);

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      do_move(&(object->S_board), imove, &moves_list, FALSE);

      int your_mate = read_endgame(object, your_colour, NULL);

      undo_move(&(object->S_board), imove, &moves_list, FALSE);

      BSTRING(bmove_string)

      move2bstring(&moves_list, imove, bmove_string);

      my_printf(object->S_my_printf,
                "move=%s your_mate=%d\n",
                bdata(bmove_string),
                your_mate);

      BDESTROY(bmove_string)
    }
    my_printf(object->S_my_printf, "$");

    FATAL("best_mate != mate", EXIT_FAILURE)
  }

  HARDBUG(bcatcstr(arg_bpv_string, " ") == BSTR_ERR)

  BSTRING(bmove_string)

  move2bstring(&moves_list, ibest_move, bmove_string);

  bconcat(arg_bpv_string, bmove_string);

  BDESTROY(bmove_string)

  do_move(&(object->S_board), ibest_move, &moves_list, FALSE);

  int temp_mate;

  if (arg_mate > 0)
    temp_mate = -arg_mate + 1;
  else
    temp_mate = -arg_mate - 1;

  endgame_pv(object, temp_mate, arg_bpv_string);

  undo_move(&(object->S_board), ibest_move, &moves_list, FALSE);
}

local void inline flip_score(int *arg_score)
{
  if (!IS_SCORE_NAN(*arg_score))
    *arg_score = -*arg_score;
}

local void quiescence(search_t *object,
                      int arg_my_alpha,
                      int arg_my_beta,
                      int arg_node_type,
                      moves_list_t *arg_moves_list,
                      int arg_all_moves,
                      int arg_ncapture_extensions,
                      ui64_t arg_history,
                      int *arg_best_score,
                      pv_entries_t *arg_best_pv)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and
          ((arg_my_beta - arg_my_alpha) != 1))

  colour_enum my_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  if ((object->S_board.B_inode - object->S_board.B_root_inode) >= 128)
  {
    my_printf(object->S_my_printf,
              "quiescence inode=%d\n",
              object->S_board.B_inode - object->S_board.B_root_inode);
    print_board(&(object->S_board));
    my_printf(object->S_my_printf,
              "my_alpha=%d my_beta=%d node_type=%d\n",
              arg_my_alpha,
              arg_my_beta,
              arg_node_type);
  }

  *arg_best_score = SCORE_MINUS_INFINITY;

  ++(object->S_total_nodes);

  ++(object->S_total_quiescence_nodes);

  if ((object->S_total_quiescence_nodes % UPDATE_NODES) == 0)
  {
    // only thread[0] should check the time limit

    if (object->S_thread == thread_alpha_beta_master)
    {
      // Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      // struct timespec tv;

      // HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      // check for input

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        my_printf(object->S_my_printf, "got message\n");

        *arg_best_score = SCORE_NOT_A_NUMBER;

        goto label_return;
      }

      if (return_my_timer(&(object->S_timer), FALSE) >= options.time_limit)
      {
        if ((object->S_best_score < 0) and
            ((object->S_best_score - object->S_root_score) < (-SCORE_MAN / 4)))
        {
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(object->S_my_printf,
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
      if (return_my_timer(&(object->S_timer), FALSE) >= options.time_limit)
      {
        *arg_best_score = SCORE_NOT_A_NUMBER;

        goto label_return;
      }
    }
    else
    {
      HARDBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        *arg_best_score = SCORE_NOT_A_NUMBER;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    *arg_best_score = 0;

    goto label_return;
  }

  // check endgame

  int wdl;

  int temp_mate = read_endgame(object, my_colour, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    if (temp_mate == INVALID)
    {
      *arg_best_score = 0;
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        *arg_best_score = SCORE_WON - object->S_board.B_inode - temp_mate;
      else
        *arg_best_score =
          SCORE_WON_ABSOLUTE - object->S_board.B_inode - temp_mate;
    }
    else
    {
      if (wdl)
        *arg_best_score = SCORE_LOST + object->S_board.B_inode - temp_mate;
      else
        *arg_best_score =
          SCORE_LOST_ABSOLUTE + object->S_board.B_inode - temp_mate;
    }

    goto label_return;
  }

  moves_list_t moves_list_temp;

  if (arg_moves_list == NULL)
  {
    construct_moves_list(&moves_list_temp);

    gen_moves(&(object->S_board), &moves_list_temp);

    arg_moves_list = &moves_list_temp;
  }

  arg_history = (arg_history << 4) | (arg_moves_list->ML_ncaptx & 0xF) |
                0xF000000000000000ULL;

  if (arg_moves_list->ML_nmoves == 0)
  {
    *arg_best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode;

    goto label_return;
  }

  int nextend = arg_moves_list->ML_nmoves;

  if (!arg_all_moves)
  {
    if (arg_moves_list->ML_ncaptx > 0)
    {
      ++(object->S_total_quiescence_all_moves_captures_only);

      arg_all_moves = TRUE;
    }
    else if ((arg_moves_list->ML_nmoves <= 2) and
             !move_repetition(&(object->S_board)))
    {
      ++(object->S_total_quiescence_all_moves_le2_moves);

      arg_all_moves = TRUE;
    }
    else if ((options.quiescence_combination_length > 0) and
             (return_combination_length(arg_history) >=
              options.quiescence_combination_length))
    {
      ++(object->S_total_quiescence_all_moves_combination);

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
        ++(object->S_total_quiescence_all_moves_extended);

        arg_all_moves = TRUE;
      }
    }
  }

  if (!arg_all_moves and IS_PV(arg_node_type) and !IS_SCORE_LOST(arg_my_alpha))
  {
    object->S_total_quiescence_extension_searches++;

    if ((options.quiescence_combination_length > 0) and
        (return_combination_length(arg_history) >=
         options.quiescence_combination_length))
    {
      object->S_total_quiescence_extension_searches_combination++;

      nextend = arg_moves_list->ML_nmoves;

      arg_all_moves = TRUE;
    }
    else if (options.quiescence_extension_search_window > 0)
    {
      int reduced_alpha =
        arg_my_alpha - options.quiescence_extension_search_window;
      int reduced_beta = reduced_alpha + 1;

      int temp_score;

      pv_entries_t temp_pv = *arg_best_pv;

      quiescence(object,
                 reduced_alpha,
                 reduced_beta,
                 MINIMAL_WINDOW_BIT,
                 arg_moves_list,
                 TRUE,
                 arg_ncapture_extensions,
                 arg_history,
                 &temp_score,
                 &temp_pv);

      if (IS_SCORE_NAN(temp_score))
      {
        *arg_best_score = SCORE_NOT_A_NUMBER;

        goto label_return;
      }

      if (temp_score <= reduced_alpha)
      {
        object->S_total_quiescence_extension_searches_le_alpha++;

        nextend = arg_moves_list->ML_nmoves;

        arg_all_moves = TRUE;
      }
      else
      {
        object->S_total_quiescence_extension_searches_ge_beta++;
      }
    }
  }

  if ((arg_ncapture_extensions > 0) and (!arg_all_moves) and (nextend == 0))
  {
    if (can_capture(&(object->S_board), TRUE))
    {
      ++(object->S_total_quiescence_capture_extensions);

      arg_ncapture_extensions--;

      nextend = arg_moves_list->ML_nmoves;

      arg_all_moves = TRUE;
    }
  }

  if ((nextend == 0) or
      (!arg_all_moves and (options.quiescence_evaluation_policy == 0)))
  {
    ++(object->S_total_evaluations);

    *arg_best_score = return_score(object);

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

    int temp_alpha;

    if (*arg_best_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = *arg_best_score;

    int temp_score = SCORE_MINUS_INFINITY;

    pv_entries_t temp_pv = *arg_best_pv;

    if (IS_MINIMAL_WINDOW(arg_node_type))
    {
      do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      quiescence(object,
                 -arg_my_beta,
                 -temp_alpha,
                 arg_node_type,
                 NULL,
                 FALSE,
                 arg_ncapture_extensions,
                 arg_history,
                 &temp_score,
                 &temp_pv);

      flip_score(&temp_score);

      undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);
    }
    else
    {
      if (imove == 0)
      {
        do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        quiescence(object,
                   -arg_my_beta,
                   -temp_alpha,
                   arg_node_type,
                   NULL,
                   FALSE,
                   arg_ncapture_extensions,
                   arg_history,
                   &temp_score,
                   &temp_pv);

        flip_score(&temp_score);

        undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        quiescence(object,
                   -temp_beta,
                   -temp_alpha,
                   MINIMAL_WINDOW_BIT,
                   NULL,
                   FALSE,
                   arg_ncapture_extensions,
                   arg_history,
                   &temp_score,
                   &temp_pv);

        flip_score(&temp_score);

        if (!IS_SCORE_NAN(temp_score) and (temp_score >= temp_beta) and
            (temp_score < arg_my_beta))
        {
          quiescence(object,
                     -arg_my_beta,
                     -temp_alpha,
                     arg_node_type,
                     NULL,
                     FALSE,
                     arg_ncapture_extensions,
                     arg_history,
                     &temp_score,
                     &temp_pv);

          flip_score(&temp_score);
        }

        undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);
      }
    }

    if (IS_SCORE_NAN(temp_score))
    {
      *arg_best_score = SCORE_NOT_A_NUMBER;

      goto label_return;
    }

    HARDBUG(temp_score == SCORE_MINUS_INFINITY)

    if (temp_score > *arg_best_score)
    {
      *arg_best_score = temp_score;

      update_pv(&temp_pv, object->S_board.B_key, jmove);

      *arg_best_pv = temp_pv;
    }

    if (*arg_best_score >= arg_my_beta)
      break;
  } // imove

  HARDBUG(*arg_best_score == SCORE_MINUS_INFINITY)

  if ((options.quiescence_evaluation_policy > 0) and
      (*arg_best_score < arg_my_beta))
  {
    ++(object->S_total_evaluations);

    int temp_score = return_score(object);

    if (temp_score > *arg_best_score)
      *arg_best_score = temp_score;
  }

label_return:

  if (!IS_SCORE_NAN(*arg_best_score) and (*arg_best_score >= arg_my_beta))
    *arg_best_score = arg_my_beta;

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void
construct_search_result(search_result_t *self, int arg_score, int arg_flags)
{
  search_result_t *object = self;

  object->SR_score = arg_score;
  object->SR_flags = arg_flags;
  object->SR_depth = INVALID;
}

local void flip_search_result(search_result_t *arg_search_result)
{
  if (!IS_SCORE_NAN(arg_search_result->SR_score))
  {
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
  a->SR_score = b->SR_score;
  a->SR_flags = b->SR_flags;

  // HARDBUG((a->SR_score != 0) and IS_SR_DRAW_BY_REP(a->SR_flags))
}

local void trace_ab_entry(search_t *object,
                          char *file,
                          const char *func,
                          int line,
                          int arg_my_alpha,
                          int arg_my_beta,
                          int arg_node_type,
                          int arg_my_depth)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  char *colour2move;

  if (IS_WHITE(object->S_board.B_colour2move))
    colour2move = "WTM";
  else
    colour2move = "BTM";

  PRINTF("(%ld,%s %s)ab_entry:::%s::%s:%d"
         " key=%llx arg_my_alpha=%d arg_my_beta=%d"
         " arg_node_type=%d arg_my_depth=%d\n",
         ++trace_line,
         bdata(trace_bpv_string),
         colour2move,
         file,
         func,
         line,
         object->S_board.B_key,
         arg_my_alpha,
         arg_my_beta,
         arg_node_type,
         arg_my_depth);
}

local void trace_ab_cache(search_t *object,
                          char *file,
                          const char *func,
                          int line,
                          char *arg_info,
                          alpha_beta_cache_slot_t *arg_cache_slot)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  moves_list_t moves_list;

  gen_moves(&(object->S_board), &moves_list);

  BSTRING(bmove_string)

  int ABCS_score = arg_cache_slot->ABCS_score;
  int ABCS_depth = arg_cache_slot->ABCS_depth;
  int ABCS_flags = arg_cache_slot->ABCS_flags;
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
         " ABCS_score=%d ABCS_depth=%d ABCS_flags=%s ABCS_move=%s\n",
         ++trace_line,
         bdata(trace_bpv_string),
         file,
         func,
         line,
         object->S_board.B_key,
         arg_info,
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
                           int arg_alpha,
                           int arg_beta,
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

  PRINTF("(%ld,%s)ab_window:::%s::%s:%d"
         " arg_trace_key=%llx arg_info=%s arg_alpha=%d arg_beta=%d"
         " arg_jmove=%s arg_result->SR_score=%d arg_result->SR_flags=%s"
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
         arg_result->SR_score,
         bdata(bSR_flags),
         arg_result->SR_depth);

  BDESTROY(bSR_flags)

  BDESTROY(bmove_string)
}

local void trace_ab_exit(search_t *object,
                         char *file,
                         const char *func,
                         int line,
                         int arg_my_alpha,
                         int arg_my_beta,
                         int arg_node_type,
                         int arg_my_depth,
                         search_result_t *arg_best_result)
{
  for (int idepth = 0; idepth < trace_depth; idepth++)
    PRINTF(".");

  char *colour2move;

  if (IS_WHITE(object->S_board.B_colour2move))
    colour2move = "WTM";
  else
    colour2move = "BTM";

  moves_list_t moves_list;

  gen_moves(&(object->S_board), &moves_list);

  BSTRING(bmove_string)

  int best_move = probe_pv(&(arg_best_result->SR_pv), object->S_board.B_key);

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
         " key=%llx arg_my_alpha=%d arg_my_beta=%d "
         " arg_node_type=%d arg_my_depth=%d"
         " arg_best_result->SR_score=%d arg_best_result->SR_flags=%s"
         " arg_best_result->SR_pv[0]=%s arg_best_result->SR_depth=%d\n",
         ++trace_line,
         bdata(trace_bpv_string),
         colour2move,
         bdata(bmove_string),
         file,
         func,
         line,
         object->S_board.B_key,
         arg_my_alpha,
         arg_my_beta,
         arg_node_type,
         arg_my_depth,
         arg_best_result->SR_score,
         bdata(bSR_flags),
         bdata(bmove_string),
         arg_best_result->SR_depth);

  BDESTROY(bSR_flags)

  BDESTROY(bmove_string)
}

local void inline fail_hard(int arg_my_alpha,
                            int arg_my_beta,
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

local void inline check_search_result(int arg_my_alpha,
                                      int arg_my_beta,
                                      search_result_t *arg_search_result)
{
  int ok = TRUE;

  if (IS_SR_GE_BETA(arg_search_result->SR_flags) and
      (arg_search_result->SR_score < arg_my_beta))
    ok = FALSE;

  if (IS_SR_LE_ALPHA(arg_search_result->SR_flags) and
      (arg_search_result->SR_score > arg_my_alpha))
    ok = FALSE;

  if (IS_SR_TRUE_SCORE(arg_search_result->SR_flags) and
      ((arg_search_result->SR_score <= arg_my_alpha) or
       (arg_search_result->SR_score >= arg_my_beta)))
    ok = FALSE;

  if (!ok)
  {
    BSTRING(bSR_flags)

    binary(bSR_flags, arg_search_result->SR_flags, 8);

    PRINTF("arg_my_alpha=%d"
           " arg_my_beta=%d"
           " arg_search_result->SR_score=%d"
           " arg_search_result->SR_flags=%s\n",
           arg_my_alpha,
           arg_my_beta,
           arg_search_result->SR_score,
           bdata(bSR_flags));

    FATAL("eh?", EXIT_FAILURE)
  }
}

local void
alpha_beta(search_t *, int, int, int, int, int, int, ui64_t, search_result_t *);

local void alpha_beta_captures(search_t *object,
                               int arg_my_alpha,
                               int arg_my_beta,
                               int arg_my_depth,
                               int arg_node_type,
                               int arg_reduction_depth_root,
                               int arg_my_tweaks,
                               moves_list_t *arg_moves_list,
                               int arg_sort[NMOVES_MAX],
                               ui64_t arg_history,
                               search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(object, arg_my_alpha, arg_my_beta, arg_node_type, arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 0)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and
          ((arg_my_beta - arg_my_alpha) != 1))

  HARDBUG(arg_history == 0)

  ++(object->S_total_nodes);

  ++(object->S_total_capture_nodes);

  HARDBUG(arg_moves_list->ML_nmoves == 0)

  construct_search_result(arg_search_result, SCORE_MINUS_INFINITY, SR_NULL);

  int your_tweaks = 0;

  /*
    if (((moves_list.ML_nmoves <= 3) and !move_repetition(&(object->S_board)))
    or (options.captures_are_transparent and (moves_list.ML_ncaptx > 0)))
  */

  int nproven = 0;

  search_result_t temp_result;

  temp_result.SR_pv = arg_search_result->SR_pv;

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = arg_sort[imove];

    int temp_alpha;

    if (arg_search_result->SR_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = arg_search_result->SR_score;

    construct_search_result(&temp_result, SCORE_MINUS_INFINITY, SR_NULL);

    TRACE_KEY(object)

    do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

    if (IS_MINIMAL_WINDOW(arg_node_type))
    {
      TRACE_AB_WINDOW(trace_key,
                      "minimal window search",
                      temp_alpha,
                      arg_my_beta,
                      arg_moves_list,
                      jmove)

      TRACE_INDENT(arg_moves_list, jmove)

      alpha_beta(object,
                 -arg_my_beta,
                 -temp_alpha,
                 arg_my_depth,
                 arg_node_type,
                 arg_reduction_depth_root,
                 your_tweaks,
                 arg_history,
                 &temp_result);

      TRACE_DEDENT

      flip_search_result(&temp_result);

      TRACE_AB_RESULT(trace_key,
                      "result minimal window search",
                      temp_alpha,
                      arg_my_beta,
                      arg_moves_list,
                      jmove,
                      &temp_result);
    }
    else
    {
      if (imove == 0)
      {
        TRACE_AB_WINDOW(trace_key,
                        "full window search first move",
                        temp_alpha,
                        arg_my_beta,
                        arg_moves_list,
                        jmove)

        TRACE_INDENT(arg_moves_list, jmove)

        alpha_beta(object,
                   -arg_my_beta,
                   -temp_alpha,
                   arg_my_depth,
                   arg_node_type,
                   arg_reduction_depth_root,
                   your_tweaks,
                   arg_history,
                   &temp_result);

        TRACE_DEDENT

        flip_search_result(&temp_result);

        TRACE_AB_RESULT(trace_key,
                        "result full window search first move",
                        temp_alpha,
                        arg_my_beta,
                        arg_moves_list,
                        jmove,
                        &temp_result);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        TRACE_AB_WINDOW(trace_key,
                        "minimal window search next move",
                        temp_alpha,
                        temp_beta,
                        arg_moves_list,
                        jmove)

        TRACE_INDENT(arg_moves_list, jmove)

        alpha_beta(object,
                   -temp_beta,
                   -temp_alpha,
                   arg_my_depth,
                   MINIMAL_WINDOW_BIT,
                   arg_reduction_depth_root,
                   your_tweaks,
                   arg_history,
                   &temp_result);

        TRACE_DEDENT

        flip_search_result(&temp_result);

        TRACE_AB_RESULT(trace_key,
                        "result minimal window search next move",
                        temp_alpha,
                        temp_beta,
                        arg_moves_list,
                        jmove,
                        &temp_result);

        if (!IS_SCORE_NAN(temp_result.SR_score) and
            (temp_result.SR_score >= temp_beta) and
            (temp_result.SR_score < arg_my_beta))
        {
          TRACE_AB_WINDOW(trace_key,
                          "fail high search next move",
                          temp_alpha,
                          arg_my_beta,
                          arg_moves_list,
                          jmove)

          TRACE_INDENT(arg_moves_list, jmove)

          alpha_beta(object,
                     -arg_my_beta,
                     -temp_alpha,
                     arg_my_depth,
                     arg_node_type,
                     arg_reduction_depth_root,
                     your_tweaks,
                     arg_history,
                     &temp_result);

          TRACE_DEDENT

          flip_search_result(&temp_result);

          TRACE_AB_RESULT(trace_key,
                          "result fail high search next move",
                          temp_alpha,
                          arg_my_beta,
                          arg_moves_list,
                          jmove,
                          &temp_result);
        }
      }
    }

    undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);

    if (IS_SCORE_NAN(temp_result.SR_score))
    {
      construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

      goto label_return;
    }

    if (temp_result.SR_depth == DEPTH_MAX)
      ++nproven;

    if (compare_search_results(&temp_result, SR_OPERATOR_GT, arg_search_result))
    {
      assign_search_result_b2a(arg_search_result, &temp_result);

      update_pv(&(temp_result.SR_pv), object->S_board.B_key, jmove);

      arg_search_result->SR_pv = temp_result.SR_pv;

      arg_search_result->SR_depth = temp_result.SR_depth;

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
  }

  HARDBUG(arg_search_result->SR_depth == INVALID)

  if ((nproven < arg_moves_list->ML_nmoves) and
      (arg_search_result->SR_depth == DEPTH_MAX) and
      !IS_SR_LOST(arg_search_result->SR_flags) and
      !IS_SR_WON(arg_search_result->SR_flags))
  {
    // print_board(&(object->S_board));
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

  TRACE_AB_EXIT(object,
                arg_my_alpha,
                arg_my_beta,
                arg_node_type,
                arg_my_depth,
                arg_search_result);

  check_search_result(arg_my_alpha, arg_my_beta, arg_search_result);

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void alpha_beta_minimal(search_t *object,
                              int arg_my_alpha,
                              int arg_my_beta,
                              int arg_my_depth,
                              int arg_node_type,
                              int arg_reduction_depth_root,
                              int arg_my_tweaks,
                              moves_list_t *arg_moves_list,
                              int arg_sort[NMOVES_MAX],
                              ui64_t arg_history,
                              search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(object, arg_my_alpha, arg_my_beta, arg_node_type, arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 1)

  HARDBUG(arg_node_type == 0)

  HARDBUG(!IS_MINIMAL_WINDOW(arg_node_type))

  HARDBUG((arg_my_beta - arg_my_alpha) != 1)

  HARDBUG(arg_moves_list->ML_nmoves == 0)

  HARDBUG(arg_moves_list->ML_ncaptx > 0)

  HARDBUG(arg_history == 0)

  construct_search_result(arg_search_result, SCORE_MINUS_INFINITY, SR_NULL);

  colour_enum my_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  ++(object->S_total_nodes);

  ++(object->S_total_minimal_window_nodes);

  int your_depth = arg_my_depth - 1;

  HARDBUG(your_depth < 0)

  int allow_reductions =
    options.use_reductions and
    !(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
    !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_NULL_BIT) and
    !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_REDUCED_BIT) and
    !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
    (arg_moves_list->ML_ncaptx == 0) and
    (arg_moves_list->ML_nmoves >= options.reduction_moves_min) and
    (your_depth >= options.reduction_depth_leaf) and
    (arg_reduction_depth_root == 0) and !IS_SCORE_LOST(arg_my_alpha) and
    !IS_SCORE_WON(arg_my_beta);

  if (allow_reductions and (options.reduction_combination_length > 0) and
      (return_combination_length(arg_history) >=
       options.reduction_combination_length))
  {
    object->S_total_reductions_combinations++;

    allow_reductions = FALSE;
  }

  int your_tweaks = 0;

  int nmoves_searched_at_full_depth = 0;

  int reduced_depth = INVALID;

#define SCALE 1000000000LL

  if (allow_reductions)
  {
    i64_t mobility = SCALE;

    int npieces = BIT_COUNT(object->S_board.B_bit_boards[my_colour][MAN_ENUM]);

    if (npieces > 0)
    {
      mobility =
        ((i64_t)arg_moves_list->ML_nmoves * SCALE) / (2LL * (i64_t)npieces);

      if (mobility > SCALE)
        mobility = SCALE;
    }

    i64_t reduced_depth_ll =
      (SCALE - mobility) * ((i64_t)your_depth - 1L) +
      (mobility * (i64_t)your_depth * (i64_t)options.reduction_percentage) /
        100LL;

#ifdef DEBUG
    double dmobility = 1.0;

    if (npieces > 0)
    {
      dmobility = (double)arg_moves_list->ML_nmoves / (2.0 * (double)npieces);

      if (dmobility > 1.0)
        dmobility = 1.0;
    }

    double dreduced_depth = (1.0 - dmobility) * ((double)your_depth - 1.0) +
                            dmobility * (double)your_depth *
                              (double)options.reduction_percentage / 100.0d;

    if (llabs(reduced_depth_ll - llround(dreduced_depth * (double)SCALE)) > 10)
    {
      PRINTF("reduced_depth_ll=%lld dreduced_depth=%.9f"
             " llround(dreduced_depth * (double) SCALE)=%lld\n",
             reduced_depth_ll,
             dreduced_depth,
             llround(dreduced_depth * (double)SCALE));

      FATAL("eh?", EXIT_FAILURE)
    }

#endif

    reduced_depth = reduced_depth_ll / SCALE;

    if (reduced_depth < 1)
      reduced_depth = 1;
  }

  HARDBUG(allow_reductions and (reduced_depth == INVALID))

  int nproven = 0;

  search_result_t temp_result;

  temp_result.SR_pv = arg_search_result->SR_pv;

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = arg_sort[imove];

    construct_search_result(&temp_result, SCORE_MINUS_INFINITY, SR_NULL);

    TRACE_KEY(object)

    do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

    int stage = STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH;

    if (!allow_reductions or
        (nmoves_searched_at_full_depth < options.reduction_full_min) or
        MOVE_DO_NOT_REDUCE(arg_moves_list->ML_move_flags[jmove]))
    {
      stage = STAGE_SEARCH_MOVE_AT_FULL_DEPTH;
    }

    if (stage == STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH)
    {
      ++(object->S_total_reductions);

      HARDBUG(reduced_depth == INVALID)
      HARDBUG(reduced_depth < 1)

      int reduced_alpha = arg_search_result->SR_score;

      int reduced_beta = reduced_alpha + 1;

      TRACE_AB_WINDOW(trace_key,
                      "reduced depth search",
                      reduced_alpha,
                      reduced_beta,
                      arg_moves_list,
                      jmove)

      TRACE_INDENT(arg_moves_list, jmove)

      alpha_beta(object,
                 -reduced_beta,
                 -reduced_alpha,
                 reduced_depth,
                 arg_node_type,
                 arg_reduction_depth_root,
                 your_tweaks | TWEAK_PREVIOUS_MOVE_REDUCED_BIT,
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
        undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

        goto label_return;
      }

      if (compare_search_results(&temp_result,
                                 SR_OPERATOR_LE,
                                 arg_search_result))
      {
        ++(object->S_total_reductions_le_alpha);

        stage = STAGE_SEARCHED_MOVE_AT_REDUCED_DEPTH;
      }
      else
      {
        ++(object->S_total_reductions_ge_beta);

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

      alpha_beta(object,
                 -arg_my_beta,
                 -arg_my_alpha,
                 your_depth,
                 arg_node_type,
                 arg_reduction_depth_root,
                 your_tweaks,
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

    undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);

    if (IS_SCORE_NAN(temp_result.SR_score))
    {
      construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

      goto label_return;
    }

    HARDBUG(temp_result.SR_score == SCORE_MINUS_INFINITY)

    if ((stage == STAGE_SEARCHED_MOVE_AT_FULL_DEPTH) and
        (temp_result.SR_depth == DEPTH_MAX))
      ++nproven;

    if (compare_search_results(&temp_result, SR_OPERATOR_GT, arg_search_result))
    {
      HARDBUG(stage != STAGE_SEARCHED_MOVE_AT_FULL_DEPTH)

      assign_search_result_b2a(arg_search_result, &temp_result);

      update_pv(&(temp_result.SR_pv), object->S_board.B_key, jmove);

      arg_search_result->SR_pv = temp_result.SR_pv;

      if (temp_result.SR_depth < (DEPTH_MAX - 1))
        temp_result.SR_depth++;

      arg_search_result->SR_depth = temp_result.SR_depth;

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
        goto label_return;
    }
  } // for (int imove

  HARDBUG(arg_search_result->SR_depth == INVALID)

  if ((nproven < arg_moves_list->ML_nmoves) and
      (arg_search_result->SR_depth == DEPTH_MAX) and
      !IS_SR_LOST(arg_search_result->SR_flags) and
      !IS_SR_WON(arg_search_result->SR_flags))
  {
    // print_board(&(object->S_board));
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

  TRACE_AB_EXIT(object,
                arg_my_alpha,
                arg_my_beta,
                arg_node_type,
                arg_my_depth,
                arg_search_result);

  check_search_result(arg_my_alpha, arg_my_beta, arg_search_result);

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void alpha_beta_pv(search_t *object,
                         int arg_my_alpha,
                         int arg_my_beta,
                         int arg_my_depth,
                         int arg_node_type,
                         int arg_reduction_depth_root,
                         int arg_my_tweaks,
                         moves_list_t *arg_moves_list,
                         int arg_sort[NMOVES_MAX],
                         ui64_t arg_history,
                         search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(object, arg_my_alpha, arg_my_beta, arg_node_type, arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 1)

  HARDBUG(arg_node_type == 0)

  HARDBUG(!IS_PV(arg_node_type))

  HARDBUG(arg_history == 0)

  ++(object->S_total_nodes);

  ++(object->S_total_pv_nodes);

  construct_search_result(arg_search_result, SCORE_MINUS_INFINITY, SR_NULL);

  int your_tweaks = 0;

  int your_depth = arg_my_depth - 1;

  HARDBUG(your_depth < 0)

  search_result_t temp_result;

  temp_result.SR_pv = arg_search_result->SR_pv;

  if (!(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
      !IS_SCORE_LOST(arg_my_alpha))
  {
    if ((options.pv_combination_length > 0) and
        (return_combination_length(arg_history) >=
         options.pv_combination_length))
    {
      object->S_total_pv_extension_searches_combination++;

      your_depth++;

      your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;
    }
    else if (options.pv_extension_search_window > 0)
    {
      object->S_total_pv_extension_searches++;

      int reduced_alpha = arg_my_alpha - options.pv_extension_search_window;
      int reduced_beta = reduced_alpha + 1;

      int reduced_depth = your_depth;

      TRACE_KEY(object)

      TRACE_AB_WINDOW(trace_key,
                      "extension search",
                      reduced_alpha,
                      reduced_beta,
                      arg_moves_list,
                      INVALID)

      alpha_beta(object,
                 reduced_alpha,
                 reduced_beta,
                 reduced_depth,
                 MINIMAL_WINDOW_BIT,
                 arg_reduction_depth_root,
                 arg_my_tweaks | TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT,
                 arg_history,
                 &temp_result);

      TRACE_AB_RESULT(trace_key,
                      "result extension search",
                      reduced_alpha,
                      reduced_beta,
                      arg_moves_list,
                      INVALID,
                      &temp_result);

      if (IS_SCORE_NAN(temp_result.SR_score))
      {
        construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

        goto label_return;
      }

      if (temp_result.SR_score <= reduced_alpha)
      {
        object->S_total_pv_extension_searches_le_alpha++;

        your_depth = your_depth + 1;

        your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;
      }
      else
      {
        object->S_total_pv_extension_searches_ge_beta++;
      }
    }
  }

  int nproven = INVALID;

  for (int ipass = 1; ipass <= 2; ++ipass)
  {
    int nsingle_replies = 0;

    nproven = 0;

    for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    {
      int jmove = arg_sort[imove];

      int temp_alpha;

      if (arg_search_result->SR_score < arg_my_alpha)
        temp_alpha = arg_my_alpha;
      else
        temp_alpha = arg_search_result->SR_score;

      construct_search_result(&temp_result, SCORE_MINUS_INFINITY, SR_NULL);

      TRACE_KEY(object)

      do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      if (imove == 0)
      {
        TRACE_AB_WINDOW(trace_key,
                        "full window search first move",
                        temp_alpha,
                        arg_my_beta,
                        arg_moves_list,
                        jmove)

        TRACE_INDENT(arg_moves_list, jmove)

        alpha_beta(object,
                   -arg_my_beta,
                   -temp_alpha,
                   your_depth,
                   arg_node_type,
                   arg_reduction_depth_root,
                   your_tweaks,
                   arg_history,
                   &temp_result);

        TRACE_DEDENT

        flip_search_result(&temp_result);

        TRACE_AB_RESULT(trace_key,
                        "result full window search first move",
                        temp_alpha,
                        arg_my_beta,
                        arg_moves_list,
                        jmove,
                        &temp_result);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        TRACE_AB_WINDOW(trace_key,
                        "minimal window search next move",
                        temp_alpha,
                        temp_beta,
                        arg_moves_list,
                        jmove)

        TRACE_INDENT(arg_moves_list, jmove)

        alpha_beta(object,
                   -temp_beta,
                   -temp_alpha,
                   your_depth,
                   MINIMAL_WINDOW_BIT,
                   arg_reduction_depth_root,
                   your_tweaks,
                   arg_history,
                   &temp_result);

        TRACE_DEDENT

        flip_search_result(&temp_result);

        TRACE_AB_RESULT(trace_key,
                        "result minimal window search next move",
                        temp_alpha,
                        temp_beta,
                        arg_moves_list,
                        jmove,
                        &temp_result);

        if (!IS_SCORE_NAN(temp_result.SR_score) and
            (temp_result.SR_score >= temp_beta) and
            (temp_result.SR_score < arg_my_beta))
        {
          TRACE_AB_WINDOW(trace_key,
                          "fail high search next move",
                          temp_alpha,
                          arg_my_beta,
                          arg_moves_list,
                          jmove)

          TRACE_INDENT(arg_moves_list, jmove)

          alpha_beta(object,
                     -arg_my_beta,
                     -temp_alpha,
                     your_depth,
                     arg_node_type,
                     arg_reduction_depth_root,
                     your_tweaks,
                     arg_history,
                     &temp_result);

          TRACE_DEDENT

          flip_search_result(&temp_result);

          TRACE_AB_RESULT(trace_key,
                          "result fail high search next move",
                          temp_alpha,
                          arg_my_beta,
                          arg_moves_list,
                          jmove,
                          &temp_result);
        }
      }

      undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      if (IS_SCORE_NAN(temp_result.SR_score))
      {
        construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

        goto label_return;
      }

      HARDBUG(temp_result.SR_score == SCORE_MINUS_INFINITY)

      if (temp_result.SR_score > arg_my_alpha)
      {
        HARDBUG(!IS_PV(arg_node_type))

        nsingle_replies++;
      }

      if (temp_result.SR_depth == DEPTH_MAX)
        ++nproven;

      if (compare_search_results(&temp_result,
                                 SR_OPERATOR_GT,
                                 arg_search_result))
      {
        assign_search_result_b2a(arg_search_result, &temp_result);

        update_pv(&(temp_result.SR_pv), object->S_board.B_key, jmove);

        arg_search_result->SR_pv = temp_result.SR_pv;

        if (temp_result.SR_depth < (DEPTH_MAX - 1))
          temp_result.SR_depth++;

        arg_search_result->SR_depth = temp_result.SR_depth;

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
          goto label_return;
      }
    } // imove

    if ((ipass == 1) and (options.use_single_reply_extensions) and
        (nsingle_replies == 1))
    {
      object->S_total_single_reply_extensions++;

      construct_search_result(arg_search_result, SCORE_MINUS_INFINITY, SR_NULL);

      your_depth = your_depth + 1;

      your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;

      continue;
    }

    break;
  } // ipass

  HARDBUG(arg_search_result->SR_depth == INVALID)

  if ((nproven < arg_moves_list->ML_nmoves) and
      (arg_search_result->SR_depth == DEPTH_MAX) and
      !IS_SR_LOST(arg_search_result->SR_flags) and
      !IS_SR_WON(arg_search_result->SR_flags))
  {
    // print_board(&(object->S_board));
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

  TRACE_AB_EXIT(object,
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

local void alpha_beta(search_t *object,
                      int arg_my_alpha,
                      int arg_my_beta,
                      int arg_my_depth,
                      int arg_node_type,
                      int arg_reduction_depth_root,
                      int arg_my_tweaks,
                      ui64_t arg_history,
                      search_result_t *arg_search_result)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  TRACE_AB_ENTRY(object, arg_my_alpha, arg_my_beta, arg_node_type, arg_my_depth)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 0)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and
          ((arg_my_beta - arg_my_alpha) != 1))

  HARDBUG(arg_history == 0)

  colour_enum my_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  if (object->S_thread == thread_alpha_beta_master)
  {
    update_bucket(&bucket_depth, arg_my_depth);
  }

  if ((object->S_board.B_inode - object->S_board.B_root_inode) >= 128)
  {
    my_printf(object->S_my_printf,
              "alpha_beta inode=%d\n",
              object->S_board.B_inode - object->S_board.B_root_inode);
    print_board(&(object->S_board));
    my_printf(object->S_my_printf,
              "my_alpha=%d my_beta=%d my_depth=%d node_type=%d\n",
              arg_my_alpha,
              arg_my_beta,
              arg_my_depth,
              arg_node_type);
  }

  construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

  ++(object->S_total_nodes);

  ++(object->S_total_alpha_beta_nodes);

  if ((object->S_total_alpha_beta_nodes % UPDATE_NODES) == 0)
  {
    // only thread[0] should check the time limit

    if (object->S_thread == thread_alpha_beta_master)
    {
      // Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      // struct timespec tv;

      // HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      // check for input

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        my_printf(object->S_my_printf, "got message\n");

        construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

        goto label_return;
      }

      if (return_my_timer(&(object->S_timer), FALSE) >= options.time_limit)
      {
        if ((object->S_best_score < 0) and
            ((object->S_best_score - object->S_root_score) < (-SCORE_MAN / 4)))
        {
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(object->S_my_printf,
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
      if (return_my_timer(&(object->S_timer), FALSE) >= options.time_limit)
      {
        construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

        goto label_return;
      }
    }
    else
    {
      HARDBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        construct_search_result(arg_search_result, SCORE_NOT_A_NUMBER, SR_NULL);

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    construct_search_result(arg_search_result, 0, SR_DRAW_BY_REP_BIT);

    arg_search_result->SR_depth = arg_my_depth;

    goto label_return;
  }

  // check endgame

  int wdl;

  int temp_mate = read_endgame(object, my_colour, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    if (temp_mate == INVALID)
    {
      construct_search_result(arg_search_result, 0, SR_EGTB_DRAW_BIT);
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        construct_search_result(arg_search_result,
                                SCORE_WON - object->S_board.B_inode - temp_mate,
                                SR_WON_BIT);
      else
        construct_search_result(arg_search_result,
                                SCORE_WON_ABSOLUTE - object->S_board.B_inode -
                                  temp_mate,
                                SR_WON_BIT);
    }
    else
    {
      if (wdl)
        construct_search_result(arg_search_result,
                                SCORE_LOST + object->S_board.B_inode -
                                  temp_mate,
                                SR_NULL);
      else
        construct_search_result(arg_search_result,
                                SCORE_LOST_ABSOLUTE + object->S_board.B_inode -
                                  temp_mate,
                                SR_NULL);
    }

    arg_search_result->SR_depth = DEPTH_MAX;

    goto label_return;
  }

  int alpha_beta_cache_hit = FALSE;

  alpha_beta_cache_slot_t alpha_beta_cache_slot;

  if (options.alpha_beta_cache_size > 0)
  {
    alpha_beta_cache_hit = probe_alpha_beta_cache(object,
                                                  arg_node_type,
                                                  FALSE,
                                                  alpha_beta_cache[my_colour],
                                                  &alpha_beta_cache_slot);

    if (alpha_beta_cache_hit)
    {
      ++(object->S_total_alpha_beta_cache_hits);

      if (!IS_PV(arg_node_type) and
          (alpha_beta_cache_slot.ABCS_depth >= arg_my_depth))
      {
        ++(object->S_total_alpha_beta_cache_depth_hits);

        if (alpha_beta_cache_slot.ABCS_flags & LE_ALPHA_BIT)
        {
          ++(object->S_total_alpha_beta_cache_le_alpha_hits);

          if (alpha_beta_cache_slot.ABCS_score <= arg_my_alpha)
          {
            TRACE_AB_CACHE(object, "LE ALPHA HIT", &alpha_beta_cache_slot)

            ++(object->S_total_alpha_beta_cache_le_alpha_cutoffs);

            construct_search_result(arg_search_result,
                                    alpha_beta_cache_slot.ABCS_score,
                                    SR_NULL);

            update_pv(&(arg_search_result->SR_pv),
                      object->S_board.B_key,
                      alpha_beta_cache_slot.ABCS_moves[0]);

            arg_search_result->SR_depth = alpha_beta_cache_slot.ABCS_depth;

            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot.ABCS_flags & GE_BETA_BIT)
        {
          ++(object->S_total_alpha_beta_cache_ge_beta_hits);

          if (alpha_beta_cache_slot.ABCS_score >= arg_my_beta)
          {
            TRACE_AB_CACHE(object, "GE BETA HIT", &alpha_beta_cache_slot)

            ++(object->S_total_alpha_beta_cache_ge_beta_cutoffs);

            construct_search_result(arg_search_result,
                                    alpha_beta_cache_slot.ABCS_score,
                                    SR_NULL);

            update_pv(&(arg_search_result->SR_pv),
                      object->S_board.B_key,
                      alpha_beta_cache_slot.ABCS_moves[0]);

            arg_search_result->SR_depth = alpha_beta_cache_slot.ABCS_depth;

            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot.ABCS_flags & TRUE_SCORE_BIT)
        {
          TRACE_AB_CACHE(object, "TRUE SCORE HIT", &alpha_beta_cache_slot)

          ++(object->S_total_alpha_beta_cache_true_score_hits);

          construct_search_result(arg_search_result,
                                  alpha_beta_cache_slot.ABCS_score,
                                  SR_NULL);

          update_pv(&(arg_search_result->SR_pv),
                    object->S_board.B_key,
                    alpha_beta_cache_slot.ABCS_moves[0]);

          arg_search_result->SR_depth = alpha_beta_cache_slot.ABCS_depth;

          goto label_return;
        }
      }
    }
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(object->S_board), &moves_list);

  if (moves_list.ML_nmoves == 0)
  {
    construct_search_result(arg_search_result,
                            SCORE_LOST_ABSOLUTE + object->S_board.B_inode,
                            SR_LOST_BIT);

    arg_search_result->SR_depth = DEPTH_MAX;

    goto label_return;
  }

  int moves_weight[NMOVES_MAX];

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    moves_weight[imove] = moves_list.ML_move_weights[imove];

  if (alpha_beta_cache_hit and
      ((alpha_beta_cache_slot.ABCS_depth +
        options.alpha_beta_cache_depth_margin) >= arg_my_depth))
  {
    for (int imove = 0; imove < NMOVES; imove++)
    {
      int jmove = alpha_beta_cache_slot.ABCS_moves[imove];

      if (jmove == INVALID)
        break;

      if (jmove >= moves_list.ML_nmoves)
        object->S_total_alpha_beta_cache_nmoves_errors++;
      else
        moves_weight[jmove] = L_MAX - imove;
    }
  }

  int sort[NMOVES_MAX];

  sort_moves_weight(moves_list.ML_nmoves, moves_weight, sort);

  construct_search_result(arg_search_result, SCORE_MINUS_INFINITY, SR_NULL);

  HARDBUG(arg_search_result->SR_depth != INVALID)

  arg_history =
    (arg_history << 4) | (moves_list.ML_ncaptx & 0xF) | 0xF000000000000000ULL;

  if (((moves_list.ML_nmoves <= 3) and !move_repetition(&(object->S_board))) or
      (options.captures_are_transparent and (moves_list.ML_ncaptx > 0)))
  {
    alpha_beta_captures(object,
                        arg_my_alpha,
                        arg_my_beta,
                        arg_my_depth,
                        arg_node_type,
                        arg_reduction_depth_root,
                        arg_my_tweaks,
                        &moves_list,
                        sort,
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
    int quiescence_score;

    quiescence(object,
               arg_my_alpha,
               arg_my_beta,
               arg_node_type,
               &moves_list,
               FALSE,
               options.quiescence_capture_extensions,
               arg_history,
               &quiescence_score,
               &(arg_search_result->SR_pv));

    construct_search_result(arg_search_result, quiescence_score, SR_NULL);

    arg_search_result->SR_depth = 0;

    goto label_return;
  }

  if (IS_MINIMAL_WINDOW(arg_node_type))
  {
    alpha_beta_minimal(object,
                       arg_my_alpha,
                       arg_my_beta,
                       arg_my_depth,
                       arg_node_type,
                       arg_reduction_depth_root,
                       arg_my_tweaks,
                       &moves_list,
                       sort,
                       arg_history,
                       arg_search_result);

    if (IS_SCORE_NAN(arg_search_result->SR_score))
      goto label_return;
  }
  else
  {
    alpha_beta_pv(object,
                  arg_my_alpha,
                  arg_my_beta,
                  arg_my_depth,
                  arg_node_type,
                  arg_reduction_depth_root,
                  arg_my_tweaks,
                  &moves_list,
                  sort,
                  arg_history,
                  arg_search_result);

    if (IS_SCORE_NAN(arg_search_result->SR_score))
      goto label_return;
  }

label_store:

  // we always searched move 0

  HARDBUG(arg_search_result->SR_score == SCORE_MINUS_INFINITY)

  HARDBUG(arg_search_result->SR_depth == INVALID)

  HARDBUG(arg_search_result->SR_depth > DEPTH_MAX)

  // #ifdef DEBUG
  int frequency[NMOVES_MAX] = {0};

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    int key = sort[imove];

    HARDBUG((key < 0) or (key >= moves_list.ML_nmoves))

    frequency[key]++;
  }

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    HARDBUG(frequency[sort[imove]] != 1)

  // #endif

  if (FALSE and (options.alpha_beta_cache_size > 0) and
      IS_SR_DRAW_BY_REP(arg_search_result->SR_flags))
  {
    // HARDBUG(arg_search_result->SR_score != 0)

    my_printf(object->S_my_printf, "NOT STORING DRAW BY REP!\n");
  }

  if ((options.alpha_beta_cache_size > 0) and
      !IS_SR_DRAW_BY_REP(arg_search_result->SR_flags))
  {
    HARDBUG(arg_search_result->SR_score > SCORE_PLUS_INFINITY)
    HARDBUG(arg_search_result->SR_score < SCORE_MINUS_INFINITY)

    alpha_beta_cache_slot.ABCS_key = object->S_board.B_key;

    alpha_beta_cache_slot.ABCS_score = arg_search_result->SR_score;

    alpha_beta_cache_slot.ABCS_depth = arg_search_result->SR_depth;

    for (int imove = 0; imove < NMOVES; imove++)
      alpha_beta_cache_slot.ABCS_moves[imove] = INVALID;

    int nmoves = NMOVES;
    if (moves_list.ML_nmoves < nmoves)
      nmoves = moves_list.ML_nmoves;

    for (int imove = 0; imove < nmoves; imove++)
      alpha_beta_cache_slot.ABCS_moves[imove] = sort[imove];

    if (arg_search_result->SR_score <= arg_my_alpha)
    {
      HARDBUG(IS_SR_GE_BETA(arg_search_result->SR_flags))

      ++(object->S_total_alpha_beta_cache_le_alpha_stored);

      alpha_beta_cache_slot.ABCS_flags = LE_ALPHA_BIT;

      TRACE_AB_CACHE(object, "STORE LE ALPHA", &alpha_beta_cache_slot)
    }
    else if (arg_search_result->SR_score >= arg_my_beta)
    {
      HARDBUG(IS_SR_LE_ALPHA(arg_search_result->SR_flags))

      ++(object->S_total_alpha_beta_cache_ge_beta_stored);

      alpha_beta_cache_slot.ABCS_score = arg_my_beta;

      alpha_beta_cache_slot.ABCS_flags = GE_BETA_BIT;

      TRACE_AB_CACHE(object, "STORE GE BETA", &alpha_beta_cache_slot)
    }
    else
    {
      ++(object->S_total_alpha_beta_cache_true_score_stored);

      alpha_beta_cache_slot.ABCS_flags = TRUE_SCORE_BIT;

      TRACE_AB_CACHE(object, "STORE TRUE SCORE", &alpha_beta_cache_slot)
    }

    alpha_beta_cache_slot.ABCS_crc64 =
      return_crc64_from_crc32(&alpha_beta_cache_slot);

    update_alpha_beta_cache(object,
                            arg_node_type,
                            alpha_beta_cache[my_colour],
                            &alpha_beta_cache_slot);
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

  TRACE_AB_EXIT(object,
                arg_my_alpha,
                arg_my_beta,
                arg_node_type,
                arg_my_depth,
                arg_search_result);

  check_search_result(arg_my_alpha, arg_my_beta, arg_search_result);

  END_BLOCK

  POP_NAME(__FUNC__)
}

void do_pv(search_t *object,
           pv_entries_t *arg_best_pv,
           int arg_pv_score,
           bstring arg_bpv_string)
{
  colour_enum my_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

  if (draw_by_repetition(&(object->S_board), FALSE))
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

  int temp_mate = read_endgame(object, my_colour, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    temp_mate = read_endgame(object, my_colour, NULL);

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
      endgame_pv(object, temp_mate, arg_bpv_string);
    }
    else
    {
      if (!options.hub)
      {
        HARDBUG(bformata(arg_bpv_string, " lost in %d", -temp_mate) == BSTR_ERR)
      }
      endgame_pv(object, temp_mate, arg_bpv_string);
    }
    return;
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(object->S_board), &moves_list);

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

  int jmove = probe_pv(arg_best_pv, object->S_board.B_key);

  if (jmove == INVALID)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      int temp_score;

      temp_score = return_score(object);

      HARDBUG(bformata(arg_bpv_string, " %d", temp_score) == BSTR_ERR)

      if (temp_score != arg_pv_score)
      {
        HARDBUG(bcatcstr(arg_bpv_string, " PV ERROR") == BSTR_ERR)
      }
    }
  }
  else
  {
    // reload pv

    if ((options.alpha_beta_cache_size > 0) and (options.pv2cache))
    {
      alpha_beta_cache_slot_t alpha_beta_cache_slot =
        alpha_beta_cache_slot_default;

      alpha_beta_cache_slot.ABCS_moves[0] = jmove;

      alpha_beta_cache_slot.ABCS_key = object->S_board.B_key;

      alpha_beta_cache_slot.ABCS_crc64 =
        return_crc64_from_crc32(&alpha_beta_cache_slot);

      update_alpha_beta_cache(object,
                              PV_BIT,
                              alpha_beta_cache[my_colour],
                              &alpha_beta_cache_slot);
    }

    BSTRING(bmove_string)

    move2bstring(&moves_list, jmove, bmove_string);

    HARDBUG(bcatcstr(arg_bpv_string, " ") == BSTR_ERR)

    HARDBUG(bconcat(arg_bpv_string, bmove_string) == BSTR_ERR)

    BDESTROY(bmove_string)

    do_move(&(object->S_board), jmove, &moves_list, FALSE);

    do_pv(object, arg_best_pv, -arg_pv_score, arg_bpv_string);

    undo_move(&(object->S_board), jmove, &moves_list, FALSE);
  }
}

local void print_pv(search_t *object,
                    int arg_my_depth,
                    int arg_imove,
                    int arg_jmove,
                    moves_list_t *arg_moves_list,
                    int arg_move_score,
                    pv_entries_t *arg_best_pv,
                    char *arg_move_status)
{
  BSTRING(bmove_string)

  move2bstring(arg_moves_list, arg_jmove, bmove_string);

  BSTRING(bpv_string)

  if (options.hub)
  {
    if (options.nthreads == INVALID)
      global_nodes = object->S_total_nodes;
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
             return_my_timer(&(object->S_timer), FALSE),
             global_nodes / return_my_timer(&(object->S_timer), TRUE) /
               1000000.0,
             bdata(bmove_string));
  }
  else
  {
    bformata(bpv_string,
             "d=%d i=%d j=%d m=%s s=%d (%.2f) r=%d t=%.2f n=%lld/%lld/%lld "
             "x=%s pv=%s",
             arg_my_depth,
             arg_imove,
             arg_jmove,
             bdata(bmove_string),
             arg_move_score,
             (float)arg_move_score / (float)SCORE_MAN,
             object->S_root_score,
             return_my_timer(&(object->S_timer), FALSE),
             object->S_total_nodes,
             object->S_total_alpha_beta_nodes,
             object->S_total_quiescence_nodes,
             arg_move_status,
             bdata(bmove_string));
  }

  do_move(&(object->S_board), arg_jmove, arg_moves_list, FALSE);

  do_pv(object, arg_best_pv, -arg_move_score, bpv_string);

  undo_move(&(object->S_board), arg_jmove, arg_moves_list, FALSE);

  my_printf(object->S_my_printf, "%s\n", bdata(bpv_string));

  // flush

  if (object->S_thread == NULL)
    my_printf(object->S_my_printf, "$");

  if (options.mode == GAME_MODE)
  {
    if ((object->S_thread != NULL) and
        (object->S_thread == thread_alpha_beta_master))
      enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string));
  }

  BDESTROY(bpv_string)

  BDESTROY(bmove_string)
}

void do_search(search_t *object,
               moves_list_t *arg_moves_list,
               int arg_depth_min,
               int arg_depth_max,
               int arg_root_score,
               my_random_t *arg_shuffle)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  colour_enum my_colour;
  colour_enum your_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
  {
    my_colour = WHITE_ENUM;
    your_colour = BLACK_ENUM;
  }
  else
  {
    my_colour = BLACK_ENUM;
    your_colour = WHITE_ENUM;
  }

  my_printf(object->S_my_printf, "time_limit=%.2f\n", options.time_limit);

  my_printf(object->S_my_printf, "time_ntrouble=%d\n", options.time_ntrouble);

  for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
    my_printf(object->S_my_printf,
              "itrouble=%d time_trouble=%.2f\n",
              itrouble,
              options.time_trouble[itrouble]);

  if (object->S_thread == thread_alpha_beta_master)
  {
    clear_bucket(&bucket_depth);
  }

  int my_depth = INVALID;

  HARDBUG(arg_moves_list->ML_nmoves == 0)

  if (arg_depth_min == INVALID)
    arg_depth_min = 1;
  if (arg_depth_max == INVALID)
    arg_depth_max = options.depth_limit;

  object->S_board.B_root_inode = object->S_board.B_inode;

  clear_totals(object);

  reset_my_timer(&(object->S_timer));

  int sort[NMOVES_MAX];

  shuffle(sort, arg_moves_list->ML_nmoves, arg_shuffle);

  for (int idepth = 0; idepth < DEPTH_MAX; idepth++)
    object->S_best_score_by_depth[idepth] = SCORE_MINUS_INFINITY;

  // check for known endgame

  int egtb_mate = read_endgame(object, my_colour, NULL);

  if (egtb_mate != ENDGAME_UNKNOWN)
  {
    print_board(&(object->S_board));

    my_printf(object->S_my_printf, "known endgame egtb_mate=%d\n", egtb_mate);

    object->S_best_score = SCORE_MINUS_INFINITY;

    int your_mate = INVALID;

    for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    {
      int jmove = sort[imove];

      do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      ++(object->S_total_nodes);

      int temp_mate = read_endgame(object, your_colour, NULL);

      HARDBUG(temp_mate == ENDGAME_UNKNOWN)

      int temp_score;

      if (temp_mate == INVALID)
        temp_score = 0;
      else if (temp_mate > 0)
        temp_score = SCORE_WON - object->S_board.B_inode - temp_mate;
      else
        temp_score = SCORE_LOST + object->S_board.B_inode - temp_mate;

      undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      int move_score = -temp_score;

      BSTRING(bmove_string)

      move2bstring(arg_moves_list, imove, bmove_string);

      my_printf(object->S_my_printf,
                "%s %d\n",
                bdata(bmove_string),
                move_score);

      BDESTROY(bmove_string)

      if (move_score > object->S_best_score)
      {
        for (int kmove = imove; kmove > 0; kmove--)
          sort[kmove] = sort[kmove - 1];

        sort[0] = jmove;

        object->S_best_score = move_score;

        object->S_best_move = jmove;

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

    HARDBUG(object->S_best_score == SCORE_MINUS_INFINITY)

    object->S_best_score_kind = SEARCH_BEST_SCORE_EGTB;

    object->S_best_depth = 0;

    BSTRING(bmove_string)

    move2bstring(arg_moves_list, sort[0], bmove_string);

    BSTRING(bpv_string)

    if (options.hub)
    {
      HARDBUG(bformata(bpv_string,
                       "depth=127 score=%d nps=0.00 pv=\"%s",
                       object->S_best_score,
                       bdata(bmove_string)) == BSTR_ERR)
    }
    else
    {
      HARDBUG(bformata(bpv_string,
                       "s=%d (%d) x=egtb pv=%s ",
                       object->S_best_score,
                       egtb_mate,
                       bdata(bmove_string)) == BSTR_ERR)
    }

    if (your_mate != INVALID)
    {
      do_move(&(object->S_board), sort[0], arg_moves_list, FALSE);

      endgame_pv(object, your_mate, bpv_string);

      undo_move(&(object->S_board), sort[0], arg_moves_list, FALSE);
    }
    else
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(bpv_string, "\"") == BSTR_ERR)
      }
    }

    my_printf(object->S_my_printf, "%s\n", bdata(bpv_string));

    if (options.mode == GAME_MODE)
    {
      if ((object->S_thread != NULL) and
          (object->S_thread == thread_alpha_beta_master))
        enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string));
    }

    BDESTROY(bpv_string)

    BDESTROY(bmove_string)

    goto label_limit;
  }

  int your_tweaks = 0;

  object->S_root_simple_score = return_score(object);

  search_result_t temp_result;

  temp_result.SR_pv = pv_entries_default;

  if (FALSE and (arg_moves_list->ML_ncaptx == 0) and
      (arg_moves_list->ML_nmoves > 2))
  {
    do_move(&(object->S_board), INVALID, NULL, FALSE);

    alpha_beta(object,
               SCORE_MINUS_INFINITY,
               SCORE_PLUS_INFINITY,
               1,
               PV_BIT,
               options.reduction_depth_root,
               TWEAK_PREVIOUS_MOVE_NULL_BIT,
               0xFULL,
               &temp_result);

    object->S_root_score = -temp_result.SR_score;

    undo_move(&(object->S_board), INVALID, NULL, FALSE);
  }
  else
  {
    alpha_beta(object,
               SCORE_MINUS_INFINITY,
               SCORE_PLUS_INFINITY,
               1,
               PV_BIT,
               options.reduction_depth_root,
               0,
               0xFULL,
               &temp_result);

    object->S_root_score = temp_result.SR_score;
  }

  if (IS_SCORE_NAN(temp_result.SR_score))
  {
    object->S_root_score = return_score(object);

    object->S_best_score_kind = SEARCH_BEST_SCORE_AB;

    object->S_best_move = 0;

    object->S_best_depth = INVALID;

    goto label_limit;
  }

  my_printf(object->S_my_printf,
            "root_simple_score=%d root_score=%d\n",
            object->S_root_simple_score,
            object->S_root_score);

  object->S_best_score_kind = SEARCH_BEST_SCORE_AB;

  object->S_best_move = 0;

  object->S_best_depth = 0;

  if (arg_root_score == SCORE_MINUS_INFINITY)
    object->S_best_score = object->S_root_score;
  else
    object->S_best_score = arg_root_score;

  // publish work

  if ((object->S_thread != NULL) and
      (object->S_thread == thread_alpha_beta_master))
  {
    BSTRING(bmove_string)

    move2bstring(arg_moves_list, 0, bmove_string);

    BSTRING(bmessage)

    HARDBUG(bformata(bmessage,
                     "%s %d %d %d %d",
                     bdata(bmove_string),
                     arg_depth_min,
                     arg_depth_max,
                     object->S_best_score,
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

    int node_type = PV_BIT;

    // best_score_margin determines if the current search
    // should be aborted if the score of the current move drops too low
    // if so, we should try the next move
    // with a window around the current best score
    // not around the score of the current move

    int best_score_margin = options.best_score_margin;

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

        int window_alpha;
        int window_beta;

        // initial search window

        if (IS_PV(node_type))
        {
          window_alpha = 25;
          window_beta = 25;
        }
        else
        {
          window_alpha = 0;
          window_beta = 1;
        }

        int move_score = object->S_best_score;

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
          int my_alpha = move_score - window_alpha;
          int my_beta = move_score + window_beta;

          if (my_alpha < SCORE_MINUS_INFINITY)
            my_alpha = SCORE_MINUS_INFINITY;
          if (my_beta > SCORE_PLUS_INFINITY)
            my_beta = SCORE_PLUS_INFINITY;

          TRACE_KEY(object)

          do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

          TRACE_AB_WINDOW(trace_key,
                          "search",
                          my_alpha,
                          my_beta,
                          arg_moves_list,
                          jmove)

          TRACE_INDENT(arg_moves_list, jmove)

          alpha_beta(object,
                     -my_beta,
                     -my_alpha,
                     your_depth,
                     node_type,
                     options.reduction_depth_root,
                     your_tweaks,
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

          undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);

          if (IS_SCORE_NAN(temp_result.SR_score))
            goto label_limit;

          if (iteration == 0)
          {
            window_alpha = options.aspiration_window;
            window_beta = options.aspiration_window;
          }

          if (move_score <= my_alpha)
          {
            // if a minimal-window search fails low we are done

            if (IS_MINIMAL_WINDOW(node_type))
            {
              HARDBUG(iteration != 0)

              goto label_next_move;
            }

            // if we already have a true score and the current move fails low
            // we are done with this move and continue with the next move

            if ((nbest_moves > 0) and (my_alpha <= object->S_best_score))
            {
              node_type = MINIMAL_WINDOW_BIT;

              goto label_next_move;
            }

            // three other cases:

            // suppose ((nbest_moves > 0) and
            //          (my_alpha > object->S_best_score))
            // this can happen if the move failed high with a margin of say +100
            // we research the move with margins (-25, +25)
            // suppose the move now fails low with a margin of -50
            // the move seems to be a fail-high with respect to
            // the best score but it could still be a fail low
            // so we have to research

            // suppose ((nbest_moves == 0) and
            //          (my_alpha > object->S_best_score))
            // we have to research

            // suppose ((nbest_moves == 0) and
            //          (my_alpha <= object->S_best_score))
            // this could be a really bad move

            if ((nbest_moves == 0) and (best_score_margin > 0) and
                ((my_alpha + best_score_margin) <= object->S_best_score))
            {
              // abandon this move and search the next move with a PV window

              HARDBUG(!IS_PV(node_type))

              goto label_next_move;
            }

            print_pv(object,
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

            print_pv(object,
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

            if ((best_score_margin > 0) and
                (move_score + best_score_margin) <= object->S_best_score)
              goto label_next_move;

            print_pv(object,
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

        if ((nbest_moves == 0) or (move_score > object->S_best_score))
        {
          HARDBUG(!IS_PV(node_type))

          if (sort[0] != jmove)
          {
            for (int kmove = imove; kmove > 0; kmove--)
              sort[kmove] = sort[kmove - 1];

            sort[0] = jmove;

            if ((object->S_thread != NULL) and
                (object->S_thread == thread_alpha_beta_master) and
                (options.lazy_smp_policy > 0))
            {
              BSTRING(bmove_string)

              move2bstring(arg_moves_list, jmove, bmove_string);

              BSTRING(bmessage)

              HARDBUG(bformata(bmessage,
                               "%s %d %d %d %d",
                               bdata(bmove_string),
                               arg_depth_min,
                               arg_depth_max,
                               object->S_best_score,
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

          object->S_best_score = move_score;

          object->S_best_move = jmove;

          object->S_best_depth = my_depth;

          object->S_best_score_by_depth[my_depth] = object->S_best_score;

          print_pv(object,
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

      if (IS_SCORE_WON(object->S_best_score))
        goto label_limit;
      if (IS_SCORE_LOST(object->S_best_score))
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

  if (object->S_thread == thread_alpha_beta_master)
  {
    if (FALSE and (object->S_best_score < -(SCORE_MAN / 4)))
    {
      BSTRING(bfen)

      board2fen(&(object->S_board), bfen, TRUE);

      BSTRING(bmove_string)

      move2bstring(arg_moves_list, object->S_best_move, bmove_string);

      my_printf(object->S_my_printf,
                "%s bs=%d bm=%s md=%d bd=%d\n",
                bdata(bfen),
                object->S_best_score,
                bdata(bmove_string),
                my_depth,
                object->S_best_depth);

      char stamp[MY_LINE_MAX];

      time_t t = time(NULL);

      HARDBUG(strftime(stamp, MY_LINE_MAX, "%d%b%Y-%H%M%S", localtime(&t)) == 0)

      FILE *fbugs;

      HARDBUG((fbugs = fopen("bugs.txt", "a")) == NULL)

      fprintf(fbugs,
              "%s %s %s bs=%d bm=%s md=%d bd=%d\n",
              bdata(bfen),
              stamp,
              REVISION,
              object->S_best_score,
              bdata(bmove_string),
              my_depth,
              object->S_best_depth);

      FCLOSE(fbugs)

      BDESTROY(bmove_string)

      BDESTROY(bfen)
    }
  }

  stop_my_timer(&(object->S_timer));

  BSTRING(bmove_string)

  move2bstring(arg_moves_list, object->S_best_move, bmove_string),

    my_printf(object->S_my_printf,
              "best_move=%s best_score=%d\n",
              bdata(bmove_string),
              object->S_best_score);

  BDESTROY(bmove_string)

  my_printf(object->S_my_printf,
            "%lld nodes in %.2f seconds\n"
            "%.0f nodes/second\n",
            object->S_total_nodes,
            return_my_timer(&(object->S_timer), FALSE),
            object->S_total_nodes / return_my_timer(&(object->S_timer), FALSE));

  if (options.verbose > 0)
    print_totals(object);

  if ((object->S_thread != NULL) and
      (object->S_thread == thread_alpha_beta_master))
  {
    global_nodes = 0;

    for (int ithread = 0; ithread < options.nthreads; ithread++)
    {
      thread_t *with_thread = threads + ithread;

      global_nodes += with_thread->T_search.S_total_nodes;
    }

    my_printf(object->S_my_printf,
              "%lld global_nodes in %.2f seconds\n"
              "%.0f global_nodes/second\n",
              global_nodes,
              return_my_timer(&(object->S_timer), FALSE),
              global_nodes / return_my_timer(&(object->S_timer), FALSE));
  }

  if (object->S_thread == thread_alpha_beta_master)
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

local void clear_alpha_beta_caches(void)
{
  PRINTF("clear_alpha_beta_caches..\n");

  for (i64_t ientry = 0;
       ientry < (nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries);
       ientry++)
  {
    alpha_beta_cache[WHITE_ENUM][ientry] = alpha_beta_cache_entry_default;
  }

  for (i64_t ientry = 0;
       ientry < (nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries);
       ientry++)
  {
    alpha_beta_cache[BLACK_ENUM][ientry] = alpha_beta_cache_entry_default;
  }

  PRINTF("..done\n");
}

void construct_search(void *self, my_printf_t *arg_my_printf, void *arg_thread)
{
  search_t *object = self;

  object->S_my_printf = arg_my_printf;

  object->S_thread = arg_thread;

  construct_board(&(object->S_board), arg_my_printf);

  construct_my_timer(&(object->S_timer), "board", object->S_my_printf, FALSE);

  if (arg_thread == NULL)
    construct_my_random(&(object->S_random), 0);
  else
    construct_my_random(&(object->S_random), INVALID);

  entry_i16_t entry_i16_default = {MATE_DRAW, MATE_DRAW};

  i64_t key_default = -1;

  i64_t endgame_entry_cache_size = options.egtb_entry_cache_size * MBYTE;

  BSTRING(bname)

  HARDBUG(bformata(bname, "S_%p_entry_cache\n", object) == BSTR_ERR)

  construct_cache(&(object->S_endgame_entry_cache),
                  bdata(bname),
                  endgame_entry_cache_size,
                  CACHE_ENTRY_KEY_TYPE_I64_T,
                  &key_default,
                  sizeof(entry_i16_t),
                  &entry_i16_default);

  HARDBUG(bassigncstr(bname, "") == BSTR_ERR)

  HARDBUG(bformata(bname, "S_%p_wdl_entry_cache\n", object) == BSTR_ERR)

  construct_cache(&(object->S_endgame_wdl_entry_cache),
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
  PRINTF("sizeof(alpha_beta_cache_entry_t)=%lld\n",
         (i64_t)sizeof(alpha_beta_cache_entry_t));

  global_nodes = 0;

  // alpha_beta cache

  alpha_beta_cache[WHITE_ENUM] = NULL;
  alpha_beta_cache[BLACK_ENUM] = NULL;

  nalpha_beta_pv_cache_entries = 0;
  nalpha_beta_cache_entries = 0;

  if (options.alpha_beta_cache_size > 0)
  {
    float alpha_beta_cache_size = options.alpha_beta_cache_size * MBYTE / 2;

    nalpha_beta_pv_cache_entries = first_prime_below(
      roundf(alpha_beta_cache_size * (options.pv_cache_fraction / 100.0f) /
             sizeof(alpha_beta_cache_entry_t)));

    HARDBUG(nalpha_beta_pv_cache_entries < 3)

    nalpha_beta_cache_entries = first_prime_below(roundf(
      alpha_beta_cache_size * ((100 - options.pv_cache_fraction) / 100.0f) /
      sizeof(alpha_beta_cache_entry_t)));

    HARDBUG(nalpha_beta_cache_entries < 3)

    PRINTF("nalpha_beta_pv_cache_entries=%lld nalpha_beta_cache_entries=%lld\n",
           nalpha_beta_pv_cache_entries,
           nalpha_beta_cache_entries);

    alpha_beta_cache_slot_default.ABCS_key = 0;
    alpha_beta_cache_slot_default.ABCS_score = SCORE_MINUS_INFINITY;
    alpha_beta_cache_slot_default.ABCS_depth = INVALID;
    alpha_beta_cache_slot_default.ABCS_flags = 0;
    for (int imove = 0; imove < NMOVES; imove++)
      alpha_beta_cache_slot_default.ABCS_moves[imove] = INVALID;
    alpha_beta_cache_slot_default.ABCS_pad = 0;

    alpha_beta_cache_slot_default.ABCS_crc64 =
      return_crc64_from_crc32(&alpha_beta_cache_slot_default);

    for (int islot = 0; islot < NSLOTS; islot++)
      alpha_beta_cache_entry_default.ABCE_slots[islot] =
        alpha_beta_cache_slot_default;

    MY_MALLOC_BY_TYPE(alpha_beta_cache[WHITE_ENUM],
                      alpha_beta_cache_entry_t,
                      nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries)

    MY_MALLOC_BY_TYPE(alpha_beta_cache[BLACK_ENUM],
                      alpha_beta_cache_entry_t,
                      nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries)

    clear_alpha_beta_caches();
  }

  for (int islot = 0; islot < NPV_SLOTS; islot++)
    pv_entry_default.PVE_slots[islot] = PV_SLOT_IMOVE_MASK;

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
