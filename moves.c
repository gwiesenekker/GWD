// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "globals.h"

local const int white_dir[4] = {-6, -5, 5, 6};
local const int black_dir[4] = {5, 6, -6, -5};

void gen_moves_legacy(board_t *arg_with_board, moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  colour_enum my_colour;
  colour_enum your_colour;
  const int *my_dir;
  const int *my_row;

  if (is_white(arg_with_board->B_colour2move))
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

  ui64_t my_bb = (arg_with_board->my_men_bb | arg_with_board->my_kings_bb);
  ui64_t your_bb =
    (arg_with_board->your_men_bb | arg_with_board->your_kings_bb);
  ui64_t empty_bb = arg_with_board->B_valid_bb & ~(my_bb | your_bb);

  while (my_bb != 0)
  {
    int ifield1;

    if (is_white(arg_with_board->B_colour2move))
      ifield1 = BIT_CTZ(my_bb);
    else
      ifield1 = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(ifield1);

    for (int idirection = 0; idirection < 4; idirection++)
    {
      int jfield1 = ifield1 + my_dir[idirection];

      if (arg_with_board->my_men_bb & BITULL(ifield1))
      {
        if ((idirection < 2) and (arg_moves_list->ML_ncaptx == 0))
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
          jfield1 += my_dir[idirection];
        }
      }

      if (your_bb & BITULL(jfield1))
      {
        int kfield1 = jfield1 + my_dir[idirection];

        if (empty_bb & BITULL(kfield1))
        {
          int idir[NPIECES_MAX + 1];
          int jdir[NPIECES_MAX + 1];
          int ifield[NPIECES_MAX + 1];
          int jfield[NPIECES_MAX + 1];

          jfield[0] = 0;

          // you may pass the square you started on again..

          empty_bb |= BITULL(ifield1);

          idir[1] = my_dir[idirection];

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

          if (arg_with_board->my_kings_bb & BITULL(ifield1))
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

          if (arg_with_board->my_kings_bb & BITULL(ifield1))
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

          if ((arg_with_board->my_kings_bb & BITULL(ifield1)) and
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

    if (arg_with_board->my_men_bb & BITULL(ifield))
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
local inline ui64_t move_shift(ui64_t arg_bb, int arg_direction)
{
  return (arg_direction > 0 ? arg_bb << arg_direction
                            : arg_bb >> -arg_direction);
}

local inline void append_bb_move(moves_list_t *arg_moves_list,
                                 int arg_from_field,
                                 ui64_t arg_to_bb,
                                 ui64_t arg_captures_bb,
                                 int arg_ncaptures)
{
  if (arg_ncaptures < arg_moves_list->ML_ncaptx)
    return;

  if (arg_ncaptures > arg_moves_list->ML_ncaptx)
  {
    arg_moves_list->ML_ncaptx = arg_ncaptures;
    arg_moves_list->ML_nmoves = 0;
  }

  HARDBUG(arg_moves_list->ML_nmoves >= NMOVES_MAX)

  move_t *move = &arg_moves_list->ML_moves[arg_moves_list->ML_nmoves++];

  move->M_move_from = arg_from_field;
  move->M_move_to = BIT_CTZ(arg_to_bb);
  move->M_captures_bb = arg_captures_bb;
}

/* Captured pieces remain blockers until the entire move is finished. The
 * origin, however, is empty: a capture may return to it (rondslag).
 * Keep legacy's traversal: positive perpendicular, negative perpendicular,
 * then straight ahead. Do not deduplicate different capture routes. */
local void capture_man_bb(moves_list_t *arg_moves_list,
                          int arg_from_field,
                          ui64_t arg_current_bb,
                          ui64_t arg_empty_bb,
                          ui64_t arg_opponents_bb,
                          ui64_t arg_captures_bb,
                          int arg_ncaptures,
                          int arg_incoming_direction)
{
  append_bb_move(arg_moves_list,
                 arg_from_field,
                 arg_current_bb,
                 arg_captures_bb,
                 arg_ncaptures);

  int perpendicular = 11 - abs(arg_incoming_direction);
  int directions[3] = {perpendicular, -perpendicular, arg_incoming_direction};
  ui64_t available_bb = arg_opponents_bb & ~arg_captures_bb;

  for (int idirection = 0; idirection < 3; ++idirection)
  {
    ui64_t victim_bb =
      move_shift(arg_current_bb, directions[idirection]) & available_bb;
    ui64_t landing_bb =
      move_shift(victim_bb, directions[idirection]) & arg_empty_bb;

    if (landing_bb)
      capture_man_bb(arg_moves_list,
                     arg_from_field,
                     landing_bb,
                     arg_empty_bb,
                     arg_opponents_bb,
                     arg_captures_bb | victim_bb,
                     arg_ncaptures + 1,
                     directions[idirection]);
  }
}

local void capture_king_bb(moves_list_t *arg_moves_list,
                           int arg_from_field,
                           ui64_t arg_current_bb,
                           ui64_t arg_empty_bb,
                           ui64_t arg_opponents_bb,
                           ui64_t arg_captures_bb,
                           int arg_ncaptures,
                           int arg_incoming_direction)
{
  int perpendicular = 11 - abs(arg_incoming_direction);
  ui64_t available_bb = arg_opponents_bb & ~arg_captures_bb;

  do
  {
    append_bb_move(arg_moves_list,
                   arg_from_field,
                   arg_current_bb,
                   arg_captures_bb,
                   arg_ncaptures);

    for (int direction = perpendicular; direction != 0;
         direction = direction > 0 ? -direction : 0)
    {
      ui64_t victim_bb = move_shift(arg_current_bb, direction);

      while (victim_bb & arg_empty_bb)
        victim_bb = move_shift(victim_bb, direction);
      victim_bb &= available_bb;

      ui64_t landing_bb = move_shift(victim_bb, direction) & arg_empty_bb;

      if (landing_bb)
        capture_king_bb(arg_moves_list,
                        arg_from_field,
                        landing_bb,
                        arg_empty_bb,
                        arg_opponents_bb,
                        arg_captures_bb | victim_bb,
                        arg_ncaptures + 1,
                        direction);
    }
    arg_current_bb = move_shift(arg_current_bb, arg_incoming_direction);
  } while (arg_current_bb & arg_empty_bb);

  ui64_t victim_bb = arg_current_bb & available_bb;
  ui64_t landing_bb =
    move_shift(victim_bb, arg_incoming_direction) & arg_empty_bb;

  if (landing_bb)
    capture_king_bb(arg_moves_list,
                    arg_from_field,
                    landing_bb,
                    arg_empty_bb,
                    arg_opponents_bb,
                    arg_captures_bb | victim_bb,
                    arg_ncaptures + 1,
                    arg_incoming_direction);
}

void gen_moves(board_t *arg_with_board, moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)
  BEGIN_BLOCK(__FUNC__)

  int white = is_white(arg_with_board->B_colour2move);
  colour_enum my_colour = white ? WHITE_ENUM : BLACK_ENUM;
  colour_enum your_colour = white ? BLACK_ENUM : WHITE_ENUM;
  const int *directions = white ? white_dir : black_dir;
  const int *rows = white ? white_row : black_row;
  ui64_t men_bb = arg_with_board->my_men_bb;
  ui64_t kings_bb = arg_with_board->my_kings_bb;
  ui64_t opponents_bb =
    arg_with_board->your_men_bb | arg_with_board->your_kings_bb;
  ui64_t empty_bb =
    arg_with_board->B_valid_bb & ~(men_bb | kings_bb | opponents_bb);
  moves_list_t *with_moves_list = arg_moves_list;

  with_moves_list->ML_nmoves = with_moves_list->ML_ncaptx = 0;

  /* Test all men simultaneously, without visiting non-capturing pieces. */
  ui64_t sources_bb = men_bb & (((opponents_bb >> 5) & (empty_bb >> 10)) |
                                ((opponents_bb >> 6) & (empty_bb >> 12)) |
                                ((opponents_bb << 5) & (empty_bb << 10)) |
                                ((opponents_bb << 6) & (empty_bb << 12)));
  sources_bb |= kings_bb;

  while (sources_bb)
  {
    int from_field = white ? BIT_CTZ(sources_bb) : 63 - BIT_CLZ(sources_bb);
    ui64_t origin_bb = BITULL(from_field);

    sources_bb &= ~origin_bb;

    for (int idirection = 0; idirection < 4; ++idirection)
    {
      int direction = directions[idirection];
      ui64_t victim_bb = move_shift(origin_bb, direction);

      if (origin_bb & kings_bb)
        while (victim_bb & empty_bb)
          victim_bb = move_shift(victim_bb, direction);
      victim_bb &= opponents_bb;

      ui64_t landing_bb = move_shift(victim_bb, direction) & empty_bb;

      if (!landing_bb)
        continue;

      if (origin_bb & men_bb)
        capture_man_bb(with_moves_list,
                       from_field,
                       landing_bb,
                       empty_bb | origin_bb,
                       opponents_bb,
                       victim_bb,
                       1,
                       direction);
      else
        capture_king_bb(with_moves_list,
                        from_field,
                        landing_bb,
                        empty_bb | origin_bb,
                        opponents_bb,
                        victim_bb,
                        1,
                        direction);
    }
  }

  if (with_moves_list->ML_ncaptx == 0)
  {
    sources_bb =
      kings_bb | (men_bb & (white ? (empty_bb << 6) | (empty_bb << 5)
                                  : (empty_bb >> 5) | (empty_bb >> 6)));

    while (sources_bb)
    {
      int from_field = white ? BIT_CTZ(sources_bb) : 63 - BIT_CLZ(sources_bb);
      ui64_t origin_bb = BITULL(from_field);

      sources_bb &= ~origin_bb;

      int king = (origin_bb & kings_bb) != 0;

      for (int idirection = 0; idirection < (king ? 4 : 2); ++idirection)
      {
        ui64_t landing_bb =
          move_shift(origin_bb, directions[idirection]) & empty_bb;

        while (landing_bb)
        {
          append_bb_move(with_moves_list, from_field, landing_bb, 0, 0);

          if (!king)
            break;
          landing_bb =
            move_shift(landing_bb, directions[idirection]) & empty_bb;
        }
      }
    }
  }

  for (int imove = 0; imove < with_moves_list->ML_nmoves; ++imove)
  {
    const move_t *move = &with_moves_list->ML_moves[imove];

    with_moves_list->ML_move_flags[imove] =
      (men_bb & BITULL(move->M_move_from)) and (rows[move->M_move_to] >= 9)
        ? MOVE_DO_NOT_REDUCE_BIT | MOVE_EXTEND_IN_QUIESCENCE_BIT
        : 0;
  }
  arg_with_board->B_nodes[arg_with_board->B_inode].node_ncaptx =
    with_moves_list->ML_ncaptx;
  END_BLOCK
  POP_NAME(__FUNC__)
}

int can_capture(const board_t *arg_with_board, int arg_kings)
{
  BEGIN_BLOCK(__FUNC__)

  colour_enum my_colour;
  colour_enum your_colour;

  if (is_white(arg_with_board->B_colour2move))
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

  ui64_t my_bb = (arg_with_board->my_men_bb | arg_with_board->my_kings_bb);
  ui64_t your_bb =
    (arg_with_board->your_men_bb | arg_with_board->your_kings_bb);
  ui64_t empty_bb = arg_with_board->B_valid_bb & ~(my_bb | your_bb);

  while (my_bb != 0)
  {
    int ifield;

    if (is_white(arg_with_board->B_colour2move))
      ifield = BIT_CTZ(my_bb);
    else
      ifield = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(ifield);

    if (arg_with_board->my_men_bb & BITULL(ifield))
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

    if (arg_with_board->my_kings_bb & BITULL(ifield))
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
             const moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  if (arg_moves_list != NULL)
    HARDBUG(arg_imove >= arg_moves_list->ML_nmoves)

  colour_enum my_colour;
  colour_enum your_colour;
  const int *my_row;

  if (is_white(arg_with_board->B_colour2move))
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

  const move_t *move = arg_moves_list->ML_moves + arg_imove;

  int ifield = move->M_move_from;
  int kfield = move->M_move_to;

  if (ifield != kfield)
  {
    if (arg_with_board->my_men_bb & BITULL(ifield))
    {
      move_key = my_man_key[ifield];

      arg_with_board->B_key ^= my_man_key[ifield];

      arg_with_board->my_men_bb &= ~BITULL(ifield);

      if (my_row[kfield] == 10)
        arg_with_board->my_kings_bb |= BITULL(kfield);
      else
        arg_with_board->my_men_bb |= BITULL(kfield);
    }
    else
    {
      move_key = my_king_key[ifield];

      arg_with_board->B_key ^= my_king_key[ifield];

      arg_with_board->my_kings_bb &= ~BITULL(ifield);

      arg_with_board->my_kings_bb |= BITULL(kfield);
    }

    if (arg_with_board->my_men_bb & BITULL(kfield))
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

    if (arg_with_board->your_men_bb & BITULL(jfield))
    {
      move_key ^= your_man_key[jfield];

      arg_with_board->B_key ^= your_man_key[jfield];

      arg_with_board->your_men_bb &= ~BITULL(jfield);
    }
    else
    {
      move_key ^= your_king_key[jfield];

      arg_with_board->B_key ^= your_king_key[jfield];

      arg_with_board->your_kings_bb &= ~BITULL(jfield);
    }
  }

label_null:

  node->node_move_key = move_key;

  arg_with_board->B_inode++;

  HARDBUG(arg_with_board->B_inode >= NODE_MAX)

  node = arg_with_board->B_nodes + arg_with_board->B_inode;

  node->node_ncaptx = INVALID;
  node->node_move_key = 0;

  if (is_white(arg_with_board->B_colour2move))
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
               const moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  if (arg_moves_list != NULL)
    HARDBUG(arg_imove >= arg_moves_list->ML_nmoves)

  if (is_white(arg_with_board->B_colour2move))
    arg_with_board->B_colour2move = BLACK_ENUM;
  else
    arg_with_board->B_colour2move = WHITE_ENUM;

  const move_t *move = arg_moves_list->ML_moves + arg_imove;

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

void move2bstring(const moves_list_t *arg_with_moves_list,
                  int arg_imove,
                  bstring arg_bmove_string)
{
  HARDBUG(arg_imove < 0)

  HARDBUG(arg_imove >= arg_with_moves_list->ML_nmoves)

  const move_t *move = arg_with_moves_list->ML_moves + arg_imove;

  int ifield = move->M_move_from;
  int kfield = move->M_move_to;

  HARDBUG(btrunc(arg_bmove_string, 0) == BSTR_ERR)

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

local int move2ints(const_bstring arg_bmove, int arg_fields[50])
{
  BSTRING(bseparators)

  HARDBUG(bassigncstr(bseparators, "-x") == BSTR_ERR)

  struct bstrList *btokens;

  HARDBUG((btokens = bsplits(arg_bmove, bseparators)) == NULL)

  HARDBUG(btokens->qty > 50)

  for (int itoken = 0; itoken < btokens->qty; itoken++)
  {
    if (my_sscanf(bdata(btokens->entry[itoken]), "%d", arg_fields + itoken) !=
        1)
    {
      PRINTF("bmove=%s\n", bdata(arg_bmove));
      FATAL("eh?", EXIT_FAILURE)
    }
  }

  int result = btokens->qty;

  HARDBUG(bstrListDestroy(btokens) == BSTR_ERR)

  BDESTROY(bseparators)

  return (result);
}

int search_move(const moves_list_t *arg_with_moves_list,
                const_bstring arg_bmove)
{
  int ihit = INVALID;

  int nhits = 0;

  int requested_fields[50];

  int nrequested_fields = move2ints(arg_bmove, requested_fields);

  if (nrequested_fields < 2)
    return (INVALID);

  BSTRING(bmove_string)

  for (int imove = 0; imove < arg_with_moves_list->ML_nmoves; imove++)
  {
    int candidate_fields[50];

    move2bstring(arg_with_moves_list, imove, bmove_string);

    int ncandidate_fields = move2ints(bmove_string, candidate_fields);

    if (ncandidate_fields < 2)
      return (INVALID);

    if ((requested_fields[0] == candidate_fields[0]) and
        (requested_fields[1] == candidate_fields[1]))
    {
      int nmatches = 1;

      for (int irequested = 2; irequested < nrequested_fields; irequested++)
      {
        // Match each requested capture against the candidate capture fields.

        nmatches = 0;

        for (int icandidate = 2; icandidate < ncandidate_fields; icandidate++)
          if (requested_fields[irequested] == candidate_fields[icandidate])
            nmatches++;

        if (nmatches != 1)
          break;
      }

      if (nmatches == 1)
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

void fprintf_moves_list(const moves_list_t *arg_with_moves_list,
                        my_printf_t *arg_my_printf,
                        int arg_verbose)
{
  BSTRING(bmove_string)

  if (arg_verbose == 0)
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
