//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#include "globals.h"

#define IS_MINE(X) ((X) & MY_BIT)
#define IS_YOURS(X) ((X) & YOUR_BIT)

int map[1 + 50] =
{
  0,
    6, 7, 8, 9,10,
  11,12,13,14,15,
    17,18,19,20,21,
  22,23,24,25,26,
    28,29,30,31,32,
  33,34,35,36,37,
    39,40,41,42,43,
  44,45,46,47,48,
    50,51,52,53,54,
  55,56,57,58,59
};

int inverse_map[BOARD_MAX] =
{
  -1, -1, -1, -1, -1, -1,
     0,  1,  2,  3,  4,
   5,  6,  7,  8,  9, -1,
    10, 11, 12, 13, 14,
  15, 16, 17, 18, 19, -1,
    20, 21, 22, 23, 24,
  25, 26, 27, 28, 29, -1,
    30, 31, 32, 33, 34,
  35, 36, 37, 38, 39, -1,
    40, 41, 42, 43, 44,
  45, 46, 47, 48, 49, -1,
    -1, -1, -1, -1, -1
};

char *nota[BOARD_MAX] =
{
  "??","??","??","??","??","??",
    "01","02","03","04","05",
  "06","07","08","09","10","??",
    "11","12","13","14","15",
  "16","17","18","19","20","??",
    "21","22","23","24","25",
  "26","27","28","29","30","??",
    "31","32","33","34","35",
  "36","37","38","39","40","??",
    "41","42","43","44","45",
  "46","47","48","49","50","??",
    "??","??","??","??","??"
};

int white_row[BOARD_MAX] = 
{
  -1, -1, -1, -1, -1, -1,
    10, 10, 10, 10, 10,
   9,  9,  9,  9,  9, -1,
     8,  8,  8,  8,  8,
   7,  7,  7,  7,  7, -1,
     6,  6,  6,  6,  6,
   5,  5,  5,  5,  5, -1,
     4,  4,  4,  4,  4,
   3,  3,  3,  3,  3, -1,
     2,  2,  2,  2,  2,
   1,  1,  1,  1,  1, -1,
    -1, -1, -1, -1, -1
};

int black_row[BOARD_MAX] = 
{
  -1, -1, -1, -1, -1, -1,
     1,  1,  1,  1,  1,
   2,  2,  2,  2,  2, -1,
     3,  3,  3,  3,  3,
   4,  4,  4,  4,  4, -1,
     5,  5,  5,  5,  5,
   6,  6,  6,  6,  6, -1,
     7,  7,  7,  7,  7,
   8,  8,  8,  8,  8, -1,
     9,  9,  9,  9,  9,
  10, 10, 10, 10, 10, -1,
    -1, -1, -1, -1, -1
};

my_random_t boards_random;

hash_key_t white_man_key[BOARD_MAX];
hash_key_t white_king_key[BOARD_MAX];

hash_key_t black_man_key[BOARD_MAX];
hash_key_t black_king_key[BOARD_MAX];

ui64_t left_wing_bb;
ui64_t center_bb;
ui64_t right_wing_bb;

ui64_t left_half_bb;
ui64_t right_half_bb;

ui64_t white_row_empty_bb;
ui64_t black_row_empty_bb;

#define MY_BIT      WHITE_BIT
#define YOUR_BIT    BLACK_BIT
#define my_colour   white
#define your_colour black
#include "boards.d"

#undef my_colour
#undef your_colour
#undef MY_BIT
#undef YOUR_BIT

#define MY_BIT      BLACK_BIT
#define YOUR_BIT    WHITE_BIT
#define my_colour   black
#define your_colour white

#include "boards.d"

#undef my_colour
#undef your_colour
#undef MY_BIT
#undef YOUR_BIT

void construct_board(void *self, my_printf_t *arg_my_printf, int arg_skip_load)
{
  board_t *object = self;

  object->board_my_printf = arg_my_printf;

  MY_MALLOC(object->board_pattern_mask, pattern_mask_t, 1)

  construct_network(&(object->board_network), options.network_name,
    arg_skip_load, TRUE);

  object->board_nstate = 0;
}

void push_board_state(void *self)
{
  board_t *object = self;

  HARDBUG(object->board_nstate >= NODE_MAX)
  
  board_state_t *state = object->board_states + object->board_nstate;

  state->BS_key = object->board_key;
  state->BS_white_man_bb = object->board_white_man_bb;
  state->BS_white_king_bb = object->board_white_king_bb;
  state->BS_black_man_bb = object->board_black_man_bb;
  state->BS_black_king_bb = object->board_black_king_bb;
 
  state->BS_npieces = 
    BIT_COUNT(object->board_white_man_bb) +
    BIT_COUNT(object->board_white_king_bb) +
    BIT_COUNT(object->board_black_man_bb) +
    BIT_COUNT(object->board_black_king_bb);

  object->board_nstate++;
}

void pop_board_state(void *self)
{
  board_t *object = self;

  object->board_nstate--;

  HARDBUG(object->board_nstate < 0)
  
  board_state_t *state = object->board_states + object->board_nstate;

  object->board_key = state->BS_key;
  object->board_white_man_bb = state->BS_white_man_bb;
  object->board_white_king_bb = state->BS_white_king_bb;
  object->board_black_man_bb = state->BS_black_man_bb;
  object->board_black_king_bb = state->BS_black_king_bb;

  SOFTBUG(state->BS_npieces !=
          (BIT_COUNT(object->board_white_man_bb) +
           BIT_COUNT(object->board_white_king_bb) +
           BIT_COUNT(object->board_black_man_bb) +
           BIT_COUNT(object->board_black_king_bb)))
}

hash_key_t return_key_from_bb(void *self)
{
  board_t *object = self;

  hash_key_t result;

  CLR_HASH_KEY(result)

  ui64_t bb = object->board_white_man_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, white_man_key[iboard])

    bb &= ~BITULL(iboard);
  }

  bb = object->board_white_king_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, white_king_key[iboard])

    bb &= ~BITULL(iboard);
  }

  bb = object->board_black_man_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, black_man_key[iboard])

    bb &= ~BITULL(iboard);
  }

  bb = object->board_black_king_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, black_king_key[iboard])

    bb &= ~BITULL(iboard);
  }

  return(result);
}

void string2board(board_t *self, char *arg_s, int arg_init_score)
{
  board_t *object = self;

  object->board_empty_bb = 0;

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    object->board_empty_bb |= BITULL(iboard);
  }

  object->board_white_man_bb = 0;
  object->board_white_king_bb = 0;
  object->board_black_man_bb = 0;
  object->board_black_king_bb = 0;

  char wO_local = *wO;
  char wX_local = *wX;
  char bO_local = *bO;
  char bX_local = *bX;
  char nn_local = *nn;

  for (int i = 1; i <= 50; ++i)
  { 
    if ((arg_s[i] == *wO_hub) or (arg_s[i] == *wX_hub))
    {
      wO_local = *wO_hub;
      wX_local = *wX_hub;
      bO_local = *bO_hub;
      bX_local = *bX_hub;
      nn_local = *nn_hub;

      break;
    } 
  }

  for (int i = 1; i <= 50; ++i)
  {
    int iboard;

    iboard = map[i];

    if (arg_s[i] == wO_local)
    {
      HARDBUG(i <= 5)

      object->board_white_man_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == wX_local)
    {
      object->board_white_king_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == bO_local)
    {
      HARDBUG(i >= 46)

      object->board_black_man_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == bX_local)
    {
      object->board_black_king_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == nn_local)
    {
      //do nothing
    }
    else
      FATAL("arg_s[i] error", EXIT_FAILURE)
  }

  object->board_key = return_key_from_bb(object);

  object->board_nstate = 0;

  object->board_inode = 0;

  node_t *node = object->board_nodes;

  node->node_ncaptx = INVALID;
  node->node_move_key = 0;

  if ((*arg_s == 'w') or (*arg_s == 'W'))
    object->board_colour2move = WHITE_BIT;
  else if ((*arg_s == 'b') or (*arg_s == 'B'))
    object->board_colour2move = BLACK_BIT;
  else
    FATAL("colour2move error", EXIT_FAILURE)

  board2patterns(object);

  board2network(object, FALSE);

  if (arg_init_score)
    return_network_score_scaled(&(object->board_network), FALSE, FALSE);
}

void fen2board(board_t *self, char *arg_fen, int arg_init_score)
{
  char s[51];
  for (int i = 1; i <= 50; ++i) s[i] = *nn;

  char *f = arg_fen;

  //W or B

  if (*f == 'W')
    s[0] = 'w';
  else if (*f == 'B')
    s[0] = 'b';
  else
    FATAL("s[0] error", EXIT_FAILURE)

  f++;

  //:

  HARDBUG(*f != ':')

  f++;

  //W or B

  HARDBUG((*f != 'W') and (*f != 'B'))

  char c = *f++;

  char k = 'M';

  while(TRUE)
  {
    if ((*f == '.') or (*f == '\0')) break;

    if (*f == '"') break;

    if ((*f == 'W') or (*f == 'B'))
    {
      c = *f++;
      continue;
    }

    if (*f == 'K')
    {
      k = *f++;
      continue;
    }

    if (isdigit(*f))
    {
      int i = 0;
      while(isdigit(*f))
      {
        i = i * 10 + (*f - '0');
        f++;
      }
      HARDBUG(i < 1)
      HARDBUG(i > 50)
      
      int j = i;

      if (*f == '-')
      {
        f++;

        HARDBUG(!isdigit(*f))

        j = 0;

        while(isdigit(*f))
        {
          j = j * 10 + (*f - '0');
          f++;
        }

        HARDBUG(j < 1)
        HARDBUG(j > 50)
      }

      while(i <= j)
      {
        if (k == 'M')
        {
          if (c == 'W')
            s[i] = *wO;
          else if (c == 'B')
            s[i] = *bO;
          else
            FATAL("c error", EXIT_FAILURE)
        }
        else if (k == 'K')
        {
          if (c == 'W')
            s[i] = *wX;
          else if (c == 'B')
            s[i] = *bX;
          else
            FATAL("c error", EXIT_FAILURE)
        }
        else
          FATAL("k error", EXIT_FAILURE)

        i++;
      }

      k = 'M';

      continue;
    }
  
    HARDBUG((*f != ',') and (*f != ':'))

    f++;
  }

  string2board(self, s, arg_init_score);
}

void board2fen(board_t *self, bstring arg_bfen, int arg_brackets)
{
  board_t *object = self;

  BSTRING(bwfen)

  HARDBUG(bassigncstr(bwfen, ":W") == BSTR_ERR)

  BSTRING(bbfen)

  HARDBUG(bassigncstr(bbfen, ":B") == BSTR_ERR)

  btrunc(arg_bfen, 0);

  int wcomma = FALSE;
  int bcomma = FALSE;

  if (arg_brackets) bcatcstr(arg_bfen, "[FEN \"");

  if (IS_WHITE(object->board_colour2move))
    bcatcstr(arg_bfen, "W");
  else
    bcatcstr(arg_bfen, "B");

  ui64_t white_bb = object->board_white_man_bb | object->board_white_king_bb;
  ui64_t black_bb = object->board_black_man_bb | object->board_black_king_bb;

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    if (white_bb & BITULL(iboard))
    {
      if (wcomma) bcatcstr(bwfen, ",");

      if (object->board_white_king_bb & BITULL(iboard))
        bcatcstr(bwfen, "K");

      bcatcstr(bwfen, nota[iboard]);

      wcomma = TRUE;
    }
    else if (black_bb & BITULL(iboard))
    {
      if (bcomma) bcatcstr(bbfen, ",");

      if (object->board_black_king_bb & BITULL(iboard))
        bcatcstr(bbfen, "K");

      bcatcstr(bbfen, nota[iboard]);

      bcomma = TRUE;
    }
  }

  bconcat(arg_bfen, bwfen);
  bconcat(arg_bfen, bbfen);

  if (arg_brackets) bcatcstr(arg_bfen, ".\"]");

  BDESTROY(bbfen)
  BDESTROY(bwfen)
}

void print_board(board_t *self)
{
  board_t *object = self;

  int i = 0;
  for (int irow = 10; irow >= 1; --irow)
  {
    if ((irow & 1) == 0) my_printf(object->board_my_printf, "  ");
    for (int icol = 1; icol <= 5; ++icol)
    {
      ++i;

      int iboard = map[i];

      if (object->board_white_man_bb & BITULL(iboard))
        my_printf(object->board_my_printf, "  wO");
      else if (object->board_white_king_bb & BITULL(iboard))
        my_printf(object->board_my_printf, "  wX");
      else if (object->board_black_man_bb & BITULL(iboard))
        my_printf(object->board_my_printf, "  bO");
      else if (object->board_black_king_bb & BITULL(iboard))
        my_printf(object->board_my_printf, "  bX");
      else
        my_printf(object->board_my_printf, "  ..");
    }
    my_printf(object->board_my_printf, "\n");
  }
  my_printf(object->board_my_printf, 
    "(%d) (%dx%d) (%dx%d)",
    BIT_COUNT(object->board_white_man_bb | object->board_white_king_bb | 
              object->board_black_man_bb | object->board_black_king_bb),
    BIT_COUNT(object->board_white_man_bb),
    BIT_COUNT(object->board_black_man_bb),
    BIT_COUNT(object->board_white_king_bb),
    BIT_COUNT(object->board_black_king_bb));

  if (IS_WHITE(object->board_colour2move))
    my_printf(object->board_my_printf, " WHITE to move\n");
  else if (IS_BLACK(object->board_colour2move))
    my_printf(object->board_my_printf, " BLACK to move\n");
  else
    FATAL("object->board_colour2move error", EXIT_FAILURE)

  BSTRING(bfen)

  board2fen(object, bfen, FALSE);

  my_printf(object->board_my_printf, "%s\n", bdata(bfen));

  BDESTROY(bfen)
}

//helper function

local void init_key(hash_key_t *k)
{
  for (int i = 0; i < BOARD_MAX; i++) 
    k[i] = return_my_random(&boards_random);
}

char *board2string(board_t *self, int arg_hub)
{
  board_t *object = self;

  for (int i = 0; i <= 50; ++i) object->board_string[i] = '?';

  if (IS_WHITE(object->board_colour2move))
  {
    if (!arg_hub)
      object->board_string[0] = 'w'; 
    else
      object->board_string[0] = 'W'; 
  }
  else
  {
    if (!arg_hub)
      object->board_string[0] = 'b';
    else
      object->board_string[0] = 'B';
  }

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    if (object->board_white_man_bb & BITULL(iboard))
    {
      if (!arg_hub)
        object->board_string[i] = *wO;
      else
        object->board_string[i] = *wO_hub;
    }
    else if (object->board_white_king_bb & BITULL(iboard))
    {
      if (!arg_hub)
        object->board_string[i] = *wX;
      else
        object->board_string[i] = *wX_hub;
    }
    else if (object->board_black_man_bb & BITULL(iboard))
    {
      if (!arg_hub)
        object->board_string[i] = *bO;
      else
        object->board_string[i] = *bO_hub;
    }
    else if (object->board_black_king_bb & BITULL(iboard))
    {
      if (!arg_hub)
        object->board_string[i] = *bX;
      else
        object->board_string[i] = *bX_hub;
    }
    else
    {
      if (!arg_hub)
        object->board_string[i] = *nn;
      else
        object->board_string[i] = *nn_hub;
    }
  }
  object->board_string[51] = '\0';

  return(object->board_string);
}

local int left_wing[BOARD_MAX] = 
{
  -1, -1, -1, -1, -1, -1,
     1,  0,  0,  0,  0,
   1,  1,  0,  0,  0, -1,
     1,  0,  0,  0,  0,
   1,  1,  0,  0,  0, -1,
     1,  0,  0,  0,  0,
   1,  1,  0,  0,  0, -1,
     1,  0,  0,  0,  0,
   1,  1,  0,  0,  0, -1,
     1,  0,  0,  0,  0,
   1,  1,  0,  0,  0, -1,
    -1, -1, -1, -1, -1
};

local int center[BOARD_MAX] = 
{
  -1, -1, -1, -1, -1, -1,
     0,  1,  1,  0,  0,
   0,  0,  1,  1,  0, -1,
     0,  1,  1,  0,  0,
   0,  0,  1,  1,  0, -1,
     0,  1,  1,  0,  0,
   0,  0,  1,  1,  0, -1,
     0,  1,  1,  0,  0,
   0,  0,  1,  1,  0, -1,
     0,  1,  1,  0,  0,
   0,  0,  1,  1,  0, -1,
    -1, -1, -1, -1, -1
};

local int right_wing[BOARD_MAX] = 
{
  -1, -1, -1, -1, -1, -1,
     0,  0,  0,  1,  1,
   0,  0,  0,  0,  1, -1,
     0,  0,  0,  1,  1,
   0,  0,  0,  0,  1, -1,
     0,  0,  0,  1,  1,
   0,  0,  0,  0,  1, -1,
     0,  0,  0,  1,  1,
   0,  0,  0,  0,  1, -1,
     0,  0,  0,  1,  1,
   0,  0,  0,  0,  1, -1,
    -1, -1, -1, -1, -1
};


local int left_half[BOARD_MAX] = 
{
  -1, -1, -1, -1, -1, -1,
     1,  1,  0,  0,  0,
   1,  1,  1,  0,  0, -1,
     1,  1,  0,  0,  0,
   1,  1,  1,  0,  0, -1,
     1,  1,  0,  0,  0,
   1,  1,  1,  0,  0, -1,
     1,  1,  0,  0,  0,
   1,  1,  1,  0,  0, -1,
     1,  1,  0,  0,  0,
   1,  1,  1,  0,  0, -1,
    -1, -1, -1, -1, -1
};

local int right_half[BOARD_MAX] = 
{
  -1, -1, -1, -1, -1, -1,
     0,  0,  1,  1,  1,
   0,  0,  0,  1,  1, -1,
     0,  0,  1,  1,  1,
   0,  0,  0,  1,  1, -1,
     0,  0,  1,  1,  1,
   0,  0,  0,  1,  1, -1,
     0,  0,  1,  1,  1,
   0,  0,  0,  1,  1, -1,
     0,  0,  1,  1,  1,
   0,  0,  0,  1,  1, -1,
    -1, -1, -1, -1, -1
};

void init_boards(void)
{
  construct_my_random(&boards_random, 0);

  MARK_BLOCK_READ_ONLY(map, sizeof(map))

  MARK_BLOCK_READ_ONLY(inverse_map, sizeof(inverse_map))

  MARK_BLOCK_READ_ONLY(white_row, sizeof(inverse_map))

  MARK_BLOCK_READ_ONLY(black_row, sizeof(inverse_map))

  init_key(white_man_key);
  init_key(white_king_key);

  MARK_BLOCK_READ_ONLY(white_man_key, sizeof(inverse_map))
  MARK_BLOCK_READ_ONLY(white_king_key, sizeof(inverse_map))

  init_key(black_man_key);
  init_key(black_king_key);

  MARK_BLOCK_READ_ONLY(black_man_key, sizeof(inverse_map))
  MARK_BLOCK_READ_ONLY(black_king_key, sizeof(inverse_map))

  left_wing_bb = center_bb = right_wing_bb = 0;

  left_half_bb = right_half_bb = 0;

  white_row_empty_bb = black_row_empty_bb = 0;

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    if (left_wing[iboard]) left_wing_bb |= BITULL(iboard);
    if (center[iboard]) center_bb |= BITULL(iboard);
    if (right_wing[iboard]) right_wing_bb |= BITULL(iboard);

    if (left_half[iboard]) left_half_bb |= BITULL(iboard);
    if (right_half[iboard]) right_half_bb |= BITULL(iboard);

    if (white_row[iboard] >= 8) white_row_empty_bb |= BITULL(iboard);
    if (black_row[iboard] >= 8) black_row_empty_bb |= BITULL(iboard);
  }

  HARDBUG(left_wing_bb & center_bb)
  HARDBUG(left_wing_bb & right_wing_bb)
  HARDBUG(center_bb & right_wing_bb)

  HARDBUG(BIT_COUNT(left_half_bb) != 25)
  HARDBUG(BIT_COUNT(right_half_bb) != 25)
  HARDBUG(left_half_bb & right_half_bb)
} 

void state2board(board_t *self, state_t *arg_game)
{
  board_t *object = self;

  fen2board(object, arg_game->get_starting_position(arg_game), TRUE);

  //read game moves

  moves_list_t moves_list;
  construct_moves_list(&moves_list);

  cJSON *game_move;

  cJSON_ArrayForEach(game_move, arg_game->get_moves(arg_game))
  {
    cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

    HARDBUG(!cJSON_IsString(move_string))

    BSTRING(bmove)
  
    HARDBUG(bassigncstr(bmove, cJSON_GetStringValue(move_string)) == BSTR_ERR)

    my_printf(object->board_my_printf, "bmove=%s\n", bdata(bmove));

    gen_moves(object, &moves_list, FALSE);

    int imove;

    if ((imove = search_move(&moves_list, bmove)) == INVALID)
    {
      print_board(object);

      my_printf(object->board_my_printf, "move=%s\n", bdata(bmove));

      FATAL("bmove not found", EXIT_FAILURE)
    }

    BDESTROY(bmove)

    do_move(object, imove, &moves_list);
  }
}

