/* Standalone differential test; no networks, MPI runtime, or EGTB required.
 * Compile with function/data sections and link with --gc-sections, moves.c,
 * Test stubs below are deliberately not part of the engine. */
#include "../moves.h"
#include "../boards.h"
#include "../common.h"
#include <assert.h>
#undef clock

int zzzzzz_invocation;
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

void print_board(const board_t *arg_board)
{
  fprintf(stderr,
          "side=%d men=%llx/%llx kings=%llx/%llx\n",
          arg_board->B_colour2move,
          arg_board->B_white_men_bb,
          arg_board->B_black_men_bb,
          arg_board->B_white_kings_bb,
          arg_board->B_black_kings_bb);
}
void zzzzzz(const char *arg_file,
            const char *arg_func,
            long arg_line,
            const char *arg_message,
            int arg_code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", arg_file, arg_line, arg_func, arg_message);
  exit(arg_code);
}

static my_uint64_t rng = 0x123456789abcdefULL;
static my_uint64_t random64(void)
{
  rng ^= rng << 13;
  rng ^= rng >> 7;
  rng ^= rng << 17;

  return (rng);
}

static unsigned long long tested, roundtrips;
static int benchmark;
static void play(board_t *, const move_t *);
static void compare(board_t *arg_board, moves_list_t *arg_moves)
{
  if (benchmark)
  {
    if (benchmark == 1)
      gen_moves_legacy(arg_board, arg_moves);
    else
      gen_moves(arg_board, arg_moves);
    ++tested;

    return;
  }

  moves_list_t legacy;

  gen_moves_legacy(arg_board, &legacy);

  int ncapt = arg_board->B_nodes[arg_board->B_inode].node_ncaptx;

  gen_moves(arg_board, arg_moves);

  assert(ncapt == arg_board->B_nodes[arg_board->B_inode].node_ncaptx);
  assert(arg_moves->ML_nmoves == legacy.ML_nmoves);
  assert(arg_moves->ML_ncaptx == legacy.ML_ncaptx);

  for (int imove = 0; imove < arg_moves->ML_nmoves; ++imove)
  {
    const move_t *generated_move = &arg_moves->ML_moves[imove],
                 *legacy_move = &legacy.ML_moves[imove];

    assert(generated_move->M_move_from == legacy_move->M_move_from);
    assert(generated_move->M_move_to == legacy_move->M_move_to);
    assert(generated_move->M_captures_bb == legacy_move->M_captures_bb);
    assert(arg_moves->ML_move_flags[imove] == legacy.ML_move_flags[imove]);

    roundtrips += generated_move->M_move_from == generated_move->M_move_to;
  }
  ++tested;
}

static void play(board_t *arg_board, const move_t *arg_move)
{
  int side = arg_board->B_colour2move;
  my_uint64_t from = BITULL(arg_move->M_move_from),
              to = BITULL(arg_move->M_move_to);
  int king = (arg_board->B_bit_boards[side][KING_ENUM] & from) != 0;

  arg_board->B_bit_boards[side][king ? KING_ENUM : MAN_ENUM] &= ~from;

  if ((side == WHITE_ENUM ? white_row : black_row)[arg_move->M_move_to] == 10)
    king = 1;
  arg_board->B_bit_boards[side][king ? KING_ENUM : MAN_ENUM] |= to;
  arg_board->B_bit_boards[1 - side][MAN_ENUM] &= ~arg_move->M_captures_bb;
  arg_board->B_bit_boards[1 - side][KING_ENUM] &= ~arg_move->M_captures_bb;
  arg_board->B_colour2move = 1 - side;
}

int main(int argc, char **argv)
{
  if (argc > 1)
    benchmark = strcmp(argv[1], "legacy") == 0 ? 1 : 2;

  clock_t start = clock();
  static board_t board;
  moves_list_t moves;

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    assert(square2field[isquare] == isquare + 5 + (isquare - 1) / 10);
    assert(white_row[square2field[isquare]] == 10 - (isquare - 1) / 5);
    assert(black_row[square2field[isquare]] == 1 + (isquare - 1) / 5);
    board.B_valid_bb |= BITULL(square2field[isquare]);
  }
  /* Random legal playouts from the initial position and varied material,
   * plus both sides of each synthetic position (including king loops). */
  for (int igame = 0; igame < 20000; ++igame)
  {
    memset(board.B_bit_boards, 0, sizeof(board.B_bit_boards));
    board.B_colour2move = igame & 1;

    if (igame < 5000)
    {
      for (int isquare = 1; isquare <= 20; ++isquare)
      {
        board.B_black_men_bb |= BITULL(square2field[isquare]);
        board.B_white_men_bb |= BITULL(square2field[51 - isquare]);
      }
    }
    else
    {
      my_uint64_t occupied = 0;

      for (int side = 0; side < 2; ++side)
        for (int ipiece = 0, npieces = 1 + random64() % 15; ipiece < npieces;
             ++ipiece)
        {
          int square;

          do
          {
            square = 1 + random64() % 50;
          } while (occupied & BITULL(square2field[square]));

          my_uint64_t bit = BITULL(square2field[square]);

          occupied |= bit;

          int king = (random64() % 3 == 0) or
                     (side == WHITE_ENUM ? square <= 5 : square > 45);
          board.B_bit_boards[side][king ? KING_ENUM : MAN_ENUM] |= bit;
        }
    }

    for (int iply = 0; iply < 150; ++iply)
    {
      compare(&board, &moves);
      board.B_colour2move = 1 - board.B_colour2move;
      compare(&board, &moves);
      board.B_colour2move = 1 - board.B_colour2move;
      compare(&board, &moves);

      if (!moves.ML_nmoves)
        break;
      play(&board, &moves.ML_moves[random64() % moves.ML_nmoves]);
    }
  }

  if (benchmark)
    printf("%s: %llu generations, %.3f CPU seconds.\n",
           argv[1],
           tested,
           (double)(clock() - start) / CLOCKS_PER_SEC);
  else
  {
    assert(roundtrips > 0);

    printf(
      "Passed %llu ordered move-list comparisons; %llu round-trip moves.\n",
      tested,
      roundtrips);
  }

  return (0);
}
