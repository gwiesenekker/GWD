// SCU REVISION 8.1265 wo 11 nov 2026  2:19:30 CET
#include "globals.h"

// clang-format off

int square2field[1 + 50] =
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

int field2square[BOARD_MAX] =
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

// clang-format on

my_random_t boards_random;

hash_key_t hash_keys[NCOLOUR_ENUM][NPIECE_ENUM][BOARD_MAX];

void construct_board(board_t *arg_with_board,
                     my_printf_t *arg_my_printf,
                     int arg_network_id)
{
  HARDBUG((arg_network_id < 0) or (arg_network_id >= NETWORKS_MAX))

  arg_with_board->B_my_printf = arg_my_printf;

  arg_with_board->B_network_id = arg_network_id;

  construct_network_state(&(arg_with_board->B_network_state),
                          arg_with_board->B_network_id,
                          TRUE);

  arg_with_board->B_nstate = 0;
}

void push_board_state(board_t *arg_with_board)
{
  HARDBUG(arg_with_board->B_nstate >= NODE_MAX)

  board_state_t *state = arg_with_board->B_states + arg_with_board->B_nstate;

  state->BS_key = arg_with_board->B_key;

  memcpy(state->BS_bit_boards,
         arg_with_board->B_bit_boards,
         sizeof(ui64_t) * NCOLOUR_ENUM * NPIECE_ENUM);

  state->BS_npieces = BIT_COUNT(arg_with_board->B_white_man_bb) +
                      BIT_COUNT(arg_with_board->B_white_king_bb) +
                      BIT_COUNT(arg_with_board->B_black_man_bb) +
                      BIT_COUNT(arg_with_board->B_black_king_bb);

  arg_with_board->B_nstate++;
}

void pop_board_state(board_t *arg_with_board)
{
  arg_with_board->B_nstate--;

  HARDBUG(arg_with_board->B_nstate < 0)

  board_state_t *state = arg_with_board->B_states + arg_with_board->B_nstate;

  arg_with_board->B_key = state->BS_key;

  memcpy(arg_with_board->B_bit_boards,
         state->BS_bit_boards,
         sizeof(ui64_t) * NCOLOUR_ENUM * NPIECE_ENUM);

  HARDBUG(state->BS_npieces != (BIT_COUNT(arg_with_board->B_white_man_bb) +
                                BIT_COUNT(arg_with_board->B_white_king_bb) +
                                BIT_COUNT(arg_with_board->B_black_man_bb) +
                                BIT_COUNT(arg_with_board->B_black_king_bb)))
}

hash_key_t return_key_from_bb(board_t *arg_with_board)
{
  hash_key_t result = 0;

  ui64_t bb = arg_with_board->B_white_man_bb;

  while (bb != 0)
  {
    int ifield = BIT_CTZ(bb);

    result ^= hash_keys[WHITE_ENUM][MAN_ENUM][ifield];

    bb &= ~BITULL(ifield);
  }

  bb = arg_with_board->B_white_king_bb;

  while (bb != 0)
  {
    int ifield = BIT_CTZ(bb);

    result ^= hash_keys[WHITE_ENUM][KING_ENUM][ifield];

    bb &= ~BITULL(ifield);
  }

  bb = arg_with_board->B_black_man_bb;

  while (bb != 0)
  {
    int ifield = BIT_CTZ(bb);

    result ^= hash_keys[BLACK_ENUM][MAN_ENUM][ifield];

    bb &= ~BITULL(ifield);
  }

  bb = arg_with_board->B_black_king_bb;

  while (bb != 0)
  {
    int ifield = BIT_CTZ(bb);

    result ^= hash_keys[BLACK_ENUM][KING_ENUM][ifield];

    bb &= ~BITULL(ifield);
  }

  return (result);
}

void string2board(board_t *arg_with_board, char *arg_s)
{
  arg_with_board->B_valid_bb = 0;

  for (int i = 1; i <= 50; ++i)
  {
    int ifield = square2field[i];

    arg_with_board->B_valid_bb |= BITULL(ifield);
  }

  arg_with_board->B_white_man_bb = 0;
  arg_with_board->B_white_king_bb = 0;
  arg_with_board->B_black_man_bb = 0;
  arg_with_board->B_black_king_bb = 0;

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
    int ifield;

    ifield = square2field[i];

    if (arg_s[i] == wO_local)
    {
      HARDBUG(i <= 5)

      arg_with_board->B_white_man_bb |= BITULL(ifield);
    }
    else if (arg_s[i] == wX_local)
    {
      arg_with_board->B_white_king_bb |= BITULL(ifield);
    }
    else if (arg_s[i] == bO_local)
    {
      HARDBUG(i >= 46)

      arg_with_board->B_black_man_bb |= BITULL(ifield);
    }
    else if (arg_s[i] == bX_local)
    {
      arg_with_board->B_black_king_bb |= BITULL(ifield);
    }
    else if (arg_s[i] == nn_local)
    {
      // do nothing
    }
    else
      FATAL("arg_s[i] error", EXIT_FAILURE)
  }

  arg_with_board->B_key = return_key_from_bb(arg_with_board);

  arg_with_board->B_nstate = 0;

  arg_with_board->B_inode = 0;

  node_t *node = arg_with_board->B_nodes;

  node->node_ncaptx = INVALID;
  node->node_move_key = 0;

  if ((*arg_s == 'w') or (*arg_s == 'W'))
    arg_with_board->B_colour2move = WHITE_ENUM;
  else if ((*arg_s == 'b') or (*arg_s == 'B'))
    arg_with_board->B_colour2move = BLACK_ENUM;
  else
    FATAL("colour2move error", EXIT_FAILURE)
}

void fen2board(board_t *arg_with_board, char *arg_fen)
{
  char s[51];
  for (int i = 1; i <= 50; ++i)
    s[i] = *nn;

  char *f = arg_fen;

  // W or B

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

  // W or B

  HARDBUG((*f != 'W') and (*f != 'B'))

  char c = *f++;

  char k = 'M';

  while (TRUE)
  {
    if ((*f == '.') or (*f == '\0'))
      break;

    if (*f == '"')
      break;

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
      while (isdigit(*f))
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

        while (isdigit(*f))
        {
          j = j * 10 + (*f - '0');
          f++;
        }

        HARDBUG(j < 1)
        HARDBUG(j > 50)
      }

      while (i <= j)
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

  string2board(arg_with_board, s);
}

void board2fen(board_t *arg_with_board, bstring arg_bfen, int arg_brackets)
{
  BSTRING(bwfen)

  HARDBUG(bassigncstr(bwfen, ":W") == BSTR_ERR)

  BSTRING(bbfen)

  HARDBUG(bassigncstr(bbfen, ":B") == BSTR_ERR)

  btrunc(arg_bfen, 0);

  int wcomma = FALSE;
  int bcomma = FALSE;

  if (arg_brackets)
    bcatcstr(arg_bfen, "[FEN \"");

  if (IS_WHITE(arg_with_board->B_colour2move))
    bcatcstr(arg_bfen, "W");
  else
    bcatcstr(arg_bfen, "B");

  ui64_t white_bb =
    arg_with_board->B_white_man_bb | arg_with_board->B_white_king_bb;
  ui64_t black_bb =
    arg_with_board->B_black_man_bb | arg_with_board->B_black_king_bb;

  for (int i = 1; i <= 50; ++i)
  {
    int ifield = square2field[i];

    if (white_bb & BITULL(ifield))
    {
      if (wcomma)
        bcatcstr(bwfen, ",");

      if (arg_with_board->B_white_king_bb & BITULL(ifield))
        bcatcstr(bwfen, "K");

      bcatcstr(bwfen, nota[ifield]);

      wcomma = TRUE;
    }
    else if (black_bb & BITULL(ifield))
    {
      if (bcomma)
        bcatcstr(bbfen, ",");

      if (arg_with_board->B_black_king_bb & BITULL(ifield))
        bcatcstr(bbfen, "K");

      bcatcstr(bbfen, nota[ifield]);

      bcomma = TRUE;
    }
  }

  bconcat(arg_bfen, bwfen);
  bconcat(arg_bfen, bbfen);

  if (arg_brackets)
    bcatcstr(arg_bfen, ".\"]");

  BDESTROY(bbfen)
  BDESTROY(bwfen)
}

void print_board(board_t *arg_with_board)
{
  int i = 0;
  for (int irow = 10; irow >= 1; --irow)
  {
    if ((irow & 1) == 0)
      my_printf(arg_with_board->B_my_printf, "  ");
    for (int icol = 1; icol <= 5; ++icol)
    {
      ++i;

      int ifield = square2field[i];

      if (arg_with_board->B_white_man_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  wO");
      else if (arg_with_board->B_white_king_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  wX");
      else if (arg_with_board->B_black_man_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  bO");
      else if (arg_with_board->B_black_king_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  bX");
      else
        my_printf(arg_with_board->B_my_printf, "  ..");
    }
    my_printf(arg_with_board->B_my_printf, "\n");
  }
  my_printf(
    arg_with_board->B_my_printf,
    "(%d) (%dx%d) (%dx%d)",
    BIT_COUNT(arg_with_board->B_white_man_bb | arg_with_board->B_white_king_bb |
              arg_with_board->B_black_man_bb | arg_with_board->B_black_king_bb),
    BIT_COUNT(arg_with_board->B_white_man_bb),
    BIT_COUNT(arg_with_board->B_black_man_bb),
    BIT_COUNT(arg_with_board->B_white_king_bb),
    BIT_COUNT(arg_with_board->B_black_king_bb));

  if (IS_WHITE(arg_with_board->B_colour2move))
    my_printf(arg_with_board->B_my_printf, " WHITE to move\n");
  else if (IS_BLACK(arg_with_board->B_colour2move))
    my_printf(arg_with_board->B_my_printf, " BLACK to move\n");
  else
    FATAL("arg_with_board->B_colour2move error", EXIT_FAILURE)

  BSTRING(bfen)

  board2fen(arg_with_board, bfen, FALSE);

  my_printf(arg_with_board->B_my_printf, "%s\n", bdata(bfen));

  BDESTROY(bfen)
}

char *board2string(board_t *arg_with_board, int arg_hub)
{
  for (int i = 0; i <= 50; ++i)
    arg_with_board->B_string[i] = '?';

  if (IS_WHITE(arg_with_board->B_colour2move))
  {
    if (!arg_hub)
      arg_with_board->B_string[0] = 'w';
    else
      arg_with_board->B_string[0] = 'W';
  }
  else
  {
    if (!arg_hub)
      arg_with_board->B_string[0] = 'b';
    else
      arg_with_board->B_string[0] = 'B';
  }

  for (int i = 1; i <= 50; ++i)
  {
    int ifield = square2field[i];

    if (arg_with_board->B_white_man_bb & BITULL(ifield))
    {
      if (!arg_hub)
        arg_with_board->B_string[i] = *wO;
      else
        arg_with_board->B_string[i] = *wO_hub;
    }
    else if (arg_with_board->B_white_king_bb & BITULL(ifield))
    {
      if (!arg_hub)
        arg_with_board->B_string[i] = *wX;
      else
        arg_with_board->B_string[i] = *wX_hub;
    }
    else if (arg_with_board->B_black_man_bb & BITULL(ifield))
    {
      if (!arg_hub)
        arg_with_board->B_string[i] = *bO;
      else
        arg_with_board->B_string[i] = *bO_hub;
    }
    else if (arg_with_board->B_black_king_bb & BITULL(ifield))
    {
      if (!arg_hub)
        arg_with_board->B_string[i] = *bX;
      else
        arg_with_board->B_string[i] = *bX_hub;
    }
    else
    {
      if (!arg_hub)
        arg_with_board->B_string[i] = *nn;
      else
        arg_with_board->B_string[i] = *nn_hub;
    }
  }
  arg_with_board->B_string[51] = '\0';

  return (arg_with_board->B_string);
}

void init_boards(void)
{
  construct_my_random(&boards_random, NULL);

  MARK_ARRAY_READ_ONLY(square2field, sizeof(square2field))

  MARK_ARRAY_READ_ONLY(field2square, sizeof(field2square))

  MARK_ARRAY_READ_ONLY(white_row, sizeof(field2square))

  MARK_ARRAY_READ_ONLY(black_row, sizeof(field2square))

  for (int i = 0; i < BOARD_MAX; i++)
    hash_keys[WHITE_ENUM][MAN_ENUM][i] = return_my_random(&boards_random);

  for (int i = 0; i < BOARD_MAX; i++)
    hash_keys[WHITE_ENUM][KING_ENUM][i] = return_my_random(&boards_random);

  for (int i = 0; i < BOARD_MAX; i++)
    hash_keys[BLACK_ENUM][MAN_ENUM][i] = return_my_random(&boards_random);

  for (int i = 0; i < BOARD_MAX; i++)
    hash_keys[BLACK_ENUM][KING_ENUM][i] = return_my_random(&boards_random);

  MARK_ARRAY_READ_ONLY(hash_keys, sizeof(hash_keys))
}

void state2board(board_t *arg_with_board, game_state_t *arg_game)
{
  fen2board(arg_with_board, arg_game->get_starting_position(arg_game));

  // read game moves

  moves_list_t moves_list;
  construct_moves_list(&moves_list);

  cJSON *game_move;

  BSTRING(bmove)

  cJSON_ArrayForEach(game_move, arg_game->get_moves(arg_game))
  {
    cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

    HARDBUG(!cJSON_IsString(move_string))

    HARDBUG(bassigncstr(bmove, cJSON_GetStringValue(move_string)) == BSTR_ERR)

    gen_moves(arg_with_board, &moves_list);

    int imove;

    if ((imove = search_move(&moves_list, bmove)) == INVALID)
    {
      print_board(arg_with_board);

      my_printf(arg_with_board->B_my_printf, "bdata(bmove)=%s\n", bdata(bmove));

      FATAL("bmove not found", EXIT_FAILURE)
    }

    do_move(arg_with_board, imove, &moves_list);
  }
  BDESTROY(bmove)
}
