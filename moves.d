//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
void gen_my_moves(board_t *object, moves_list_t *arg_moves_list, int arg_quiescence)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(object->B_colour2move != MY_BIT)

  arg_moves_list->ML_nmoves = 0;
  arg_moves_list->ML_ncaptx = 0;
  arg_moves_list->ML_nblocked = 0;

  ui64_t my_bb = (object->my_man_bb | object->my_king_bb);
  ui64_t your_bb = (object->your_man_bb | object->your_king_bb);
  ui64_t empty_bb = object->B_valid_bb & ~(my_bb | your_bb);

  while(my_bb != 0)
  {
    int iboard1;

    if (IS_WHITE(object->B_colour2move))
      iboard1 = BIT_CTZ(my_bb);
    else
      iboard1 = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(iboard1);

    for (int i = 0; i < 4; i++)
    {
      int jboard1 = iboard1 + my_dir[i];

      if (object->my_man_bb & BITULL(iboard1))
      {
        if ((i < 2) and (arg_moves_list->ML_ncaptx == 0))
        {
          if (empty_bb & BITULL(jboard1))
          {
            SOFTBUG(arg_moves_list->ML_nmoves >= MOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;
            
            move->M_from = iboard1;
            move->M_move_to = jboard1;
            move->M_captures_bb = 0;
       
            arg_moves_list->ML_nmoves++;
          }  
          else if (inverse_map[jboard1] != INVALID)
          {
            arg_moves_list->ML_nblocked++;
          }
        }
      }
      else
      {
        while (empty_bb & BITULL(jboard1))
        {
          if (arg_moves_list->ML_ncaptx == 0)
          {
            SOFTBUG(arg_moves_list->ML_nmoves >= MOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;
            
            move->M_from = iboard1;
            move->M_move_to = jboard1;
            move->M_captures_bb = 0;
       
            arg_moves_list->ML_nmoves++;
          }
          jboard1 += my_dir[i];
        }
      }
      if (your_bb & BITULL(jboard1))
      {
        int kboard1 = jboard1 + my_dir[i];

        if (empty_bb & BITULL(kboard1))
        {  
          int idir[NPIECES_MAX + 1];
          int jdir[NPIECES_MAX + 1];
          int iboard[NPIECES_MAX + 1];
          int jboard[NPIECES_MAX + 1];

          jboard[0] = 0;

          //you may pass the square you started on again..

          empty_bb |= BITULL(iboard1);

          idir[1] = my_dir[i];

          int ncapt = 0;

          ui64_t captures_bb = 0;

          label_next_capt:

          ++ncapt;

          captures_bb |= BITULL(jboard1);

          jboard[ncapt] = jboard1;
          iboard[ncapt] = kboard1;

          label_king:

          if (ncapt > arg_moves_list->ML_ncaptx)
          {
            arg_moves_list->ML_ncaptx = ncapt;

            arg_moves_list->ML_nmoves = 0;
            arg_moves_list->ML_nblocked = 0;
          }
          if (ncapt == arg_moves_list->ML_ncaptx)
          {
if (arg_moves_list->ML_nmoves >= MOVES_MAX)
{
print_board(object);
}
            SOFTBUG(arg_moves_list->ML_nmoves >= MOVES_MAX)

            move_t *move = arg_moves_list->ML_moves + arg_moves_list->ML_nmoves;
            
            move->M_from = iboard1;
            move->M_move_to = kboard1;
            move->M_captures_bb = captures_bb;
       
            arg_moves_list->ML_nmoves++;
          }
          if (idir[ncapt] > 0)
            jdir[ncapt] = 11 - idir[ncapt];
          else
            jdir[ncapt] = 11 + idir[ncapt];

          jboard1 = iboard[ncapt] + jdir[ncapt];

          if (object->my_king_bb & BITULL(iboard1))
            while (empty_bb & BITULL(jboard1)) jboard1 += jdir[ncapt];

          //..but you may not capture the same piece twice!
          if ((your_bb & BITULL(jboard1)) and
              !(captures_bb & BITULL(jboard1)))
          {
            kboard1 = jboard1 + jdir[ncapt];

            if (empty_bb & BITULL(kboard1))
            {
              idir[ncapt + 1] = jdir[ncapt];

              goto label_next_capt;
            }
          }

          label_neg_dir:

          jdir[ncapt] = -jdir[ncapt];

          jboard1 = iboard[ncapt] + jdir[ncapt];

          if (object->my_king_bb & BITULL(iboard1))
            while (empty_bb & BITULL(jboard1)) jboard1 += jdir[ncapt];

          if ((your_bb & BITULL(jboard1)) and
              !(captures_bb & BITULL(jboard1)))
          {
            kboard1 = jboard1 + jdir[ncapt];

            if (empty_bb & BITULL(kboard1))
            {
              idir[ncapt + 1] = jdir[ncapt];

              goto label_next_capt;
            }
          }

          label_dir:

          jdir[ncapt] = 0;

          jboard1 = iboard[ncapt] + idir[ncapt];

          if ((object->my_king_bb & BITULL(iboard1)) and
              (empty_bb & BITULL(jboard1)))
          {
            kboard1 = iboard[ncapt] = jboard1;

            goto label_king;
          }

          if ((your_bb & BITULL(jboard1)) and
              !(captures_bb & BITULL(jboard1)))
          {
            kboard1 = jboard1 + idir[ncapt];

            if (empty_bb & BITULL(kboard1))
            {
              idir[ncapt + 1] = idir[ncapt];

              goto label_next_capt;
            }
          }

          label_undo_capt:

          captures_bb &= ~BITULL(jboard[ncapt]);

          --ncapt;

          if (ncapt > 0)
          {
            if (jdir[ncapt] > 0) goto label_neg_dir;

            if (jdir[ncapt] < 0) goto label_dir;

            goto label_undo_capt;
          }
          empty_bb &= ~BITULL(iboard1);
        }
      }
    }
  }

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {   
    move_t *move = arg_moves_list->ML_moves + imove;
     
    int iboard = move->M_from;
    int kboard = move->M_move_to;

    int move_score_delta = 0;

    ui64_t captures_bb = move->M_captures_bb;

    while(captures_bb != 0)
    {
      int jboard = BIT_CTZ(captures_bb);

      if (object->your_man_bb & BITULL(jboard))
      {
        move_score_delta += SCORE_MAN;
      }
      else
      {
        move_score_delta += SCORE_KING;
      }

      captures_bb &= ~BITULL(jboard);
    }

    int move_weight = move_score_delta;

    int move_flag = 0;

    if (object->my_man_bb & BITULL(iboard))
    {
      if (my_row[kboard] == 8)
      {
        move_weight = move_score_delta + 10;
      }
      else if (my_row[kboard] == 9)
      {
        move_weight = move_score_delta + 20;

        move_flag = MOVE_DO_NOT_REDUCE_BIT | MOVE_EXTEND_IN_QUIESCENCE_BIT;
      }
      else if (my_row[kboard] == 10)
      {
        move_score_delta += SCORE_KING - SCORE_MAN;

        move_weight = move_score_delta + 30;

        move_flag = MOVE_DO_NOT_REDUCE_BIT | MOVE_EXTEND_IN_QUIESCENCE_BIT;
      }
    }

    arg_moves_list->ML_move_weights[imove] = move_weight;

    arg_moves_list->ML_move_flags[imove] = move_flag;
  }

  object->B_nodes[object->B_inode].node_ncaptx = arg_moves_list->ML_ncaptx;

  //PRINTF("use case ntempo=%d\n", moves_list->ntempo);

  END_BLOCK
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
    int iboard;

    if (IS_WHITE(object->B_colour2move))
      iboard = BIT_CTZ(my_bb);
    else
      iboard = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(iboard);

    if (object->my_man_bb & BITULL(iboard))
    {    
      if ((your_bb & BITULL(iboard - 6)) and
          (empty_bb & BITULL(iboard - 6 - 6)))
      {
        goto label_return;
      }
      if ((your_bb & BITULL(iboard - 5)) and
          (empty_bb & BITULL(iboard - 5 - 5)))
      {
        goto label_return;
      }
      if ((your_bb & BITULL(iboard + 5)) and
          (empty_bb & BITULL(iboard + 5 + 5)))
      {
        goto label_return;
      }
      if ((your_bb & BITULL(iboard + 6)) and
          (empty_bb & BITULL(iboard + 6 + 6)))
      {
        goto label_return;
      }
    }

    if (!arg_kings) continue;
   
    if (object->my_king_bb & BITULL(iboard))
    {    
      int jboard = iboard - 6;

      while(empty_bb & BITULL(jboard)) jboard -= 6;

      if ((your_bb & BITULL(jboard)) and
          (empty_bb & BITULL(jboard - 6)))
      {
        goto label_return;
      }

      jboard = iboard - 5;

      while(empty_bb & BITULL(jboard)) jboard -= 5;

      if ((your_bb & BITULL(jboard)) and
          (empty_bb & BITULL(jboard - 5)))
      {
        goto label_return;
      }

      jboard = iboard + 5;

      while(empty_bb & BITULL(jboard)) jboard += 5;

      if ((your_bb & BITULL(jboard)) and
          (empty_bb & BITULL(jboard + 5)))
      {
        goto label_return;
      }

      jboard = iboard + 6;

      while(empty_bb & BITULL(jboard)) jboard += 6;

      if ((your_bb & BITULL(jboard)) and
          (empty_bb & BITULL(jboard + 6)))
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
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(object->B_colour2move != MY_BIT)

  HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

  if (options.material_only) arg_quick = TRUE;

  push_board_state(object);

  node_t *node = object->B_nodes + object->B_inode;

  ui64_t move_key = 0;

  if (!arg_quick)
  {
    push_pattern_mask_state(object->B_pattern_mask);
    push_network_state(&(object->B_network));
  }
 
  if (arg_moves_list == NULL) goto label_null;

  move_t *move = arg_moves_list->ML_moves + arg_imove;

  int iboard = move->M_from;
  int kboard = move->M_move_to;

  if (iboard != kboard)
  {
    if (object->my_man_bb & BITULL(iboard))
    {
      move_key = my_man_key[iboard];

      XOR_HASH_KEY(object->B_key, my_man_key[iboard])


      if (!arg_quick)
        update_patterns_and_layer0(object, MY_BIT, iboard,  -1);

      object->my_man_bb &= ~BITULL(iboard);

      if (my_row[kboard] == 10)
        object->my_king_bb |= BITULL(kboard);
      else
        object->my_man_bb |= BITULL(kboard);
    }
    else
    {
      move_key = my_king_key[iboard];

      XOR_HASH_KEY(object->B_key, my_king_key[iboard]);

      object->my_king_bb &= ~BITULL(iboard);

      object->my_king_bb |= BITULL(kboard);
    }
  
    if (object->my_man_bb & BITULL(kboard))
    {
      move_key ^= my_man_key[kboard];

      XOR_HASH_KEY(object->B_key, my_man_key[kboard])

      if (!arg_quick)
        update_patterns_and_layer0(object, MY_BIT, kboard, 1);
    }
    else
    {
      move_key ^= my_king_key[kboard];

      XOR_HASH_KEY(object->B_key, my_king_key[kboard])
    }
  }

  ui64_t captures_bb = move->M_captures_bb;

  while(captures_bb != 0)
  {
    int jboard = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jboard);

    if (object->your_man_bb & BITULL(jboard))
    {
      move_key ^= your_man_key[jboard];

      XOR_HASH_KEY(object->B_key, your_man_key[jboard])

      if (!arg_quick)
        update_patterns_and_layer0(object, YOUR_BIT, jboard, -1);

      object->your_man_bb &= ~BITULL(jboard);
    }
    else
    {
      move_key ^= your_king_key[jboard];

      XOR_HASH_KEY(object->B_key, your_king_key[jboard])

      object->your_king_bb &= ~BITULL(jboard);
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
  hash_key_t temp_key;
  
  temp_key = return_key_from_bb(object);

  HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))

  if (!arg_quick)
  {
    check_board_patterns(object, (char *) __FUNC__);

    //check fails due to roundoff accumulation?
 
    check_layer0(&(object->B_network));
  }
#endif

  END_BLOCK
}

void undo_my_move(board_t *object, int arg_imove, moves_list_t *arg_moves_list,
  int arg_quick)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(object->B_colour2move != YOUR_BIT)

  HARDBUG(arg_imove >= arg_moves_list->ML_nmoves);

  if (options.material_only) arg_quick = TRUE;

  pop_board_state(object);

  object->B_inode--;
 
  SOFTBUG(object->B_inode < 0)

  node_t *node = object->B_nodes + object->B_inode;

  node->node_move_key = 0;

  object->B_colour2move = MY_BIT;

  if (!arg_quick)
  {
    pop_pattern_mask_state(object->B_pattern_mask);
    pop_network_state(&(object->B_network));
  }

#ifdef DEBUG
  hash_key_t temp_key;
  
  temp_key = return_key_from_bb(object);

  HARDBUG(!HASH_KEY_EQ(temp_key, object->B_key))

  if (!arg_quick)
  {
    check_board_patterns(object, (char *) __FUNC__);

    //check fails due to roundoff accumulation?

    check_layer0(&(object->B_network));
  }

#endif

  END_BLOCK
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

