//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

#undef GEN_CSV

#define MY_BIT      WHITE_BIT
#define my_colour   white
#define your_colour black

#include "score.d"

#undef MY_BIT
#undef my_colour
#undef your_colour

#define MY_BIT      BLACK_BIT
#define my_colour   black
#define your_colour white

#include "score.d"

#undef MY_BIT
#undef my_colour
#undef your_colour

int return_npieces(board_t *self)
{
  board_t *object = self;

  return(BIT_COUNT(object->B_white_man_bb) +
         BIT_COUNT(object->B_white_king_bb) +
         BIT_COUNT(object->B_black_man_bb) +
         BIT_COUNT(object->B_black_king_bb));
}

int return_material_score(board_t *self)
{
  board_t *object = self;

  int nwhite_man = BIT_COUNT(object->B_white_man_bb);
  int nblack_man = BIT_COUNT(object->B_black_man_bb);
  int nwhite_king = BIT_COUNT(object->B_white_king_bb);
  int nblack_king = BIT_COUNT(object->B_black_king_bb);

  int material_score = (nwhite_man - nblack_man) * SCORE_MAN +
                       (nwhite_king - nblack_king) * SCORE_KING;
  if (IS_WHITE(object->B_colour2move))
    return(material_score);
  else
    return(-material_score);
}
