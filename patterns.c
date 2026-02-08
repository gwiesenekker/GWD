// SCU REVISION 8.1178 zo  8 feb 2026  6:06:03 CET

#include "globals.h"

patterns_shared_t patterns_shared;

materials_global_t materials_global;

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

local void sort_int(int n, int *a)
{
  for (int i = 0; i < n - 1; i++)
  {
    int k = i;

    for (int j = i + 1; j < n; j++)
      if (a[j] < a[k])
        k = j;

    int t = a[i];
    a[i] = a[k];
    a[k] = t;
  }
  for (int i = 0; i < n - 1; i++)
    HARDBUG(a[i] > a[i + 1])
}

local void
add_pattern(patterns_shared_t *self, int arg_nlinear, int *arg_fields)
{
  patterns_shared_t *object = self;

  HARDBUG(arg_nlinear > NLINEAR_MAX)

  sort_int(arg_nlinear, arg_fields);

  for (int ilinear = 0; ilinear < arg_nlinear; ilinear++)
    PRINTF("ilinear=%d arg_fields=%s\n", ilinear, nota[arg_fields[ilinear]]);

  HARDBUG(object->PSS_npatterns >= NPATTERNS_MAX)

  pattern_shared_t *with_pattern_shared =
    object->PSS_patterns + object->PSS_npatterns;

  with_pattern_shared->PS_nlinear = arg_nlinear;

  PRINTF("object->PSS_npatterns=%d with_pattern_shared->PS_nlinear=%d\n",
         object->PSS_npatterns,
         with_pattern_shared->PS_nlinear);

  for (int ifield = 0; ifield < BOARD_MAX; ifield++)
    with_pattern_shared->PS_field2linear[ifield] = INVALID;

  for (int ilinear = 0; ilinear < with_pattern_shared->PS_nlinear; ilinear++)
  {
    int ifield = arg_fields[ilinear];

    int ipattern;

    for (ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
      if (object->PSS_patterns_map[ifield][ipattern] == INVALID)
        break;

    HARDBUG(ipattern >= NPATTERNS_MAX)

    object->PSS_patterns_map[ifield][ipattern] = object->PSS_npatterns;

    with_pattern_shared->PS_field2linear[ifield] = ilinear;
  }

  int nstates = 1;

  for (int ilinear = 0; ilinear < with_pattern_shared->PS_nlinear; ilinear++)
    nstates *= 3;

  with_pattern_shared->PS_nstates = nstates;

  with_pattern_shared->PS_nembed = INVALID;

  object->PSS_npatterns++;
}

local void add_quincunx(patterns_shared_t *self, int arg_irow, int arg_icol)
{
  patterns_shared_t *object = self;

  int fields[NLINEAR_MAX];

  int nlinear = 0;

  int ifield;

  if ((ifield = map_row_col2field(arg_irow, arg_icol)) != INVALID)
    fields[nlinear++] = ifield;
  if ((ifield = map_row_col2field(arg_irow, arg_icol + 2)) != INVALID)
    fields[nlinear++] = ifield;
  if ((ifield = map_row_col2field(arg_irow + 1, arg_icol + 1)) != INVALID)
    fields[nlinear++] = ifield;
  if ((ifield = map_row_col2field(arg_irow + 2, arg_icol)) != INVALID)
    fields[nlinear++] = ifield;
  if ((ifield = map_row_col2field(arg_irow + 2, arg_icol + 2)) != INVALID)
    fields[nlinear++] = ifield;

  if (nlinear == 0)
    return;

  add_pattern(object, nlinear, fields);
}

local void add_zigzag(patterns_shared_t *self,
                      int arg_nrows,
                      int arg_ncols,
                      int arg_irow,
                      int arg_icol,
                      int arg_delta)
{
  patterns_shared_t *object = self;

  int fields[NLINEAR_MAX];

  int nlinear = 0;

  for (int irow = 0; irow < arg_nrows; irow++)
  {
    int delta = (irow % 2) == 0 ? 0 : arg_delta;

    for (int icol = 0; icol < arg_ncols; icol++)
    {
      int ifield =
        map_row_col2field(arg_irow + irow, arg_icol + icol * 2 + delta);

      if (ifield != INVALID)
        fields[nlinear++] = ifield;
    }
  }

  if (nlinear == 0)
    return;

  add_pattern(object, nlinear, fields);
}

void construct_patterns_shared(patterns_shared_t *self, char *arg_shape)
{
  patterns_shared_t *object = self;

  for (int ifield = 0; ifield < BOARD_MAX; ifield++)
    object->PSS_patterns_map[ifield] = NULL;

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    int ifield = square2field[isquare];

    MY_MALLOC_BY_TYPE(object->PSS_patterns_map[ifield], int, NPATTERNS_MAX)

    for (int ipattern = 0; ipattern < NPATTERNS_MAX; ipattern++)
      object->PSS_patterns_map[ifield][ipattern] = INVALID;
  }

  object->PSS_npatterns = 0;

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
      for (int icol = -1; icol < 10; icol++)
      {
        HARDBUG(bassigncstr(string, "quincunx") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4) or (irow == 6)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_quincunx(object, irow, icol);
          }

          if (((irow == 1) or (irow == 3) or (irow == 5) or (irow == 7)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_quincunx(object, irow, icol);
          }

          ++nshapes;
        }

        HARDBUG(bassigncstr(string, "zigzag6x2") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (((irow == 0) or (irow == 4)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_zigzag(object, 6, 2, irow, icol, -1);
          }

          ++nshapes;
        }

        HARDBUG(bassigncstr(string, "zigzag6x2-11") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (((irow == 0) or (irow == 4)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_zigzag(object, 6, 2, irow, icol, -1);
          }

          if ((irow == 3) and ((icol == 0) or (icol == 2) or (icol == 4)))
          {
            add_zigzag(object, 4, 3, irow, icol, 1);
          }

          ++nshapes;
        }

        HARDBUG(bassigncstr(string, "zigzag4x2") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (((irow == 0) or (irow == 2) or (irow == 4) or (irow == 6)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_zigzag(object, 4, 2, irow, icol, -1);
          }

          ++nshapes;
        }

        HARDBUG(bassigncstr(string, "zigzag4x2-12") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (((irow == 0) or (irow == 6)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_zigzag(object, 4, 2, irow, icol, -1);
          }

          if ((irow == 3) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_zigzag(object, 4, 2, irow, icol, 1);
          }

          ++nshapes;
        }

        HARDBUG(bassigncstr(string, "zigzag3x2-16") == BSTR_ERR)

        if (bstrcmp(string, btokens->entry[itoken]) == 0)
        {
          if (((irow == 0) or (irow == 2)) and
              ((icol == 1) or (icol == 3) or (icol == 5) or (icol == 7)))
          {
            add_zigzag(object, 3, 2, irow, icol, -1);
          }

          if (((irow == 5) or (irow == 7)) and
              ((icol == 0) or (icol == 2) or (icol == 4) or (icol == 6)))
          {
            add_zigzag(object, 3, 2, irow, icol, 1);
          }

          ++nshapes;
        }
      }
    }
  }
  HARDBUG(nshapes == 0)

  BDESTROY(string)

  HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

  BDESTROY(b)

  BDESTROY(barg_shape)

  PRINTF("object->PSS_npatterns=%d\n", object->PSS_npatterns);

  for (int ipattern = 0; ipattern < object->PSS_npatterns; ipattern++)
  {
    pattern_shared_t *with = object->PSS_patterns + ipattern;

    PRINTF("ipattern=%d PS_nlinear=%d", ipattern, with->PS_nlinear);

    for (int ifield = 0; ifield < BOARD_MAX; ifield++)
    {
      int ilinear = with->PS_field2linear[ifield];

      if (ilinear != INVALID)
        PRINTF(" %s", nota[ifield]);
    }
    PRINTF("\n");
  }

  PRINTF("object->PSS_patterns_map\n");

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    int ifield = square2field[isquare];

    PRINTF("ifield=%s object->PSS_patterns_map=", nota[ifield]);

    for (int ipattern = 0; ipattern < object->PSS_npatterns; ipattern++)
    {
      if (object->PSS_patterns_map[ifield][ipattern] == INVALID)
        break;

      PRINTF(" %d", object->PSS_patterns_map[ifield][ipattern]);
    }
    PRINTF("\n");
  }
}

void board2patterns_thread(board_t *self)
{
  board_t *object = self;

  patterns_thread_t *with_patterns_thread =
    &(object->B_network_thread.NT_patterns);

  for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      patterns_shared.PSS_patterns + ipattern;

    pattern_thread_t *with_pattern_thread =
      with_patterns_thread->PT_patterns + ipattern;

    for (int ilinear = 0; ilinear < with_pattern_shared->PS_nlinear; ilinear++)
      with_pattern_thread->PT_embed[ilinear] = 0;
  }

  for (int isquare = 1; isquare <= 50; ++isquare)
  {
    int ifield = square2field[isquare];

    for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
    {
      int jpattern = patterns_shared.PSS_patterns_map[ifield][ipattern];

      if (jpattern == INVALID)
        break;

      pattern_shared_t *with_pattern_shared =
        patterns_shared.PSS_patterns + jpattern;

      pattern_thread_t *with_pattern_thread =
        with_patterns_thread->PT_patterns + jpattern;

      int ilinear = with_pattern_shared->PS_field2linear[ifield];

      HARDBUG(ilinear == INVALID)
      HARDBUG(ilinear >= with_pattern_shared->PS_nlinear)

      if (object->B_white_man_bb & BITULL(ifield))
        with_pattern_thread->PT_embed[ilinear] = EMBED_WHITE_MAN;
      else if (object->B_black_man_bb & BITULL(ifield))
        with_pattern_thread->PT_embed[ilinear] = EMBED_BLACK_MAN;
      else
        with_pattern_thread->PT_embed[ilinear] = EMBED_EMPTY;
    }
  }

  check_board_patterns_thread(object, (char *)__FUNC__, FALSE);
}

void check_board_patterns_thread(board_t *self, char *arg_where, int input_set)
{
  board_t *object = self;

  patterns_thread_t *with_patterns_thread =
    &(object->B_network_thread.NT_patterns);

  for (int ipattern = 0; ipattern < patterns_shared.PSS_npatterns; ipattern++)
  {
    pattern_shared_t *with_pattern_shared =
      patterns_shared.PSS_patterns + ipattern;

    pattern_thread_t *with_pattern_thread =
      with_patterns_thread->PT_patterns + ipattern;

    for (int isquare = 1; isquare <= 50; ++isquare)
    {
      int ifield = square2field[isquare];

      int ilinear = with_pattern_shared->PS_field2linear[ifield];

      if (ilinear != INVALID)
      {
        if (with_pattern_thread->PT_embed[ilinear] == EMBED_WHITE_MAN)
        {
          HARDBUG(!(object->B_white_man_bb & BITULL(ifield)))
        }
        else if (with_pattern_thread->PT_embed[ilinear] == EMBED_BLACK_MAN)
        {
          HARDBUG(!(object->B_black_man_bb & BITULL(ifield)))
        }
        else
        {
          HARDBUG(with_pattern_thread->PT_embed[ilinear] != 0)
        }
      }
    }
  }
}
