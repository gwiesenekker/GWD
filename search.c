//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#include "globals.h"

#define FAST_MODULO64(H, N) ((ui64_t)(((__uint128_t)(H) * (N)) >> 64));

#define TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT BIT(0)
#define TWEAK_PREVIOUS_MOVE_NULL_BIT BIT(1)
#define TWEAK_PREVIOUS_MOVE_REDUCED_BIT BIT(2)
#define TWEAK_PREVIOUS_MOVE_EXTENDED_BIT BIT(3)

#define STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH BIT(0)
#define STAGE_SEARCH_MOVE_AT_FULL_DEPTH BIT(1)
#define STAGE_RESEARCH_MOVE_AT_FULL_DEPTH BIT(2)
#define STAGE_SEARCHED_MOVE_AT_REDUCED_DEPTH BIT(3)
#define STAGE_SEARCHED_MOVE_AT_FULL_DEPTH BIT(4)

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

local i64_t global_nodes;

local bucket_t bucket_depth;

#undef LAZY_STATS
#ifdef LAZY_STATS
local stats_t lazy_stats[2 * NPIECES_MAX + 1][2 * NPIECES_MAX + 1];
#endif

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

local int probe_alpha_beta_cache(
    search_t *object,
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

local void update_alpha_beta_cache_slot_moves(
    alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot,
    int arg_jmove)
{
  int kmove;

  for (kmove = 0; kmove < NMOVES; kmove++)
    if (arg_alpha_beta_cache_slot->ABCS_moves[kmove] == arg_jmove)
      break;

  if (kmove == NMOVES)
    kmove = NMOVES - 1;

  while (kmove > 0)
  {
    arg_alpha_beta_cache_slot->ABCS_moves[kmove] =
        arg_alpha_beta_cache_slot->ABCS_moves[kmove - 1];

    kmove--;
  }

  arg_alpha_beta_cache_slot->ABCS_moves[0] = arg_jmove;

#ifdef DEBUG
  for (int idebug = 0; idebug < NMOVES; idebug++)
  {
    if (arg_alpha_beta_cache_slot->ABCS_moves[idebug] == INVALID)
    {
      for (int jdebug = idebug + 1; jdebug < NMOVES; jdebug++)
        HARDBUG(arg_alpha_beta_cache_slot->ABCS_moves[jdebug] != INVALID)

      break;
    }

    int ndebug = 0;

    for (int jdebug = 0; jdebug < NMOVES; jdebug++)
    {
      if (arg_alpha_beta_cache_slot->ABCS_moves[jdebug] ==
          arg_alpha_beta_cache_slot->ABCS_moves[idebug])
        ndebug++;
    }
    HARDBUG(ndebug != 1)
  }
#endif
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
  SOFTBUG(ibest_move == INVALID)

  SOFTBUG(best_mate == ENDGAME_UNKNOWN)

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

local int quiescence(search_t *object,
                     int arg_my_alpha,
                     int arg_my_beta,
                     int arg_node_type,
                     moves_list_t *arg_moves_list,
                     int arg_all_moves,
                     int arg_ncapture_extensions,
                     ui64_t arg_history,
                     pv_t *arg_best_pv,
                     int *arg_best_depth)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(arg_my_alpha >= arg_my_beta)

  SOFTBUG(arg_node_type == 0)

  SOFTBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  SOFTBUG(IS_MINIMAL_WINDOW(arg_node_type) and
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

  *arg_best_pv = INVALID;

  *arg_best_depth = INVALID;

  int best_score = SCORE_MINUS_INFINITY;

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

        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

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
        object->S_interrupt = BOARD_INTERRUPT_TIME;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
    else
    {
      SOFTBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    best_score = 0;

    *arg_best_depth = 0;

    goto label_return;
  }

  // check endgame

  int wdl;

  int temp_mate = read_endgame(object, my_colour, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    *arg_best_depth = DEPTH_MAX;

    if (temp_mate == INVALID)
    {
      best_score = 0;

      *arg_best_depth = 0;
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        best_score = SCORE_WON - object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_WON_ABSOLUTE - object->S_board.B_inode - temp_mate;
    }
    else
    {
      if (wdl)
        best_score = SCORE_LOST + object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode - temp_mate;
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
    best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode;

    *arg_best_depth = DEPTH_MAX;

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

  if (!arg_all_moves and IS_PV(arg_node_type) and
      (arg_my_alpha >= (SCORE_LOST + NODE_MAX)))
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

      pv_t temp_pv[PV_MAX];

      int temp_depth;

      temp_score = quiescence(object,
                              reduced_alpha,
                              reduced_beta,
                              MINIMAL_WINDOW_BIT,
                              arg_moves_list,
                              TRUE,
                              arg_ncapture_extensions,
                              arg_history,
                              temp_pv,
                              &temp_depth);

      if (object->S_interrupt != 0)
      {
        best_score = SCORE_PLUS_INFINITY;

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

    best_score = return_score(object);

    *arg_best_depth = 0;

    if ((nextend == 0) or (best_score >= arg_my_beta))
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

    SOFTBUG(moves_weight[jmove] == L_MIN)

    moves_weight[jmove] = L_MIN;

    if (!arg_all_moves)
    {
      if (!MOVE_EXTEND_IN_QUIESCENCE(arg_moves_list->ML_move_flags[jmove]))
        continue;
    }

    int temp_alpha;

    if (best_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = best_score;

    int temp_score = SCORE_MINUS_INFINITY;

    pv_t temp_pv[PV_MAX];

    int temp_depth;

    if (IS_MINIMAL_WINDOW(arg_node_type))
    {
      do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      temp_score = -quiescence(object,
                               -arg_my_beta,
                               -temp_alpha,
                               arg_node_type,
                               NULL,
                               FALSE,
                               arg_ncapture_extensions,
                               arg_history,
                               temp_pv + 1,
                               &temp_depth);

      undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);
    }
    else
    {
      if (imove == 0)
      {
        do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        temp_score = -quiescence(object,
                                 -arg_my_beta,
                                 -temp_alpha,
                                 arg_node_type,
                                 NULL,
                                 FALSE,
                                 arg_ncapture_extensions,
                                 arg_history,
                                 temp_pv + 1,
                                 &temp_depth);

        undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        temp_score = -quiescence(object,
                                 -temp_beta,
                                 -temp_alpha,
                                 MINIMAL_WINDOW_BIT,
                                 NULL,
                                 FALSE,
                                 arg_ncapture_extensions,
                                 arg_history,
                                 temp_pv + 1,
                                 &temp_depth);

        if ((object->S_interrupt == 0) and (temp_score >= temp_beta) and
            (temp_score < arg_my_beta))
        {
          temp_score = -quiescence(object,
                                   -arg_my_beta,
                                   -temp_alpha,
                                   arg_node_type,
                                   NULL,
                                   FALSE,
                                   arg_ncapture_extensions,
                                   arg_history,
                                   temp_pv + 1,
                                   &temp_depth);
        }

        undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);
      }
    }

    if (object->S_interrupt != 0)
    {
      best_score = SCORE_PLUS_INFINITY;

      goto label_return;
    }

    SOFTBUG(temp_score == SCORE_MINUS_INFINITY)

    if (temp_score > best_score)
    {
      best_score = temp_score;

      *arg_best_pv = jmove;

      for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++)
        ;

      *arg_best_depth = temp_depth;
    }

    if (best_score >= arg_my_beta)
      break;
  } // imove

  SOFTBUG(best_score == SCORE_MINUS_INFINITY)

  SOFTBUG(*arg_best_depth == INVALID)

  if ((options.quiescence_evaluation_policy > 0) and (best_score < arg_my_beta))
  {
    ++(object->S_total_evaluations);

    int temp_score = return_score(object);

    if (temp_score > best_score)
    {
      best_score = temp_score;

      *arg_best_depth = 0;
    }
  }

label_return:

  if (FALSE and (*arg_best_depth == DEPTH_MAX))
  {
    HARDBUG((best_score < (SCORE_WON - NODE_MAX)) and
            (best_score > (SCORE_LOST + NODE_MAX)) and (best_score != 0))
  }

  if (best_score >= arg_my_beta)
  {
    best_score = arg_my_beta;
    *arg_best_depth = 0;
  }

  END_BLOCK

  POP_NAME(__FUNC__)

  return (best_score);
}

local int alpha_beta(search_t *object,
                     int arg_my_alpha,
                     int arg_my_beta,
                     int arg_my_depth,
                     int arg_node_type,
                     int arg_reduction_depth_root,
                     int arg_my_tweaks,
                     ui64_t arg_history,
                     pv_t *arg_best_pv,
                     int *arg_best_depth)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(arg_my_alpha >= arg_my_beta)

  SOFTBUG(arg_my_depth < 0)

  SOFTBUG(arg_node_type == 0)

  SOFTBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

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

  *arg_best_pv = INVALID;

  *arg_best_depth = INVALID;

  int best_score = SCORE_PLUS_INFINITY;

  int best_move = INVALID;

  ++(object->S_total_nodes);

  ++(object->S_total_alpha_beta_nodes);

  if (IS_MINIMAL_WINDOW(arg_node_type))
    ++(object->S_total_minimal_window_nodes);
  else if (IS_PV(arg_node_type))
    ++(object->S_total_pv_nodes);
  else
    FATAL("unknown node_type", EXIT_FAILURE)

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

        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

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
        object->S_interrupt = BOARD_INTERRUPT_TIME;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
    else
    {
      SOFTBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    best_score = 0;

    *arg_best_depth = 0;

    goto label_return;
  }

  // check endgame

  int wdl;

  int temp_mate = read_endgame(object, my_colour, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    *arg_best_depth = DEPTH_MAX;

    if (temp_mate == INVALID)
    {
      best_score = 0;

      *arg_best_depth = 0;
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        best_score = SCORE_WON - object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_WON_ABSOLUTE - object->S_board.B_inode - temp_mate;
    }
    else
    {
      if (wdl)
        best_score = SCORE_LOST + object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode - temp_mate;
    }

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
            ++(object->S_total_alpha_beta_cache_le_alpha_cutoffs);

            best_score = alpha_beta_cache_slot.ABCS_score;

            arg_best_pv[0] = alpha_beta_cache_slot.ABCS_moves[0];

            arg_best_pv[1] = INVALID;

            *arg_best_depth = alpha_beta_cache_slot.ABCS_depth;

            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot.ABCS_flags & GE_BETA_BIT)
        {
          ++(object->S_total_alpha_beta_cache_ge_beta_hits);

          if (alpha_beta_cache_slot.ABCS_score >= arg_my_beta)
          {
            ++(object->S_total_alpha_beta_cache_ge_beta_cutoffs);

            best_score = alpha_beta_cache_slot.ABCS_score;

            arg_best_pv[0] = alpha_beta_cache_slot.ABCS_moves[0];

            arg_best_pv[1] = INVALID;

            *arg_best_depth = alpha_beta_cache_slot.ABCS_depth;

            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot.ABCS_flags & TRUE_SCORE_BIT)
        {
          ++(object->S_total_alpha_beta_cache_true_score_hits);

          best_score = alpha_beta_cache_slot.ABCS_score;

          arg_best_pv[0] = alpha_beta_cache_slot.ABCS_moves[0];

          arg_best_pv[1] = INVALID;

          *arg_best_depth = alpha_beta_cache_slot.ABCS_depth;

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
    best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode;

    *arg_best_depth = DEPTH_MAX;

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

  best_score = SCORE_MINUS_INFINITY;

  best_move = INVALID;

  SOFTBUG(*arg_best_depth != INVALID)

  int your_tweaks = 0;

  arg_history =
      (arg_history << 4) | (moves_list.ML_ncaptx & 0xF) | 0xF000000000000000ULL;

  if (((moves_list.ML_nmoves <= 3) and !move_repetition(&(object->S_board))) or
      (options.captures_are_transparent and (moves_list.ML_ncaptx > 0)))
  {
    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      int jmove = 0;

      for (int kmove = 1; kmove < moves_list.ML_nmoves; kmove++)
      {
        if (moves_weight[kmove] == L_MIN)
          continue;

        if (moves_weight[kmove] > moves_weight[jmove])
          jmove = kmove;
      }

      SOFTBUG(moves_weight[jmove] == L_MIN)

      moves_weight[jmove] = L_MIN;

      int temp_alpha;
      pv_t temp_pv[PV_MAX];
      int temp_depth;

      if (best_score < arg_my_alpha)
        temp_alpha = arg_my_alpha;
      else
        temp_alpha = best_score;

      int temp_score = SCORE_MINUS_INFINITY;

      if (IS_MINIMAL_WINDOW(arg_node_type))
      {
        do_move(&(object->S_board), jmove, &moves_list, FALSE);

        temp_score = -alpha_beta(object,
                                 -arg_my_beta,
                                 -temp_alpha,
                                 arg_my_depth,
                                 arg_node_type,
                                 arg_reduction_depth_root,
                                 your_tweaks,
                                 arg_history,
                                 temp_pv + 1,
                                 &temp_depth);

        undo_move(&(object->S_board), jmove, &moves_list, FALSE);
      }
      else
      {
        if (imove == 0)
        {
          do_move(&(object->S_board), jmove, &moves_list, FALSE);

          temp_score = -alpha_beta(object,
                                   -arg_my_beta,
                                   -temp_alpha,
                                   arg_my_depth,
                                   arg_node_type,
                                   arg_reduction_depth_root,
                                   your_tweaks,
                                   arg_history,
                                   temp_pv + 1,
                                   &temp_depth);

          undo_move(&(object->S_board), jmove, &moves_list, FALSE);
        }
        else
        {
          int temp_beta = temp_alpha + 1;

          do_move(&(object->S_board), jmove, &moves_list, FALSE);

          temp_score = -alpha_beta(object,
                                   -temp_beta,
                                   -temp_alpha,
                                   arg_my_depth,
                                   MINIMAL_WINDOW_BIT,
                                   arg_reduction_depth_root,
                                   your_tweaks,
                                   arg_history,
                                   temp_pv + 1,
                                   &temp_depth);

          if ((object->S_interrupt == 0) and (temp_score >= temp_beta) and
              (temp_score < arg_my_beta))
          {
            temp_score = -alpha_beta(object,
                                     -arg_my_beta,
                                     -temp_alpha,
                                     arg_my_depth,
                                     arg_node_type,
                                     arg_reduction_depth_root,
                                     your_tweaks,
                                     arg_history,
                                     temp_pv + 1,
                                     &temp_depth);
          }

          undo_move(&(object->S_board), jmove, &moves_list, FALSE);
        }
      }

      if (object->S_interrupt != 0)
      {
        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }

      if (temp_score > best_score)
      {
        best_score = temp_score;

        best_move = jmove;

        *arg_best_pv = jmove;

        for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++)
          ;

        *arg_best_depth = temp_depth;

        if (options.alpha_beta_cache_size > 0)
          update_alpha_beta_cache_slot_moves(&alpha_beta_cache_slot, jmove);

        if (best_score >= arg_my_beta)
          break;
      }
    }
    goto label_break;
  }

  SOFTBUG(
      ((moves_list.ML_nmoves <= 2) and !move_repetition(&(object->S_board))) or
      (options.captures_are_transparent and (moves_list.ML_ncaptx > 0)))

  if (arg_reduction_depth_root > 0)
    arg_reduction_depth_root--;

  int your_depth = arg_my_depth - 1;

  if (your_depth < 0)
  {
    best_score = quiescence(object,
                            arg_my_alpha,
                            arg_my_beta,
                            arg_node_type,
                            &moves_list,
                            FALSE,
                            options.quiescence_capture_extensions,
                            arg_history,
                            arg_best_pv,
                            arg_best_depth);

    if (object->S_interrupt != 0)
      best_score = SCORE_PLUS_INFINITY;

    goto label_return;
  }

  if (IS_MINIMAL_WINDOW(arg_node_type))
  {
    int allow_reductions =
        options.use_reductions and
        !(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
        !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_NULL_BIT) and
        !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_REDUCED_BIT) and
        !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
        (moves_list.ML_ncaptx == 0) and
        (moves_list.ML_nmoves >= options.reduction_moves_min) and
        (your_depth >= options.reduction_depth_leaf) and
        (arg_reduction_depth_root == 0) and
        (arg_my_alpha >= (SCORE_LOST + NODE_MAX)) and
        (arg_my_beta <= (SCORE_WON - NODE_MAX));

    if (allow_reductions and (options.reduction_combination_length > 0) and
        (return_combination_length(arg_history) >=
         options.reduction_combination_length))
    {
      object->S_total_reductions_combinations++;

      allow_reductions = FALSE;
    }

    int stage[NMOVES_MAX];
    int reduced_score[NMOVES_MAX];

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      stage[imove] = STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH;

      reduced_score[imove] = SCORE_MINUS_INFINITY;

      if (!allow_reductions or
          MOVE_DO_NOT_REDUCE(moves_list.ML_move_flags[imove]))
      {
        stage[imove] = STAGE_SEARCH_MOVE_AT_FULL_DEPTH;
      }
    }

    int nmoves_searched_at_full_depth = 0;

    for (int ipass = 1; ipass <= 2; ++ipass)
    {
      for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
      {
        int jmove = INVALID;

        if (ipass == 1)
        {
          for (jmove = 0; jmove < moves_list.ML_nmoves; jmove++)
          {
            if ((stage[jmove] == STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH) or
                (stage[jmove] == STAGE_SEARCH_MOVE_AT_FULL_DEPTH))
              break;
          }

          HARDBUG(jmove >= moves_list.ML_nmoves)

          for (int kmove = jmove + 1; kmove < moves_list.ML_nmoves; kmove++)
          {
            if ((stage[kmove] != STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH) and
                (stage[kmove] != STAGE_SEARCH_MOVE_AT_FULL_DEPTH))
              continue;

            if (moves_weight[kmove] > moves_weight[jmove])
              jmove = kmove;
          }
        }
        else
        {
          jmove = 0;

          for (jmove = 0; jmove < moves_list.ML_nmoves; jmove++)
            if (stage[jmove] == STAGE_RESEARCH_MOVE_AT_FULL_DEPTH)
              break;

          if (jmove >= moves_list.ML_nmoves)
            break;

          for (int kmove = 1; kmove < moves_list.ML_nmoves; kmove++)
          {
            if (stage[kmove] != stage[jmove])
              continue;

            if (reduced_score[kmove] > reduced_score[jmove])
              jmove = kmove;
          }

          if (reduced_score[jmove] <
              (best_score - options.reduction_research_margin))
            break;

          stage[jmove] = STAGE_SEARCH_MOVE_AT_FULL_DEPTH;
        }

        SOFTBUG(jmove == INVALID)

        HARDBUG(stage[jmove] == STAGE_SEARCHED_MOVE_AT_REDUCED_DEPTH)
        HARDBUG(stage[jmove] == STAGE_SEARCHED_MOVE_AT_FULL_DEPTH)

        int temp_score = SCORE_MINUS_INFINITY;

        pv_t temp_pv[PV_MAX];

        int temp_depth;

        do_move(&(object->S_board), jmove, &moves_list, FALSE);

        int reduced_depth = your_depth / options.reduction_strength;

        if (nmoves_searched_at_full_depth < options.reduction_full_min)
          stage[jmove] = STAGE_SEARCH_MOVE_AT_FULL_DEPTH;

        if (stage[jmove] == STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH)
        {
          for (int iprobe = options.reduction_probes; iprobe >= 0; --iprobe)
          {
            if ((reduced_depth - iprobe) < 0)
              continue;

            ++(object->S_total_reductions);

            int reduced_alpha =
                best_score - iprobe * options.reduction_probe_window;

            int reduced_beta = reduced_alpha + 1;

            temp_score =
                -alpha_beta(object,
                            -reduced_beta,
                            -reduced_alpha,
                            reduced_depth - iprobe,
                            arg_node_type,
                            arg_reduction_depth_root,
                            your_tweaks | TWEAK_PREVIOUS_MOVE_REDUCED_BIT,
                            arg_history,
                            temp_pv + 1,
                            &temp_depth);

            if (object->S_interrupt != 0)
            {
              undo_move(&(object->S_board), jmove, &moves_list, FALSE);

              best_score = SCORE_PLUS_INFINITY;

              goto label_return;
            }
            if (temp_score <= reduced_alpha)
              break;
          }

          if (temp_score < (SCORE_LOST + NODE_MAX))
          {
            ++(object->S_total_reductions_lost);

            stage[jmove] = STAGE_SEARCHED_MOVE_AT_FULL_DEPTH;

            ++nmoves_searched_at_full_depth;
          }
          else if (temp_score <= best_score)
          {
            ++(object->S_total_reductions_le_alpha);

            stage[jmove] = STAGE_RESEARCH_MOVE_AT_FULL_DEPTH;

            reduced_score[jmove] = temp_score;
          }
          else
          {
            ++(object->S_total_reductions_ge_beta);

            stage[jmove] = STAGE_SEARCH_MOVE_AT_FULL_DEPTH;
          }
        } // if (stage[jmove]

        if (stage[jmove] == STAGE_SEARCH_MOVE_AT_FULL_DEPTH)
        {
          temp_score = -alpha_beta(object,
                                   -arg_my_beta,
                                   -arg_my_alpha,
                                   your_depth,
                                   arg_node_type,
                                   arg_reduction_depth_root,
                                   your_tweaks,
                                   arg_history,
                                   temp_pv + 1,
                                   &temp_depth);

          stage[jmove] = STAGE_SEARCHED_MOVE_AT_FULL_DEPTH;

          nmoves_searched_at_full_depth++;
        }

        undo_move(&(object->S_board), jmove, &moves_list, FALSE);

        if (object->S_interrupt != 0)
        {
          best_score = SCORE_PLUS_INFINITY;

          goto label_return;
        }

        SOFTBUG(temp_score == SCORE_MINUS_INFINITY)

        if (temp_score > best_score)
        {
          HARDBUG(stage[jmove] != STAGE_SEARCHED_MOVE_AT_FULL_DEPTH)

          best_score = temp_score;

          best_move = jmove;

          *arg_best_pv = jmove;

          for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++)
            ;

          if ((options.returned_depth_includes_captures or
               (moves_list.ML_ncaptx == 0)) and
              (temp_depth < (DEPTH_MAX - 1)))
          {
            temp_depth++;
          }

          *arg_best_depth = temp_depth;

          if (options.alpha_beta_cache_size > 0)
            update_alpha_beta_cache_slot_moves(&alpha_beta_cache_slot, jmove);

          if (best_score >= arg_my_beta)
            goto label_break;
        }
      } // for (int imove
    }   // for (int ipass

    HARDBUG(nmoves_searched_at_full_depth == 0)
  }
  else // IS_MINIMAL_WINDOW(node_type) NEW
  {
    // check if position should be extended

    if (!(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
        !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
        (arg_my_alpha >= (SCORE_LOST + NODE_MAX)))
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

        pv_t temp_pv[PV_MAX];

        int temp_depth;

        int temp_score =
            alpha_beta(object,
                       reduced_alpha,
                       reduced_beta,
                       reduced_depth,
                       MINIMAL_WINDOW_BIT,
                       arg_reduction_depth_root,
                       arg_my_tweaks | TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT,
                       arg_history,
                       temp_pv,
                       &temp_depth);

        if (object->S_interrupt != 0)
        {
          best_score = SCORE_PLUS_INFINITY;

          goto label_return;
        }

        if (temp_score <= reduced_alpha)
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

    for (int ipass = 1; ipass <= 2; ++ipass)
    {
      int nsingle_replies = 0;

      for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
      {
        int jmove = 0;

        for (int kmove = 1; kmove < moves_list.ML_nmoves; kmove++)
        {
          if (moves_weight[kmove] == L_MIN)
            continue;

          if (moves_weight[kmove] > moves_weight[jmove])
            jmove = kmove;
        }

        if (moves_weight[jmove] == L_MIN)
        {
          if (ipass == 1)
            HARDBUG(moves_weight[jmove] == L_MIN)
          else
            break;
        }

        moves_weight[jmove] = L_MIN;

        int temp_alpha;

        if (best_score < arg_my_alpha)
          temp_alpha = arg_my_alpha;
        else
          temp_alpha = best_score;

        int temp_score = SCORE_MINUS_INFINITY;

        pv_t temp_pv[PV_MAX];

        int temp_depth;

        if (imove == 0)
        {
          do_move(&(object->S_board), jmove, &moves_list, FALSE);

          temp_score = -alpha_beta(object,
                                   -arg_my_beta,
                                   -temp_alpha,
                                   your_depth,
                                   arg_node_type,
                                   arg_reduction_depth_root,
                                   your_tweaks,
                                   arg_history,
                                   temp_pv + 1,
                                   &temp_depth);

          undo_move(&(object->S_board), jmove, &moves_list, FALSE);
        }
        else
        {
          int temp_beta = temp_alpha + 1;

          do_move(&(object->S_board), jmove, &moves_list, FALSE);

          temp_score = -alpha_beta(object,
                                   -temp_beta,
                                   -temp_alpha,
                                   your_depth,
                                   MINIMAL_WINDOW_BIT,
                                   arg_reduction_depth_root,
                                   your_tweaks,
                                   arg_history,
                                   temp_pv + 1,
                                   &temp_depth);

          if ((object->S_interrupt == 0) and (temp_score >= temp_beta) and
              (temp_score < arg_my_beta))
          {
            temp_score = -alpha_beta(object,
                                     -arg_my_beta,
                                     -temp_alpha,
                                     your_depth,
                                     arg_node_type,
                                     arg_reduction_depth_root,
                                     your_tweaks,
                                     arg_history,
                                     temp_pv + 1,
                                     &temp_depth);
          }

          undo_move(&(object->S_board), jmove, &moves_list, FALSE);
        }

        if (object->S_interrupt != 0)
        {
          best_score = SCORE_PLUS_INFINITY;

          goto label_return;
        }

        SOFTBUG(temp_score == SCORE_MINUS_INFINITY)

        if (temp_score > arg_my_alpha)
        {
          SOFTBUG(!IS_PV(arg_node_type))

          nsingle_replies++;
        }

        if (temp_score > best_score)
        {
          best_score = temp_score;

          best_move = jmove;

          *arg_best_pv = jmove;

          for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++)
            ;

          if ((options.returned_depth_includes_captures or
               (moves_list.ML_ncaptx == 0)) and
              (temp_depth < (DEPTH_MAX - 1)))
          {
            temp_depth++;
          }

          *arg_best_depth = temp_depth;

          if (options.alpha_beta_cache_size > 0)
            update_alpha_beta_cache_slot_moves(&alpha_beta_cache_slot, jmove);

          if (best_score >= arg_my_beta)
            goto label_break;
        }
      } // imove

      if ((ipass == 1) and (options.use_single_reply_extensions) and
          (nsingle_replies == 1))
      {
        object->S_total_single_reply_extensions++;

        *arg_best_pv = INVALID;

        *arg_best_depth = INVALID;

        best_score = SCORE_MINUS_INFINITY;

        your_depth = your_depth + 1;

        your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;

        for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
          moves_weight[imove] = moves_list.ML_move_weights[imove];

        moves_weight[best_move] = L_MAX;

        continue;
      }

      break;
    } // ipass
  }   // IS_MINIMAL_WINDOW(node_type) NEW

label_break:

  // we always searched move 0

  SOFTBUG(best_move == INVALID)

  SOFTBUG(best_score == SCORE_MINUS_INFINITY)

  SOFTBUG(*arg_best_depth == INVALID)

  SOFTBUG(*arg_best_depth > DEPTH_MAX)

  // if (best_score == 0) goto label_return;

#ifdef DEBUG
  if (options.alpha_beta_cache_size > 0)
  {
    for (int idebug = 0; idebug < NMOVES; idebug++)
    {
      if (alpha_beta_cache_slot.ABCS_moves[idebug] == INVALID)
      {
        for (int jdebug = idebug + 1; jdebug < NMOVES; jdebug++)
          HARDBUG(alpha_beta_cache_slot.ABCS_moves[jdebug] != INVALID)

        break;
      }

      int ndebug = 0;

      for (int jdebug = 0; jdebug < NMOVES; jdebug++)
      {
        if (alpha_beta_cache_slot.ABCS_moves[jdebug] ==
            alpha_beta_cache_slot.ABCS_moves[idebug])
          ndebug++;
      }
      HARDBUG(ndebug != 1)
    }
  }
#endif

  if (options.alpha_beta_cache_size > 0)
  {
    SOFTBUG(best_score > SCORE_PLUS_INFINITY)
    SOFTBUG(best_score < SCORE_MINUS_INFINITY)

    alpha_beta_cache_slot.ABCS_score = best_score;

    alpha_beta_cache_slot.ABCS_depth = *arg_best_depth;

    if (best_score <= arg_my_alpha)
    {
      ++(object->S_total_alpha_beta_cache_le_alpha_stored);

      alpha_beta_cache_slot.ABCS_flags = LE_ALPHA_BIT;
    }
    else if (best_score >= arg_my_beta)
    {
      ++(object->S_total_alpha_beta_cache_ge_beta_stored);

      alpha_beta_cache_slot.ABCS_score = arg_my_beta;

      alpha_beta_cache_slot.ABCS_flags = GE_BETA_BIT;
    }
    else
    {
      ++(object->S_total_alpha_beta_cache_true_score_stored);

      alpha_beta_cache_slot.ABCS_flags = TRUE_SCORE_BIT;
    }

    alpha_beta_cache_slot.ABCS_key = object->S_board.B_key;

    alpha_beta_cache_slot.ABCS_crc64 =
        return_crc64_from_crc32(&alpha_beta_cache_slot);

    update_alpha_beta_cache(object,
                            arg_node_type,
                            alpha_beta_cache[my_colour],
                            &alpha_beta_cache_slot);
  }

label_return:

  if (FALSE and (*arg_best_depth == DEPTH_MAX))
  {
    HARDBUG((best_score < (SCORE_WON - NODE_MAX)) and
            (best_score > (SCORE_LOST + NODE_MAX)) and (best_score != 0))
  }

  if (best_score >= arg_my_beta)
  {
    best_score = arg_my_beta;
    //*arg_best_depth = arg_my_depth;
  }

  END_BLOCK

  POP_NAME(__FUNC__)

  return (best_score);
}

void do_pv(search_t *object,
           pv_t *arg_best_pv,
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

  int jmove = *arg_best_pv;

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

    do_pv(object, arg_best_pv + 1, -arg_pv_score, arg_bpv_string);

    undo_move(&(object->S_board), jmove, &moves_list, FALSE);
  }
}

local void print_pv(search_t *object,
                    int arg_my_depth,
                    int arg_imove,
                    int arg_jmove,
                    moves_list_t *arg_moves_list,
                    int arg_move_score,
                    pv_t *arg_best_pv,
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

  do_pv(object, arg_best_pv + 1, -arg_move_score, bpv_string);

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

  SOFTBUG(arg_moves_list->ML_nmoves == 0)

  if (arg_depth_min == INVALID)
    arg_depth_min = 1;
  if (arg_depth_max == INVALID)
    arg_depth_max = options.depth_limit;

  object->S_interrupt = 0;

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

      SOFTBUG(temp_mate == ENDGAME_UNKNOWN)

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

        object->S_best_pv[0] = jmove;
        object->S_best_pv[1] = INVALID;

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

    SOFTBUG(object->S_best_score == SCORE_MINUS_INFINITY)

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

  return_network_score_scaled(&(object->S_board.B_network_thread));

  object->S_root_simple_score = return_score(object);

  pv_t temp_pv[PV_MAX];

  int temp_depth;

  if (FALSE and (arg_moves_list->ML_ncaptx == 0) and
      (arg_moves_list->ML_nmoves > 2))
  {
    do_move(&(object->S_board), INVALID, NULL, FALSE);

    object->S_root_score = -alpha_beta(object,
                                       SCORE_MINUS_INFINITY,
                                       SCORE_PLUS_INFINITY,
                                       1,
                                       PV_BIT,
                                       options.reduction_depth_root,
                                       TWEAK_PREVIOUS_MOVE_NULL_BIT,
                                       0xFULL,
                                       temp_pv + 1,
                                       &temp_depth);

    undo_move(&(object->S_board), INVALID, NULL, FALSE);
  }
  else
  {
    object->S_root_score = alpha_beta(object,
                                      SCORE_MINUS_INFINITY,
                                      SCORE_PLUS_INFINITY,
                                      1,
                                      PV_BIT,
                                      options.reduction_depth_root,
                                      0,
                                      0xFULL,
                                      temp_pv,
                                      &temp_depth);
  }

  if (object->S_interrupt != 0)
  {
    object->S_root_score = return_score(object);

    object->S_best_score_kind = SEARCH_BEST_SCORE_AB;

    object->S_best_move = 0;

    object->S_best_depth = INVALID;

    object->S_best_pv[0] = 0;
    object->S_best_pv[1] = INVALID;

    goto label_limit;
  }

  my_printf(object->S_my_printf,
            "root_simple_score=%d root_score=%d\n",
            object->S_root_simple_score,
            object->S_root_score);

  object->S_best_score_kind = SEARCH_BEST_SCORE_AB;

  object->S_best_move = 0;

  object->S_best_depth = 0;

  object->S_best_pv[0] = 0;
  object->S_best_pv[1] = INVALID;

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

          do_move(&(object->S_board), jmove, arg_moves_list, FALSE);

          move_score = -alpha_beta(object,
                                   -my_beta,
                                   -my_alpha,
                                   your_depth,
                                   node_type,
                                   options.reduction_depth_root,
                                   your_tweaks,
                                   0xFULL,
                                   temp_pv + 1,
                                   &temp_depth);

          undo_move(&(object->S_board), jmove, arg_moves_list, FALSE);

          if (object->S_interrupt != 0)
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
                     temp_pv,
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
                     temp_pv,
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
                     temp_pv,
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

          // with->S_best_depth = temp_depth + my_depth - your_depth;

          object->S_best_depth = my_depth;

          object->S_best_score_by_depth[my_depth] = object->S_best_score;

          object->S_best_pv[0] = jmove;

          for (int ipv = 1; (object->S_best_pv[ipv] = temp_pv[ipv]) != INVALID;
               ipv++)
            ;

          print_pv(object,
                   my_depth,
                   imove,
                   jmove,
                   arg_moves_list,
                   move_score,
                   object->S_best_pv,
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

      if (object->S_best_score > (SCORE_WON - NODE_MAX))
        break;
      if (object->S_best_score < (SCORE_LOST + NODE_MAX))
        break;

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
