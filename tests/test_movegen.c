/* Standalone differential test; no networks, MPI runtime, or EGTB required.
 * Compile with function/data sections and link with --gc-sections, moves.c,
 * Test stubs below are deliberately not part of the engine. */
#include "../globals.h"
#include <assert.h>
#undef clock

int zzzzzz_invocation;
int square2field[51], white_row[BOARD_MAX], black_row[BOARD_MAX];
void print_board(board_t *board)
{
  fprintf(stderr, "side=%d men=%llx/%llx kings=%llx/%llx\n",
          board->B_colour2move, board->B_white_man_bb, board->B_black_man_bb,
          board->B_white_king_bb, board->B_black_king_bb);
}
void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

static ui64_t rng = 0x123456789abcdefULL;
static ui64_t random64(void)
{
  rng ^= rng << 13;
  rng ^= rng >> 7;
  rng ^= rng << 17;
  return rng;
}

static unsigned long long tested, roundtrips;
static int benchmark;
static void play(board_t *, const move_t *);
static void compare(board_t *board, moves_list_t *moves)
{
  if (benchmark)
  {
    if (benchmark == 1)
      gen_moves_legacy(board, moves);
    else
      gen_moves(board, moves);
    ++tested;
    return;
  }
  moves_list_t legacy;
  gen_moves_legacy(board, &legacy);
  int ncapt = board->B_nodes[board->B_inode].node_ncaptx;
  gen_moves(board, moves);
  assert(ncapt == board->B_nodes[board->B_inode].node_ncaptx);
  assert(moves->ML_nmoves == legacy.ML_nmoves);
  assert(moves->ML_ncaptx == legacy.ML_ncaptx);
  for (int i = 0; i < moves->ML_nmoves; ++i)
  {
    const move_t *a = &moves->ML_moves[i], *b = &legacy.ML_moves[i];
    assert(a->M_move_from == b->M_move_from);
    assert(a->M_move_to == b->M_move_to);
    assert(a->M_captures_bb == b->M_captures_bb);
    assert(moves->ML_move_flags[i] == legacy.ML_move_flags[i]);
    roundtrips += a->M_move_from == a->M_move_to;
  }
  ++tested;
}

static void play(board_t *board, const move_t *move)
{
  int side = board->B_colour2move;
  ui64_t from = BITULL(move->M_move_from), to = BITULL(move->M_move_to);
  int king = (board->B_bit_boards[side][KING_ENUM] & from) != 0;
  board->B_bit_boards[side][king ? KING_ENUM : MAN_ENUM] &= ~from;
  if ((side == WHITE_ENUM ? white_row : black_row)[move->M_move_to] == 10)
    king = 1;
  board->B_bit_boards[side][king ? KING_ENUM : MAN_ENUM] |= to;
  board->B_bit_boards[1 - side][MAN_ENUM] &= ~move->M_captures_bb;
  board->B_bit_boards[1 - side][KING_ENUM] &= ~move->M_captures_bb;
  board->B_colour2move = 1 - side;
}

int main(int argc, char **argv)
{
  if (argc > 1)
    benchmark = strcmp(argv[1], "legacy") == 0 ? 1 : 2;
  clock_t start = clock();
  static board_t board;
  moves_list_t moves;
  for (int i = 1; i <= 50; ++i)
  {
    square2field[i] = i + 5 + (i - 1) / 10;
    white_row[square2field[i]] = 10 - (i - 1) / 5;
    black_row[square2field[i]] = 1 + (i - 1) / 5;
    board.B_valid_bb |= BITULL(square2field[i]);
  }
  /* Random legal playouts from the initial position and varied material,
   * plus both sides of each synthetic position (including king loops). */
  for (int game = 0; game < 20000; ++game)
  {
    memset(board.B_bit_boards, 0, sizeof(board.B_bit_boards));
    board.B_colour2move = game & 1;
    if (game < 5000)
    {
      for (int i = 1; i <= 20; ++i)
      {
        board.B_black_man_bb |= BITULL(square2field[i]);
        board.B_white_man_bb |= BITULL(square2field[51 - i]);
      }
    }
    else
    {
      ui64_t occupied = 0;
      for (int side = 0; side < 2; ++side)
        for (int i = 0, n = 1 + random64() % 15; i < n; ++i)
        {
          int square;
          do { square = 1 + random64() % 50; }
          while (occupied & BITULL(square2field[square]));
          ui64_t bit = BITULL(square2field[square]);
          occupied |= bit;
          int king = random64() % 3 == 0 ||
                     (side == WHITE_ENUM ? square <= 5 : square > 45);
          board.B_bit_boards[side][king ? KING_ENUM : MAN_ENUM] |= bit;
        }
    }
    for (int ply = 0; ply < 150; ++ply)
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
    printf("%s: %llu generations, %.3f CPU seconds.\n", argv[1], tested,
           (double)(clock() - start) / CLOCKS_PER_SEC);
  else
  {
    assert(roundtrips > 0);
    printf("Passed %llu ordered move-list comparisons; %llu round-trip moves.\n",
           tested, roundtrips);
  }
  return 0;
}
