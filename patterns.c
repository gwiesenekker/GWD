// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST

#include "globals.h"

ui64_t row_bb[10];
int row_bb_ctz[10];

ui64_t diagonal_5_46_bb;

local int map_row_col[10][10] = {{-1, 1, -1, 2, -1, 3, -1, 4, -1, 5},
                                 {6, -1, 7, -1, 8, -1, 9, -1, 10, -1},
                                 {-1, 11, -1, 12, -1, 13, -1, 14, -1, 15},
                                 {16, -1, 17, -1, 18, -1, 19, -1, 20, -1},
                                 {-1, 21, -1, 22, -1, 23, -1, 24, -1, 25},
                                 {26, -1, 27, -1, 28, -1, 29, -1, 30, -1},
                                 {-1, 31, -1, 32, -1, 33, -1, 34, -1, 35},
                                 {36, -1, 37, -1, 38, -1, 39, -1, 40, -1},
                                 {-1, 41, -1, 42, -1, 43, -1, 44, -1, 45},
                                 {46, -1, 47, -1, 48, -1, 49, -1, 50, -1}};

local int square_1_5[5] = {1, 2, 3, 4, 5};
local int square_6_10[5] = {6, 7, 8, 9, 10};
local int square_11_15[5] = {11, 12, 13, 14, 15};
local int square_16_20[5] = {16, 17, 18, 19, 20};
local int square_21_25[5] = {21, 22, 23, 24, 25};
local int square_26_30[5] = {26, 27, 28, 29, 30};
local int square_31_35[5] = {31, 32, 33, 34, 35};
local int square_36_40[5] = {36, 37, 38, 39, 40};
local int square_41_45[5] = {41, 42, 43, 44, 45};
local int square_46_50[5] = {46, 47, 48, 49, 50};

local int diagonal_5_46[10] = {5, 10, 14, 19, 23, 28, 32, 37, 41, 46};
local int diagonal_1_45[10] = {1, 7, 12, 18, 23, 29, 34, 40, 45, 45};
local int diagonal_6_50[10] = {6, 11, 17, 22, 28, 33, 39, 44, 50, 50};

local int defenders_24[6] = {49, 50, 44, 45, 40, 35};
local int attackers_24[6] = {3, 4, 5, 9, 10, 14};

local ui64_t attackers_24_bb;
local ui64_t defenders_24_bb;

local int defenders_27[6] = {1, 2, 6, 7, 11, 16};
local int attackers_27[6] = {46, 47, 48, 41, 42, 37};

local ui64_t attackers_27_bb;
local ui64_t defenders_27_bb;

local int map_row_col2field(int irow, int icol)
{
  if ((irow < 0) or (irow > 9))
    return (INVALID);
  if ((icol < 0) or (icol > 9))
    return (INVALID);

  int isquare = map_row_col[irow][icol];

  if (isquare == INVALID)
    return (INVALID);

  return (square2field[isquare]);
}

local void print_bit_board(char *arg_info, ui64_t arg_bb)
{
  int i = 0;

  PRINTF("arg_info=%s\n", arg_info);

  for (int irow = 10; irow >= 1; --irow)
  {
    if ((irow & 1) == 0)
      PRINTF(" ");

    for (int icol = 1; icol <= 5; ++icol)
    {
      ++i;

      int ifield = square2field[i];

      if (arg_bb & BITULL(ifield))
        PRINTF(" 1");
      else
        PRINTF(" 0");
    }
    PRINTF("\n");
  }
}

local void add_pattern(patterns_shared_t *arg_with_patterns_shared,
                       char *arg_name,
                       int arg_nlinear,
                       int *arg_fields)
{
  HARDBUG(arg_nlinear > NLINEAR_MAX)

  HARDBUG(arg_with_patterns_shared->PSS_npatterns >= NPATTERNS_MAX)

  pattern_shared_t *with_pattern_shared =
    arg_with_patterns_shared->PSS_patterns +
    arg_with_patterns_shared->PSS_npatterns;

  HARDBUG((with_pattern_shared->PS_bname = bfromcstr(arg_name)) == NULL)

  with_pattern_shared->PS_nlinear = arg_nlinear;

  with_pattern_shared->PS_bb = 0;

  if (biseqcstr(with_pattern_shared->PS_bname, PATTERN_BASE3_INDEX_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_BASE3_INDEX_ID;

    int nstates = 1;

    for (int ilinear = 0; ilinear < with_pattern_shared->PS_nlinear; ilinear++)
    {
      int ifield = arg_fields[ilinear];

      with_pattern_shared->PS_bb |= BITULL(ifield);

      nstates *= 3;
    }

    with_pattern_shared->PS_nstates = nstates;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname, PATTERN_NWHITE_MAN_CNAME) ==
           1)
  {
    with_pattern_shared->PS_id = PATTERN_NWHITE_MAN_ID;

    with_pattern_shared->PS_nstates = NPIECES_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname, PATTERN_NBLACK_MAN_CNAME) ==
           1)
  {
    with_pattern_shared->PS_id = PATTERN_NBLACK_MAN_ID;

    with_pattern_shared->PS_nstates = NPIECES_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_NWHITE_KING_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_NWHITE_KING_ID;

    with_pattern_shared->PS_nstates = PATTERN_NKING_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_NBLACK_KING_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_NBLACK_KING_ID;

    with_pattern_shared->PS_nstates = PATTERN_NKING_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_WHITE_MOBILITY_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_WHITE_MOBILITY_ID;

    with_pattern_shared->PS_nstates = 2 * NPIECES_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_BLACK_MOBILITY_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_BLACK_MOBILITY_ID;

    with_pattern_shared->PS_nstates = 2 * NPIECES_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_WHITE_IMMOBILITY_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_WHITE_IMMOBILITY_ID;

    with_pattern_shared->PS_nstates = NPIECES_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_BLACK_IMMOBILITY_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_BLACK_IMMOBILITY_ID;

    with_pattern_shared->PS_nstates = NPIECES_MAX + 1;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_NWHITE_KING_DIAGONAL_5_46_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_NWHITE_KING_DIAGONAL_5_46_ID;

    with_pattern_shared->PS_nstates = 11;
  }
  else if (biseqcstr(with_pattern_shared->PS_bname,
                     PATTERN_NBLACK_KING_DIAGONAL_5_46_CNAME) == 1)
  {
    with_pattern_shared->PS_id = PATTERN_NBLACK_KING_DIAGONAL_5_46_ID;

    with_pattern_shared->PS_nstates = 11;
  }
  else
    FATAL("unknown with_pattern_shared->PS_bname", EXIT_FAILURE)

  with_pattern_shared->PS_nembed = INVALID;

  arg_with_patterns_shared->PSS_npatterns++;
}

local void add_rectangle(patterns_shared_t *arg_with_patterns_shared,
                         int arg_nrows,
                         int arg_ncols,
                         int arg_irow,
                         int arg_icol)
{
  int fields[NLINEAR_MAX];

  int nlinear = 0;

  for (int irow = 0; irow < arg_nrows; irow++)
  {
    for (int icol = 0; icol < arg_ncols; icol++)
    {
      int ifield = map_row_col2field(arg_irow + irow, arg_icol + icol);

      if (ifield != INVALID)
        fields[nlinear++] = ifield;
    }
  }

  if (nlinear == 0)
    return;

  add_pattern(arg_with_patterns_shared,
              PATTERN_BASE3_INDEX_CNAME,
              nlinear,
              fields);
}

local void add_squares(patterns_shared_t *arg_with_patterns_shared,
                       int arg_nsquares,
                       int *arg_squares)
{
  int fields[NLINEAR_MAX];

  int nlinear = 0;

  for (int isquare = 0; isquare < arg_nsquares; isquare++)
  {
    int ifield = square2field[arg_squares[isquare]];

    HARDBUG(ifield == INVALID)
    HARDBUG(nlinear >= NLINEAR_MAX)

    fields[nlinear++] = ifield;
  }

  if (nlinear == 0)
    return;

  add_pattern(arg_with_patterns_shared,
              PATTERN_BASE3_INDEX_CNAME,
              nlinear,
              fields);
}

local ui64_t array2bb(int n, int *a)
{
  ui64_t bb = 0;

  for (int i = 0; i < n; i++)
    bb |= BITULL(square2field[a[i]]);

  return (bb);
}

float return_signal_24(board_t *arg_board)
{
  float result = 0.0;

  if (arg_board->B_white_man_bb & BITULL(square2field[24]))
  {
    int ndefenders = BIT_COUNT(arg_board->B_white_man_bb & defenders_24_bb);
    int nattackers = BIT_COUNT(arg_board->B_black_man_bb & attackers_24_bb);

    result = (float)(ndefenders - nattackers) / 6.0f;
  }

  return (result);
}

float return_signal_27(board_t *arg_board)
{
  float result = 0.0;

  if (arg_board->B_black_man_bb & BITULL(square2field[27]))
  {
    int ndefenders = BIT_COUNT(arg_board->B_black_man_bb & defenders_27_bb);
    int nattackers = BIT_COUNT(arg_board->B_white_man_bb & attackers_27_bb);

    result = (float)(ndefenders - nattackers) / 6.0f;
  }

  return (result);
}

void construct_patterns_shared(patterns_shared_t *arg_with_patterns_shared,
                               char *arg_shape)
{
  arg_with_patterns_shared->PSS_npatterns = 0;

  if (compat_strcasecmp(arg_shape, "NULL") == 0)
  {
    PRINTF("NO PATTERNS!\n");

    return;
  }

  BSTRING(barg_shape)

  HARDBUG(bassigncstr(barg_shape, arg_shape) == BSTR_ERR)

  BSTRING(b)

  bassigncstr(b, ",:;");

  struct bstrList *btokens;

  HARDBUG((btokens = bsplits(barg_shape, b)) == NULL)

  BSTRING(string)

  int nshapes = 0;

  for (int itoken = 0; itoken < btokens->qty; itoken++)
  {
    for (int irow = 0; irow < 10; irow++)
    {
      for (int icol = 0; icol < 10; icol++)
      {
        if (biseqcstr(btokens->entry[itoken], "3x3") == 1)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4) or (irow == 6)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_rectangle(arg_with_patterns_shared, 3, 3, irow, icol);
          }
          if (((irow == 1) or (irow == 3) or (irow == 5) or (irow == 7)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 3, 3, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "5x3") == 1)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_rectangle(arg_with_patterns_shared, 5, 3, irow, icol);
          }
          if (((irow == 1) or (irow == 3) or (irow == 5)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 5, 3, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "5x4") == 1)
        {
          if ((irow <= 5) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 5, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "5x4-16") == 1)
        {
          if (((irow == 0) or (irow == 2) or (irow == 3) or (irow == 5)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 5, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "4x4") == 1)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4) or (irow == 6)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 4, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "5x5") == 1)
        {
          if (((irow == 0) or (irow == 5)) and ((icol == 0) or (icol == 5)))
          {
            add_rectangle(arg_with_patterns_shared, 5, 5, irow, icol);
          }

          ++nshapes;
        }

        if ((biseqcstr(btokens->entry[itoken], "6x3") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x3-m4") == 1))
        {
          if (((irow == 0) or (irow == 4)) and (icol <= 7))
          {
            add_rectangle(arg_with_patterns_shared, 6, 3, irow, icol);
          }

          ++nshapes;
        }

        if ((biseqcstr(btokens->entry[itoken], "6x4") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x4-d") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x4-12") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x4-4x6") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x4-m4") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x4-m8") == 1))
        {
          if (((irow == 0) or (irow == 4)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 6, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "6x4-12") == 1)
        {
          if ((irow == 2) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 6, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "6x4-4x6") == 1)
        {
          if ((irow == 3) and ((icol == 0) or (icol == 2) or (icol == 4)))
          {
            add_rectangle(arg_with_patterns_shared, 4, 6, irow, icol);
          }

          ++nshapes;
        }
      } // for (int icol
    }   // for (int irow

    if ((biseqcstr(btokens->entry[itoken], "6x3-m4") == 1) or
        (biseqcstr(btokens->entry[itoken], "6x4-m4") == 1) or
        (biseqcstr(btokens->entry[itoken], "6x4-m8") == 1))
    {
      add_pattern(arg_with_patterns_shared, PATTERN_NWHITE_MAN_CNAME, 0, NULL);
      add_pattern(arg_with_patterns_shared, PATTERN_NBLACK_MAN_CNAME, 0, NULL);
      add_pattern(arg_with_patterns_shared, PATTERN_NWHITE_KING_CNAME, 0, NULL);
      add_pattern(arg_with_patterns_shared, PATTERN_NBLACK_KING_CNAME, 0, NULL);

      ++nshapes;
    }

    if (biseqcstr(btokens->entry[itoken], "6x4-m8") == 1)
    {
      add_pattern(arg_with_patterns_shared,
                  PATTERN_WHITE_MOBILITY_CNAME,
                  0,
                  NULL);
      add_pattern(arg_with_patterns_shared,
                  PATTERN_BLACK_MOBILITY_CNAME,
                  0,
                  NULL);
      add_pattern(arg_with_patterns_shared,
                  PATTERN_WHITE_IMMOBILITY_CNAME,
                  0,
                  NULL);
      add_pattern(arg_with_patterns_shared,
                  PATTERN_BLACK_IMMOBILITY_CNAME,
                  0,
                  NULL);

      ++nshapes;
    }

    if (biseqcstr(btokens->entry[itoken], "6x4-d") == 1)
    {
      add_pattern(arg_with_patterns_shared,
                  PATTERN_NWHITE_KING_DIAGONAL_5_46_CNAME,
                  0,
                  NULL);
      add_pattern(arg_with_patterns_shared,
                  PATTERN_NBLACK_KING_DIAGONAL_5_46_CNAME,
                  0,
                  NULL);

      ++nshapes;
    }
  }

  HARDBUG(nshapes == 0)

  BDESTROY(string)

  HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

  BDESTROY(b)

  BDESTROY(barg_shape)

  PRINTF("arg_with_patterns_shared->PSS_npatterns=%d\n",
         arg_with_patterns_shared->PSS_npatterns);

  for (int ipattern = 0; ipattern < arg_with_patterns_shared->PSS_npatterns;
       ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      arg_with_patterns_shared->PSS_patterns + ipattern;

    PRINTF("ipattern=%d with_pattern_shared->PS_bname=%s"
           " with_pattern_shared->PS_id=%d"
           " with_pattern_shared->PS_nlinear=%d"
           " with_pattern_shared->PS_nstates=%d",
           ipattern,
           bdata(with_pattern_shared->PS_bname),
           with_pattern_shared->PS_id,
           with_pattern_shared->PS_nlinear,
           with_pattern_shared->PS_nstates);

    ui64_t bb = with_pattern_shared->PS_bb;

    while (bb != 0)
    {
      int ifield = BIT_CTZ(bb);

      bb &= ~BITULL(ifield);

      PRINTF(" %s", nota[ifield]);
    }
    PRINTF("\n");
  }

  diagonal_5_46_bb = array2bb(10, diagonal_5_46);

  print_bit_board("diagonal_5_46_bb", diagonal_5_46_bb);

  defenders_24_bb = array2bb(6, defenders_24);
  attackers_24_bb = array2bb(6, attackers_24);

  print_bit_board("defenders_24_bb", defenders_24_bb);
  print_bit_board("attackers_24_bb", attackers_24_bb);

  defenders_27_bb = array2bb(6, defenders_27);
  attackers_27_bb = array2bb(6, attackers_27);

  print_bit_board("defenders_27_bb", defenders_27_bb);
  print_bit_board("attackers_27_bb", attackers_27_bb);

  row_bb[0] = array2bb(5, square_1_5);
  row_bb[1] = array2bb(5, square_6_10);
  row_bb[2] = array2bb(5, square_11_15);
  row_bb[3] = array2bb(5, square_16_20);
  row_bb[4] = array2bb(5, square_21_25);
  row_bb[5] = array2bb(5, square_26_30);
  row_bb[6] = array2bb(5, square_31_35);
  row_bb[7] = array2bb(5, square_36_40);
  row_bb[8] = array2bb(5, square_41_45);
  row_bb[9] = array2bb(5, square_46_50);

  print_bit_board("square_1_5_bb", row_bb[0]);
  print_bit_board("square_6_10_bb", row_bb[1]);
  print_bit_board("square_11_15_bb", row_bb[2]);
  print_bit_board("square_16_20_bb", row_bb[3]);
  print_bit_board("square_21_25_bb", row_bb[4]);
  print_bit_board("square_26_30_bb", row_bb[5]);
  print_bit_board("square_31_35_bb", row_bb[6]);
  print_bit_board("square_36_40_bb", row_bb[7]);
  print_bit_board("square_41_45_bb", row_bb[8]);
  print_bit_board("square_46_50_bb", row_bb[9]);

  for (int irow = 0; irow < 10; irow++)
  {
    row_bb_ctz[irow] = BIT_CTZ(row_bb[irow]);

    PRINTF("irow=%d row_bb_ctz[irow]=%d\n", irow, row_bb_ctz[irow]);
  }
}
