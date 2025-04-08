//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
int my_mcts_quiescence(search_t *object, int arg_nply, int arg_my_alpha, int arg_my_beta, 
  int arg_node_type, moves_list_t *arg_moves_list, int *arg_best_pv, int arg_all_moves)
{
  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and ((arg_my_beta - arg_my_alpha) != 1))

  *arg_best_pv = INVALID;

  int best_score = SCORE_PLUS_INFINITY;

  if (mcts_globals.mcts_globals_time_limit != 0.0)
  {
    if ((mcts_globals.mcts_globals_nodes++ % 100) == 0)
    {
      if (return_my_timer(&(object->S_timer), FALSE) >=
          fabs(mcts_globals.mcts_globals_time_limit)) goto label_return;
    }
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    best_score = 0;

    goto label_return;
  }

  //check endgame

  int wdl;

  int temp_mate = read_endgame(object, MY_BIT, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    if (temp_mate == INVALID)
    {
      best_score = 0;
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        best_score = SCORE_WON;
      else
        best_score = SCORE_WON_ABSOLUTE;
    }
    else
    {
      if (wdl)
        best_score = SCORE_LOST;
      else
        best_score = SCORE_LOST_ABSOLUTE;
    }

    goto label_return;
  }

  moves_list_t my_moves_list;

  if (arg_moves_list == NULL)
  {
    construct_moves_list(&my_moves_list);

    gen_my_moves(&(object->S_board), &my_moves_list, TRUE);
  
    arg_moves_list = &my_moves_list;
  }

  if (arg_moves_list->ML_nmoves == 0)
  {
    best_score = SCORE_LOST_ABSOLUTE;

    goto label_return;
  }

  if (!arg_all_moves)
  {
    if (arg_moves_list->ML_ncaptx > 0)
    {
      arg_all_moves = TRUE;
    } 
    else if (arg_moves_list->ML_nmoves <= 2)
    {
      arg_all_moves = TRUE;
    }
    else
    {
      int nextend = 0;
  
      for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
      {
        if (MOVE_EXTEND_IN_QUIESCENCE(arg_moves_list->ML_move_flags[imove]))
          nextend++;
      }

      if (nextend == arg_moves_list->ML_nmoves)
      {
        arg_all_moves = TRUE;
      }
    }
  }

  best_score = SCORE_MINUS_INFINITY;

  if (!arg_all_moves)
  {
    best_score = return_material_score(&(object->S_board));

    if (best_score >= arg_my_beta) goto label_return;
  }

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    moves_weight[imove] = arg_moves_list->ML_move_weights[imove];

  arg_nply++;

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = 0;
   
    for (int kmove = 1; kmove < arg_moves_list->ML_nmoves; kmove++)
    {
      if (moves_weight[kmove] == L_MIN) continue;
      
      if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
    }

    HARDBUG(moves_weight[jmove] == L_MIN)
  
    moves_weight[jmove] = L_MIN;

    if (!arg_all_moves)
    {
      if (!MOVE_EXTEND_IN_QUIESCENCE(arg_moves_list->ML_move_flags[jmove])) continue;
    }

    int temp_alpha;
  
    if (best_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = best_score;

    int temp_score = SCORE_PLUS_INFINITY;

    int temp_pv;

    if (IS_MINIMAL_WINDOW(arg_node_type))
    {
      do_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);

      temp_score = -your_mcts_quiescence(object, arg_nply,
        -arg_my_beta, -temp_alpha,
        arg_node_type, NULL, &temp_pv, FALSE);

      undo_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -your_mcts_quiescence(object, arg_nply,
          -arg_my_beta, -temp_alpha,
          arg_node_type, NULL, &temp_pv, FALSE);

        undo_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -your_mcts_quiescence(object, arg_nply,
          -temp_beta, -temp_alpha,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv, FALSE);

        //if time-limit temp_score = -(SCORE_PLUS_INFINITY))

        if ((temp_score >= temp_beta) and (temp_score < arg_my_beta))
        {
          temp_score = -your_mcts_quiescence(object, arg_nply,
            -arg_my_beta, -temp_score,
            arg_node_type, NULL, &temp_pv, FALSE);

        }

        undo_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);
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
  
      *arg_best_pv = jmove;
    }
  
    if (best_score >= arg_my_beta) break;
  } //imove

  //we always assigned best_score
  //*best_pv can be invalid, for example if (!all_moves) and
  //move scores are below alpha

  HARDBUG(best_score == SCORE_MINUS_INFINITY)
   
  label_return:

  return(best_score);
}

int my_mcts_alpha_beta(search_t *object, int arg_nply,
  int arg_my_alpha, int arg_my_beta, int arg_my_depth,
  int arg_node_type, moves_list_t *arg_moves_list, int *arg_best_pv)
{
  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 0)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and ((arg_my_beta - arg_my_alpha) != 1))

  *arg_best_pv = INVALID;

  int best_score = SCORE_PLUS_INFINITY;

  if (mcts_globals.mcts_globals_time_limit != 0.0)
  {
    if ((mcts_globals.mcts_globals_nodes++ % 100) == 0)
    {
      if (return_my_timer(&(object->S_timer), FALSE) >=
          fabs(mcts_globals.mcts_globals_time_limit)) goto label_return;
    }
  }

  if ((arg_my_depth == 0) or (arg_nply >= MCTS_PLY_MAX))
  {
    best_score = my_mcts_quiescence(object, INVALID, arg_my_alpha, arg_my_beta,
      arg_node_type, NULL, arg_best_pv, FALSE);

    goto label_return;
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    best_score = 0;

    goto label_return;
  }

  //check endgame

  int wdl;

  int temp_mate = read_endgame(object, MY_BIT, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    if (temp_mate == INVALID)
    {
      best_score = 0;
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        best_score = SCORE_WON;
      else
        best_score = SCORE_WON_ABSOLUTE;
    }
    else
    {
      if (wdl)
        best_score = SCORE_LOST;
      else
        best_score = SCORE_LOST_ABSOLUTE;
    }

    goto label_return;
  }

  moves_list_t my_moves_list;

  if (arg_moves_list == NULL)
  {
    construct_moves_list(&my_moves_list);

    gen_my_moves(&(object->S_board), &my_moves_list, FALSE);

    arg_moves_list = &my_moves_list;
  }

  if (arg_moves_list->ML_nmoves == 0)
  {
    best_score = SCORE_LOST_ABSOLUTE;

    goto label_return;
  }

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    moves_weight[imove] = arg_moves_list->ML_move_weights[imove];

  best_score = SCORE_MINUS_INFINITY;

  arg_nply++;

  int your_depth = arg_my_depth;

  if (arg_moves_list->ML_ncaptx == 0) your_depth = arg_my_depth - 1;

  HARDBUG(your_depth < 0)

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = 0;
   
    for (int kmove = 1; kmove < arg_moves_list->ML_nmoves; kmove++)
    {
      if (moves_weight[kmove] == L_MIN) continue;
      
      if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
    }

    HARDBUG(moves_weight[jmove] == L_MIN)
  
    moves_weight[jmove] = L_MIN;

    int temp_alpha;
  
    if (best_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = best_score;

    int temp_score = SCORE_PLUS_INFINITY;

    int temp_pv;

    if (IS_MINIMAL_WINDOW(arg_node_type))
    {
      HARDBUG(temp_alpha != arg_my_alpha)

      do_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);

      temp_score = -your_mcts_alpha_beta(object, arg_nply,
        -arg_my_beta, -arg_my_alpha, your_depth,
        arg_node_type, NULL, &temp_pv);

      undo_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -your_mcts_alpha_beta(object, arg_nply,
          -arg_my_beta, -temp_alpha, your_depth,
          arg_node_type, NULL, &temp_pv);

        undo_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -your_mcts_alpha_beta(object, arg_nply,
          -temp_beta, -temp_alpha, your_depth,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv);

        if ((temp_score >= temp_beta) and (temp_score < arg_my_beta))
        {
          temp_score = -your_mcts_alpha_beta(object, arg_nply,
            -arg_my_beta, -temp_score, your_depth,
            arg_node_type, NULL, &temp_pv);

        }

        undo_my_move(&(object->S_board), jmove, arg_moves_list, TRUE);
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
  
      *arg_best_pv = jmove;
    }
  
    if (best_score >= arg_my_beta) goto label_break;
  } //imove

  label_break:
  
  //we always searched move 0

  HARDBUG(*arg_best_pv == INVALID)

  HARDBUG(best_score == SCORE_MINUS_INFINITY)
   
  label_return:

  return(best_score);
}
