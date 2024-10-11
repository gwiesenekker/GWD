//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
int my_mcts_quiescence(board_t *with, int nply, int my_alpha, int my_beta, 
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  HARDBUG(my_alpha >= my_beta)

  HARDBUG(node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and IS_PV(node_type))

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and ((my_beta - my_alpha) != 1))

  *best_pv = INVALID;

  int best_score = SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR;

  if ((mcts_globals.mcts_globals_nodes++ % 100) == 0)
  {
    if (return_my_timer(&(mcts_globals.mcts_globals_timer), FALSE) >=
        mcts_globals.mcts_globals_time_limit)
    {
      PRINTF("time limit in my_mcts_quiescence!\n");

      goto label_return;
    }
  }

  if (draw_by_repetition(with, FALSE))
  {
    best_score = 0;

    best_score = return_scaled_float_score(best_score, with->board_key);

    goto label_return;
  }

  //check endgame

  int temp_mate = read_endgame(with, MY_BIT, FALSE);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    if (temp_mate == INVALID)
    {
      best_score = 0;
    }
    else if (temp_mate > 0)
    {
      best_score = SCORE_WON;
    }
    else
    {
      best_score = SCORE_LOST;
    }

    best_score = return_scaled_float_score(best_score, with->board_key);

    goto label_return;
  }

  moves_list_t my_moves_list;

  if (moves_list == NULL)
  {
    create_moves_list(&my_moves_list);

    gen_my_moves(with, &my_moves_list, TRUE);
  
    moves_list = &my_moves_list;
  }

  if (moves_list->nmoves == 0)
  {
    best_score = SCORE_LOST;

    best_score = return_scaled_float_score(best_score, with->board_key);

    goto label_return;
  }

  int all_moves = FALSE;

  if (moves_list->ncaptx > 0)
  {
    all_moves = TRUE;
  } 
  else if (moves_list->nmoves <= 2)
  {
    all_moves = TRUE;
  }
  else
  {
    int ntactical = 0;

    for (int imove = 0; imove < moves_list->nmoves; imove++)
      if (moves_list->moves_tactical[imove] != 0) ntactical++;

    if (ntactical == moves_list->nmoves)
    {
      all_moves = TRUE;
    }
    else if ((ntactical == 0) and 
             (with->board_inode > 0) and
             (with->board_nodes[with->board_inode - 1].node_move_tactical))
    {
      all_moves = TRUE;
    }
  }

  best_score = SCORE_MINUS_INFINITY * SCALED_FLOAT_FACTOR;

  if (!all_moves)
  {
/*
    int nmy_man = BIT_COUNT(with->my_man_bb);
    int nmy_crown = BIT_COUNT(with->my_crown_bb);
    int nyour_man = BIT_COUNT(with->your_man_bb);
    int nyour_crown = BIT_COUNT(with->your_crown_bb);
            
    best_score = (nmy_man - nyour_man) * SCORE_MAN +
                 (nmy_crown - nyour_crown) * SCORE_CROWN;

    best_score = return_scaled_float_score(best_score, with->board_key);
*/

    best_score = return_my_score(with);
  }

  if (best_score >= my_beta) goto label_return;

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < moves_list->nmoves; imove++)
    moves_weight[imove] = moves_list->moves_weight[imove];

  nply++;

  for (int imove = 0; imove < moves_list->nmoves; imove++)
  {
    int jmove = 0;
   
    for (int kmove = 1; kmove < moves_list->nmoves; kmove++)
    {
      if (moves_weight[kmove] == L_MIN) continue;
      
      if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
    }

    HARDBUG(moves_weight[jmove] == L_MIN)
  
    moves_weight[jmove] = L_MIN;

    if (!all_moves)
    {
      if (moves_list->moves_tactical[jmove] == 0) continue;
    }

    int temp_alpha;
  
    if (best_score < my_alpha)
      temp_alpha = my_alpha;
    else
      temp_alpha = best_score;

    int temp_score = SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR;

    int temp_pv;

    if (IS_MINIMAL_WINDOW(node_type))
    {
      do_my_move(with, jmove, moves_list);

      temp_score = -your_mcts_quiescence(with, nply,
        -my_beta, -temp_alpha,
        node_type, NULL, &temp_pv);

      undo_my_move(with, jmove, moves_list);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(with, jmove, moves_list);

        temp_score = -your_mcts_quiescence(with, nply,
          -my_beta, -temp_alpha,
          node_type, NULL, &temp_pv);

        undo_my_move(with, jmove, moves_list);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(with, jmove, moves_list);

        temp_score = -your_mcts_quiescence(with, nply,
          -temp_beta, -temp_alpha,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv);

        undo_my_move(with, jmove, moves_list);
  
        if ((temp_score >= temp_beta) and (temp_score < my_beta))
        {
          do_my_move(with, jmove, moves_list);

          temp_score = -your_mcts_quiescence(with, nply,
            -my_beta, -temp_score,
            node_type, NULL, &temp_pv);

          undo_my_move(with, jmove, moves_list);
        }
      }
    }

    HARDBUG(temp_score == (SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR))

    if (temp_score == (SCORE_MINUS_INFINITY * SCALED_FLOAT_FACTOR))
    {
      best_score = -temp_score;

      goto label_return;
    }

    if (temp_score > best_score)
    {
      best_score = temp_score;
  
      *best_pv = temp_pv;
    }
  
    if (best_score >= my_beta) break;
  } //imove

  //we always assigned best_score
  //*best_pv can be invalid, for example if (!all_moves) and
  //move scores are below alpha

  HARDBUG(best_score == (SCORE_MINUS_INFINITY * SCALED_FLOAT_FACTOR))
   
  label_return:

  return(best_score);
}

int my_mcts_alpha_beta(board_t *with, int nply,
  int my_alpha, int my_beta, int my_depth,
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  HARDBUG(my_alpha >= my_beta)

  HARDBUG(my_depth < 0)

  HARDBUG(node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and IS_PV(node_type))

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and ((my_beta - my_alpha) != 1))

  *best_pv = INVALID;

  int best_score = SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR;

  if ((mcts_globals.mcts_globals_nodes++ % 100) == 0)
  {
    if (return_my_timer(&(mcts_globals.mcts_globals_timer), FALSE) >=
        mcts_globals.mcts_globals_time_limit)
    {
      PRINTF("time limit in my_mcts_alpha_beta!\n");

      goto label_return;
    }
  }

  if ((my_depth == 0) or (nply >= MCTS_PLY_MAX))
  {
    best_score = my_mcts_quiescence(with, INVALID, my_alpha, my_beta,
      node_type, NULL, best_pv);

    goto label_return;
  }

  if (draw_by_repetition(with, FALSE))
  {
    best_score = 0;

    best_score = return_scaled_float_score(best_score, with->board_key);

    goto label_return;
  }

  //check endgame

  int temp_mate = read_endgame(with, MY_BIT, FALSE);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    if (temp_mate == INVALID)
    {
      best_score = 0;
    }
    else if (temp_mate > 0)
    {
      best_score = SCORE_WON;
    }
    else
    {
      best_score = SCORE_LOST;
    }

    best_score = return_scaled_float_score(best_score, with->board_key);

    goto label_return;
  }

  moves_list_t my_moves_list;

  if (moves_list == NULL)
  {
    create_moves_list(&my_moves_list);

    gen_my_moves(with, &my_moves_list, FALSE);

    moves_list = &my_moves_list;
  }

  if (moves_list->nmoves == 0)
  {
    best_score = SCORE_LOST;

    best_score = return_scaled_float_score(best_score, with->board_key);

    goto label_return;
  }

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < moves_list->nmoves; imove++)
    moves_weight[imove] = moves_list->moves_weight[imove];
  
  best_score = SCORE_MINUS_INFINITY * SCALED_FLOAT_FACTOR;

  nply++;

  if ((moves_list->nmoves <= 2) or
      (options.captures_are_transparent and (moves_list->ncaptx > 0)))
  {
    for (int imove = 0; imove < moves_list->nmoves; imove++)
    {
      int jmove = 0;
   
      for (int kmove = 1; kmove < moves_list->nmoves; kmove++)
      {
        if (moves_weight[kmove] == L_MIN) continue;
        
        if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
      }
  
      HARDBUG(moves_weight[jmove] == L_MIN)
  
      moves_weight[jmove] = L_MIN;

      int temp_alpha;

      int temp_pv;
  
      if (best_score < my_alpha)
        temp_alpha = my_alpha;
      else
        temp_alpha = best_score;
  
      int temp_score = SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR;

      if (IS_MINIMAL_WINDOW(node_type))
      {
        do_my_move(with, jmove, moves_list);

        temp_score = -your_mcts_alpha_beta(with, nply,
          -my_beta, -temp_alpha, my_depth,
          node_type, NULL, &temp_pv);

        undo_my_move(with, jmove, moves_list);
      }
      else
      {
        if (imove == 0)
        {
          do_my_move(with, jmove, moves_list);

          temp_score = -your_mcts_alpha_beta(with, nply,
            -my_beta, -temp_alpha, my_depth,
            node_type, NULL, &temp_pv);

          undo_my_move(with, jmove, moves_list);
        } 
        else
        {
          int temp_beta = temp_alpha + 1;

          do_my_move(with, jmove, moves_list);

          temp_score = -your_mcts_alpha_beta(with, nply,
            -temp_beta, -temp_alpha, my_depth,
            MINIMAL_WINDOW_BIT, NULL, &temp_pv);

          undo_my_move(with, jmove, moves_list);
  
          if ((temp_score >= temp_beta) and (temp_score < my_beta))
          {
            do_my_move(with, jmove, moves_list);

            temp_score = -your_mcts_alpha_beta(with, nply,
              -my_beta, -temp_score, my_depth,
              node_type, NULL, &temp_pv);

            undo_my_move(with, jmove, moves_list);
          }
        }
      }

      HARDBUG(temp_score == (SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR))

      if (temp_score == (SCORE_MINUS_INFINITY * SCALED_FLOAT_FACTOR))
      {
        best_score = -temp_score;
  
        goto label_return;
      }

      if (temp_score > best_score)
      {
        best_score = temp_score;
    
        *best_pv = jmove;
      }
    
      if (best_score >= my_beta) break;
    }
    goto label_break;
  }

  HARDBUG((moves_list->nmoves <= 2) or 
          (options.captures_are_transparent and (moves_list->ncaptx > 0))) 

  int your_depth = my_depth - 1;

  HARDBUG(your_depth < 0)

  for (int imove = 0; imove < moves_list->nmoves; imove++)
  {
    int jmove = 0;
   
    for (int kmove = 1; kmove < moves_list->nmoves; kmove++)
    {
      if (moves_weight[kmove] == L_MIN) continue;
      
      if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
    }

    HARDBUG(moves_weight[jmove] == L_MIN)
  
    moves_weight[jmove] = L_MIN;

    int temp_alpha;
  
    if (best_score < my_alpha)
      temp_alpha = my_alpha;
    else
      temp_alpha = best_score;

    int temp_score = SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR;

    int temp_pv;

    if (IS_MINIMAL_WINDOW(node_type))
    {
      HARDBUG(temp_alpha != my_alpha)

      do_my_move(with, jmove, moves_list);

      temp_score = -your_mcts_alpha_beta(with, nply,
        -my_beta, -my_alpha, your_depth,
        node_type, NULL, &temp_pv);

      undo_my_move(with, jmove, moves_list);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(with, jmove, moves_list);

        temp_score = -your_mcts_alpha_beta(with, nply,
          -my_beta, -temp_alpha, your_depth,
          node_type, NULL, &temp_pv);

        undo_my_move(with, jmove, moves_list);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(with, jmove, moves_list);

        temp_score = -your_mcts_alpha_beta(with, nply,
          -temp_beta, -temp_alpha, your_depth,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv);

        undo_my_move(with, jmove, moves_list);
  
        if ((temp_score >= temp_beta) and (temp_score < my_beta))
        {
          do_my_move(with, jmove, moves_list);

          temp_score = -your_mcts_alpha_beta(with, nply,
            -my_beta, -temp_score, your_depth,
            node_type, NULL, &temp_pv);

          undo_my_move(with, jmove, moves_list);
        }
      }
    } //IS_MINIMAL_WINDOW
  
    HARDBUG(temp_score == (SCORE_PLUS_INFINITY * SCALED_FLOAT_FACTOR))

    if (temp_score == (SCORE_MINUS_INFINITY * SCALED_FLOAT_FACTOR))
    {
      best_score = -temp_score;
  
      goto label_return;
    }

    if (temp_score > best_score)
    {
      best_score = temp_score;
  
      *best_pv = jmove;
    }
  
    if (best_score >= my_beta) goto label_break;
  } //imove

  label_break:
  
  //we always searched move 0

  HARDBUG(*best_pv == INVALID)

  HARDBUG(best_score == (SCORE_MINUS_INFINITY * SCALED_FLOAT_FACTOR))
   
  label_return:

  return(best_score);
}
