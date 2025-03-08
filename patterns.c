//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET

#include "globals.h"

#define SWAPINT(X, Y) {int T = (X); (X) = (Y); (Y) = T;}

typedef struct
{
  int U_root_square;
  int U_nlinear;
  int U_mask;
} unique_t;

local unique_t uniques[NINPUTS_MAX];
local int nuniques;
local int nduplicates;

local void set_mask2inputs(pattern_t *self, int arg_nlinear, int arg_valid_mask,
  int *arg_ninputs)
{
  pattern_t *object = self;

  object->P_nlinear = arg_nlinear;

  HARDBUG(object->P_nlinear >= NMASK2INPUTS_MAX)

  object->P_valid_mask = arg_valid_mask;

  int nsub_patterns = 1;

  for (int ilinear = 0; ilinear < object->P_nlinear; ilinear++)
    nsub_patterns *= 3;

  PRINTF("nsub_patterns=%d\n", nsub_patterns);
 
  for (int imask = 0; imask < NMASK2INPUTS_MAX; imask++)
    object->P_mask2inputs[imask] = INVALID;

  for (int isub_pattern = 0; isub_pattern < nsub_patterns; isub_pattern++)
  {
    int base3[NLINEAR_MAX];

    int temp = isub_pattern;

    for (int ilinear = 0; ilinear < object->P_nlinear; ilinear++)
    {
      base3[ilinear] = temp % 3;

      temp /= 3;
    }

    int mask = 0;

    for (int ilinear = object->P_nlinear - 1; ilinear >= 0;
         ilinear--)
    {
      PRINTF("ilinear=%d base3=%d\n", ilinear, base3[ilinear]);

      if (base3[ilinear] == 0)
        mask = (mask << 2) | MASK_EMPTY;
      else if (base3[ilinear] == 1)
        mask = (mask << 2) | MASK_WHITE_MAN;
      else if (base3[ilinear] == 2)
        mask = (mask << 2) | MASK_BLACK_MAN;
      else
        FATAL("% 3 ERROR", EXIT_FAILURE)
    }

    HARDBUG(mask >= NMASK2INPUTS_MAX)
  
    HARDBUG(object->P_mask2inputs[mask] != INVALID)

    if ((mask & object->P_valid_mask) == object->P_valid_mask)
    {
      int unique = TRUE;

      for (int iunique = 0; iunique < nuniques; iunique++)
      {
        unique_t *with_unique = uniques + iunique;

        if ((with_unique->U_root_square == object->P_root_square) and
            (with_unique->U_nlinear == object->P_nlinear) and
            (with_unique->U_mask == mask))
        {
          unique = FALSE;

          break;
        }
      }

      if (!unique)
      {
        object->P_mask2inputs[mask] = NINPUTS_MAX;

        ++nduplicates;
      }
      else
      {
        HARDBUG(nuniques >= NINPUTS_MAX)

        unique_t *with_unique = uniques + nuniques++;

        with_unique->U_root_square = object->P_root_square;
        with_unique->U_nlinear = object->P_nlinear;
        with_unique->U_mask = mask;
      
        HARDBUG(*arg_ninputs >= NINPUTS_MAX)

        object->P_mask2inputs[mask] = (*arg_ninputs)++;
      }
    } 

    PRINTF("isub_pattern=%d mask=%#X input=%d\n",
           isub_pattern, mask, object->P_mask2inputs[mask]);
  }
}

local void add_rectangle(patterns_t *self, int arg_nrows, int arg_ncols,
  int arg_irow, int arg_icol, int arg_valid_mask, int *arg_ninputs)
{
  patterns_t *object = self;

  for (int jrow = 0; jrow < arg_nrows; jrow++)
  {
    if ((arg_irow + jrow) >= 10) return;

    for (int jcol = 0; jcol < arg_ncols; jcol++)
    {
      if ((arg_icol + jcol) >= 5) return;

      int jsquare = (arg_irow + jrow) * 5 + arg_icol + jcol + 1;

      HARDBUG(jsquare > 50)

      int jboard = map[jsquare];

      HARDBUG(jboard >= BOARD_MAX)

      HARDBUG(inverse_map[jboard] == INVALID)
    }
  }

  HARDBUG(object->npatterns >= NPATTERNS_MAX)

  pattern_t *with = object->patterns + object->npatterns;

  int iboard = map[arg_irow * 5 + arg_icol + 1];

  with->P_root_square = iboard;

  with->P_square2linear = 0;

  PRINTF("object->npatterns=%d irow=%d icol=%d iboard=%d\n",
    object->npatterns, arg_irow, arg_icol, iboard);

  int nlinear = arg_nrows * arg_ncols;

  for (int jrow = 0; jrow < arg_nrows; jrow++)
  {
    for (int jcol = 0; jcol < arg_ncols; jcol++)
    {
      int jboard = map[(arg_irow + jrow) * 5 + arg_icol + jcol + 1];

      HARDBUG(jboard >= BOARD_MAX)

      HARDBUG(inverse_map[jboard] == INVALID)

      int ipattern;
  
      for (ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
        if (object->patterns_map[jboard][ipattern] == INVALID) break;
  
      HARDBUG(ipattern >= NPATTERNS_MAX)
  
      object->patterns_map[jboard][ipattern] = object->npatterns;

      int jlinear = jrow * arg_ncols + jcol;

      HARDBUG(jlinear > 15)

      int nshift = (jboard - iboard) * 4;

      HARDBUG(nshift < 0)
      HARDBUG(nshift > 60)

      PRINTF("jlinear=%d nshift=%d\n", jlinear, nshift);

      with->P_square2linear |= ((ui64_t) jlinear << nshift);

      int klinear = (with->P_square2linear >> nshift) & 0xF;

      HARDBUG(klinear != jlinear)
    }
  }

  set_mask2inputs(with, nlinear, arg_valid_mask, arg_ninputs);

  object->npatterns++;
}

local void add_diagonal(patterns_t *self, int arg_ndiagonals, int arg_irow, int arg_icol,
  int arg_idir, int arg_valid_mask, int *arg_ninputs)
{
  patterns_t *object = self;

  int sort[NLINEAR_MAX];

  for (int ilinear = 0; ilinear < NLINEAR_MAX; ilinear++)
    sort[ilinear] = BOARD_MAX;

  sort[0] = map[arg_irow * 5 + arg_icol + 1];

  HARDBUG(inverse_map[sort[0]] == INVALID)

  for (int ilinear = 1; ilinear < arg_ndiagonals; ilinear++)
  {
    sort[ilinear] = sort[ilinear - 1] + arg_idir;

    if (inverse_map[sort[ilinear]] == INVALID) return;
  }

  HARDBUG(arg_ndiagonals > NLINEAR_MAX)

  if (sort[1] < sort[0]) SWAPINT(sort[1], sort[0])
  if (sort[2] < sort[1]) SWAPINT(sort[2], sort[1])
  if (sort[1] < sort[0]) SWAPINT(sort[1], sort[0])

  HARDBUG(sort[0] > sort[1])
  HARDBUG(sort[1] > sort[2])

  HARDBUG(object->npatterns >= NPATTERNS_MAX)

  pattern_t *with = object->patterns + object->npatterns;

  int iboard = sort[0];

  with->P_root_square = iboard;

  with->P_square2linear = 0;

  PRINTF("object->npatterns=%d irow=%d icol=%d iboard=%s\n",
    object->npatterns, arg_irow, arg_icol, nota[iboard]);

  for (int ilinear = 0; ilinear < arg_ndiagonals; ilinear++)
  {
    int jboard = sort[ilinear];

    HARDBUG(jboard >= BOARD_MAX)
    HARDBUG(inverse_map[jboard] == INVALID)

    int ipattern;

    for (ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
      if (object->patterns_map[jboard][ipattern] == INVALID) break;

    HARDBUG(ipattern >= NPATTERNS_MAX)

    object->patterns_map[jboard][ipattern] = object->npatterns;

    int nshift = (jboard - iboard) * 4;

    HARDBUG(nshift < 0)
    HARDBUG(nshift > 60)

    PRINTF("jboard=%s ilinear=%d nshift=%d\n",
           nota[jboard], ilinear, nshift);

    with->P_square2linear |= ((ui64_t) ilinear << nshift);

    int jlinear = (with->P_square2linear >> nshift) & 0xF;

    HARDBUG(jlinear != ilinear)
  }

  set_mask2inputs(with, arg_ndiagonals, arg_valid_mask, arg_ninputs);

  object->npatterns++;
}

local void add_triangle(patterns_t *self, int arg_irow, int arg_icol,
  int arg_left_dir, int arg_right_dr, int arg_valid_mask, int *arg_ninputs)
{
  patterns_t *object = self;

  int sort[3];

  sort[0] = map[arg_irow * 5 + arg_icol + 1];

  HARDBUG(inverse_map[sort[0]] == INVALID)

  sort[1] = sort[0] + arg_left_dir;
  sort[2] = sort[0] + arg_right_dr;

  if (inverse_map[sort[1]] == INVALID) return;
  if (inverse_map[sort[2]] == INVALID) return;

  if (sort[1] < sort[0]) SWAPINT(sort[1], sort[0])
  if (sort[2] < sort[1]) SWAPINT(sort[2], sort[1])
  if (sort[1] < sort[0]) SWAPINT(sort[1], sort[0])

  HARDBUG(sort[0] > sort[1])
  HARDBUG(sort[1] > sort[2])

  HARDBUG(object->npatterns >= NPATTERNS_MAX)
  
  pattern_t *with = object->patterns + object->npatterns;
  
  int iboard = sort[0];

  with->P_root_square = iboard;
  
  with->P_square2linear = 0;
  
  PRINTF("object->npatterns=%d irow=%d icol=%d iboard=%d\n",
    object->npatterns, arg_irow, arg_icol, iboard);

  for (int ilinear = 0; ilinear < 3; ilinear++)
  {
    int jboard = sort[ilinear];
  
    HARDBUG(jboard >= BOARD_MAX)
    HARDBUG(inverse_map[jboard] == INVALID)

    int ipattern;

    for (ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
      if (object->patterns_map[jboard][ipattern] == INVALID) break;
  
    HARDBUG(ipattern >= NPATTERNS_MAX)
  
    object->patterns_map[jboard][ipattern] = object->npatterns;
 
    int nshift = (jboard - iboard) * 4;
 
    HARDBUG(nshift < 0)
    HARDBUG(nshift > 60)
  
    PRINTF("ilinear=%d nshift=%d\n", ilinear, nshift);
  
    with->P_square2linear |= ((ui64_t) ilinear << nshift);

    int jlinear = (with->P_square2linear >> nshift) & 0xF;
  
    HARDBUG(jlinear != ilinear)
  }

  set_mask2inputs(with, 3, arg_valid_mask, arg_ninputs);

  object->npatterns++;
}

local void add_quincunx(patterns_t *self, int arg_irow, int arg_icol,
  int arg_valid_mask, int *arg_ninputs)
{
  patterns_t *object = self;

  int iboard =  map[arg_irow * 5 + arg_icol + 1];

  HARDBUG(inverse_map[iboard] == INVALID)

  if (inverse_map[iboard + 1] == INVALID) return;
  if (inverse_map[iboard + 6] == INVALID) return;
  if (inverse_map[iboard + 6 + 5] == INVALID) return;
  if (inverse_map[iboard + 6 + 6] == INVALID) return;

  HARDBUG(object->npatterns >= NPATTERNS_MAX)

  pattern_t *with = object->patterns + object->npatterns;

  with->P_root_square = iboard;

  with->P_square2linear = 0;

  PRINTF("object->npatterns=%d irow=%d icol=%d iboard=%d\n",
    object->npatterns, arg_irow, arg_icol, iboard);

  int nlinear = 5;

  for (int ilinear = 0; ilinear < nlinear; ilinear++)
  {
    int jboard = INVALID;

    if (ilinear == 0)
      jboard = iboard;
    else if (ilinear == 1)
      jboard = iboard + 1;
    else if (ilinear == 2)
      jboard = iboard + 6;
    else if (ilinear == 3)
      jboard = iboard + 6 + 5;
    else if (ilinear == 4)
      jboard = iboard + 6 + 6;
    else
      FATAL("ilinear > 4", EXIT_FAILURE)
    
    HARDBUG(jboard >= BOARD_MAX)

    HARDBUG(inverse_map[jboard] == INVALID)

    int ipattern;

    for (ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
      if (object->patterns_map[jboard][ipattern] == INVALID) break;

    HARDBUG(ipattern >= NPATTERNS_MAX)

    object->patterns_map[jboard][ipattern] = object->npatterns;

    int nshift = (jboard - iboard) * 4;

    HARDBUG(nshift < 0)
    HARDBUG(nshift > 60)

    PRINTF("jboard=%s ilinear=%d nshift=%d\n",
           nota[jboard], ilinear, nshift);

    with->P_square2linear |= ((ui64_t) ilinear << nshift);

    int jlinear = (with->P_square2linear >> nshift) & 0xF;

    HARDBUG(jlinear != ilinear)
  }

  set_mask2inputs(with, nlinear, arg_valid_mask, arg_ninputs);

  object->npatterns++;
}

local void add_diamond(patterns_t *self, int arg_irow, int arg_icol,
  int arg_valid_mask, int *arg_ninputs)
{
  patterns_t *object = self;

  int iboard =  map[arg_irow * 5 + arg_icol + 1];

  HARDBUG(inverse_map[iboard] == INVALID)

  if (inverse_map[iboard + 5] == INVALID) return;
  if (inverse_map[iboard + 6] == INVALID) return;
  if (inverse_map[iboard + 5 + 6] == INVALID) return;

  HARDBUG(object->npatterns >= NPATTERNS_MAX)

  pattern_t *with = object->patterns + object->npatterns;

  with->P_root_square = iboard;

  with->P_square2linear = 0;

  PRINTF("object->npatterns=%d irow=%d icol=%d iboard=%d\n",
    object->npatterns, arg_irow, arg_icol, iboard);

  int nlinear = 4;

  for (int ilinear = 0; ilinear < nlinear; ilinear++)
  {
    int jboard = INVALID;

    if (ilinear == 0)
      jboard = iboard;
    else if (ilinear == 1)
      jboard = iboard + 5;
    else if (ilinear == 2)
      jboard = iboard + 6;
    else if (ilinear == 3)
      jboard = iboard + 5 + 6;
    else
      FATAL("ilinear >= nlinear", EXIT_FAILURE)
    
    HARDBUG(jboard >= BOARD_MAX)

    HARDBUG(inverse_map[jboard] == INVALID)

    int ipattern;

    for (ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
      if (object->patterns_map[jboard][ipattern] == INVALID) break;

    HARDBUG(ipattern >= NPATTERNS_MAX)

    object->patterns_map[jboard][ipattern] = object->npatterns;

    int nshift = (jboard - iboard) * 4;

    HARDBUG(nshift < 0)
    HARDBUG(nshift > 60)

    PRINTF("jboard=%s ilinear=%d nshift=%d\n",
           nota[jboard], ilinear, nshift);

    with->P_square2linear |= ((ui64_t) ilinear << nshift);

    int jlinear = (with->P_square2linear >> nshift) & 0xF;

    HARDBUG(jlinear != ilinear)
  }

  set_mask2inputs(with, nlinear, arg_valid_mask, arg_ninputs);

  object->npatterns++;
}

int construct_patterns(patterns_t *self, char *arg_shape)
{
  patterns_t *object = self;

  for (int iboard = 0; iboard < BOARD_MAX; iboard++)
    object->patterns_map[iboard] = NULL;

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    MY_MALLOC(object->patterns_map[iboard], int, NPATTERNS_MAX)

    for (int ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
      object->patterns_map[iboard][ipattern] = INVALID;
  }

  object->npatterns = 0;

  int ninputs = 0;
  nuniques = 0;
  nduplicates = 0;

  BSTRING(barg_shape)

  HARDBUG(bassigncstr(barg_shape, arg_shape) == BSTR_ERR)

  BSTRING(bsplit)

  bassigncstr(bsplit, ",:;");

  struct bstrList *btokens;
  
  HARDBUG((btokens = bsplits(barg_shape, bsplit)) == NULL)

  BSTRING(string)

  for (int irow = 0; irow < 10; irow++)
  {
    for (int icol = 0; icol < 5; icol++)
    {
      for (int itoken = 0; itoken < btokens->qty; itoken++)
      {
        HARDBUG(bassigncstr(string, "1x1") == BSTR_ERR)
   
        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_rectangle(object, 1, 1, irow, icol, 0, &ninputs);
        }

        HARDBUG(bassigncstr(string, "1x1-emp") == BSTR_ERR)
   
        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_rectangle(object, 1, 1, irow, icol, MASK_EMPTY, &ninputs);
        }

        HARDBUG(bassigncstr(string, "1x5") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
          add_rectangle(object, 1, 5, irow, icol, 0, &ninputs);

        HARDBUG(bassigncstr(string, "1x2") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
          add_rectangle(object, 1, 2, irow, icol, 0, &ninputs);

        HARDBUG(bassigncstr(string, "2x2") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
          add_rectangle(object, 2, 2, irow, icol, 0, &ninputs);

        HARDBUG(bassigncstr(string, "2d") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_diagonal(object, 2, irow, icol, -6, 0, 
                       &ninputs);
          add_diagonal(object, 2, irow, icol, -5, 0,
                       &ninputs);
          add_diagonal(object, 2, irow, icol, 5, 0, &ninputs);
          add_diagonal(object, 2, irow, icol, 6, 0, &ninputs);
        }

        HARDBUG(bassigncstr(string, "2d-man") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_diagonal(object, 2, irow, icol, -6, MASK_WHITE_MAN << 2,
                       &ninputs);
          add_diagonal(object, 2, irow, icol, -5, MASK_WHITE_MAN << 2,
                       &ninputs);
          add_diagonal(object, 2, irow, icol, 5, MASK_BLACK_MAN, &ninputs);
          add_diagonal(object, 2, irow, icol, 6, MASK_BLACK_MAN, &ninputs);
        }

        HARDBUG(bassigncstr(string, "3d") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_diagonal(object, 3, irow, icol, -6, 0,
                       &ninputs);
          add_diagonal(object, 3, irow, icol, -5, 0,
                       &ninputs);
          add_diagonal(object, 3, irow, icol, 5, 0, &ninputs);
          add_diagonal(object, 3, irow, icol, 6, 0, &ninputs);
        }

        HARDBUG(bassigncstr(string, "3d-man") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_diagonal(object, 3, irow, icol, -6, MASK_WHITE_MAN << 4,
                       &ninputs);
          add_diagonal(object, 3, irow, icol, -5, MASK_WHITE_MAN << 4,
                       &ninputs);
          add_diagonal(object, 3, irow, icol, 5, MASK_BLACK_MAN, &ninputs);
          add_diagonal(object, 3, irow, icol, 6, MASK_BLACK_MAN, &ninputs);
        }

        HARDBUG(bassigncstr(string, "d3") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_diagonal(object, 3, irow, icol, -6, 0, &ninputs);
          add_diagonal(object, 3, irow, icol, -5, 0, &ninputs);
          add_diagonal(object, 3, irow, icol, 5, 0, &ninputs);
          add_diagonal(object, 3, irow, icol, 6, 0, &ninputs);
        }

        HARDBUG(bassigncstr(string, "forward-triangle") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_triangle(object, irow, icol, -6, -5, 0,
                       &ninputs);
          add_triangle(object, irow, icol, 5, 6, 0, &ninputs);
        }

        HARDBUG(bassigncstr(string, "forward-triangle-man") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_triangle(object, irow, icol, -6, -5, MASK_WHITE_MAN << 4,
                       &ninputs);
          add_triangle(object, irow, icol, 5, 6, MASK_BLACK_MAN, &ninputs);
        }

        HARDBUG(bassigncstr(string, "backward-triangle-man") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (irow > 0)
            add_triangle(object, irow, icol, 5, 6, MASK_WHITE_MAN, &ninputs);
          if (irow < 9)
            add_triangle(object, irow, icol, -6, -5, MASK_BLACK_MAN << 4, &ninputs);
        }

        HARDBUG(bassigncstr(string, "promotion") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (irow == 1)
          {
            add_diagonal(object, 2, irow, icol, -6, MASK_WHITE_MAN << 2,
                         &ninputs);
            add_diagonal(object, 2, irow, icol, -5, MASK_WHITE_MAN << 2,
                         &ninputs);
          }
          if (irow == 8)
          {
            add_diagonal(object, 2, irow, icol, 5, MASK_BLACK_MAN, &ninputs);
            add_diagonal(object, 2, irow, icol, 6, MASK_BLACK_MAN, &ninputs);
          }
        }

        HARDBUG(bassigncstr(string, "quincunx") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_quincunx(object, irow, icol, 0, &ninputs);
        }

        HARDBUG(bassigncstr(string, "quincunx-man") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_quincunx(object, irow, icol, MASK_WHITE_MAN << 6,
                       &ninputs);
          add_quincunx(object, irow, icol, MASK_BLACK_MAN << 6,
                       &ninputs);
        }

        HARDBUG(bassigncstr(string, "diamond") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          add_diamond(object, irow, icol, 0, &ninputs);
        }
      }
    }
  }

  BDESTROY(string)

  HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

  BDESTROY(bsplit)

  BDESTROY(barg_shape)

  PRINTF("object->npatterns=%d\n", object->npatterns);

  HARDBUG(ninputs == 0)
  HARDBUG(ninputs >= NINPUTS_MAX)
  HARDBUG(nuniques != ninputs)

  PRINTF("ninputs=%d nuniques=%d nduplicates=%d\n",
         ninputs, nuniques, nduplicates);

  for (int ipattern = 0; ipattern < object->npatterns; ipattern++)
  {
    pattern_t *with = object->patterns + ipattern;

    PRINTF("ipattern=%d P_nlinear=%d P_valid_mask=%#X P_root_square=%s",
           ipattern, with->P_nlinear, with->P_valid_mask,
           nota[with->P_root_square]);

    for (int iboard = 0; iboard < 16; iboard++)
    {
      int ilinear = (with->P_square2linear >> (iboard * 4)) & 0xF;

      if (ilinear > 0) PRINTF(" %s", nota[with->P_root_square + iboard]);
    }
    PRINTF("\n");
  }

  PRINTF("object->patterns_map\n");

  for (int i = 1; i <= 50; ++i)
  {
    int iboard = map[i];

    PRINTF("iboard=%s object->patterns_map=", nota[iboard]);

    for (int ipattern = 0; ipattern < object->npatterns; ipattern++)
    {
      if (object->patterns_map[iboard][ipattern] == INVALID) break;

      PRINTF(" %d", object->patterns_map[iboard][ipattern]);
    }

    PRINTF("\n");
  }

  return(ninputs);
}

void push_pattern_mask_state(pattern_mask_t *self)
{
  pattern_mask_t *object = self;

  HARDBUG(object->PM_nstates >= NODE_MAX)
  
  pattern_mask_state_t *state = object->PM_states + object->PM_nstates;

  memcpy(state, object->PM_mask, sizeof(int) * object->PM_npatterns);

  object->PM_nstates++;
}

void pop_pattern_mask_state(pattern_mask_t *self)
{
  pattern_mask_t *object = self;

  object->PM_nstates--;

  HARDBUG(object->PM_nstates < 0)
  
  pattern_mask_state_t *state = object->PM_states + object->PM_nstates;

  memcpy(object->PM_mask, state, sizeof(int) * object->PM_npatterns);
}

void board2patterns(board_t *self)
{
  board_t *object = self;

  ui64_t empty_bb = object->board_white_man_bb | 
                    object->board_white_king_bb | 
                    object->board_black_man_bb | 
                    object->board_black_king_bb;

  empty_bb = object->board_empty_bb & ~empty_bb;

  patterns_t *with_patterns = object->board_network.network_patterns;

  object->board_pattern_mask->PM_npatterns = with_patterns->npatterns;
  object->board_pattern_mask->PM_nstates = 0;

  for (int ipattern = 0; ipattern < with_patterns->npatterns; ipattern++)
    object->board_pattern_mask->PM_mask[ipattern] = 0;

  for (int i = 1; i <= 50; i++)
  {
    int iboard = map[i];

    for (int ipattern = 0; ipattern < with_patterns->npatterns; ipattern++)
    {
      int jpattern = with_patterns->patterns_map[iboard][ipattern];

      if (jpattern == INVALID) break;

      int *mask = object->board_pattern_mask->PM_mask + jpattern;

      int nshift =
        (iboard - with_patterns->patterns[jpattern].P_root_square) * 4;

      HARDBUG(nshift < 0)
      HARDBUG(nshift > 60)

      pattern_t *with_pattern = with_patterns->patterns + jpattern;

      int ilinear =
        (with_pattern->P_square2linear >> nshift) & 0xF;

      HARDBUG(ilinear >= with_pattern->P_nlinear)

      if (object->board_white_man_bb & BITULL(iboard))
      {
        *mask |= (MASK_WHITE_MAN << (2 * ilinear));
      }
      else if (object->board_black_man_bb & BITULL(iboard))
      {
        *mask |= (MASK_BLACK_MAN << (2 * ilinear));
      }
      else
      {
        *mask |= (MASK_EMPTY << (2 * ilinear));
      }
    }
  }

  check_board_patterns(object, (char *) __FUNC__);
}

void check_board_patterns(board_t *self, char *arg_where)
{
  board_t *object = self;

  patterns_t *with_patterns = object->board_network.network_patterns;

  int ok = TRUE;

  for (int ipattern = 0; ipattern < with_patterns->npatterns; ipattern++)
  {
    pattern_t *with_pattern = with_patterns->patterns + ipattern;

    //*mask is the current occupation

    int *mask = object->board_pattern_mask->PM_mask + ipattern;

    //input is the input associated with the current occupation

    int input = with_pattern->P_mask2inputs[*mask];

    if (input == INVALID)
    {
      //there is no input associated with the current occupation

      if ((*mask & with_pattern->P_valid_mask) == with_pattern->P_valid_mask)
      {
        PRINTF("INVALID input but mask is VALID ipattern=%d input=%d\n",
               ipattern, input);

        ok = FALSE;
      }
    }
    else
    {
      //valid input OR duplicate

      if ((*mask & with_pattern->P_valid_mask) != with_pattern->P_valid_mask)
      {
        PRINTF("VALID input but mask is INVALID ipattern=%d input=%d\n",
               ipattern, input);

        ok = FALSE;
      }
    }
  }
  if (!ok)
  {
    PRINTF("%s failed where=%s\n", __FUNC__, arg_where);

    FATAL("eh??", EXIT_FAILURE)
  }
}
