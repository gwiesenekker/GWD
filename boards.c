//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

#define NBOARDS_MAX     128
#define BOARD_ID_OFFSET 2048

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

local my_mutex_t boards_mutex;

hash_key_t white_man_key[BOARD_MAX];
hash_key_t white_crown_key[BOARD_MAX];

hash_key_t black_man_key[BOARD_MAX];
hash_key_t black_crown_key[BOARD_MAX];

local hash_key_t white2move_key;
local hash_key_t black2move_key;

hash_key_t pv_key;

local board_t board_objs[NBOARDS_MAX];

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

board_t *return_with_board(int arg_board_id)
{
  int iboard = arg_board_id - BOARD_ID_OFFSET;
  HARDBUG(iboard < 0)
  HARDBUG(iboard >= NBOARDS_MAX)

  board_t *with = board_objs + iboard;
  HARDBUG(with->board_id != arg_board_id)

  return(with);
}

int create_board(my_printf_t *arg_my_printf, int arg_my_thread_id)
{
  int iboard;
  board_t *with;

  HARDBUG(my_mutex_lock(&boards_mutex) != 0)

  //search empty slot

  for (iboard = 0; iboard < NBOARDS_MAX; iboard++)
  {
    with = board_objs + iboard;
    if (with->board_id == INVALID) break;
  }
  HARDBUG(iboard >= NBOARDS_MAX)
  with->board_id = BOARD_ID_OFFSET + iboard;

  with->board_my_printf = arg_my_printf;

  with->board_thread_id = arg_my_thread_id;
  
  construct_my_timer(&(with->board_timer), "board",
    with->board_my_printf, FALSE);

  load_neural(options.neural_name0, &(with->board_neural0), TRUE);
  load_neural(options.neural_name1, &(with->board_neural1), TRUE);

  entry_i16_t entry_i16_default = {MATE_DRAW, MATE_DRAW};
            
  i64_t key_default = -1;
            
  i64_t endgame_entry_cache_size =
    options.egtb_entry_cache_size * MBYTE;
              
  char name[MY_LINE_MAX];

  snprintf(name, MY_LINE_MAX, "board_%d_entry_cache\n", with->board_id);

  construct_cache(&(with->board_endgame_entry_cache),
                  name, endgame_entry_cache_size,
                  CACHE_ENTRY_KEY_TYPE_I64_T, &key_default,
                  sizeof(entry_i16_t), &entry_i16_default);

  CLR_HASH_KEY(with->board_board2string_key)

  HARDBUG(my_mutex_unlock(&boards_mutex) != 0)

  return(with->board_id);
}

void destroy_board(int arg_board_id)
{
  HARDBUG(my_mutex_lock(&boards_mutex) != 0)

  board_t *with = return_with_board(arg_board_id);

  //TODO: add init_board

  with->board_my_printf = NULL;

  with->board_thread_id = INVALID;

  with->board_id = INVALID;

  HARDBUG(my_mutex_unlock(&boards_mutex) != 0)
}

hash_key_t return_key_from_bb(board_t *with)
{
  hash_key_t result;

  CLR_HASH_KEY(result)

  ui64_t bb = with->board_white_man_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, white_man_key[iboard])

    bb &= ~BITULL(iboard);
  }

  bb = with->board_white_crown_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, white_crown_key[iboard])

    bb &= ~BITULL(iboard);
  }

  bb = with->board_black_man_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, black_man_key[iboard])

    bb &= ~BITULL(iboard);
  }

  bb = with->board_black_crown_bb;

  while(bb != 0)
  {
    int iboard = BIT_CTZ(bb);

    XOR_HASH_KEY(result, black_crown_key[iboard])

    bb &= ~BITULL(iboard);
  }

  return(result);
}

hash_key_t return_key_from_inputs(neural_t *with)
{
  hash_key_t result;

  CLR_HASH_KEY(result)

  for (int i = 6; i <= 50; ++i)
  {
    int iboard = map[i];

    if (with->neural_inputs[with->white_man_input_map[iboard]])
      XOR_HASH_KEY(result, white_man_key[iboard])
  }

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    if (with->neural_inputs[with->white_crown_input_map[iboard]])
      XOR_HASH_KEY(result, white_crown_key[iboard])
  }

  for (int i = 1; i <= 45; ++i)
  {
    int iboard = map[i];

    if (with->neural_inputs[with->black_man_input_map[iboard]])
      XOR_HASH_KEY(result, black_man_key[iboard])
  }

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    if (with->neural_inputs[with->black_crown_input_map[iboard]])
      XOR_HASH_KEY(result, black_crown_key[iboard])
  }

  return(result);
}

hash_key_t return_book_key(board_t *with)
{
  hash_key_t result;

  result = with->board_key;

  if (IS_WHITE(with->board_colour2move))
    XOR_HASH_KEY(result, white2move_key)
  else
    XOR_HASH_KEY(result, black2move_key)

  return(result);
}

void string2board(char *arg_s, int arg_board_id)
{
  board_t *with = return_with_board(arg_board_id);

  with->board_empty_bb = 0;

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    with->board_empty_bb |= BITULL(iboard);
  }

  with->board_white_man_bb = 0;
  with->board_white_crown_bb = 0;
  with->board_black_man_bb = 0;
  with->board_black_crown_bb = 0;

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

      with->board_white_man_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == wX_local)
    {
      with->board_white_crown_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == bO_local)
    {
      HARDBUG(i >= 46)

      with->board_black_man_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == bX_local)
    {
      with->board_black_crown_bb |= BITULL(iboard);
    }
    else if (arg_s[i] == nn_local)
    {
      //do nothing
    }
    else
      FATAL("arg_s[i] error", EXIT_FAILURE)
  }

  with->board_key = return_key_from_bb(with);

  with->board_inode = 0;

  node_t *node = with->board_nodes;

  node->node_ncaptx = INVALID;
  node->node_key = 0;
  node->node_move_key = 0;
  node->node_move_tactical = INVALID;

  if ((*arg_s == 'w') or (*arg_s == 'W'))
    with->board_colour2move = WHITE_BIT;
  else if ((*arg_s == 'b') or (*arg_s == 'B'))
    with->board_colour2move = BLACK_BIT;
  else
    FATAL("colour2move error", EXIT_FAILURE)

  board2neural(with, &(with->board_neural0), FALSE);
  board2neural(with, &(with->board_neural1), FALSE);
  
  return_neural_score_scaled(&(with->board_neural0), FALSE, FALSE);
  return_neural_score_scaled(&(with->board_neural1), FALSE, FALSE);
}

void fen2board(char *arg_fen, int arg_board_id)
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
  string2board(s, arg_board_id);
}

void board2fen(int arg_board_id, char arg_fen[MY_LINE_MAX], int brackets)
{
  board_t *with = return_with_board(arg_board_id);

  char wfen[MY_LINE_MAX];
  char bfen[MY_LINE_MAX];

  strcpy(arg_fen, "");
  strcpy(wfen, ":W");
  strcpy(bfen, ":B");

  int wcomma = FALSE;
  int bcomma = FALSE;

  if (brackets) STRNCAT(arg_fen, "[FEN \"");

  if (IS_WHITE(with->board_colour2move))
    STRNCAT(arg_fen, "W");
  else
    STRNCAT(arg_fen, "B");

  ui64_t white_bb = with->board_white_man_bb | with->board_white_crown_bb;
  ui64_t black_bb = with->board_black_man_bb | with->board_black_crown_bb;

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    if (white_bb & BITULL(iboard))
    {
      if (wcomma) STRNCAT(wfen, ",");

      if (with->board_white_crown_bb & BITULL(iboard))
        STRNCAT(wfen, "K");

      STRNCAT(wfen, nota[iboard]);

      wcomma = TRUE;
    }
    else if (black_bb & BITULL(iboard))
    {
      if (bcomma) STRNCAT(bfen, ",");

      if (with->board_black_crown_bb & BITULL(iboard))
        STRNCAT(bfen, "K");

      STRNCAT(bfen, nota[iboard]);

      bcomma = TRUE;
    }
  }
  STRNCAT(arg_fen, wfen);
  STRNCAT(arg_fen, bfen);
  if (brackets) STRNCAT(arg_fen, ".\"]");
}

void print_board(int arg_board_id)
{
  board_t *with = return_with_board(arg_board_id);

  int i = 0;
  for (int irow = 10; irow >= 1; --irow)
  {
    if ((irow & 1) == 0) my_printf(with->board_my_printf, "  ");
    for (int icol = 1; icol <= 5; ++icol)
    {
      ++i;

      int iboard = map[i];

      if (with->board_white_man_bb & BITULL(iboard))
        my_printf(with->board_my_printf, "  wO");
      else if (with->board_white_crown_bb & BITULL(iboard))
        my_printf(with->board_my_printf, "  wX");
      else if (with->board_black_man_bb & BITULL(iboard))
        my_printf(with->board_my_printf, "  bO");
      else if (with->board_black_crown_bb & BITULL(iboard))
        my_printf(with->board_my_printf, "  bX");
      else
        my_printf(with->board_my_printf, "  ..");
    }
    my_printf(with->board_my_printf, "\n");
  }
  my_printf(with->board_my_printf, 
    "(%d) (%dx%d) (%dx%d)",
    BIT_COUNT(with->board_white_man_bb | with->board_white_crown_bb | 
              with->board_black_man_bb | with->board_black_crown_bb),
    BIT_COUNT(with->board_white_man_bb),
    BIT_COUNT(with->board_black_man_bb),
    BIT_COUNT(with->board_white_crown_bb),
    BIT_COUNT(with->board_black_crown_bb));

  if (IS_WHITE(with->board_colour2move))
    my_printf(with->board_my_printf, " WHITE to move\n");
  else if (IS_BLACK(with->board_colour2move))
    my_printf(with->board_my_printf, " BLACK to move\n");
  else
    FATAL("with->board_colour2move error", EXIT_FAILURE)

  char fen[MY_LINE_MAX];

  board2fen(arg_board_id, fen, FALSE);

  my_printf(with->board_my_printf, "%s\n", fen);
}

//helper function

local void init_key(hash_key_t *k)
{
  for (int i = 0; i < BOARD_MAX; i++) RANDULL_HASH_KEY(k[i])
}

local char *self_board2string(board_t *self, int hub)
{
  if HASH_KEY_EQ(self->board_key, self->board_board2string_key)
  {
    //PRINTF("return cached board_string!\n");
    return(self->board_string);
  }

  for (int i = 0; i <= 50; ++i) self->board_string[i] = '?';

  if (IS_WHITE(self->board_colour2move))
  {
    if (!hub)
      self->board_string[0] = 'w'; 
    else
      self->board_string[0] = 'W'; 
  }
  else
  {
    if (!hub)
      self->board_string[0] = 'b';
    else
      self->board_string[0] = 'B';
  }

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    if (self->board_white_man_bb & BITULL(iboard))
    {
      if (!hub)
        self->board_string[i] = *wO;
      else
        self->board_string[i] = *wO_hub;
    }
    else if (self->board_white_crown_bb & BITULL(iboard))
    {
      if (!hub)
        self->board_string[i] = *wX;
      else
        self->board_string[i] = *wX_hub;
    }
    else if (self->board_black_man_bb & BITULL(iboard))
    {
      if (!hub)
        self->board_string[i] = *bO;
      else
        self->board_string[i] = *bO_hub;
    }
    else if (self->board_black_crown_bb & BITULL(iboard))
    {
      if (!hub)
        self->board_string[i] = *bX;
      else
        self->board_string[i] = *bX_hub;
    }
    else
    {
      if (!hub)
        self->board_string[i] = *nn;
      else
        self->board_string[i] = *nn_hub;
    }
  }
  self->board_string[51] = '\0';

  self->board_board2string_key = self->board_key;

  return(self->board_string);
}

void init_boards(void)
{
  MARK_ARRAY_READONLY(map)

  MARK_ARRAY_READONLY(inverse_map)

  MARK_ARRAY_READONLY(white_row)

  MARK_ARRAY_READONLY(black_row)

  for (int iboard = 0; iboard < NBOARDS_MAX; iboard++)
  {
    board_t *with = board_objs + iboard;

    with->board_id = INVALID;
    with->board2string = self_board2string;
  }

  init_key(white_man_key);
  init_key(white_crown_key);

  MARK_ARRAY_READONLY(white_man_key)
  MARK_ARRAY_READONLY(white_crown_key)

  init_key(black_man_key);
  init_key(black_crown_key);

  MARK_ARRAY_READONLY(black_man_key)
  MARK_ARRAY_READONLY(black_crown_key)

  //book backward compatability

  RANDULL_HASH_KEY(white2move_key)
  RANDULL_HASH_KEY(black2move_key)

  pv_key = randull(0);

  HARDBUG(my_mutex_init(&boards_mutex) != 0)
} 

void state2board(board_t *with, state_t *game)
{
  fen2board(game->get_starting_position(game), with->board_id);

  //read game moves

  moves_list_t moves_list;
  create_moves_list(&moves_list);

  cJSON *game_move;

  int ndone = 0;

  cJSON_ArrayForEach(game_move, game->get_moves(game))
  {
    cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

    HARDBUG(!cJSON_IsString(move_string))

    my_printf(with->board_my_printf, "move_string=%s\n",
      cJSON_GetStringValue(move_string));

    ++ndone;

    gen_moves(with, &moves_list, FALSE);

    int imove;

    if ((imove = search_move(&moves_list,
                             cJSON_GetStringValue(move_string))) ==
        INVALID)
    {
      print_board(with->board_id);

      my_printf(with->board_my_printf, "move_string=%s\n",
        cJSON_GetStringValue(move_string));

      FATAL("move_string not found", EXIT_FAILURE)
    }
    do_move(with, imove, &moves_list);
  }
}

