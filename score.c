//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#define the_score_cache(X) cat2(X, _score_cache)
#define my_score_cache     the_score_cache(my_colour)
#define your_score_cache   the_score_cache(your_colour)

typedef struct
{ 
  ui64_t SCS_key;
  i32_t SCS_score;
  ui32_t SCS_crc32;
} score_cache_slot_t;

local i64_t nscore_cache_entries;
local score_cache_slot_t score_cache_slot_default;
local score_cache_slot_t *white_score_cache;
local score_cache_slot_t *black_score_cache;

local int probe_score_cache(search_t *object,
  score_cache_slot_t *arg_score_cache,
  score_cache_slot_t *arg_score_cache_slot)
{
  int result = FALSE;

  *arg_score_cache_slot = 
    arg_score_cache[object->S_board.B_key % nscore_cache_entries];

  ui32_t crc32 = 0xFFFFFFFF;
  crc32 = HW_CRC32_U64(crc32, arg_score_cache_slot->SCS_key);
  crc32 = HW_CRC32_U32(crc32, arg_score_cache_slot->SCS_score);
  crc32 = ~crc32;
    
  if (crc32 != arg_score_cache_slot->SCS_crc32)
  {
    object->S_total_score_cache_crc32_errors++;

    *arg_score_cache_slot = score_cache_slot_default;
  }
  else if (arg_score_cache_slot->SCS_key == object->S_board.B_key)
  {
      result = TRUE;
  }
   
  return(result);
}

local void update_score_cache(search_t *object,
  score_cache_slot_t *arg_score_cache,
  score_cache_slot_t *arg_score_cache_slot)
{
  arg_score_cache[object->S_board.B_key % nscore_cache_entries] = 
    *arg_score_cache_slot;
}

#undef GEN_CSV

#define MY_BIT      WHITE_BIT
#define my_colour   white
#define your_colour black

#include "score.d"

#undef MY_BIT
#undef my_colour
#undef your_colour

#define MY_BIT      BLACK_BIT
#define my_colour   black
#define your_colour white

#include "score.d"

#undef MY_BIT
#undef my_colour
#undef your_colour

int return_npieces(board_t *self)
{
  board_t *object = self;

  return(BIT_COUNT(object->B_white_man_bb) +
         BIT_COUNT(object->B_white_king_bb) +
         BIT_COUNT(object->B_black_man_bb) +
         BIT_COUNT(object->B_black_king_bb));
}

int return_material_score(board_t *self)
{
  board_t *object = self;

  int nwhite_man = BIT_COUNT(object->B_white_man_bb);
  int nblack_man = BIT_COUNT(object->B_black_man_bb);
  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

  int material_score = (nwhite_man - nblack_man) * SCORE_MAN +
                       (nwhite_king - nblack_king) * SCORE_KING;
  if (IS_WHITE(object->B_colour2move))
    return(material_score);
  else
    return(-material_score);
}

local void clear_score_caches(void)
{
  PRINTF("clear_score_caches..\n");

  for (i64_t ientry = 0; ientry < nscore_cache_entries; ientry++)
    white_score_cache[ientry] = score_cache_slot_default;

  for (i64_t ientry = 0; ientry < nscore_cache_entries; ientry++)
    black_score_cache[ientry] = score_cache_slot_default;

  PRINTF("..done\n");
}

void init_score(void)
{
  PRINTF("sizeof(score_cache_slot_t)=%lld\n",
         (i64_t) sizeof(score_cache_slot_t));

  white_score_cache = NULL;
  black_score_cache = NULL;

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
  
    MY_MALLOC_BY_TYPE(white_score_cache, score_cache_slot_t, nscore_cache_entries)
  
    MY_MALLOC_BY_TYPE(black_score_cache, score_cache_slot_t, nscore_cache_entries)

    clear_score_caches();
  }
}
