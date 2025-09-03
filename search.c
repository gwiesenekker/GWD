//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#define TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT BIT(0)
#define TWEAK_PREVIOUS_MOVE_NULL_BIT       BIT(1)
#define TWEAK_PREVIOUS_MOVE_REDUCED_BIT    BIT(2)
#define TWEAK_PREVIOUS_MOVE_EXTENDED_BIT   BIT(3)

#define STAGE_SEARCH_MOVE_AT_REDUCED_DEPTH   BIT(0)
#define STAGE_SEARCH_MOVE_AT_FULL_DEPTH      BIT(1)
#define STAGE_SEARCHED_MOVE_AT_REDUCED_DEPTH BIT(2)
#define STAGE_SEARCHED_MOVE_AT_FULL_DEPTH    BIT(3)

#undef CHECK_TIME_LIMIT_IN_QUIESCENCE

#define UPDATE_NODES 1000

#define SEARCH_WINDOW   (SCORE_MAN / 4)

#define the_alpha_beta_cache(X) cat2(X, _alpha_beta_cache)
#define my_alpha_beta_cache     the_alpha_beta_cache(my_colour)
#define your_alpha_beta_cache   the_alpha_beta_cache(your_colour)

#define the_endgame_pv(X) cat2(X, _endgame_pv)
#define my_endgame_pv     the_endgame_pv(my_colour)
#define your_endgame_pv   the_endgame_pv(your_colour)

#define the_quiescence(X) cat2(X, _quiescence)
#define my_quiescence     the_quiescence(my_colour)
#define your_quiescence   the_quiescence(your_colour)

#define the_alpha_beta(X) cat2(X, _alpha_beta)
#define my_alpha_beta     the_alpha_beta(my_colour)
#define your_alpha_beta   the_alpha_beta(your_colour)

#define the_pv(X) cat2(X, _pv)
#define my_pv     the_pv(my_colour)
#define your_pv   the_pv(your_colour)

#define print_the_pv(X) cat3(print_, X, _pv)
#define print_my_pv     print_the_pv(my_colour)
#define print_your_pv   print_the_pv(your_colour)

#define the_search(X) cat2(X, _search)
#define my_search     the_search(my_colour)

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
  ui32_t ABCS_alignment;
  ui32_t ABCS_crc32;
} alpha_beta_cache_slot_t;

typedef struct
{
  alpha_beta_cache_slot_t ABCE_slots[NSLOTS];
} alpha_beta_cache_entry_t;

local i64_t nalpha_beta_pv_cache_entries;
local i64_t nalpha_beta_cache_entries;
local alpha_beta_cache_slot_t alpha_beta_cache_slot_default;
local alpha_beta_cache_entry_t alpha_beta_cache_entry_default;
local alpha_beta_cache_entry_t *white_alpha_beta_cache;
local alpha_beta_cache_entry_t *black_alpha_beta_cache;

local i64_t global_nodes;

local bucket_t bucket_depth;

#undef LAZY_STATS
#ifdef LAZY_STATS
local stats_t lazy_stats[2 * NPIECES_MAX + 1][2 * NPIECES_MAX + 1];
#endif

local void white_endgame_pv(search_t *, int, bstring);
local void black_endgame_pv(search_t *, int, bstring);

local int white_quiescence(search_t *, int, int, int, moves_list_t *,
  pv_t *, int *, int);
local int black_quiescence(search_t *, int, int, int, moves_list_t *,
  pv_t *, int *, int);

local int white_alpha_beta(search_t *, int, int, int, int, int, int,
  pv_t *, int *);
local int black_alpha_beta(search_t *, int, int, int, int, int, int,
  pv_t *, int *);

local void white_pv(search_t *, pv_t *, int, bstring);
local void black_pv(search_t *, pv_t *, int, bstring);

local void print_white_pv(search_t *, int, int, int, moves_list_t *, int,
  pv_t *, char *);
local void print_black_pv(search_t *, int, int, int, moves_list_t *, int,
  pv_t *, char *);

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

  return(result);
}

local int move_repetition(board_t *object)
{
  if ((object->B_inode - 2) < 0) return(FALSE);

  int npieces;

  if (IS_WHITE(object->B_colour2move))
  {
    npieces = BIT_COUNT(object->B_white_man_bb | object->B_white_king_bb);
  }
  else
  {
    npieces = BIT_COUNT(object->B_black_man_bb | object->B_black_king_bb);
  }

  if (npieces > 1) return(FALSE);

  //50-45
  //
  //45-50
  // 
  //50-45
  //
  //45-50
  // 
  //50-45
  //
  //45-50
  // 
  //so now black will play 50-45 again
  //we look two inodes back for 45-40, and then two more inodes back
  //as Zobrist hashing will return the same key for 50-45 and 45-50

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

        return(TRUE);
      }
    }
    else
    {
      return(FALSE);
    }
  }

  return(FALSE);
}

void clear_totals(search_t *object)
{
  object->S_total_move_repetitions = 0;

  object->S_total_quiescence_nodes = 0;
  object->S_total_quiescence_all_moves_captures_only = 0;
  object->S_total_quiescence_all_moves_le2_moves = 0;
  object->S_total_quiescence_all_moves_extended = 0;

  object->S_total_nodes = 0;
  object->S_total_alpha_beta_nodes = 0;
  object->S_total_minimal_window_nodes = 0;
  object->S_total_pv_nodes = 0;

  object->S_total_quiescence_extension_searches = 0;
  object->S_total_quiescence_extension_searches_le_alpha = 0;
  object->S_total_quiescence_extension_searches_ge_beta = 0;

  object->S_total_pv_extension_searches = 0;
  object->S_total_pv_extension_searches_le_alpha = 0;
  object->S_total_pv_extension_searches_ge_beta = 0;

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
  object->S_total_alpha_beta_cache_crc32_errors = 0;

  object->S_total_score_cache_hits = 0;
  object->S_total_score_cache_crc32_errors = 0;
}

#define PRINTF_TOTAL(X) my_printf(object->S_my_printf, #X "=%lld\n", object->X);

void print_totals(search_t *object)
{
  PRINTF_TOTAL(S_total_move_repetitions)

  PRINTF_TOTAL(S_total_quiescence_nodes)
  PRINTF_TOTAL(S_total_quiescence_all_moves_captures_only)
  PRINTF_TOTAL(S_total_quiescence_all_moves_le2_moves)
  PRINTF_TOTAL(S_total_quiescence_all_moves_extended)

  PRINTF_TOTAL(S_total_nodes)
  PRINTF_TOTAL(S_total_alpha_beta_nodes)
  PRINTF_TOTAL(S_total_minimal_window_nodes)
  PRINTF_TOTAL(S_total_pv_nodes)

  PRINTF_TOTAL(S_total_quiescence_extension_searches)
  PRINTF_TOTAL(S_total_quiescence_extension_searches_le_alpha)
  PRINTF_TOTAL(S_total_quiescence_extension_searches_ge_beta)

  PRINTF_TOTAL(S_total_pv_extension_searches)
  PRINTF_TOTAL(S_total_pv_extension_searches_le_alpha)
  PRINTF_TOTAL(S_total_pv_extension_searches_ge_beta)

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
  PRINTF_TOTAL(S_total_alpha_beta_cache_crc32_errors)

  PRINTF_TOTAL(S_total_score_cache_hits)
  PRINTF_TOTAL(S_total_score_cache_crc32_errors)
}

local int probe_alpha_beta_cache(search_t *object,
  int arg_node_type, int arg_pv_only,
  alpha_beta_cache_entry_t *arg_alpha_beta_cache,
  alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot)
{
  int result = FALSE;

  alpha_beta_cache_entry_t *with = NULL;

  if (IS_PV(arg_node_type))
  {
    with = arg_alpha_beta_cache +
           object->S_board.B_key % nalpha_beta_pv_cache_entries;

    int islot;

    for (islot = 0; islot < NSLOTS; islot++)
    {
      *arg_alpha_beta_cache_slot = with->ABCE_slots[islot];

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = HW_CRC32_U64(crc32, arg_alpha_beta_cache_slot->ABCS_key);
      crc32 = HW_CRC32_U64(crc32, arg_alpha_beta_cache_slot->ABCS_data);
      crc32 = ~crc32;
    
      if (crc32 != arg_alpha_beta_cache_slot->ABCS_crc32)
      {
        object->S_total_alpha_beta_cache_crc32_errors++;

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
    with = arg_alpha_beta_cache +
           nalpha_beta_pv_cache_entries + 
           object->S_board.B_key % nalpha_beta_cache_entries;

#ifdef DEBUG
    for (int idebug = 0; idebug < NSLOTS; idebug++)
    {
      if (with->ABCE_slots[idebug].ABCS_key == 0) continue;

      int ndebug = 0;
      
      for (int jdebug = 0; jdebug < NSLOTS; jdebug++)
      {
        if (with->ABCE_slots[idebug].ABCS_key ==
            with->ABCE_slots[jdebug].ABCS_key) ndebug++;
      }
      HARDBUG(ndebug != 1)
    }
#endif

    int islot;

    for (islot = 0; islot < NSLOTS; islot++)
    {
      *arg_alpha_beta_cache_slot = with->ABCE_slots[islot];

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = HW_CRC32_U64(crc32, arg_alpha_beta_cache_slot->ABCS_key);
      crc32 = HW_CRC32_U64(crc32, arg_alpha_beta_cache_slot->ABCS_data);
      crc32 = ~crc32;
    
      if (crc32 != arg_alpha_beta_cache_slot->ABCS_crc32)
      {
        object->S_total_alpha_beta_cache_crc32_errors++;

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
   
  return(result);
}

local void update_alpha_beta_cache_slot_moves(
  alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot, int arg_jmove)
{
  int kmove;
  
  for (kmove = 0; kmove < NMOVES; kmove++)
    if (arg_alpha_beta_cache_slot->ABCS_moves[kmove] == arg_jmove) break;
  
  if (kmove == NMOVES) kmove = NMOVES - 1;
  
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
          arg_alpha_beta_cache_slot->ABCS_moves[idebug]) ndebug++;
    }
    HARDBUG(ndebug != 1)
  }
#endif
}

local void update_alpha_beta_cache(search_t *object, int arg_node_type,
  alpha_beta_cache_entry_t *arg_alpha_beta_cache,
  alpha_beta_cache_slot_t *arg_alpha_beta_cache_slot)
{
  alpha_beta_cache_entry_t *with = NULL;

  if (IS_PV(arg_node_type))
  {
    with = arg_alpha_beta_cache +
           object->S_board.B_key % nalpha_beta_pv_cache_entries;
  }
  else
  {
    with = arg_alpha_beta_cache +
           nalpha_beta_pv_cache_entries + 
           object->S_board.B_key % nalpha_beta_cache_entries;
  }

  int islot = INVALID;

  for (islot = 0; islot < NSLOTS; islot++)
    if (with->ABCE_slots[islot].ABCS_key == object->S_board.B_key) break;

  if (islot == NSLOTS) islot = NSLOTS - 1;

  while(islot > 0)
  {
    with->ABCE_slots[islot] = with->ABCE_slots[islot - 1];
 
    --islot;
  }

  with->ABCE_slots[0] = *arg_alpha_beta_cache_slot;
}

#define MY_BIT      WHITE_BIT
#define YOUR_BIT    BLACK_BIT
#define my_colour   white
#define your_colour black

#include "search.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

#define MY_BIT      BLACK_BIT
#define YOUR_BIT    WHITE_BIT
#define my_colour   black
#define your_colour white

#include "search.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

local void clear_alpha_beta_caches(void)
{
  PRINTF("clear_alpha_beta_caches..\n");

  for (i64_t ientry = 0;
       ientry < (nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries);
       ientry++)
  {
    white_alpha_beta_cache[ientry] = alpha_beta_cache_entry_default;
  }

  for (i64_t ientry = 0;
       ientry < (nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries);
       ientry++)
  {
    black_alpha_beta_cache[ientry] = alpha_beta_cache_entry_default;
  }

  PRINTF("..done\n");
}

void construct_search(void *self,
  my_printf_t *arg_my_printf, void *arg_thread)
{
  search_t *object = self;

  object->S_my_printf = arg_my_printf;

  object->S_thread = arg_thread;

  construct_board(&(object->S_board), arg_my_printf);
  
  construct_my_timer(&(object->S_timer), "board",
    object->S_my_printf, FALSE);

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
                  bdata(bname), endgame_entry_cache_size,
                  CACHE_ENTRY_KEY_TYPE_I64_T, &key_default,
                  sizeof(entry_i16_t), &entry_i16_default);

  HARDBUG(bassigncstr(bname, "") == BSTR_ERR)

  HARDBUG(bformata(bname, "S_%p_wdl_entry_cache\n", object) == BSTR_ERR)

  construct_cache(&(object->S_endgame_wdl_entry_cache),
                  bdata(bname), endgame_entry_cache_size,
                  CACHE_ENTRY_KEY_TYPE_I64_T, &key_default,
                  sizeof(entry_i16_t), &entry_i16_default);

  BDESTROY(bname)
}

void init_search(void)
{
  PRINTF("sizeof(alpha_beta_cache_entry_t)=%lld\n",
         (i64_t) sizeof(alpha_beta_cache_entry_t));

  global_nodes = 0;

  //alpha_beta cache

  white_alpha_beta_cache = NULL;
  black_alpha_beta_cache = NULL;

  nalpha_beta_pv_cache_entries = 0;
  nalpha_beta_cache_entries = 0;

  if (options.alpha_beta_cache_size > 0)
  {
    float alpha_beta_cache_size = options.alpha_beta_cache_size * MBYTE / 2;

    nalpha_beta_pv_cache_entries = 
      first_prime_below(roundf(alpha_beta_cache_size *
                               (options.pv_cache_fraction / 100.0f) /
                               sizeof(alpha_beta_cache_entry_t)));
    
    HARDBUG(nalpha_beta_pv_cache_entries < 3)
  
    nalpha_beta_cache_entries = 
      first_prime_below(roundf(alpha_beta_cache_size *
                               ((100 - options.pv_cache_fraction) / 100.0f) /
                               sizeof(alpha_beta_cache_entry_t)));
  
    HARDBUG(nalpha_beta_cache_entries < 3)
  
    PRINTF("nalpha_beta_pv_cache_entries=%lld nalpha_beta_cache_entries=%lld\n",
      nalpha_beta_pv_cache_entries, nalpha_beta_cache_entries);
  
    alpha_beta_cache_slot_default.ABCS_key = 0;
    alpha_beta_cache_slot_default.ABCS_score = SCORE_MINUS_INFINITY;
    alpha_beta_cache_slot_default.ABCS_depth = INVALID;
    alpha_beta_cache_slot_default.ABCS_flags = 0;
    for (int imove = 0; imove < NMOVES; imove++) 
      alpha_beta_cache_slot_default.ABCS_moves[imove] = INVALID;
  
    ui32_t crc32 = 0xFFFFFFFF;
    crc32 = HW_CRC32_U64(crc32, alpha_beta_cache_slot_default.ABCS_key);
    crc32 = HW_CRC32_U64(crc32, alpha_beta_cache_slot_default.ABCS_data);
    alpha_beta_cache_slot_default.ABCS_crc32 = ~crc32;
  
    for (int islot = 0; islot < NSLOTS; islot++)
      alpha_beta_cache_entry_default.ABCE_slots[islot] = 
        alpha_beta_cache_slot_default;
  
    MY_MALLOC_BY_TYPE(white_alpha_beta_cache, alpha_beta_cache_entry_t,
              nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries)
  
    MY_MALLOC_BY_TYPE(black_alpha_beta_cache, alpha_beta_cache_entry_t,
              nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries)

    clear_alpha_beta_caches();
  }

  construct_bucket(&bucket_depth, "bucket_depth",
                   1, 0, DEPTH_MAX, BUCKET_LINEAR);

#ifdef LAZY_STATS
  for (int ndelta_man = -NPIECES_MAX; ndelta_man <= NPIECES_MAX; ++ndelta_man)
  {
    for (int ndelta_king = -NPIECES_MAX; ndelta_king <= NPIECES_MAX;
         ++ndelta_king)
    {
      construct_stats(&(lazy_stats[ndelta_man + NPIECES_MAX][ndelta_king + NPIECES_MAX]), "lazy_stats");

    }
  }
#endif
}

void fin_search(void)
{
}

void do_search(search_t *with, moves_list_t *moves_list,
  int depth_min, int depth_max, int root_score,
  my_random_t *shuffle)
{
  if (IS_WHITE(with->S_board.B_colour2move))
    white_search(with, moves_list, depth_min, depth_max, root_score,
      shuffle);
  else
    black_search(with, moves_list, depth_min, depth_max, root_score,
      shuffle);
}

