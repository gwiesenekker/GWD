//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#define the_dir(X) cat2(X, _dir)
#define my_dir     the_dir(my_colour)
#define your_dir   the_dir(your_colour)

#define check_the_moves(X) cat3(check_, X, _moves)
#define check_my_moves     check_the_moves(my_colour)
#define check_your_moves   check_the_moves(your_colour)

local int white_dir[4] = {-6, -5, 5, 6};
local int black_dir[4] = {5, 6, -6, -5};

//we always have to update the mask
//not always the input

local void update_patterns(board_t *self,
  int arg_piece, int arg_ifield, int arg_delta)
{
  board_t *object = self;

  //loop over all PS_patterns in which ifield occurs

  patterns_shared_t *with_patterns_shared = &patterns_shared;

  patterns_thread_t *with_patterns_thread =
    &(object->B_network_thread.NT_patterns);

  for (int ipattern = 0; ipattern < with_patterns_shared->PS_npatterns; ipattern++)
  {
    int jpattern = with_patterns_shared->PS_patterns_map[arg_ifield][ipattern];

    //last pattern

    if (jpattern == INVALID) break;

    //ifield occurs in pattern jpattern

    pattern_shared_t *with_pattern_shared = with_patterns_shared->PS_patterns + jpattern;

    pattern_thread_t *with_pattern_thread =
      with_patterns_thread->PT_patterns + jpattern;

    int ilinear = with_pattern_shared->PS_field2linear[arg_ifield];

    HARDBUG(ilinear == INVALID)
    HARDBUG(ilinear >= with_pattern_shared->PS_nlinear)

    //update P_embed

    int embed = INVALID;

    if (arg_piece == (WHITE_BIT | MAN_BIT))
      embed = EMBED_WHITE_MAN;
    else if (arg_piece == (BLACK_BIT | MAN_BIT))
      embed = EMBED_BLACK_MAN;

    HARDBUG(embed == INVALID)

    if (arg_delta > 0)
    {
      HARDBUG(with_pattern_thread->PT_embed[ilinear] != EMBED_EMPTY)

      with_pattern_thread->PT_embed[ilinear] = embed;
    } 
    else
    {
      HARDBUG(with_pattern_thread->PT_embed[ilinear] != embed)

      with_pattern_thread->PT_embed[ilinear] = EMBED_EMPTY;
    }
  }
}

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

void move2bstring(void *self, int arg_imove, bstring arg_bmove_string)
{
  moves_list_t *object = self;

  HARDBUG(arg_imove < 0)

  HARDBUG(arg_imove >= object->ML_nmoves)

  move_t *move = object->ML_moves + arg_imove;
   
  int ifield = move->M_from;
  int kfield = move->M_move_to;

  btrunc(arg_bmove_string, 0);

  ui64_t captures_bb = move->M_captures_bb;

  if (captures_bb == 0)
    HARDBUG(bformata(arg_bmove_string, "%s-%s",
                     nota[ifield], nota[kfield]) == BSTR_ERR)
  else
  {
    int jfield = BIT_CTZ(captures_bb);
   
    captures_bb &= ~BITULL(jfield);

    HARDBUG(bformata(arg_bmove_string, "%sx%sx%s",
                     nota[ifield], nota[kfield], nota[jfield]) == BSTR_ERR)

    while(captures_bb != 0)
    {
      jfield = BIT_CTZ(captures_bb);

      captures_bb &= ~BITULL(jfield);

      HARDBUG(bformata(arg_bmove_string, "x%s", nota[jfield]) == BSTR_ERR)
    }
  }
}

local int move2ints(bstring arg_bmove, int arg_m[50])
{
  BSTRING(bsplit)

  bassigncstr(bsplit, "-x");

  struct bstrList *btokens;
  
  HARDBUG((btokens = bsplits(arg_bmove, bsplit)) == NULL)

  HARDBUG(btokens->qty > 50)

  for (int itoken = 0; itoken < btokens->qty; itoken++)
  {
    if (sscanf(bdata(btokens->entry[itoken]), "%d", arg_m + itoken) != 1)
    {
      PRINTF("bmove=%s\n", bdata(arg_bmove));
      FATAL("eh?", EXIT_FAILURE)
    }
  }

  int result = btokens->qty;

  HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

  BDESTROY(bsplit)

  return(result);
}

int search_move(void *self, bstring arg_bmove)
{
  moves_list_t *object = self;

  int ihit = INVALID;

  int nhits = 0;

  int m1[50];

  int n1 = move2ints(arg_bmove, m1);

  if (n1 < 2) return(INVALID);

  BSTRING(bmove_string)
 
  for (int imove = 0; imove < object->ML_nmoves; imove++)
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

void gen_moves(board_t *object, moves_list_t *arg_moves_list, int arg_quiescence)
{
  if (IS_WHITE(object->B_colour2move))
    gen_white_moves(object, arg_moves_list, arg_quiescence);
  else
    gen_black_moves(object, arg_moves_list, arg_quiescence);
}

void do_move(board_t *object, int arg_imove, moves_list_t *arg_moves_list)
{
  if (IS_WHITE(object->B_colour2move))
    do_white_move(object, arg_imove, arg_moves_list, FALSE);
  else
    do_black_move(object, arg_imove, arg_moves_list, FALSE);
}

void undo_move(board_t *object, int arg_imove, moves_list_t *arg_moves_list)
{
  if (IS_WHITE(object->B_colour2move))
    undo_black_move(object, arg_imove, arg_moves_list, FALSE);
  else
    undo_white_move(object, arg_imove, arg_moves_list, FALSE);
}

void check_moves(board_t *object, moves_list_t *arg_moves_list)
{
  if (IS_WHITE(object->B_colour2move))
    check_white_moves(object, arg_moves_list);
  else
    check_black_moves(object, arg_moves_list);
}

void construct_moves_list(void *self)
{
  moves_list_t *object = self;

  object->ML_nmoves = 0;
}

void fprintf_moves_list(void *self, my_printf_t *arg_my_printf,
  int verbose)
{ 
  moves_list_t *object = self;

  BSTRING(bmove_string)

  if (verbose == 0)
  {
    for (int imove = 0; imove < object->ML_nmoves; imove++) 
    {
      move2bstring(object, imove, bmove_string);

      my_printf(arg_my_printf, "%s\n", bdata(bmove_string));
    }
  }
  else
  {
    for (int imove = 0; imove < object->ML_nmoves; imove++)
    {
      move2bstring(object, imove, bmove_string);

      my_printf(arg_my_printf, "imove=%d move=%s\n", imove,
                bdata(bmove_string));
    }
  }

  BDESTROY(bmove_string)
}
