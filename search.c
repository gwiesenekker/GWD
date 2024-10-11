//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"


#define TWEAK_PREVIOUS_MOVE_REDUCED_BIT  BIT(0)
#define TWEAK_PREVIOUS_MOVE_CAPTURE_BIT  BIT(1)
#define TWEAK_PREVIOUS_MOVE_EXTENDED_BIT BIT(2)

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

local bucket_t *bucket_depth;

local void white_endgame_pv(board_t *, int, char *);
local void black_endgame_pv(board_t *, int, char *);

local int white_quiescence(board_t *, int, int, int, moves_list_t *,
  pv_t *, int *, int);
local int black_quiescence(board_t *, int, int, int, moves_list_t *,
  pv_t *, int *, int);

local int white_alpha_beta(board_t *, int, int, int, int, int, int,
  pv_t *, int *);
local int black_alpha_beta(board_t *, int, int, int, int, int, int,
  pv_t *, int *);

local void white_pv(board_t *, pv_t *, int, char *);
local void black_pv(board_t *, pv_t *, int, char *);

local void print_white_pv(board_t *, int, int, int, moves_list_t *, int,
  pv_t *, char *);
local void print_black_pv(board_t *, int, int, int, moves_list_t *, int,
  pv_t *, char *);

int draw_by_repetition(board_t *with, int strict)
{
  int result = FALSE;

  int n = 1;

  for (int jnode = with->board_inode - 2; jnode >= 0; jnode -= 2)
  {
    if (with->board_nodes[jnode + 1].node_ncaptx > 0)
      goto label_return;

    if (with->board_nodes[jnode].node_ncaptx > 0)
      goto label_return;

    if (HASH_KEY_EQ(with->board_nodes[jnode].node_key, with->board_key))
    {
      ++n;
      if (!strict or (n >= 3))
      {
        result = TRUE;
        goto label_return;
      }
    }
  }

  label_return:
  return(result);
}

local int move_repetition(board_t *with)
{
  if ((with->board_inode - 2) < 0) return(FALSE);

  int npieces;

  if (IS_WHITE(with->board_colour2move))
  {
    npieces = BIT_COUNT(with->board_white_man_bb | with->board_white_crown_bb);
  }
  else
  {
    npieces = BIT_COUNT(with->board_black_man_bb | with->board_black_crown_bb);
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

  for (int jnode = with->board_inode - 4; jnode >= 0; jnode -= 2)
  {
    HARDBUG(with->board_nodes[jnode].node_move_key == 0)

    if (HASH_KEY_EQ(with->board_nodes[with->board_inode - 2].node_move_key, 
                    with->board_nodes[jnode].node_move_key))
    {
      ++n;

      if (n >= 3)
      {
        ++(with->total_move_repetitions);

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

void clear_totals(board_t *with)
{
  with->total_move_repetitions = 0;

  with->total_quiescence_nodes = 0;
  with->total_quiescence_all_moves_captures_only = 0;
  with->total_quiescence_all_moves_le2_moves = 0;
  with->total_quiescence_all_moves_tactical = 0;

  with->total_nodes = 0;
  with->total_alpha_beta_nodes = 0;
  with->total_minimal_window_nodes = 0;
  with->total_pv_nodes = 0;

  with->total_reductions_delta_strong = 0;
  with->total_reductions_delta_strong_lost = 0;
  with->total_reductions_delta_strong_le_alpha = 0;
  with->total_reductions_delta_strong_ge_beta = 0;

  with->total_reductions = 0;
  with->total_reductions_simple = 0;
  with->total_reductions_le_alpha = 0;
  with->total_reductions_ge_beta = 0;

  with->total_single_reply_extensions = 0;

  with->total_evaluations = 0;
  with->total_material_only_evaluations = 0;
  with->total_neural_evaluations = 0;

  with->total_alpha_beta_cache_hits = 0;
  with->total_alpha_beta_cache_depth_hits = 0;
  with->total_alpha_beta_cache_le_alpha_hits = 0;
  with->total_alpha_beta_cache_le_alpha_cutoffs = 0;
  with->total_alpha_beta_cache_ge_beta_hits = 0;
  with->total_alpha_beta_cache_ge_beta_cutoffs = 0;
  with->total_alpha_beta_cache_true_score_hits = 0;
  with->total_alpha_beta_cache_le_alpha_stored = 0;
  with->total_alpha_beta_cache_ge_beta_stored = 0;
  with->total_alpha_beta_cache_true_score_stored = 0;
  with->total_alpha_beta_cache_nmoves_errors = 0;
  with->total_alpha_beta_cache_crc32_errors = 0;
}

#define PRINTF_TOTAL(X) my_printf(with->board_my_printf, #X "=%lld\n", with->X);

void print_totals(board_t *with)
{
  PRINTF_TOTAL(total_move_repetitions)

  PRINTF_TOTAL(total_quiescence_nodes)
  PRINTF_TOTAL(total_quiescence_all_moves_captures_only)
  PRINTF_TOTAL(total_quiescence_all_moves_le2_moves)
  PRINTF_TOTAL(total_quiescence_all_moves_tactical)

  PRINTF_TOTAL(total_nodes)
  PRINTF_TOTAL(total_alpha_beta_nodes)
  PRINTF_TOTAL(total_minimal_window_nodes)
  PRINTF_TOTAL(total_pv_nodes)

  PRINTF_TOTAL(total_reductions_delta_strong)
  PRINTF_TOTAL(total_reductions_delta_strong_lost)
  PRINTF_TOTAL(total_reductions_delta_strong_le_alpha)
  PRINTF_TOTAL(total_reductions_delta_strong_ge_beta)

  PRINTF_TOTAL(total_reductions)
  PRINTF_TOTAL(total_reductions_simple)
  PRINTF_TOTAL(total_reductions_le_alpha)
  PRINTF_TOTAL(total_reductions_ge_beta)

  PRINTF_TOTAL(total_single_reply_extensions)

  PRINTF_TOTAL(total_evaluations)
  PRINTF_TOTAL(total_material_only_evaluations)
  PRINTF_TOTAL(total_neural_evaluations)

  PRINTF_TOTAL(total_alpha_beta_cache_hits)
  PRINTF_TOTAL(total_alpha_beta_cache_depth_hits)
  PRINTF_TOTAL(total_alpha_beta_cache_le_alpha_hits)
  PRINTF_TOTAL(total_alpha_beta_cache_le_alpha_cutoffs)
  PRINTF_TOTAL(total_alpha_beta_cache_ge_beta_hits)
  PRINTF_TOTAL(total_alpha_beta_cache_ge_beta_cutoffs)
  PRINTF_TOTAL(total_alpha_beta_cache_true_score_hits)
  PRINTF_TOTAL(total_alpha_beta_cache_le_alpha_stored)
  PRINTF_TOTAL(total_alpha_beta_cache_ge_beta_stored)
  PRINTF_TOTAL(total_alpha_beta_cache_true_score_stored)
  PRINTF_TOTAL(total_alpha_beta_cache_nmoves_errors)
  PRINTF_TOTAL(total_alpha_beta_cache_crc32_errors)
}

local int probe_alpha_beta_cache(board_t *with, int node_type, int pv_only,
  alpha_beta_cache_entry_t *alpha_beta_cache,
  alpha_beta_cache_entry_t *alpha_beta_cache_entry,
  alpha_beta_cache_slot_t **alpha_beta_cache_slot)
{
  int result = FALSE;

  if (IS_PV(node_type))
  {
    *alpha_beta_cache_entry =
      alpha_beta_cache[with->board_key % nalpha_beta_pv_cache_entries];

#ifdef DEBUG
    for (int idebug = 0; idebug < NSLOTS; idebug++)
    {
      if (alpha_beta_cache_entry->ABCE_slots[idebug].ABCS_key == 0) continue;

      int ndebug = 0;
      
      for (int jdebug = 0; jdebug < NSLOTS; jdebug++)
      {
        if (alpha_beta_cache_entry->ABCE_slots[idebug].ABCS_key ==
            alpha_beta_cache_entry->ABCE_slots[jdebug].ABCS_key) ndebug++;
      }
      HARDBUG(ndebug != 1)
    }
#endif

    int islot;

    for (islot = 0; islot < NSLOTS; islot++)
    {
      *alpha_beta_cache_slot = alpha_beta_cache_entry->ABCE_slots + islot;

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = _mm_crc32_u64(crc32, (*alpha_beta_cache_slot)->ABCS_key);
      crc32 = _mm_crc32_u64(crc32, (*alpha_beta_cache_slot)->ABCS_data);
      crc32 = ~crc32;
    
      if (crc32 != (*alpha_beta_cache_slot)->ABCS_crc32)
      {
        with->total_alpha_beta_cache_crc32_errors++;
 
        **alpha_beta_cache_slot = alpha_beta_cache_slot_default;
      }
      else if ((*alpha_beta_cache_slot)->ABCS_key == with->board_key)
      {
        result = TRUE;

        break;
      }
    }
  }

  if (!result and !pv_only)
  {
    *alpha_beta_cache_entry =
      alpha_beta_cache[nalpha_beta_pv_cache_entries + 
                       with->board_key % nalpha_beta_cache_entries];

#ifdef DEBUG
    for (int idebug = 0; idebug < NSLOTS; idebug++)
    {
      if (alpha_beta_cache_entry->ABCE_slots[idebug].ABCS_key == 0) continue;

      int ndebug = 0;
      
      for (int jdebug = 0; jdebug < NSLOTS; jdebug++)
      {
        if (alpha_beta_cache_entry->ABCE_slots[idebug].ABCS_key ==
            alpha_beta_cache_entry->ABCE_slots[jdebug].ABCS_key) ndebug++;
      }
      HARDBUG(ndebug != 1)
    }
#endif

    int islot;

    for (islot = 0; islot < NSLOTS; islot++)
    {
      *alpha_beta_cache_slot = alpha_beta_cache_entry->ABCE_slots + islot;

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = _mm_crc32_u64(crc32, (*alpha_beta_cache_slot)->ABCS_key);
      crc32 = _mm_crc32_u64(crc32, (*alpha_beta_cache_slot)->ABCS_data);
      crc32 = ~crc32;
    
      if (crc32 != (*alpha_beta_cache_slot)->ABCS_crc32)
      {
        with->total_alpha_beta_cache_crc32_errors++;
  
        **alpha_beta_cache_slot = alpha_beta_cache_slot_default;
      }
      else if ((*alpha_beta_cache_slot)->ABCS_key == with->board_key)
      {
        result = TRUE;

        break;
      }
    }
  }

  if (!result)
  {
    //find empty slot

    int islot;

    for (islot = 0; islot < NSLOTS; islot++)
    {
      *alpha_beta_cache_slot = alpha_beta_cache_entry->ABCE_slots + islot;

      if ((*alpha_beta_cache_slot)->ABCS_key == 0) break;
    }
    if (islot == NSLOTS)
    {
      *alpha_beta_cache_slot = alpha_beta_cache_entry->ABCE_slots +
                               with->board_key % NSLOTS;

      **alpha_beta_cache_slot = alpha_beta_cache_slot_default;
    }
  }

  return(result);
}

local void update_alpha_beta_cache_slot_moves(
  alpha_beta_cache_slot_t *alpha_beta_cache_slot, int jmove)
{
  int kmove;
  
  for (kmove = 0; kmove < NMOVES; kmove++)
    if (alpha_beta_cache_slot->ABCS_moves[kmove] == jmove) break;
  
  if (kmove == NMOVES) kmove = NMOVES - 1;
  
  while (kmove > 0)
  {
    alpha_beta_cache_slot->ABCS_moves[kmove] =
      alpha_beta_cache_slot->ABCS_moves[kmove - 1];
  
    kmove--;
  }
  
  alpha_beta_cache_slot->ABCS_moves[0] = jmove;

#ifdef DEBUG
  for (int idebug = 0; idebug < NMOVES; idebug++)
  {
    if (alpha_beta_cache_slot->ABCS_moves[idebug] == INVALID)
    {  
      for (int jdebug = idebug + 1; jdebug < NMOVES; jdebug++)
        HARDBUG(alpha_beta_cache_slot->ABCS_moves[jdebug] != INVALID)

      break;
    }

    int ndebug = 0;
    
    for (int jdebug = 0; jdebug < NMOVES; jdebug++)
    {
      if (alpha_beta_cache_slot->ABCS_moves[jdebug] == 
          alpha_beta_cache_slot->ABCS_moves[idebug]) ndebug++;
    }
    HARDBUG(ndebug != 1)
  }
#endif
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

void clear_caches(void)
{
  PRINTF("clear_caches..\n");

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

void init_search(void)
{
  PRINTF("sizeof(alpha_beta_cache_entry_t)=%lld\n",
         (i64_t) sizeof(alpha_beta_cache_entry_t));

  global_nodes = 0;

  //alpha_beta cache

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
  crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot_default.ABCS_key);
  crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot_default.ABCS_data);
  alpha_beta_cache_slot_default.ABCS_crc32 = ~crc32;

  for (int islot = 0; islot < NSLOTS; islot++)
    alpha_beta_cache_entry_default.ABCE_slots[islot] = 
      alpha_beta_cache_slot_default;

  MALLOC(white_alpha_beta_cache, alpha_beta_cache_entry_t,
         nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries)

  MALLOC(black_alpha_beta_cache, alpha_beta_cache_entry_t,
         nalpha_beta_pv_cache_entries + nalpha_beta_cache_entries)

  clear_caches();

  bucket_depth = bucket_class->objects_ctor();

  bucket_depth->define_bucket(bucket_depth, 1, 0, DEPTH_MAX,
    BUCKET_LINEAR);
}

void fin_search(void)
{
}

void search(board_t *with, moves_list_t *moves_list,
  int depth_min, int depth_max, int root_score,
  int shuffle)
{
  if (IS_WHITE(with->board_colour2move))
    white_search(with, moves_list, depth_min, depth_max, root_score,
      shuffle);
  else
    black_search(with, moves_list, depth_min, depth_max, root_score,
      shuffle);
}
