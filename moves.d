//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
void gen_my_moves(board_t *with, moves_list_t *moves_list, int quiescence)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(with->board_colour2move != MY_BIT)

  moves_list->nmoves = 0;
  moves_list->nblocked = 0;
  moves_list->ncaptx = 0;

  ui64_t my_bb = (with->my_man_bb | with->my_king_bb);
  ui64_t your_bb = (with->your_man_bb | with->your_king_bb);
  ui64_t empty_bb = with->board_empty_bb & ~(my_bb | your_bb);

  ui64_t my_bb_blocked = my_bb;
  ui64_t empty_bb_blocked = empty_bb;

  while(my_bb != 0)
  {
    int iboard1;

    if (IS_WHITE(with->board_colour2move))
      iboard1 = BIT_CTZ(my_bb);
    else
      iboard1 = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(iboard1);

    for (int i = 0; i < 4; i++)
    {
      int jboard1 = iboard1 + my_dir[i];

      if (with->my_man_bb & BITULL(iboard1))
      {
        if ((i < 2) and (moves_list->ncaptx == 0))
        {
          if (empty_bb & BITULL(jboard1))
          {
            SOFTBUG(moves_list->nmoves >= MOVES_MAX)

            move_t *move = moves_list->moves + moves_list->nmoves;
            
            move->move_from = iboard1;
            move->move_to = jboard1;
            move->move_captures_bb = 0;
       
            moves_list->nmoves++;
          }
          else if (my_bb_blocked & BITULL(jboard1))
          {   
            moves_list->nblocked++;
          }
          else if ((your_bb & BITULL(jboard1)) and
                   !(empty_bb_blocked & BITULL(jboard1 + my_dir[i])))
          {   
            moves_list->nblocked++;
          }
        }
      }
      else
      {
        while (empty_bb & BITULL(jboard1))
        {
          if (moves_list->ncaptx == 0)
          {
            SOFTBUG(moves_list->nmoves >= MOVES_MAX)

            move_t *move = moves_list->moves + moves_list->nmoves;
            
            move->move_from = iboard1;
            move->move_to = jboard1;
            move->move_captures_bb = 0;
       
            moves_list->nmoves++;
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

          if (ncapt > moves_list->ncaptx)
          {
            moves_list->ncaptx = ncapt;

            moves_list->nmoves = 0;
            moves_list->nblocked = 0;
          }
          if (ncapt == moves_list->ncaptx)
          {
            SOFTBUG(moves_list->nmoves >= MOVES_MAX)

            move_t *move = moves_list->moves + moves_list->nmoves;
            
            move->move_from = iboard1;
            move->move_to = kboard1;
            move->move_captures_bb = captures_bb;
       
            moves_list->nmoves++;
          }
          if (idir[ncapt] > 0)
            jdir[ncapt] = 11 - idir[ncapt];
          else
            jdir[ncapt] = 11 + idir[ncapt];

          jboard1 = iboard[ncapt] + jdir[ncapt];

          if (with->my_king_bb & BITULL(iboard1))
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

          if (with->my_king_bb & BITULL(iboard1))
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

          if ((with->my_king_bb & BITULL(iboard1)) and
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

  for (int imove = 0; imove < moves_list->nmoves; imove++)
  {   
    move_t *move = moves_list->moves + imove;
     
    int iboard = move->move_from;
    int kboard = move->move_to;

    int move_score_delta = 0;

    ui64_t captures_bb = move->move_captures_bb;

    while(captures_bb != 0)
    {
      int jboard = BIT_CTZ(captures_bb);

      if (with->your_man_bb & BITULL(jboard))
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

    if (with->my_man_bb & BITULL(iboard))
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

    moves_list->moves_weight[imove] = move_weight;

    moves_list->moves_flag[imove] = move_flag;
  }

  with->board_nodes[with->board_inode].node_ncaptx = moves_list->ncaptx;

  END_BLOCK
}

int i_can_capture(board_t *with, int kings)
{
  int result;

  BEGIN_BLOCK(__FUNC__)

  result = TRUE;

  ui64_t my_bb = (with->my_man_bb | with->my_king_bb);
  ui64_t your_bb = (with->your_man_bb | with->your_king_bb);
  ui64_t empty_bb = with->board_empty_bb & ~(my_bb | your_bb);

  while(my_bb != 0)
  {
    int iboard;

    if (IS_WHITE(with->board_colour2move))
      iboard = BIT_CTZ(my_bb);
    else
      iboard = 63 - BIT_CLZ(my_bb);

    my_bb &= ~BITULL(iboard);

    if (with->my_man_bb & BITULL(iboard))
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

    if (!kings) continue;
   
    if (with->my_king_bb & BITULL(iboard))
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

void do_my_move(board_t *with, int imove, moves_list_t *moves_list)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(with->board_colour2move != MY_BIT)

  push_board_state(with);

  node_t *node = with->board_nodes + with->board_inode;

  ui64_t move_key = 0;

  if (!options.material_only)
    push_network_state(&(with->board_network));
 
  if (moves_list == NULL) goto label_null;

  move_t *move = moves_list->moves + imove;

  int iboard = move->move_from;
  int kboard = move->move_to;

  if (iboard != kboard)
  {
    if (with->my_man_bb & BITULL(iboard))
    {
      move_key = my_man_key[iboard];

      XOR_HASH_KEY(with->board_key, my_man_key[iboard])


      if (!options.material_only)
        update_layer0(&(with->board_network),
                      with->board_network.my_man_input_map[iboard], -1);

      with->my_man_bb &= ~BITULL(iboard);

      if (my_row[kboard] == 10)
        with->my_king_bb |= BITULL(kboard);
      else
        with->my_man_bb |= BITULL(kboard);
    }
    else
    {
      move_key = my_king_key[iboard];

      XOR_HASH_KEY(with->board_key, my_king_key[iboard]);

      with->my_king_bb &= ~BITULL(iboard);

      with->my_king_bb |= BITULL(kboard);
    }
  
    if (with->my_man_bb & BITULL(kboard))
    {
      move_key ^= my_man_key[kboard];

      XOR_HASH_KEY(with->board_key, my_man_key[kboard])

      if (!options.material_only)
        update_layer0(&(with->board_network),
                      with->board_network.my_man_input_map[kboard], 1);
    }
    else
    {
      move_key ^= my_king_key[kboard];

      XOR_HASH_KEY(with->board_key, my_king_key[kboard])
    }

    if (!options.material_only)
    {
      update_layer0(&(with->board_network),
                    with->board_network.empty_input_map[iboard], 1);

      update_layer0(&(with->board_network),
                    with->board_network.empty_input_map[kboard], -1);
    }
  }

  ui64_t captures_bb = move->move_captures_bb;

  while(captures_bb != 0)
  {
    int jboard = BIT_CTZ(captures_bb);

    captures_bb &= ~BITULL(jboard);

    if (with->your_man_bb & BITULL(jboard))
    {
      move_key ^= your_man_key[jboard];

      XOR_HASH_KEY(with->board_key, your_man_key[jboard])

      if (!options.material_only)
        update_layer0(&(with->board_network),
                      with->board_network.your_man_input_map[jboard], -1); 

      with->your_man_bb &= ~BITULL(jboard);
    }
    else
    {
      move_key ^= your_king_key[jboard];

      XOR_HASH_KEY(with->board_key, your_king_key[jboard])

      with->your_king_bb &= ~BITULL(jboard);
    }

    if (!options.material_only)
      update_layer0(&(with->board_network),
                    with->board_network.empty_input_map[jboard], 1);
  }

  label_null:

  node->node_move_key = move_key;

  with->board_inode++;

  SOFTBUG(with->board_inode >= NODE_MAX)

  node = with->board_nodes + with->board_inode;

  node->node_ncaptx = INVALID;
  node->node_move_key = 0;

  with->board_colour2move = YOUR_BIT;

#ifdef DEBUG
  hash_key_t temp_key;
  
  temp_key = return_key_from_bb(with);

  HARDBUG(!HASH_KEY_EQ(temp_key, with->board_key))

  if (!options.material_only)
  {
    check_inputs_against_board(with, &(with->board_network));

    //check fails due to roundoff accumulation?
 
    check_layer0(&(with->board_network));
  }
#endif

  END_BLOCK
}

void undo_my_move(board_t *with, int imove, moves_list_t *moves_list)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(with->board_colour2move != YOUR_BIT)

  pop_board_state(with);

  with->board_inode--;
 
  SOFTBUG(with->board_inode < 0)

  node_t *node = with->board_nodes + with->board_inode;

  node->node_move_key = 0;

  with->board_colour2move = MY_BIT;

  if (!options.material_only)
    pop_network_state(&(with->board_network));

#ifdef DEBUG
  hash_key_t temp_key;
  
  temp_key = return_key_from_bb(with);

  HARDBUG(!HASH_KEY_EQ(temp_key, with->board_key))

  if (!options.material_only)
  {
    check_inputs_against_board(with, &(with->board_network));

    //check fails due to roundoff accumulation?

    check_layer0(&(with->board_network));
  }

#endif

  END_BLOCK
}

void check_my_moves(board_t *with, moves_list_t *moves_list)
{
  BEGIN_BLOCK(__FUNC__)
  for (int imove = 0; imove < moves_list->nmoves; imove++)
  {
    do_my_move(with, imove, moves_list);

    hash_key_t temp_key;
  
    temp_key = return_key_from_bb(with);
    HARDBUG(!HASH_KEY_EQ(temp_key, with->board_key))

    undo_my_move(with, imove, moves_list);

    temp_key = return_key_from_bb(with);
    HARDBUG(!HASH_KEY_EQ(temp_key, with->board_key))
  }

  END_BLOCK
}

