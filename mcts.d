//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
int my_mcts_quiescence(search_t *with, int nply, int my_alpha, int my_beta, 
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  HARDBUG(my_alpha >= my_beta)

  HARDBUG(node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and IS_PV(node_type))

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and ((my_beta - my_alpha) != 1))

  *best_pv = INVALID;

  int best_score = SCORE_PLUS_INFINITY;

  if ((mcts_globals.mcts_globals_nodes++ % 100) == 0)
  {
    if (return_my_timer(&(with->S_timer), FALSE) >=
        mcts_globals.mcts_globals_time_limit)
    {
      PRINTF("time limit in my_mcts_quiescence!\n");

      goto label_return;
    }
  }

  if (draw_by_repetition(&(with->S_board), FALSE))
  {
    best_score = 0;

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

    goto label_return;
  }

  moves_list_t my_moves_list;

  if (moves_list == NULL)
  {
    construct_moves_list(&my_moves_list);

    gen_my_moves(&(with->S_board), &my_moves_list, TRUE);
  
    moves_list = &my_moves_list;
  }

  if (moves_list->nmoves == 0)
  {
    best_score = SCORE_LOST;

    goto label_return;
  }

  int all_moves = FALSE;

  if (IS_PV(node_type))
  {
    all_moves = TRUE;
  }
  else if (moves_list->ncaptx > 0)
  {
    all_moves = TRUE;
  } 
  else if (moves_list->nmoves <= 2)
  {
    all_moves = TRUE;
  }
  else
  {
    int nextend = 0;

    for (int imove = 0; imove < moves_list->nmoves; imove++)
      if (MOVE_EXTEND_IN_QUIESCENCE(moves_list->moves_flag[imove])) nextend++;

    if (nextend == moves_list->nmoves)
    {
      all_moves = TRUE;
    }
  }

  best_score = SCORE_MINUS_INFINITY;

  if (!all_moves)
  {
    int nmy_man = BIT_COUNT(with->S_board.my_man_bb);
    int nmy_king = BIT_COUNT(with->S_board.my_king_bb);
    int nyour_man = BIT_COUNT(with->S_board.your_man_bb);
    int nyour_king = BIT_COUNT(with->S_board.your_king_bb);
            
    best_score = (nmy_man - nyour_man) * SCORE_MAN +
                 (nmy_king - nyour_king) * SCORE_KING;

    if (best_score >= my_beta) goto label_return;
  }

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < moves_list->nmoves; imove++)
    moves_weight[imove] = moves_list->moves_weight[imove] +
                          return_my_random(&mcts_random) % 10;

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
      if (!MOVE_EXTEND_IN_QUIESCENCE(moves_list->moves_flag[jmove])) continue;
    }

    int temp_alpha;
  
    if (best_score < my_alpha)
      temp_alpha = my_alpha;
    else
      temp_alpha = best_score;

    int temp_score = SCORE_PLUS_INFINITY;

    int temp_pv;

    if (IS_MINIMAL_WINDOW(node_type))
    {
      do_my_move(&(with->S_board), jmove, moves_list);

      temp_score = -your_mcts_quiescence(with, nply,
        -my_beta, -temp_alpha,
        node_type, NULL, &temp_pv);

      undo_my_move(&(with->S_board), jmove, moves_list);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(&(with->S_board), jmove, moves_list);

        temp_score = -your_mcts_quiescence(with, nply,
          -my_beta, -temp_alpha,
          node_type, NULL, &temp_pv);

        undo_my_move(&(with->S_board), jmove, moves_list);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(&(with->S_board), jmove, moves_list);

        temp_score = -your_mcts_quiescence(with, nply,
          -temp_beta, -temp_alpha,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv);

        //if time-limit temp_score = -(SCORE_PLUS_INFINITY))

        if ((temp_score >= temp_beta) and (temp_score < my_beta))
        {
          temp_score = -your_mcts_quiescence(with, nply,
            -my_beta, -temp_score,
            node_type, NULL, &temp_pv);

        }

        undo_my_move(&(with->S_board), jmove, moves_list);
      }
    }

    HARDBUG(temp_score == SCORE_PLUS_INFINITY)

    if (temp_score == SCORE_MINUS_INFINITY)
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

  HARDBUG(best_score == SCORE_MINUS_INFINITY)
   
  label_return:

  return(best_score);
}

int my_mcts_alpha_beta(search_t *with, int nply,
  int my_alpha, int my_beta, int my_depth,
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  HARDBUG(my_alpha >= my_beta)

  HARDBUG(my_depth < 0)

  HARDBUG(node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and IS_PV(node_type))

  HARDBUG(IS_MINIMAL_WINDOW(node_type) and ((my_beta - my_alpha) != 1))

  *best_pv = INVALID;

  int best_score = SCORE_PLUS_INFINITY;

  if ((mcts_globals.mcts_globals_nodes++ % 100) == 0)
  {
    if (return_my_timer(&(with->S_timer), FALSE) >=
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

  if (draw_by_repetition(&(with->S_board), FALSE))
  {
    best_score = 0;

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

    goto label_return;
  }

  moves_list_t my_moves_list;

  if (moves_list == NULL)
  {
    construct_moves_list(&my_moves_list);

    gen_my_moves(&(with->S_board), &my_moves_list, FALSE);

    moves_list = &my_moves_list;
  }

  if (moves_list->nmoves == 0)
  {
    best_score = SCORE_LOST;

    goto label_return;
  }

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < moves_list->nmoves; imove++)
    moves_weight[imove] = moves_list->moves_weight[imove] +
                          return_my_random(&mcts_random) % 10;

  best_score = SCORE_MINUS_INFINITY;

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
  
      int temp_score = SCORE_PLUS_INFINITY;

      if (IS_MINIMAL_WINDOW(node_type))
      {
        do_my_move(&(with->S_board), jmove, moves_list);

        temp_score = -your_mcts_alpha_beta(with, nply,
          -my_beta, -temp_alpha, my_depth,
          node_type, NULL, &temp_pv);

        undo_my_move(&(with->S_board), jmove, moves_list);
      }
      else
      {
        if (imove == 0)
        {
          do_my_move(&(with->S_board), jmove, moves_list);

          temp_score = -your_mcts_alpha_beta(with, nply,
            -my_beta, -temp_alpha, my_depth,
            node_type, NULL, &temp_pv);

          undo_my_move(&(with->S_board), jmove, moves_list);
        } 
        else
        {
          int temp_beta = temp_alpha + 1;

          do_my_move(&(with->S_board), jmove, moves_list);

          temp_score = -your_mcts_alpha_beta(with, nply,
            -temp_beta, -temp_alpha, my_depth,
            MINIMAL_WINDOW_BIT, NULL, &temp_pv);
  
          if ((temp_score >= temp_beta) and (temp_score < my_beta))
          {
            temp_score = -your_mcts_alpha_beta(with, nply,
              -my_beta, -temp_score, my_depth,
              node_type, NULL, &temp_pv);
          }

          undo_my_move(&(with->S_board), jmove, moves_list);
        }
      }

      HARDBUG(temp_score == SCORE_PLUS_INFINITY)

      if (temp_score == SCORE_MINUS_INFINITY)
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

    int temp_score = SCORE_PLUS_INFINITY;

    int temp_pv;

    if (IS_MINIMAL_WINDOW(node_type))
    {
      HARDBUG(temp_alpha != my_alpha)

      do_my_move(&(with->S_board), jmove, moves_list);

      temp_score = -your_mcts_alpha_beta(with, nply,
        -my_beta, -my_alpha, your_depth,
        node_type, NULL, &temp_pv);

      undo_my_move(&(with->S_board), jmove, moves_list);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(&(with->S_board), jmove, moves_list);

        temp_score = -your_mcts_alpha_beta(with, nply,
          -my_beta, -temp_alpha, your_depth,
          node_type, NULL, &temp_pv);

        undo_my_move(&(with->S_board), jmove, moves_list);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(&(with->S_board), jmove, moves_list);

        temp_score = -your_mcts_alpha_beta(with, nply,
          -temp_beta, -temp_alpha, your_depth,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv);

        if ((temp_score >= temp_beta) and (temp_score < my_beta))
        {
          temp_score = -your_mcts_alpha_beta(with, nply,
            -my_beta, -temp_score, your_depth,
            node_type, NULL, &temp_pv);

        }

        undo_my_move(&(with->S_board), jmove, moves_list);
      }
    } //IS_MINIMAL_WINDOW
  
    HARDBUG(temp_score == SCORE_PLUS_INFINITY)

    if (temp_score == SCORE_MINUS_INFINITY)
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

  HARDBUG(best_score == SCORE_MINUS_INFINITY)
   
  label_return:

  return(best_score);
}
