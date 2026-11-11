#include "../search.c"
#include <assert.h>

int zzzzzz_invocation;
void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  static search_t search;
  moves_list_t moves = {0};
  ui8_t found[NMOVES_MAX] = {0};
  search.S_board.B_colour2move = WHITE_ENUM;
  moves.ML_nmoves = 4;
  for (int imove = 0; imove < moves.ML_nmoves; imove++)
  {
    moves.ML_moves[imove].M_move_from = 6;
    moves.ML_moves[imove].M_move_to = 11 + imove;
  }
  // Move 0 is excluded, move 1 failed, move 2 cuts, move 3 is unsearched.
  found[1] = found[2] = TRUE;
  update_history(&search, &moves, found, 2, 4);
  assert(search.S_history[0][6][11] == 0);
  assert(search.S_history[0][6][12] == -20);
  assert(search.S_history[0][6][13] == 20);
  assert(search.S_history[0][6][14] == 0);
  assert(search.S_history[1][6][13] == 0);
  for (int iupdate = 0; iupdate < 10000; iupdate++)
    update_history(&search, &moves, found, 2, 64);
  assert(search.S_history[0][6][12] >= -HISTORY_MAX);
  assert(search.S_history[0][6][13] <= HISTORY_MAX);
  search.S_board.B_colour2move = BLACK_ENUM;
  update_history(&search, &moves, found, 2, 4);
  assert(search.S_history[1][6][13] == 20);
  moves.ML_ncaptx = 1;
  update_history(&search, &moves, found, 2, 4);
  assert(search.S_history[1][6][13] == 20);
  puts("History tests passed: searched moves only, colour, saturation and captures.");
  return 0;
}
