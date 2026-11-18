// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "boards.h"

#include "bstrlib.h"
#include "cJSON.h"
#include "common.h"
#include "moves.h"
#include "my_cjson.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "my_random.h"
#include "networks.h"
#include "states.h"

// clang-format off

const int square2field[1 + 50] =
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

const int field2square[BOARD_MAX] =
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

const char *const nota[BOARD_MAX] =
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

const int white_row[BOARD_MAX] =
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

const int black_row[BOARD_MAX] =
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

  arg_with_board->B_nstates = 0;
}

void push_board_state(board_t *arg_with_board)
{
  HARDBUG(arg_with_board->B_nstates >= NODE_MAX)

  board_state_t *state = arg_with_board->B_states + arg_with_board->B_nstates;

  state->BS_key = arg_with_board->B_key;

  memcpy(state->BS_bit_boards,
         arg_with_board->B_bit_boards,
         sizeof(my_uint64_t) * NCOLOUR_ENUM * NPIECE_ENUM);

  state->BS_npieces = BIT_COUNT(arg_with_board->B_white_men_bb) +
                      BIT_COUNT(arg_with_board->B_white_kings_bb) +
                      BIT_COUNT(arg_with_board->B_black_men_bb) +
                      BIT_COUNT(arg_with_board->B_black_kings_bb);

  arg_with_board->B_nstates++;
}

void pop_board_state(board_t *arg_with_board)
{
  arg_with_board->B_nstates--;

  HARDBUG(arg_with_board->B_nstates < 0)

  board_state_t *state = arg_with_board->B_states + arg_with_board->B_nstates;

  arg_with_board->B_key = state->BS_key;

  memcpy(arg_with_board->B_bit_boards,
         state->BS_bit_boards,
         sizeof(my_uint64_t) * NCOLOUR_ENUM * NPIECE_ENUM);

  HARDBUG(state->BS_npieces != (BIT_COUNT(arg_with_board->B_white_men_bb) +
                                BIT_COUNT(arg_with_board->B_white_kings_bb) +
                                BIT_COUNT(arg_with_board->B_black_men_bb) +
                                BIT_COUNT(arg_with_board->B_black_kings_bb)))
}

hash_key_t return_key_from_bb(const board_t *arg_with_board)
{
  hash_key_t result = 0;

  my_uint64_t pieces_bb = arg_with_board->B_white_men_bb;

  while (pieces_bb != 0)
  {
    int ifield = BIT_CTZ(pieces_bb);

    result ^= hash_keys[WHITE_ENUM][MAN_ENUM][ifield];

    pieces_bb &= ~BITULL(ifield);
  }

  pieces_bb = arg_with_board->B_white_kings_bb;

  while (pieces_bb != 0)
  {
    int ifield = BIT_CTZ(pieces_bb);

    result ^= hash_keys[WHITE_ENUM][KING_ENUM][ifield];

    pieces_bb &= ~BITULL(ifield);
  }

  pieces_bb = arg_with_board->B_black_men_bb;

  while (pieces_bb != 0)
  {
    int ifield = BIT_CTZ(pieces_bb);

    result ^= hash_keys[BLACK_ENUM][MAN_ENUM][ifield];

    pieces_bb &= ~BITULL(ifield);
  }

  pieces_bb = arg_with_board->B_black_kings_bb;

  while (pieces_bb != 0)
  {
    int ifield = BIT_CTZ(pieces_bb);

    result ^= hash_keys[BLACK_ENUM][KING_ENUM][ifield];

    pieces_bb &= ~BITULL(ifield);
  }

  return (result);
}

void string2board(board_t *arg_with_board, const char *arg_position)
{
  arg_with_board->B_valid_bb = 0;

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    int ifield = square2field[isquare];

    arg_with_board->B_valid_bb |= BITULL(ifield);
  }

  arg_with_board->B_white_men_bb = 0;
  arg_with_board->B_white_kings_bb = 0;
  arg_with_board->B_black_men_bb = 0;
  arg_with_board->B_black_kings_bb = 0;

  char wO_local = *wO;
  char wX_local = *wX;
  char bO_local = *bO;
  char bX_local = *bX;
  char nn_local = *nn;

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    if ((arg_position[isquare] == *wO_hub) or
        (arg_position[isquare] == *wX_hub))
    {
      wO_local = *wO_hub;
      wX_local = *wX_hub;
      bO_local = *bO_hub;
      bX_local = *bX_hub;
      nn_local = *nn_hub;

      break;
    }
  }

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    int ifield;

    ifield = square2field[isquare];

    if (arg_position[isquare] == wO_local)
    {
      HARDBUG(isquare <= 5)

      arg_with_board->B_white_men_bb |= BITULL(ifield);
    }
    else if (arg_position[isquare] == wX_local)
    {
      arg_with_board->B_white_kings_bb |= BITULL(ifield);
    }
    else if (arg_position[isquare] == bO_local)
    {
      HARDBUG(isquare >= 46)

      arg_with_board->B_black_men_bb |= BITULL(ifield);
    }
    else if (arg_position[isquare] == bX_local)
    {
      arg_with_board->B_black_kings_bb |= BITULL(ifield);
    }
    else if (arg_position[isquare] == nn_local)
    {
      // do nothing
    }
    else
      FATAL("arg_position[isquare] error", EXIT_FAILURE)
  }

  arg_with_board->B_key = return_key_from_bb(arg_with_board);

  arg_with_board->B_nstates = 0;

  arg_with_board->B_inode = 0;

  node_t *node = arg_with_board->B_nodes;

  node->node_ncaptx = INVALID;
  node->node_move_key = 0;

  if ((*arg_position == 'w') or (*arg_position == 'W'))
    arg_with_board->B_colour2move = WHITE_ENUM;
  else if ((*arg_position == 'b') or (*arg_position == 'B'))
    arg_with_board->B_colour2move = BLACK_ENUM;
  else
    FATAL("colour2move error", EXIT_FAILURE)
}

void fen2board(board_t *arg_with_board, const char *arg_fen)
{
  char position[51];

  for (int isquare = 1; isquare <= 50; ++isquare)
    position[isquare] = *nn;

  const char *fen_cursor = arg_fen;

  // W or B

  if (*fen_cursor == 'W')
    position[0] = 'w';
  else if (*fen_cursor == 'B')
    position[0] = 'b';
  else
    FATAL("s[0] error", EXIT_FAILURE)

  fen_cursor++;

  //:

  HARDBUG(*fen_cursor != ':')

  fen_cursor++;

  // W or B

  HARDBUG((*fen_cursor != 'W') and (*fen_cursor != 'B'))

  char colour = *fen_cursor++;

  char piece = 'M';

  while (TRUE)
  {
    if ((*fen_cursor == '.') or (*fen_cursor == '\0'))
      break;

    if (*fen_cursor == '"')
      break;

    if ((*fen_cursor == 'W') or (*fen_cursor == 'B'))
    {
      colour = *fen_cursor++;

      continue;
    }

    if (*fen_cursor == 'K')
    {
      piece = *fen_cursor++;

      continue;
    }

    if (isdigit(*fen_cursor))
    {
      int isquare = 0;

      while (isdigit(*fen_cursor))
      {
        isquare = isquare * 10 + (*fen_cursor - '0');
        fen_cursor++;
      }

      HARDBUG(isquare < 1)
      HARDBUG(isquare > 50)

      int last_square = isquare;

      if (*fen_cursor == '-')
      {
        fen_cursor++;

        HARDBUG(!isdigit(*fen_cursor))

        last_square = 0;

        while (isdigit(*fen_cursor))
        {
          last_square = last_square * 10 + (*fen_cursor - '0');
          fen_cursor++;
        }

        HARDBUG(last_square < 1)
        HARDBUG(last_square > 50)
      }

      while (isquare <= last_square)
      {
        if (piece == 'M')
        {
          if (colour == 'W')
            position[isquare] = *wO;
          else if (colour == 'B')
            position[isquare] = *bO;
          else
            FATAL("c error", EXIT_FAILURE)
        }
        else if (piece == 'K')
        {
          if (colour == 'W')
            position[isquare] = *wX;
          else if (colour == 'B')
            position[isquare] = *bX;
          else
            FATAL("c error", EXIT_FAILURE)
        }
        else
          FATAL("k error", EXIT_FAILURE)

        isquare++;
      }

      piece = 'M';

      continue;
    }

    HARDBUG((*fen_cursor != ',') and (*fen_cursor != ':'))

    fen_cursor++;
  }

  string2board(arg_with_board, position);
}

void board2fen(const board_t *arg_with_board,
               bstring arg_bfen,
               int arg_brackets)
{
  BSTRING(bwfen)

  HARDBUG(bassigncstr(bwfen, ":W") == BSTR_ERR)

  BSTRING(bbfen)

  HARDBUG(bassigncstr(bbfen, ":B") == BSTR_ERR)

  HARDBUG(btrunc(arg_bfen, 0) == BSTR_ERR)

  int wcomma = FALSE;
  int bcomma = FALSE;

  if (arg_brackets)
    HARDBUG(bcatcstr(arg_bfen, "[FEN \"") == BSTR_ERR)

  if (is_white(arg_with_board->B_colour2move))
    HARDBUG(bcatcstr(arg_bfen, "W") == BSTR_ERR)
  else
    HARDBUG(bcatcstr(arg_bfen, "B") == BSTR_ERR)

  my_uint64_t white_bb =
    arg_with_board->B_white_men_bb | arg_with_board->B_white_kings_bb;

  my_uint64_t black_bb =
    arg_with_board->B_black_men_bb | arg_with_board->B_black_kings_bb;

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    int ifield = square2field[isquare];

    if (white_bb & BITULL(ifield))
    {
      if (wcomma)
        HARDBUG(bcatcstr(bwfen, ",") == BSTR_ERR)

      if (arg_with_board->B_white_kings_bb & BITULL(ifield))
        HARDBUG(bcatcstr(bwfen, "K") == BSTR_ERR)

      HARDBUG(bcatcstr(bwfen, nota[ifield]) == BSTR_ERR)

      wcomma = TRUE;
    }
    else if (black_bb & BITULL(ifield))
    {
      if (bcomma)
        HARDBUG(bcatcstr(bbfen, ",") == BSTR_ERR)

      if (arg_with_board->B_black_kings_bb & BITULL(ifield))
        HARDBUG(bcatcstr(bbfen, "K") == BSTR_ERR)

      HARDBUG(bcatcstr(bbfen, nota[ifield]) == BSTR_ERR)

      bcomma = TRUE;
    }
  }

  HARDBUG(bconcat(arg_bfen, bwfen) == BSTR_ERR)
  HARDBUG(bconcat(arg_bfen, bbfen) == BSTR_ERR)

  if (arg_brackets)
    HARDBUG(bcatcstr(arg_bfen, ".\"]") == BSTR_ERR)

  BDESTROY(bbfen)

  BDESTROY(bwfen)
}

void print_board(const board_t *arg_with_board)
{
  int isquare = 0;

  for (int irow = 10; irow >= 1; --irow)
  {
    if ((irow & 1) == 0)
      my_printf(arg_with_board->B_my_printf, "  ");

    for (int icol = 1; icol <= 5; ++icol)
    {
      ++isquare;

      int ifield = square2field[isquare];

      if (arg_with_board->B_white_men_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  wO");
      else if (arg_with_board->B_white_kings_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  wX");
      else if (arg_with_board->B_black_men_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  bO");
      else if (arg_with_board->B_black_kings_bb & BITULL(ifield))
        my_printf(arg_with_board->B_my_printf, "  bX");
      else
        my_printf(arg_with_board->B_my_printf, "  ..");
    }
    my_printf(arg_with_board->B_my_printf, "\n");
  }
  my_printf(arg_with_board->B_my_printf,
            "(%d) (%dx%d) (%dx%d)",
            BIT_COUNT(arg_with_board->B_white_men_bb |
                      arg_with_board->B_white_kings_bb |
                      arg_with_board->B_black_men_bb |
                      arg_with_board->B_black_kings_bb),
            BIT_COUNT(arg_with_board->B_white_men_bb),
            BIT_COUNT(arg_with_board->B_black_men_bb),
            BIT_COUNT(arg_with_board->B_white_kings_bb),
            BIT_COUNT(arg_with_board->B_black_kings_bb));

  if (is_white(arg_with_board->B_colour2move))
    my_printf(arg_with_board->B_my_printf, " WHITE to move\n");
  else if (is_black(arg_with_board->B_colour2move))
    my_printf(arg_with_board->B_my_printf, " BLACK to move\n");
  else
    FATAL("arg_with_board->B_colour2move error", EXIT_FAILURE)

  BSTRING(bfen)

  board2fen(arg_with_board, bfen, FALSE);

  my_printf(arg_with_board->B_my_printf, "%s\n", bdata(bfen));

  BDESTROY(bfen)
}

char *
board2string(const board_t *arg_with_board, int arg_hub, bstring arg_string)
{
  HARDBUG(bassigncstr(arg_string, "?") == BSTR_ERR)
  HARDBUG(bpattern(arg_string, 51) == BSTR_ERR)

  if (is_white(arg_with_board->B_colour2move))
  {
    if (!arg_hub)
      bdata(arg_string)[0] = 'w';
    else
      bdata(arg_string)[0] = 'W';
  }
  else
  {
    if (!arg_hub)
      bdata(arg_string)[0] = 'b';
    else
      bdata(arg_string)[0] = 'B';
  }

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    int ifield = square2field[isquare];

    if (arg_with_board->B_white_men_bb & BITULL(ifield))
    {
      if (!arg_hub)
        bdata(arg_string)[isquare] = *wO;
      else
        bdata(arg_string)[isquare] = *wO_hub;
    }
    else if (arg_with_board->B_white_kings_bb & BITULL(ifield))
    {
      if (!arg_hub)
        bdata(arg_string)[isquare] = *wX;
      else
        bdata(arg_string)[isquare] = *wX_hub;
    }
    else if (arg_with_board->B_black_men_bb & BITULL(ifield))
    {
      if (!arg_hub)
        bdata(arg_string)[isquare] = *bO;
      else
        bdata(arg_string)[isquare] = *bO_hub;
    }
    else if (arg_with_board->B_black_kings_bb & BITULL(ifield))
    {
      if (!arg_hub)
        bdata(arg_string)[isquare] = *bX;
      else
        bdata(arg_string)[isquare] = *bX_hub;
    }
    else
    {
      if (!arg_hub)
        bdata(arg_string)[isquare] = *nn;
      else
        bdata(arg_string)[isquare] = *nn_hub;
    }
  }
  bdata(arg_string)[51] = '\0';

  return (bdata(arg_string));
}

void init_boards(void)
{
  construct_my_random(&boards_random, NULL);

  for (int ifield = 0; ifield < BOARD_MAX; ifield++)
    hash_keys[WHITE_ENUM][MAN_ENUM][ifield] = return_my_random(&boards_random);

  for (int ifield = 0; ifield < BOARD_MAX; ifield++)
    hash_keys[WHITE_ENUM][KING_ENUM][ifield] = return_my_random(&boards_random);

  for (int ifield = 0; ifield < BOARD_MAX; ifield++)
    hash_keys[BLACK_ENUM][MAN_ENUM][ifield] = return_my_random(&boards_random);

  for (int ifield = 0; ifield < BOARD_MAX; ifield++)
    hash_keys[BLACK_ENUM][KING_ENUM][ifield] = return_my_random(&boards_random);

  MARK_ARRAY_READ_ONLY(hash_keys, sizeof(hash_keys))
}

void state2board(board_t *arg_with_board, const game_state_t *arg_game)
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
