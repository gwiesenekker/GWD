//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
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

void move2bstring(void *self, int imove, bstring move_string)
{
  moves_list_t *object = self;

  HARDBUG(imove < 0)

  HARDBUG(imove >= object->nmoves)

  move_t *move = object->moves + imove;
   
  int iboard = move->move_from;
  int kboard = move->move_to;

  btrunc(move_string, 0);

  ui64_t captures_bb = move->move_captures_bb;

  if (captures_bb == 0)
    HARDBUG(bformata(move_string, "%s-%s",
                     nota[iboard], nota[kboard]) != BSTR_OK)
  else
  {
    int jboard = BIT_CTZ(captures_bb);
   
    captures_bb &= ~BITULL(jboard);

    HARDBUG(bformata(move_string, "%sx%sx%s",
                     nota[iboard], nota[kboard], nota[jboard]) != BSTR_OK)

    while(captures_bb != 0)
    {
      jboard = BIT_CTZ(captures_bb);

      captures_bb &= ~BITULL(jboard);

      HARDBUG(bformata(move_string, "x%s", nota[jboard]) != BSTR_OK)
    }
  }
}

local int move2ints(bstring bmove, int m[50])
{
  BSTRING(bsplit)

  bassigncstr(bsplit, "-x");

  struct bstrList *btokens;
  
  HARDBUG((btokens = bsplits(bmove, bsplit)) == NULL)

  HARDBUG(btokens->qty > 50)

  for (int itoken = 0; itoken < btokens->qty; itoken++)
  {
    HARDBUG(sscanf(bdata(btokens->entry[itoken]), "%d", m + itoken) != 1)
  }

  int result = btokens->qty;

  HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

  BDESTROY(bsplit)

  return(result);
}

int search_move(void *self, bstring barg_move)
{
  moves_list_t *object = self;

  int ihit = INVALID;

  int nhits = 0;

  int m1[50];

  int n1 = move2ints(barg_move, m1);

  if (n1 < 2) return(INVALID);

  BSTRING(bmove_string)
 
  for (int imove = 0; imove < object->nmoves; imove++)
  {
    int m2[50];

    move2bstring(object, imove, bmove_string);

    int n2 = move2ints(bmove_string, m2);

    if (n2 < 2) return(INVALID);

    if ((m1[0] == m2[0]) and (m1[1] == m2[1]))
    {
      int n = 1;

      for (int i1 = 2; i1 < n1; i1++)
      {
        //search m1[i1] in m2[2..n2]

        n = 0;

        for (int i2 = 2; i2 < n2; i2++)
          if (m1[i1] == m2[i2]) n++;

        if (n != 1) break;
      }

      if (n == 1)
      {
        ihit = imove;

        nhits++;
      }
    }
  }

  BDESTROY(bmove_string)

  if (nhits == 0) return (INVALID);

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

void construct_moves_list(void *self)
{
  moves_list_t *object = self;

  object->nmoves = 0;
}

void fprintf_moves_list(void *self, my_printf_t *arg_my_printf,
  int verbose)
{ 
  moves_list_t *object = self;

  BSTRING(bmove_string)

  if (verbose == 0)
  {
    for (int imove = 0; imove < object->nmoves; imove++) 
    {
      move2bstring(object, imove, bmove_string);

      my_printf(arg_my_printf, "%s\n", bdata(bmove_string));
    }
  }
  else
  {
    for (int imove = 0; imove < object->nmoves; imove++)
    {
      move2bstring(object, imove, bmove_string);

      my_printf(arg_my_printf, "imove=%d move=%s\n", imove,
                bdata(bmove_string));
    }
  }

  BDESTROY(bmove_string)
}

