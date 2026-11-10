// SCU REVISION 8.1262 di 10 nov 2026 15:00:45 CET
#include "globals.h"

local int white_dir[4] = {-6, -5, 5, 6};
local int black_dir[4] = {5, 6, -6, -5};

void gen_moves_legacy(board_t *arg_with_board, moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  colour_enum my_colour;
  colour_enum your_colour;
  int *my_dir;
  int *my_row;

  if (IS_WHITE(arg_with_board->B_colour2move))
  {
    my_colour = WHITE_ENUM;
    your_colour = BLACK_ENUM;

    my_dir = white_dir;
    my_row = white_row;
  }
  else
  {
    my_colour = BLACK_ENUM;
    your_colour = WHITE_ENUM;

    my_dir = black_dir;
    my_row = black_row;
  }

  arg_moves_list->ML_nmoves = 0;
  arg_moves_list->ML_ncaptx = 0;

  ui64_t my_bb = (arg_with_board->my_man_bb | arg_with_board->my_king_bb);
  ui64_t your_bb = (arg_with_board->your_man_bb | arg_with_board->your_king_bb);
  ui64_t empty_bb = arg_with_board->B_valid_bb & ~(my_bb | your_bb);

  while (my_bb != 0)
  {
    int ifield1;

    if (IS_WHITE(arg_with_board->B_colour2move))
      ifield1 = BIT_CTZ(my_bb);
    else
      ifield1 = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(ifield1);

    for (int i = 0; i < 4; i++)
    {
      int jfield1 = ifield1 + my_dir[i];

      if (arg_with_board->my_man_bb & BITULL(ifield1))
      {
        if ((i < 2) and (arg_moves_list->ML_ncaptx == 0))
        {
          if (empty_bb & BITULL(jfield1))
          {
            HARDBUG(arg_moves_list->ML_nmoves >= NMOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;

            move->M_move_from = ifield1;
            move->M_move_to = jfield1;
            move->M_captures_bb = 0;

            arg_moves_list->ML_nmoves++;
          }
        }
      }
      else
      {
        while (empty_bb & BITULL(jfield1))
        {
          if (arg_moves_list->ML_ncaptx == 0)
          {
            HARDBUG(arg_moves_list->ML_nmoves >= NMOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;

            move->M_move_from = ifield1;
            move->M_move_to = jfield1;
            move->M_captures_bb = 0;

            arg_moves_list->ML_nmoves++;
          }
          jfield1 += my_dir[i];
        }
      }
      if (your_bb & BITULL(jfield1))
      {
        int kfield1 = jfield1 + my_dir[i];

        if (empty_bb & BITULL(kfield1))
        {
          int idir[NPIECES_MAX + 1];
          int jdir[NPIECES_MAX + 1];
          int ifield[NPIECES_MAX + 1];
          int jfield[NPIECES_MAX + 1];

          jfield[0] = 0;

          // you may pass the square you started on again..

          empty_bb |= BITULL(ifield1);

          idir[1] = my_dir[i];

          int ncapt = 0;

          ui64_t captures_bb = 0;

        label_next_capt:

          ++ncapt;

          captures_bb |= BITULL(jfield1);

          jfield[ncapt] = jfield1;
          ifield[ncapt] = kfield1;

        label_king:

          if (ncapt > arg_moves_list->ML_ncaptx)
          {
            arg_moves_list->ML_ncaptx = ncapt;

            arg_moves_list->ML_nmoves = 0;
          }
          if (ncapt == arg_moves_list->ML_ncaptx)
          {
            if (arg_moves_list->ML_nmoves >= NMOVES_MAX)
            {
              print_board(arg_with_board);
            }
            HARDBUG(arg_moves_list->ML_nmoves >= NMOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;

            move->M_move_from = ifield1;
            move->M_move_to = kfield1;
            move->M_captures_bb = captures_bb;

            arg_moves_list->ML_nmoves++;
          }
          if (idir[ncapt] > 0)
            jdir[ncapt] = 11 - idir[ncapt];
          else
            jdir[ncapt] = 11 + idir[ncapt];

          jfield1 = ifield[ncapt] + jdir[ncapt];

          if (arg_with_board->my_king_bb & BITULL(ifield1))
            while (empty_bb & BITULL(jfield1))
              jfield1 += jdir[ncapt];

          //..but you may not capture the same piece twice!
          if ((your_bb & BITULL(jfield1)) and !(captures_bb & BITULL(jfield1)))
          {
            kfield1 = jfield1 + jdir[ncapt];

            if (empty_bb & BITULL(kfield1))
            {
              idir[ncapt + 1] = jdir[ncapt];

              goto label_next_capt;
            }
          }

        label_neg_dir:

          jdir[ncapt] = -jdir[ncapt];

          jfield1 = ifield[ncapt] + jdir[ncapt];

          if (arg_with_board->my_king_bb & BITULL(ifield1))
            while (empty_bb & BITULL(jfield1))
              jfield1 += jdir[ncapt];

          if ((your_bb & BITULL(jfield1)) and !(captures_bb & BITULL(jfield1)))
          {
            kfield1 = jfield1 + jdir[ncapt];

            if (empty_bb & BITULL(kfield1))
            {
              idir[ncapt + 1] = jdir[ncapt];

              goto label_next_capt;
            }
          }

        label_dir:

          jdir[ncapt] = 0;

          jfield1 = ifield[ncapt] + idir[ncapt];

          if ((arg_with_board->my_king_bb & BITULL(ifield1)) and
              (empty_bb & BITULL(jfield1)))
          {
            kfield1 = ifield[ncapt] = jfield1;

            goto label_king;
          }

          if ((your_bb & BITULL(jfield1)) and !(captures_bb & BITULL(jfield1)))
          {
            kfield1 = jfield1 + idir[ncapt];

            if (empty_bb & BITULL(kfield1))
            {
              idir[ncapt + 1] = idir[ncapt];

              goto label_next_capt;
            }
          }

        label_undo_capt:

          captures_bb &= ~BITULL(jfield[ncapt]);

          --ncapt;

          if (ncapt > 0)
          {
            if (jdir[ncapt] > 0)
              goto label_neg_dir;

            if (jdir[ncapt] < 0)
              goto label_dir;

            goto label_undo_capt;
          }
          empty_bb &= ~BITULL(ifield1);
        }
      }
    }
  }

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    move_t *move = arg_moves_list->ML_moves + imove;

    int ifield = move->M_move_from;
    int kfield = move->M_move_to;

    arg_moves_list->ML_move_flags[imove] = 0;

    if (arg_with_board->my_man_bb & BITULL(ifield))
    {
      if (my_row[kfield] >= 9)
      {
        arg_moves_list->ML_move_flags[imove] =
          MOVE_DO_NOT_REDUCE_BIT | MOVE_EXTEND_IN_QUIESCENCE_BIT;
      }
    }
  }

  arg_with_board->B_nodes[arg_with_board->B_inode].node_ncaptx =
    arg_moves_list->ML_ncaptx;

  // PRINTF("use case ntempo=%d\n", moves_list->ntempo);

  END_BLOCK

  POP_NAME(__FUNC__)
}

/* One-hot padded squares: shifting off the board is harmless. In particular,
 * never form 1ULL << a possibly negative scalar square number. */
local inline ui64_t move_shift(ui64_t bb, int dir)
{
  return dir > 0 ? bb << dir : bb >> -dir;
}

local inline void append_bb_move(moves_list_t *list,
                                 int from,
                                 ui64_t to,
                                 ui64_t captures,
                                 int count)
{
  if (count < list->ML_ncaptx)
    return;
  if (count > list->ML_ncaptx)
  {
    list->ML_ncaptx = count;
    list->ML_nmoves = 0;
  }
  HARDBUG(list->ML_nmoves >= NMOVES_MAX)
  move_t *move = &list->ML_moves[list->ML_nmoves++];
  move->M_move_from = from;
  move->M_move_to = BIT_CTZ(to);
  move->M_captures_bb = captures;
}

/* Captured pieces remain blockers until the entire move is finished. The
 * origin, however, is empty: a capture may return to it (rondslag).
 * Keep legacy's traversal: positive perpendicular, negative perpendicular,
 * then straight ahead. Do not deduplicate different capture routes. */
local void capture_man_bb(moves_list_t *list,
                          int from,
                          ui64_t at,
                          ui64_t empty,
                          ui64_t opponents,
                          ui64_t captures,
                          int count,
                          int incoming)
{
  append_bb_move(list, from, at, captures, count);
  int perpendicular = 11 - abs(incoming);
  int dirs[3] = {perpendicular, -perpendicular, incoming};
  ui64_t available = opponents & ~captures;
  for (int i = 0; i < 3; ++i)
  {
    ui64_t victim = move_shift(at, dirs[i]) & available;
    ui64_t landing = move_shift(victim, dirs[i]) & empty;
    if (landing)
      capture_man_bb(list,
                     from,
                     landing,
                     empty,
                     opponents,
                     captures | victim,
                     count + 1,
                     dirs[i]);
  }
}

local void capture_king_bb(moves_list_t *list,
                           int from,
                           ui64_t at,
                           ui64_t empty,
                           ui64_t opponents,
                           ui64_t captures,
                           int count,
                           int incoming)
{
  int perpendicular = 11 - abs(incoming);
  ui64_t available = opponents & ~captures;
  do
  {
    append_bb_move(list, from, at, captures, count);
    for (int dir = perpendicular; dir != 0; dir = dir > 0 ? -dir : 0)
    {
      ui64_t victim = move_shift(at, dir);
      while (victim & empty)
        victim = move_shift(victim, dir);
      victim &= available;
      ui64_t landing = move_shift(victim, dir) & empty;
      if (landing)
        capture_king_bb(list,
                        from,
                        landing,
                        empty,
                        opponents,
                        captures | victim,
                        count + 1,
                        dir);
    }
    at = move_shift(at, incoming);
  } while (at & empty);

  ui64_t victim = at & available;
  ui64_t landing = move_shift(victim, incoming) & empty;
  if (landing)
    capture_king_bb(list,
                    from,
                    landing,
                    empty,
                    opponents,
                    captures | victim,
                    count + 1,
                    incoming);
}

void gen_moves(board_t *arg_with_board, moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)
  BEGIN_BLOCK(__FUNC__)

  int white = IS_WHITE(arg_with_board->B_colour2move);
  colour_enum my_colour = white ? WHITE_ENUM : BLACK_ENUM;
  colour_enum your_colour = white ? BLACK_ENUM : WHITE_ENUM;
  const int *dirs = white ? white_dir : black_dir;
  const int *rows = white ? white_row : black_row;
  ui64_t men = arg_with_board->my_man_bb;
  ui64_t kings = arg_with_board->my_king_bb;
  ui64_t opponents = arg_with_board->your_man_bb | arg_with_board->your_king_bb;
  ui64_t empty = arg_with_board->B_valid_bb & ~(men | kings | opponents);
  moves_list_t *list = arg_moves_list;
  list->ML_nmoves = list->ML_ncaptx = 0;

  /* Test all men simultaneously, without visiting non-capturing pieces. */
  ui64_t sources =
    men &
    (((opponents >> 5) & (empty >> 10)) | ((opponents >> 6) & (empty >> 12)) |
     ((opponents << 5) & (empty << 10)) | ((opponents << 6) & (empty << 12)));
  sources |= kings;
  while (sources)
  {
    int from = white ? BIT_CTZ(sources) : 63 - BIT_CLZ(sources);
    ui64_t origin = BITULL(from);
    sources &= ~origin;
    for (int i = 0; i < 4; ++i)
    {
      int dir = dirs[i];
      ui64_t victim = move_shift(origin, dir);
      if (origin & kings)
        while (victim & empty)
          victim = move_shift(victim, dir);
      victim &= opponents;
      ui64_t landing = move_shift(victim, dir) & empty;
      if (!landing)
        continue;
      if (origin & men)
        capture_man_bb(list,
                       from,
                       landing,
                       empty | origin,
                       opponents,
                       victim,
                       1,
                       dir);
      else
        capture_king_bb(list,
                        from,
                        landing,
                        empty | origin,
                        opponents,
                        victim,
                        1,
                        dir);
    }
  }

  if (list->ML_ncaptx == 0)
  {
    sources = kings | (men & (white ? (empty << 6) | (empty << 5)
                                    : (empty >> 5) | (empty >> 6)));
    while (sources)
    {
      int from = white ? BIT_CTZ(sources) : 63 - BIT_CLZ(sources);
      ui64_t origin = BITULL(from);
      sources &= ~origin;
      int king = (origin & kings) != 0;
      for (int i = 0; i < (king ? 4 : 2); ++i)
      {
        ui64_t landing = move_shift(origin, dirs[i]) & empty;
        while (landing)
        {
          append_bb_move(list, from, landing, 0, 0);
          if (!king)
            break;
          landing = move_shift(landing, dirs[i]) & empty;
        }
      }
    }
  }
  for (int i = 0; i < list->ML_nmoves; ++i)
  {
    const move_t *move = &list->ML_moves[i];
    list->ML_move_flags[i] =
      (men & BITULL(move->M_move_from)) && rows[move->M_move_to] >= 9
        ? MOVE_DO_NOT_REDUCE_BIT | MOVE_EXTEND_IN_QUIESCENCE_BIT
        : 0;
  }
  arg_with_board->B_nodes[arg_with_board->B_inode].node_ncaptx =
    list->ML_ncaptx;
  END_BLOCK
  POP_NAME(__FUNC__)
}

int can_capture(board_t *arg_with_board, int arg_kings)
{
  BEGIN_BLOCK(__FUNC__)

  colour_enum my_colour;
  colour_enum your_colour;

  if (IS_WHITE(arg_with_board->B_colour2move))
  {
    my_colour = WHITE_ENUM;
    your_colour = BLACK_ENUM;
  }
  else
  {
    my_colour = BLACK_ENUM;
    your_colour = WHITE_ENUM;
  }

  int result;

  result = TRUE;

  ui64_t my_bb = (arg_with_board->my_man_bb | arg_with_board->my_king_bb);
  ui64_t your_bb = (arg_with_board->your_man_bb | arg_with_board->your_king_bb);
  ui64_t empty_bb = arg_with_board->B_valid_bb & ~(my_bb | your_bb);

  while (my_bb != 0)
  {
    int ifield;

    if (IS_WHITE(arg_with_board->B_colour2move))
      ifield = BIT_CTZ(my_bb);
    else
      ifield = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(ifield);

    if (arg_with_board->my_man_bb & BITULL(ifield))
    {
      if ((your_bb & BITULL(ifield - 6)) and
          (empty_bb & BITULL(ifield - 6 - 6)))
      {
        goto label_return;
      }
      if ((your_bb & BITULL(ifield - 5)) and
          (empty_bb & BITULL(ifield - 5 - 5)))
      {
        goto label_return;
      }
      if ((your_bb & BITULL(ifield + 5)) and
          (empty_bb & BITULL(ifield + 5 + 5)))
      {
        goto label_return;
      }
      if ((your_bb & BITULL(ifield + 6)) and
          (empty_bb & BITULL(ifield + 6 + 6)))
      {
        goto label_return;
      }
    }

    if (!arg_kings)
      continue;

    if (arg_with_board->my_king_bb & BITULL(ifield))
    {
      int jfield = ifield - 6;

      while (empty_bb & BITULL(jfield))
        jfield -= 6;

      if ((your_bb & BITULL(jfield)) and (empty_bb & BITULL(jfield - 6)))
      {
        goto label_return;
      }

      jfield = ifield - 5;

      while (empty_bb & BITULL(jfield))
        jfield -= 5;

      if ((your_bb & BITULL(jfield)) and (empty_bb & BITULL(jfield - 5)))
      {
        goto label_return;
      }

      jfield = ifield + 5;

      while (empty_bb & BITULL(jfield))
        jfield += 5;

      if ((your_bb & BITULL(jfield)) and (empty_bb & BITULL(jfield + 5)))
      {
        goto label_return;
      }

      jfield = ifield + 6;

      while (empty_bb & BITULL(jfield))
        jfield += 6;

      if ((your_bb & BITULL(jfield)) and (empty_bb & BITULL(jfield + 6)))
      {
        goto label_return;
      }
    }
  }

  result = FALSE;

label_return:

  END_BLOCK

  return (result);
}

void do_move(board_t *arg_with_board,
             int arg_imove,
             moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  if (arg_moves_list != NULL)
    HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

  colour_enum my_colour;
  colour_enum your_colour;
  int *my_row;

  if (IS_WHITE(arg_with_board->B_colour2move))
  {
    my_colour = WHITE_ENUM;
    your_colour = BLACK_ENUM;

    my_row = white_row;
  }
  else
  {
    my_colour = BLACK_ENUM;
    your_colour = WHITE_ENUM;

    my_row = black_row;
  }

  push_board_state(arg_with_board);

  node_t *node = arg_with_board->B_nodes + arg_with_board->B_inode;

  ui64_t move_key = 0;

  if (arg_moves_list == NULL)
    goto label_null;

  move_t *move = arg_moves_list->ML_moves + arg_imove;

  int ifield = move->M_move_from;
  int kfield = move->M_move_to;

  if (ifield != kfield)
  {
    if (arg_with_board->my_man_bb & BITULL(ifield))
    {
      move_key = my_man_key[ifield];

      arg_with_board->B_key ^= my_man_key[ifield];

      arg_with_board->my_man_bb &= ~BITULL(ifield);

      if (my_row[kfield] == 10)
        arg_with_board->my_king_bb |= BITULL(kfield);
      else
        arg_with_board->my_man_bb |= BITULL(kfield);
    }
    else
    {
      move_key = my_king_key[ifield];

      arg_with_board->B_key ^= my_king_key[ifield];

      arg_with_board->my_king_bb &= ~BITULL(ifield);

      arg_with_board->my_king_bb |= BITULL(kfield);
    }

    if (arg_with_board->my_man_bb & BITULL(kfield))
    {
      move_key ^= my_man_key[kfield];

      arg_with_board->B_key ^= my_man_key[kfield];
    }
    else
    {
      move_key ^= my_king_key[kfield];

      arg_with_board->B_key ^= my_king_key[kfield];
    }
  }

  ui64_t captures_bb = move->M_captures_bb;

  while (captures_bb != 0)
  {
    int jfield = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jfield);

    if (arg_with_board->your_man_bb & BITULL(jfield))
    {
      move_key ^= your_man_key[jfield];

      arg_with_board->B_key ^= your_man_key[jfield];

      arg_with_board->your_man_bb &= ~BITULL(jfield);
    }
    else
    {
      move_key ^= your_king_key[jfield];

      arg_with_board->B_key ^= your_king_key[jfield];

      arg_with_board->your_king_bb &= ~BITULL(jfield);
    }
  }

label_null:

  node->node_move_key = move_key;

  arg_with_board->B_inode++;

  HARDBUG(arg_with_board->B_inode >= NODE_MAX)

  node = arg_with_board->B_nodes + arg_with_board->B_inode;

  node->node_ncaptx = INVALID;
  node->node_move_key = 0;

  if (IS_WHITE(arg_with_board->B_colour2move))
    arg_with_board->B_colour2move = BLACK_ENUM;
  else
    arg_with_board->B_colour2move = WHITE_ENUM;

#ifdef DEBUG
  // check_my_malloc();

  hash_key_t temp_key;

  temp_key = return_key_from_bb(arg_with_board);

  HARDBUG(temp_key != arg_with_board->B_key)
#endif

  END_BLOCK

  POP_NAME(__FUNC__)
}

void undo_move(board_t *arg_with_board,
               int arg_imove,
               moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  if (arg_moves_list != NULL)
    HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

  if (IS_WHITE(arg_with_board->B_colour2move))
    arg_with_board->B_colour2move = BLACK_ENUM;
  else
    arg_with_board->B_colour2move = WHITE_ENUM;

  move_t *move = arg_moves_list->ML_moves + arg_imove;

  // now we can restore the board

  pop_board_state(arg_with_board);

  ui64_t captures_bb = move->M_captures_bb;

  while (captures_bb != 0)
  {
    int jfield = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jfield);
  }

  arg_with_board->B_inode--;

  HARDBUG(arg_with_board->B_inode < 0)

  node_t *node = arg_with_board->B_nodes + arg_with_board->B_inode;

  node->node_move_key = 0;

#ifdef DEBUG
  // check_my_malloc();

  hash_key_t temp_key;

  temp_key = return_key_from_bb(arg_with_board);

  HARDBUG(temp_key ^= arg_with_board->B_key)
#endif

  END_BLOCK

  POP_NAME(__FUNC__)
}

void check_moves(board_t *arg_with_board, moves_list_t *arg_moves_list)
{
  BEGIN_BLOCK(__FUNC__)
  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    do_move(arg_with_board, imove, arg_moves_list);

    hash_key_t temp_key;

    temp_key = return_key_from_bb(arg_with_board);
    HARDBUG(temp_key != arg_with_board->B_key)

    undo_move(arg_with_board, imove, arg_moves_list);

    temp_key = return_key_from_bb(arg_with_board);
    HARDBUG(temp_key != arg_with_board->B_key)
  }

  END_BLOCK
}

void move2bstring(moves_list_t *arg_with_moves_list,
                  int arg_imove,
                  bstring arg_bmove_string)
{
  HARDBUG(arg_imove < 0)

  HARDBUG(arg_imove >= arg_with_moves_list->ML_nmoves)

  move_t *move = arg_with_moves_list->ML_moves + arg_imove;

  int ifield = move->M_move_from;
  int kfield = move->M_move_to;

  btrunc(arg_bmove_string, 0);

  ui64_t captures_bb = move->M_captures_bb;

  if (captures_bb == 0)
    HARDBUG(bformata(arg_bmove_string, "%s-%s", nota[ifield], nota[kfield]) ==
            BSTR_ERR)
  else
  {
    int jfield = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jfield);

    HARDBUG(bformata(arg_bmove_string,
                     "%sx%sx%s",
                     nota[ifield],
                     nota[kfield],
                     nota[jfield]) == BSTR_ERR)

    while (captures_bb != 0)
    {
      jfield = BIT_CTZ(captures_bb);

      captures_bb &= ~BITULL(jfield);

      HARDBUG(bformata(arg_bmove_string, "x%s", nota[jfield]) == BSTR_ERR)
    }
  }
}

local int move2ints(bstring arg_bmove, int arg_m[50])
{
  BSTRING(b)

  bassigncstr(b, "-x");

  struct bstrList *btokens;

  HARDBUG((btokens = bsplits(arg_bmove, b)) == NULL)

  HARDBUG(btokens->qty > 50)

  for (int itoken = 0; itoken < btokens->qty; itoken++)
  {
    if (my_sscanf(bdata(btokens->entry[itoken]), "%d", arg_m + itoken) != 1)
    {
      PRINTF("bmove=%s\n", bdata(arg_bmove));
      FATAL("eh?", EXIT_FAILURE)
    }
  }

  int result = btokens->qty;

  HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

  BDESTROY(b)

  return (result);
}

int search_move(moves_list_t *arg_with_moves_list, bstring arg_bmove)
{
  int ihit = INVALID;

  int nhits = 0;

  int m1[50];

  int n1 = move2ints(arg_bmove, m1);

  if (n1 < 2)
    return (INVALID);

  BSTRING(bmove_string)

  for (int imove = 0; imove < arg_with_moves_list->ML_nmoves; imove++)
  {
    int m2[50];

    move2bstring(arg_with_moves_list, imove, bmove_string);

    int n2 = move2ints(bmove_string, m2);

    if (n2 < 2)
      return (INVALID);

    if ((m1[0] == m2[0]) and (m1[1] == m2[1]))
    {
      int n = 1;

      for (int i1 = 2; i1 < n1; i1++)
      {
        // search m1[i1] in m2[2..n2]

        n = 0;

        for (int i2 = 2; i2 < n2; i2++)
          if (m1[i1] == m2[i2])
            n++;

        if (n != 1)
          break;
      }

      if (n == 1)
      {
        ihit = imove;

        nhits++;
      }
    }
  }

  BDESTROY(bmove_string)

  if (nhits == 0)
    return (INVALID);

  return (ihit);
}

void construct_moves_list(moves_list_t *arg_with_moves_list)
{
  arg_with_moves_list->ML_nmoves = 0;
}

void fprintf_moves_list(moves_list_t *arg_with_moves_list,
                        my_printf_t *arg_my_printf,
                        int verbose)
{
  BSTRING(bmove_string)

  if (verbose == 0)
  {
    for (int imove = 0; imove < arg_with_moves_list->ML_nmoves; imove++)
    {
      move2bstring(arg_with_moves_list, imove, bmove_string);

      my_printf(arg_my_printf, "%s\n", bdata(bmove_string));
    }
  }
  else
  {
    for (int imove = 0; imove < arg_with_moves_list->ML_nmoves; imove++)
    {
      move2bstring(arg_with_moves_list, imove, bmove_string);

      my_printf(arg_my_printf,
                "imove=%d move=%s\n",
                imove,
                bdata(bmove_string));
    }
  }

  BDESTROY(bmove_string)
}
