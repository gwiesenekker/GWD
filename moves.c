//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#include "globals.h"

#define the_dir(X) cat2(X, _dir)
#define my_dir     the_dir(my_colour)
#define your_dir   the_dir(your_colour)

#define the_man_input_map(X) cat2(X, _man_input_map)
#define my_man_input_map     the_man_input_map(my_colour)
#define your_man_input_map   the_man_input_map(your_colour)

#define the_king_input_map(X) cat2(X, _king_input_map)
#define my_king_input_map     the_king_input_map(my_colour)
#define your_king_input_map   the_king_input_map(your_colour)

#define check_the_moves(X) cat3(check_, X, _moves)
#define check_my_moves     check_the_moves(my_colour)
#define check_your_moves   check_the_moves(your_colour)

local int white_dir[4] = {-6, -5, 5, 6};
local int black_dir[4] = {5, 6, -6, -5};

#define MY_BIT      WHITE_BIT
#define YOUR_BIT    BLACK_BIT
#define my_colour   white
#define your_colour black

#include "moves.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

#define MY_BIT      BLACK_BIT
#define YOUR_BIT    WHITE_BIT
#define my_colour   black
#define your_colour white

#include "moves.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

local int move2int(char *move, int m[50])
{
  int n = 0;
  while(TRUE)
  {
    while (*move and !isdigit(*move))
      move++;
    if (*move == '\0') break;
    HARDBUG(n >= 50)
    HARDBUG(sscanf(move, "%d", &(m[n])) != 1)
    n++;
    while (*move and isdigit(*move))
      move++;
    if (*move == '\0') break;
  }
  return(n);
}

int search_move(moves_list_t *moves_list, char *arg_move)
{
  int ihit = INVALID;
  int nhit = 0;

  int m1[50];

  int n1 = move2int(arg_move, m1);

  if (n1 < 2) return(INVALID);

  for (int imove = 0; imove < moves_list->nmoves; imove++)
  {
    int m2[50];

    int n2 = move2int(moves_list->move2string(moves_list, imove), m2);

    if (n2 < 2) return(INVALID);

    if ((m1[0] == m2[0]) and (m1[1] == m2[1]))
    {
      int n = 1;
      for (int i1 = 2; i1 < n1; i1++)
      {
        n = 0;
        for (int i2 = 2; i2 < n2; i2++)
          if (m1[i1] == m2[i2]) n++;
        if (n != 1) break;
      }
      if (n == 1)
      {
        ihit = imove;
        nhit++;
      }
    }
  }
  if (nhit == 0)
    return (INVALID);
  return(ihit);  
}

void gen_moves(board_t *with, moves_list_t *moves_list, int quiescence)
{
  if (IS_WHITE(with->board_colour2move))
    gen_white_moves(with, moves_list, quiescence);
  else
    gen_black_moves(with, moves_list, quiescence);
}

void do_move(board_t *with, int imove, moves_list_t *moves_list)
{
  if (IS_WHITE(with->board_colour2move))
    do_white_move(with, imove, moves_list);
  else
    do_black_move(with, imove, moves_list);
}

void undo_move(board_t *with, int imove, moves_list_t *moves_list)
{
  if (IS_WHITE(with->board_colour2move))
    undo_black_move(with, imove, moves_list);
  else
    undo_white_move(with, imove, moves_list);
}

void check_moves(board_t *with, moves_list_t *moves_list)
{
  if (IS_WHITE(with->board_colour2move))
    check_white_moves(with, moves_list);
  else
    check_black_moves(with, moves_list);
}

local char *moves_list_move2string(moves_list_t *self, int imove)
{
  HARDBUG(imove < 0)

  HARDBUG(imove >= self->nmoves)

  move_t *move = self->moves + imove;
   
  int iboard = move->move_from;
  int kboard = move->move_to;

  ui64_t captures_bb = move->move_captures_bb;

  if (captures_bb == 0)
    snprintf(self->move_string, MOVE_STRING_MAX, "%s-%s",
      nota[iboard], nota[kboard]);
  else
  {
    int jboard = BIT_CTZ(captures_bb);
   
    captures_bb &= ~BITULL(jboard);

    snprintf(self->move_string, MOVE_STRING_MAX, "%sx%sx%s",
      nota[iboard], nota[kboard], nota[jboard]);

    while(captures_bb != 0)
    {
      jboard = BIT_CTZ(captures_bb);

      captures_bb &= ~BITULL(jboard);

      strcat(self->move_string, "x");
      strcat(self->move_string, nota[jboard]);
    }
  }

  return(self->move_string);
}

local void moves_fprintf_moves(my_printf_t *arg_my_printf, moves_list_t *self,
  int verbose)
{
  if (verbose == 0)
  {
    for (int imove = 0; imove < self->nmoves; imove++)
      my_printf(arg_my_printf, "%s\n", self->move2string(self, imove));
  }
  else
  {
    for (int imove = 0; imove < self->nmoves; imove++)
      my_printf(arg_my_printf, "imove=%d move=%s\n", imove,
        self->move2string(self, imove));
  }
}

local void moves_printf_moves(moves_list_t *self, int verbose)
{
  moves_fprintf_moves(STDOUT, self, verbose);
}

void create_moves_list(moves_list_t *self)
{
  self->nmoves = 0;

  self->move2string = moves_list_move2string;
  self->fprintf_moves = moves_fprintf_moves;
  self->printf_moves = moves_printf_moves;
}
