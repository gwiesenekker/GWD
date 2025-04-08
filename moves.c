//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
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

//we always have to update the mask
//not always the input

void update_patterns_and_layer0(board_t *self,
  int arg_colour2move, int arg_iboard, int arg_delta)
{
  board_t *object = self;

  //loop over all patterns in which iboard occurs

  patterns_t *with_patterns = object->B_network.N_patterns;

  for (int ipattern = 0; ipattern < with_patterns->npatterns; ipattern++)
  {
    int jpattern = with_patterns->patterns_map[arg_iboard][ipattern];

    //last pattern

    if (jpattern == INVALID) break;

    //iboard occurs in pattern jpattern

    pattern_t *with_pattern = with_patterns->patterns + jpattern;

    //*mask is the current occupation of pattern jpattern
    //as iboard occurs in pattern jpattern *with will always be updated
    //if delta > 0 an empty square will be removed and
    //a white man or black man will be added
    //if delta < 0 a white man or black man will be removed and
    //an empty square will be added

    int *mask = object->B_pattern_mask->PM_mask + jpattern;

    //we only have to update the associated input if
    //the current and/or the next occupation are valid

    int input = with_pattern->P_mask2inputs[*mask];

    if (input == INVALID)
    {
      //there is no input associated with the current occupation

      HARDBUG((*mask & with_pattern->P_valid_mask)
              == with_pattern->P_valid_mask)
    }
    else
    {
      //there is an input associated with the current occupation

      //valid or NINPUTS_MAX

      HARDBUG((*mask & with_pattern->P_valid_mask) != with_pattern->P_valid_mask)

      if (input != NINPUTS_MAX)
      {
        if (object->B_network.N_inputs[input] == 1)
        {
          update_layer0(&(object->B_network), input, -1);
        }
      }
    }

    int nshift =
      (arg_iboard - with_patterns->patterns[jpattern].P_root_square) * 4;

    HARDBUG(nshift < 0)
    HARDBUG(nshift > 60)

    int ilinear =
      (with_pattern->P_square2linear >> nshift) & 0xF;

    HARDBUG(ilinear >= with_pattern->P_nlinear)

    if (IS_WHITE(arg_colour2move))
    {
      if (arg_delta > 0)
      {
        HARDBUG((*mask & (MASK_EMPTY << (2 * ilinear))) !=
                (MASK_EMPTY << (2 * ilinear)))

        *mask ^= (MASK_EMPTY << (2 * ilinear));
        *mask |= (MASK_WHITE_MAN << (2 * ilinear));
      }
      else
      {
        HARDBUG((*mask & (MASK_WHITE_MAN << (2 * ilinear))) !=
                (MASK_WHITE_MAN << (2 * ilinear)))

        *mask ^= (MASK_WHITE_MAN << (2 * ilinear));
        *mask |= (MASK_EMPTY << (2 * ilinear));
      }
    }
    else
    {
      if (arg_delta > 0)
      {
        HARDBUG((*mask & (MASK_EMPTY << (2 * ilinear))) !=
                (MASK_EMPTY << (2 * ilinear)))

        *mask ^= (MASK_EMPTY << (2 * ilinear));
        *mask |= (MASK_BLACK_MAN << (2 * ilinear));
      }
      else
      {
        HARDBUG((*mask & (MASK_BLACK_MAN << (2 * ilinear))) !=
                (MASK_BLACK_MAN << (2 * ilinear)))

        *mask ^= (MASK_BLACK_MAN << (2 * ilinear));
        *mask |= (MASK_EMPTY << (2 * ilinear));
      }
    }

    input = with_pattern->P_mask2inputs[*mask];

    if ((*mask & with_pattern->P_valid_mask) == with_pattern->P_valid_mask)
    {
      HARDBUG(input == INVALID)

      //valid or NINPUTS_MAX

      if (input != NINPUTS_MAX)
      {
        HARDBUG(object->B_network.N_inputs[input] != 0)

        update_layer0(&(object->B_network), input, 1);
      }
    }
    else
    {
      HARDBUG(input != INVALID)
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
   
  int iboard = move->M_from;
  int kboard = move->M_move_to;

  btrunc(arg_bmove_string, 0);

  ui64_t captures_bb = move->M_captures_bb;

  if (captures_bb == 0)
    HARDBUG(bformata(arg_bmove_string, "%s-%s",
                     nota[iboard], nota[kboard]) == BSTR_ERR)
  else
  {
    int jboard = BIT_CTZ(captures_bb);
   
    captures_bb &= ~BITULL(jboard);

    HARDBUG(bformata(arg_bmove_string, "%sx%sx%s",
                     nota[iboard], nota[kboard], nota[jboard]) == BSTR_ERR)

    while(captures_bb != 0)
    {
      jboard = BIT_CTZ(captures_bb);

      captures_bb &= ~BITULL(jboard);

      HARDBUG(bformata(arg_bmove_string, "x%s", nota[jboard]) == BSTR_ERR)
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
