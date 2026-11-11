// SCU REVISION 8.1267 wo 11 nov 2026  3:10:47 CET

#include "globals.h"

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
        if (biseqcstr(btokens->entry[itoken], "4x4") == 1)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4) or (irow == 6)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 4, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "4x4-6x4-4x4") == 1)
        {
          if (((irow == 0) or (irow == 6)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 4, 4, irow, icol);
          }

          if ((irow == 2) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 6, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "3x6") == 1)
        {
          if (((icol == 0) or (icol == 4)) and (irow <= 7))
          {
            add_rectangle(arg_with_patterns_shared, 3, 6, irow, icol);
          }

          ++nshapes;
        }

        if ((biseqcstr(btokens->entry[itoken], "6x3") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x3-3x6") == 1))
        {
          if (((irow == 0) or (irow == 4)) and (icol <= 7))
          {
            add_rectangle(arg_with_patterns_shared, 6, 3, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "6x3-3x6") == 1)
        {
          if (((icol == 0) or (icol == 4)) and ((irow == 3) or (irow == 4)))
          {
            add_rectangle(arg_with_patterns_shared, 3, 6, irow, icol);
          }

          ++nshapes;
        }

        if ((biseqcstr(btokens->entry[itoken], "5x4") == 1) or
            (biseqcstr(btokens->entry[itoken], "5x4-4x5") == 1) or
            (biseqcstr(btokens->entry[itoken], "5x4-4x5v2") == 1))
        {
          if (((irow == 0) or (irow == 5)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 5, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "5x4-4x5") == 1)
        {
          if ((irow == 3) and ((icol == 0) or (icol == 5)))
          {
            add_rectangle(arg_with_patterns_shared, 4, 5, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "5x4-4x5v2") == 1)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4) or (irow == 6)) and
              ((icol == 0) or (icol == 5)))
          {
            add_rectangle(arg_with_patterns_shared, 4, 5, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "5x5") == 1)
        {
          if ((irow <= 5) and ((icol == 0) or (icol == 5)))
          {
            add_rectangle(arg_with_patterns_shared, 5, 5, irow, icol);
          }

          ++nshapes;
        }

        if ((biseqcstr(btokens->entry[itoken], "6x4") == 1) or
            (biseqcstr(btokens->entry[itoken], "6x4-4x6") == 1))
        {
          if (((irow == 0) or (irow == 4)) and
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

        if (biseqcstr(btokens->entry[itoken], "6x4v3") == 1)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 6, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "7x4") == 1)
        {
          if (((irow == 0) or (irow == 3)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 7, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "8x4") == 1)
        {
          if (((irow == 0) or (irow == 2)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_rectangle(arg_with_patterns_shared, 8, 4, irow, icol);
          }

          ++nshapes;
        }

        if (biseqcstr(btokens->entry[itoken], "10x3") == 1)
        {
          if ((irow == 0) and (icol <= 7))
          {
            add_rectangle(arg_with_patterns_shared, 10, 3, irow, icol);
          }

          ++nshapes;
        }
      } // for (int icol
    }   // for (int irow
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
}
