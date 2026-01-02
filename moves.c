//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

local int white_dir[4] = {-6, -5, 5, 6};
local int black_dir[4] = {5, 6, -6, -5};

//we always have to update the mask
//not always the input

local void update_patterns(board_t *self,
  colour_enum arg_colour, piece_enum arg_piece,
  int arg_ifield, int arg_delta)
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

    if ((arg_colour == WHITE_ENUM) and (arg_piece == MAN_ENUM))
      embed = EMBED_WHITE_MAN;
    else if ((arg_colour == BLACK_ENUM) and (arg_piece == MAN_ENUM))
      embed = EMBED_BLACK_MAN;
    else
      FATAL("eh?", EXIT_FAILURE)

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

void gen_moves(board_t *object, moves_list_t *arg_moves_list)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  colour_enum my_colour;
  colour_enum your_colour;
  int *my_dir;
  int *my_row;

  if (IS_WHITE(object->B_colour2move))
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
  arg_moves_list->ML_nblocked = 0;
  arg_moves_list->ML_nsafe = 0;

  ui64_t my_bb = (object->my_man_bb | object->my_king_bb);
  ui64_t your_bb = (object->your_man_bb | object->your_king_bb);
  ui64_t empty_bb = object->B_valid_bb & ~(my_bb | your_bb);

  while(my_bb != 0)
  {
    int ifield1;

    if (IS_WHITE(object->B_colour2move))
      ifield1 = BIT_CTZ(my_bb);
    else
      ifield1 = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(ifield1);

    for (int i = 0; i < 4; i++)
    {
      int jfield1 = ifield1 + my_dir[i];

      if (object->my_man_bb & BITULL(ifield1))
      {
        if ((i < 2) and (arg_moves_list->ML_ncaptx == 0))
        {
          if (empty_bb & BITULL(jfield1))
          {
            SOFTBUG(arg_moves_list->ML_nmoves >= NMOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;
            
            move->M_from = ifield1;
            move->M_move_to = jfield1;
            move->M_captures_bb = 0;
       
            arg_moves_list->ML_nmoves++;
          }  
          else if (field2square[jfield1] != INVALID)
          {
            arg_moves_list->ML_nblocked++;
          }
        }
      }
      else
      {
        while (empty_bb & BITULL(jfield1))
        {
          if (arg_moves_list->ML_ncaptx == 0)
          {
            SOFTBUG(arg_moves_list->ML_nmoves >= NMOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;
            
            move->M_from = ifield1;
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

          //you may pass the square you started on again..

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
            arg_moves_list->ML_nblocked = 0;
          }
          if (ncapt == arg_moves_list->ML_ncaptx)
          {
if (arg_moves_list->ML_nmoves >= NMOVES_MAX)
{
print_board(object);
}
            SOFTBUG(arg_moves_list->ML_nmoves >= NMOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;
            
            move->M_from = ifield1;
            move->M_move_to = kfield1;
            move->M_captures_bb = captures_bb;
       
            arg_moves_list->ML_nmoves++;
          }
          if (idir[ncapt] > 0)
            jdir[ncapt] = 11 - idir[ncapt];
          else
            jdir[ncapt] = 11 + idir[ncapt];

          jfield1 = ifield[ncapt] + jdir[ncapt];

          if (object->my_king_bb & BITULL(ifield1))
            while (empty_bb & BITULL(jfield1)) jfield1 += jdir[ncapt];

          //..but you may not capture the same piece twice!
          if ((your_bb & BITULL(jfield1)) and
              !(captures_bb & BITULL(jfield1)))
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

          if (object->my_king_bb & BITULL(ifield1))
            while (empty_bb & BITULL(jfield1)) jfield1 += jdir[ncapt];

          if ((your_bb & BITULL(jfield1)) and
              !(captures_bb & BITULL(jfield1)))
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

          if ((object->my_king_bb & BITULL(ifield1)) and
              (empty_bb & BITULL(jfield1)))
          {
            kfield1 = ifield[ncapt] = jfield1;

            goto label_king;
          }

          if ((your_bb & BITULL(jfield1)) and
              !(captures_bb & BITULL(jfield1)))
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
            if (jdir[ncapt] > 0) goto label_neg_dir;

            if (jdir[ncapt] < 0) goto label_dir;

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
     
    int ifield = move->M_from;
    int kfield = move->M_move_to;

    int move_score_delta = 0;

    ui64_t captures_bb = move->M_captures_bb;

    while(captures_bb != 0)
    {
      int jfield = BIT_CTZ(captures_bb);

      if (object->your_man_bb & BITULL(jfield))
      {
        move_score_delta += SCORE_MAN;
      }
      else
      {
        move_score_delta += SCORE_KING;
      }

      captures_bb &= ~BITULL(jfield);
    }

    int move_weight = move_score_delta;

    int move_flag = 0;

    if (object->my_man_bb & BITULL(ifield))
    {
      if (my_row[kfield] == 8)
      {
        move_weight = move_score_delta + 10;
      }
      else if (my_row[kfield] == 9)
      {
        move_weight = move_score_delta + 20;

        move_flag = MOVE_DO_NOT_REDUCE_BIT | MOVE_EXTEND_IN_QUIESCENCE_BIT;
      }
      else if (my_row[kfield] == 10)
      {
        move_score_delta += SCORE_KING - SCORE_MAN;

        move_weight = move_score_delta + 30;

        move_flag = MOVE_DO_NOT_REDUCE_BIT | MOVE_EXTEND_IN_QUIESCENCE_BIT;
      }
    }
    else
    {
      move_flag = MOVE_KING_BIT;
    }

    arg_moves_list->ML_move_weights[imove] = move_weight;

    arg_moves_list->ML_move_flags[imove] = move_flag;
  }

  object->B_nodes[object->B_inode].node_ncaptx = arg_moves_list->ML_ncaptx;

  //PRINTF("use case ntempo=%d\n", moves_list->ntempo);

  END_BLOCK

  POP_NAME(__FUNC__)
}

int can_capture(board_t *object, int arg_kings)
{
  BEGIN_BLOCK(__FUNC__)

  colour_enum my_colour;
  colour_enum your_colour;

  if (IS_WHITE(object->B_colour2move))
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

  ui64_t my_bb = (object->my_man_bb | object->my_king_bb);
  ui64_t your_bb = (object->your_man_bb | object->your_king_bb);
  ui64_t empty_bb = object->B_valid_bb & ~(my_bb | your_bb);

  while(my_bb != 0)
  {
    int ifield;

    if (IS_WHITE(object->B_colour2move))
      ifield = BIT_CTZ(my_bb);
    else
      ifield = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(ifield);

    if (object->my_man_bb & BITULL(ifield))
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

    if (!arg_kings) continue;
   
    if (object->my_king_bb & BITULL(ifield))
    {    
      int jfield = ifield - 6;

      while(empty_bb & BITULL(jfield)) jfield -= 6;

      if ((your_bb & BITULL(jfield)) and
          (empty_bb & BITULL(jfield - 6)))
      {
        goto label_return;
      }

      jfield = ifield - 5;

      while(empty_bb & BITULL(jfield)) jfield -= 5;

      if ((your_bb & BITULL(jfield)) and
          (empty_bb & BITULL(jfield - 5)))
      {
        goto label_return;
      }

      jfield = ifield + 5;

      while(empty_bb & BITULL(jfield)) jfield += 5;

      if ((your_bb & BITULL(jfield)) and
          (empty_bb & BITULL(jfield + 5)))
      {
        goto label_return;
      }

      jfield = ifield + 6;

      while(empty_bb & BITULL(jfield)) jfield += 6;

      if ((your_bb & BITULL(jfield)) and
          (empty_bb & BITULL(jfield + 6)))
      {
        goto label_return;
      }
    }
  }

  result = FALSE;

  label_return:

  END_BLOCK

  return(result);
}

void do_move(board_t *object, int arg_imove, moves_list_t *arg_moves_list,
  int arg_quick)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  if (arg_moves_list != NULL)
    HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

  colour_enum my_colour;
  colour_enum your_colour;
  int *my_row;

  if (IS_WHITE(object->B_colour2move))
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

  if (options.material_only) arg_quick = TRUE;

  push_board_state(object);

  node_t *node = object->B_nodes + object->B_inode;

  ui64_t move_key = 0;

  if (arg_moves_list == NULL) goto label_null;

  move_t *move = arg_moves_list->ML_moves + arg_imove;

  int ifield = move->M_from;
  int kfield = move->M_move_to;

  if (ifield != kfield)
  {
    if (object->my_man_bb & BITULL(ifield))
    {
      move_key = my_man_key[ifield];

      XOR_HASH_KEY(object->B_key, my_man_key[ifield])

      if (!arg_quick)
        update_patterns(object, my_colour, MAN_ENUM, ifield,  -1);

      object->my_man_bb &= ~BITULL(ifield);

      if (my_row[kfield] == 10)
        object->my_king_bb |= BITULL(kfield);
      else
        object->my_man_bb |= BITULL(kfield);
    }
    else
    {
      move_key = my_king_key[ifield];

      XOR_HASH_KEY(object->B_key, my_king_key[ifield]);

      object->my_king_bb &= ~BITULL(ifield);

      object->my_king_bb |= BITULL(kfield);
    }
  
    if (object->my_man_bb & BITULL(kfield))
    {
      move_key ^= my_man_key[kfield];

      XOR_HASH_KEY(object->B_key, my_man_key[kfield])

      if (!arg_quick)
        update_patterns(object, my_colour, MAN_ENUM, kfield, 1);
    }
    else
    {
      move_key ^= my_king_key[kfield];

      XOR_HASH_KEY(object->B_key, my_king_key[kfield])
    }
  }

  ui64_t captures_bb = move->M_captures_bb;

  while(captures_bb != 0)
  {
    int jfield = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jfield);

    if (object->your_man_bb & BITULL(jfield))
    {
      move_key ^= your_man_key[jfield];

      XOR_HASH_KEY(object->B_key, your_man_key[jfield])

      if (!arg_quick)
        update_patterns(object, your_colour, MAN_ENUM, jfield, -1);

      object->your_man_bb &= ~BITULL(jfield);
    }
    else
    {
      move_key ^= your_king_key[jfield];

      XOR_HASH_KEY(object->B_key, your_king_key[jfield])

      object->your_king_bb &= ~BITULL(jfield);
    }
  }

  label_null:

  node->node_move_key = move_key;

  object->B_inode++;

  SOFTBUG(object->B_inode >= NODE_MAX)

  node = object->B_nodes + object->B_inode;

  node->node_ncaptx = INVALID;
  node->node_move_key = 0;

  if (IS_WHITE(object->B_colour2move))
    object->B_colour2move = BLACK_ENUM;
  else
    object->B_colour2move = WHITE_ENUM;

#ifdef DEBUG
  //check_my_malloc();

  hash_key_t temp_key;
  
  temp_key = return_key_from_bb(object);

  HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))

  if (!arg_quick)
    check_board_patterns_thread(object, (char *) __FUNC__, TRUE);
#endif

  END_BLOCK

  POP_NAME(__FUNC__)
}

void undo_move(board_t *object, int arg_imove, moves_list_t *arg_moves_list,
  int arg_quick)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  if (arg_moves_list != NULL)
    HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

  if (IS_WHITE(object->B_colour2move))
    object->B_colour2move = BLACK_ENUM;
  else
    object->B_colour2move = WHITE_ENUM;

  colour_enum my_colour;
  colour_enum your_colour;

  if (IS_WHITE(object->B_colour2move))
  {
    my_colour = WHITE_ENUM;
    your_colour = BLACK_ENUM;
  }
  else
  {
    my_colour = BLACK_ENUM;
    your_colour = WHITE_ENUM;
  }

  if (options.material_only) arg_quick = TRUE;

  move_t *move = arg_moves_list->ML_moves + arg_imove;

  int ifield = move->M_from;
  int kfield = move->M_move_to;

  if (ifield != kfield)
  {
    if ((object->my_man_bb & BITULL(kfield)) and (!arg_quick))
      update_patterns(object, my_colour, MAN_ENUM, kfield, -1);
  }

  //now we can restore the board

  pop_board_state(object);

  if (ifield != kfield)
  {
    if ((object->my_man_bb & BITULL(ifield)) and (!arg_quick))
      update_patterns(object, my_colour, MAN_ENUM, ifield,  1);
  }

  ui64_t captures_bb = move->M_captures_bb;

  while(captures_bb != 0)
  {
    int jfield = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jfield);

    if ((object->your_man_bb & BITULL(jfield)) and (!arg_quick))
      update_patterns(object, your_colour, MAN_ENUM, jfield, 1);
  }

  object->B_inode--;
 
  SOFTBUG(object->B_inode < 0)

  node_t *node = object->B_nodes + object->B_inode;

  node->node_move_key = 0;

#ifdef DEBUG
  //check_my_malloc();

  hash_key_t temp_key;
  
  temp_key = return_key_from_bb(object);

  HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))

  if (!arg_quick)
    check_board_patterns_thread(object, (char *) __FUNC__, TRUE);
#endif

  END_BLOCK

  POP_NAME(__FUNC__)
}

void check_moves(board_t *object, moves_list_t *arg_moves_list)
{
  BEGIN_BLOCK(__FUNC__)
  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    do_move(object, imove, arg_moves_list, FALSE);

    hash_key_t temp_key;
  
    temp_key = return_key_from_bb(object);
    HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))

    undo_move(object, imove, arg_moves_list, FALSE);

    temp_key = return_key_from_bb(object);
    HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))
  }

  END_BLOCK
}

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
