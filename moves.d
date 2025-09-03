//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
void gen_my_moves(board_t *object, moves_list_t *arg_moves_list, int arg_quiescence)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(object->B_colour2move != MY_BIT)

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

int i_can_capture(board_t *object, int arg_kings)
{
  int result;

  BEGIN_BLOCK(__FUNC__)

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

void do_my_move(board_t *object, int arg_imove, moves_list_t *arg_moves_list,
  int arg_quick)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(object->B_colour2move != MY_BIT)

  HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

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
        update_patterns(object, MY_BIT | MAN_BIT, ifield,  -1);

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
        update_patterns(object, MY_BIT | MAN_BIT, kfield, 1);
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
        update_patterns(object, YOUR_BIT | MAN_BIT, jfield, -1);

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

  object->B_colour2move = YOUR_BIT;

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

void undo_my_move(board_t *object, int arg_imove, moves_list_t *arg_moves_list,
  int arg_quick)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(object->B_colour2move != YOUR_BIT)

  HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

  if (options.material_only) arg_quick = TRUE;

  move_t *move = arg_moves_list->ML_moves + arg_imove;

  int ifield = move->M_from;
  int kfield = move->M_move_to;

  if (ifield != kfield)
  {
    if ((object->my_man_bb & BITULL(kfield)) and (!arg_quick))
      update_patterns(object, MY_BIT | MAN_BIT, kfield, -1);
  }

  //now we can restore the board

  pop_board_state(object);

  if (ifield != kfield)
  {
    if ((object->my_man_bb & BITULL(ifield)) and (!arg_quick))
      update_patterns(object, MY_BIT | MAN_BIT, ifield,  1);
  }

  ui64_t captures_bb = move->M_captures_bb;

  while(captures_bb != 0)
  {
    int jfield = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jfield);

    if ((object->your_man_bb & BITULL(jfield)) and (!arg_quick))
      update_patterns(object, YOUR_BIT | MAN_BIT, jfield, 1);
  }

  object->B_inode--;
 
  SOFTBUG(object->B_inode < 0)

  node_t *node = object->B_nodes + object->B_inode;

  node->node_move_key = 0;

  object->B_colour2move = MY_BIT;

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

void check_my_moves(board_t *object, moves_list_t *arg_moves_list)
{
  BEGIN_BLOCK(__FUNC__)
  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    do_my_move(object, imove, arg_moves_list, FALSE);

    hash_key_t temp_key;
  
    temp_key = return_key_from_bb(object);
    HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))

    undo_my_move(object, imove, arg_moves_list, FALSE);

    temp_key = return_key_from_bb(object);
    HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))
  }

  END_BLOCK
}

