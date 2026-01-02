//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

mcts_globals_t mcts_globals;

local my_random_t mcts_random;

local int mcts_quiescence(search_t *object, int arg_nply,
  int arg_my_alpha, int arg_my_beta, int arg_node_type,
  moves_list_t *arg_moves_list, int *arg_best_pv, int arg_all_moves)
{
  PUSH_NAME(__FUNC__)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and ((arg_my_beta - arg_my_alpha) != 1))

  colour_enum my_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

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

  int temp_mate = read_endgame(object, my_colour, &wdl);

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

    gen_moves(&(object->S_board), &my_moves_list);
  
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

  int moves_weight[NMOVES_MAX];

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
      if (!MOVE_EXTEND_IN_QUIESCENCE(arg_moves_list->ML_move_flags[jmove]))
        continue;
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
      do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

      temp_score = -mcts_quiescence(object, arg_nply,
        -arg_my_beta, -temp_alpha,
        arg_node_type, NULL, &temp_pv, FALSE);

      undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    }
    else
    {
      if (imove == 0)
      {
        do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -mcts_quiescence(object, arg_nply,
          -arg_my_beta, -temp_alpha,
          arg_node_type, NULL, &temp_pv, FALSE);

        undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -mcts_quiescence(object, arg_nply,
          -temp_beta, -temp_alpha,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv, FALSE);

        //if time-limit temp_score = -(SCORE_PLUS_INFINITY))

        if ((temp_score >= temp_beta) and (temp_score < arg_my_beta))
        {
          temp_score = -mcts_quiescence(object, arg_nply,
            -arg_my_beta, -temp_score,
            arg_node_type, NULL, &temp_pv, FALSE);

        }

        undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
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

  POP_NAME(__FUNC__)

  return(best_score);
}

local int mcts_alpha_beta(search_t *object, int arg_nply,
  int arg_my_alpha, int arg_my_beta, int arg_my_depth,
  int arg_node_type, moves_list_t *arg_moves_list, int *arg_best_pv)
{
  PUSH_NAME(__FUNC__)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_my_depth < 0)

  HARDBUG(arg_node_type == 0)

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  HARDBUG(IS_MINIMAL_WINDOW(arg_node_type) and ((arg_my_beta - arg_my_alpha) != 1))

  colour_enum my_colour;

  if (IS_WHITE(object->S_board.B_colour2move))
    my_colour = WHITE_ENUM;
  else
    my_colour = BLACK_ENUM;

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
    best_score = mcts_quiescence(object, INVALID, arg_my_alpha, arg_my_beta,
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

  int temp_mate = read_endgame(object, my_colour, &wdl);

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

    gen_moves(&(object->S_board), &my_moves_list);

    arg_moves_list = &my_moves_list;
  }

  if (arg_moves_list->ML_nmoves == 0)
  {
    best_score = SCORE_LOST_ABSOLUTE;

    goto label_return;
  }

  int moves_weight[NMOVES_MAX];

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

      do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

      temp_score = -mcts_alpha_beta(object, arg_nply,
        -arg_my_beta, -arg_my_alpha, your_depth,
        arg_node_type, NULL, &temp_pv);

      undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    }
    else
    {
      if (imove == 0)
      {
        do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -mcts_alpha_beta(object, arg_nply,
          -arg_my_beta, -temp_alpha, your_depth,
          arg_node_type, NULL, &temp_pv);

        undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        temp_score = -mcts_alpha_beta(object, arg_nply,
          -temp_beta, -temp_alpha, your_depth,
          MINIMAL_WINDOW_BIT, NULL, &temp_pv);

        if ((temp_score >= temp_beta) and (temp_score < arg_my_beta))
        {
          temp_score = -mcts_alpha_beta(object, arg_nply,
            -arg_my_beta, -temp_score, your_depth,
            arg_node_type, NULL, &temp_pv);

        }

        undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
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

  POP_NAME(__FUNC__)

  return(best_score);
}

//mcts_search returns SCORE_PLUS_INFINITY in case of time limit
//returns INVALID for best_pv in case of time limit, EGTB hit,
//draw by repetition or lost because of no moves

int mcts_search(search_t *object, int arg_root_score, int arg_nply,
  int arg_mcts_depth, moves_list_t *arg_moves_list, int *arg_best_pv, int arg_verbose)
{
  PUSH_NAME(__FUNC__)

  int best_score = SCORE_PLUS_INFINITY;
  *arg_best_pv = INVALID;

  BSTRING(bmove_string)

  //check endgame

  int wdl;

  int temp_mate = read_endgame(object, object->S_board.B_colour2move, &wdl);

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

  moves_list_t temp_moves_list;

  if (arg_moves_list == NULL)
  {
    construct_moves_list(&temp_moves_list);

    gen_moves(&(object->S_board), &temp_moves_list);

    arg_moves_list = &temp_moves_list;
  }

  if (arg_moves_list->ML_nmoves == 0)
  {
    best_score = SCORE_LOST_ABSOLUTE;

    goto label_return;
  }

  int sort[NMOVES_MAX];

  shuffle(sort, arg_moves_list->ML_nmoves, &mcts_random);

  best_score = arg_root_score;
  *arg_best_pv = sort[0];

  int depth = 1;

  int ntrouble = 0;

  while(TRUE)
  {
    int jmove = sort[0];

    int window = SCORE_MAN / 4;

    int alpha = best_score - window;
    int beta = best_score + window;

    do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

    int temp_score = SCORE_PLUS_INFINITY;
    int temp_pv = INVALID;

    temp_score = -mcts_alpha_beta(object, arg_nply + 1,
      -beta, -alpha, depth - 1, 
      PV_BIT, NULL, &temp_pv);

    if (temp_score == SCORE_MINUS_INFINITY) temp_score = SCORE_PLUS_INFINITY;

    if (temp_score == SCORE_PLUS_INFINITY)
    {
      HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

      undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);

      if (mcts_globals.mcts_globals_time_limit < 0.0)
      {
        best_score = SCORE_PLUS_INFINITY;
        *arg_best_pv = INVALID;

        goto label_return;
      }

      //return best result until now
      goto label_break;
    }

    best_score = temp_score;
    *arg_best_pv = jmove;

    if (best_score >= beta)
    {
      while(TRUE)
      {
        alpha = best_score;
        beta = best_score + window;

        temp_score = -mcts_alpha_beta(object, arg_nply + 1,
          -beta, -alpha, depth - 1, 
          PV_BIT, NULL, &temp_pv);

        if (temp_score == SCORE_MINUS_INFINITY)
          temp_score = SCORE_PLUS_INFINITY;

        if (temp_score == SCORE_PLUS_INFINITY)
        {
          HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

          undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    
          if (mcts_globals.mcts_globals_time_limit < 0.0)
          {
            best_score = SCORE_PLUS_INFINITY;
            *arg_best_pv = INVALID;
    
            goto label_return;
          }
    
          //return best result until now
          goto label_break;
        }

        best_score = temp_score;
        *arg_best_pv = jmove;

        if (best_score < beta) break;

        window *= 2;
      }
    }
    else if (best_score <= alpha)
    {
      while(TRUE)
      {
        alpha = best_score - window;
        beta = best_score;

        temp_score = -mcts_alpha_beta(object, arg_nply + 1,
          -beta, -alpha, depth - 1, 
          PV_BIT, NULL, &temp_pv);

        if (temp_score == SCORE_MINUS_INFINITY)
          temp_score = SCORE_PLUS_INFINITY;

        if (temp_score == SCORE_PLUS_INFINITY)
        {
          HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

          undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    
          if (mcts_globals.mcts_globals_time_limit < 0.0)
          {
            best_score = SCORE_PLUS_INFINITY;
            *arg_best_pv = INVALID;
    
            goto label_return;
          }
    
          //return best result until now
          goto label_break;
        }

        best_score = temp_score;
        *arg_best_pv = jmove;

        if (best_score > alpha) break;

        window *= 2;
      }
    }

    HARDBUG(*arg_best_pv != jmove)

    undo_move(&(object->S_board), *arg_best_pv, arg_moves_list, TRUE);

    if ((arg_nply == 0) and arg_verbose)
    { 
      print_board(&(object->S_board));

      move2bstring(arg_moves_list, *arg_best_pv, bmove_string);

      my_printf(object->S_my_printf, "depth=%d move=%s best_score=%d\n",
                depth, bdata(bmove_string), best_score);
    }

    for (int imove = 1; imove < arg_moves_list->ML_nmoves; imove++)
    {
      jmove = sort[imove];

      alpha = best_score;
      beta = best_score + 1;

      do_move(&(object->S_board), jmove, arg_moves_list, TRUE);

      temp_score = -mcts_alpha_beta(object, arg_nply + 1,
        -beta, -alpha, depth - 1, 
        MINIMAL_WINDOW_BIT, NULL, &temp_pv);
  
      if (temp_score == SCORE_MINUS_INFINITY) temp_score = SCORE_PLUS_INFINITY;
  
      if (temp_score == SCORE_PLUS_INFINITY)
      {
        HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

        undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
  
        if (mcts_globals.mcts_globals_time_limit < 0.0)
        {
          best_score = SCORE_PLUS_INFINITY;
          *arg_best_pv = INVALID;
  
          goto label_return;
        }
  
        //return best result until now
        goto label_break;
      }

      if (temp_score < beta)
      {
        undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);

        continue;
      }

      for (int kmove = imove; kmove > 0; kmove--)
        sort[kmove] = sort[kmove - 1];

      sort[0] = jmove;

      best_score = temp_score;
      *arg_best_pv = jmove;

      if ((arg_nply == 0) and arg_verbose)
      {
        move2bstring(arg_moves_list, *arg_best_pv, bmove_string);

        my_printf(object->S_my_printf, "depth=%d move=%s best_score>=%d\n",
                  depth, bdata(bmove_string), best_score);
      }

      window = SCORE_MAN / 4;

      while(TRUE)
      {
        alpha = best_score;
        beta = best_score + window;
  
        temp_score = -mcts_alpha_beta(object, arg_nply + 1,
          -beta, -alpha, depth - 1, 
          PV_BIT, NULL, &temp_pv);
  
        if (temp_score == SCORE_MINUS_INFINITY)
          temp_score = SCORE_PLUS_INFINITY;
  
        if (temp_score == SCORE_PLUS_INFINITY)
        {
          HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

          undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    
          if (mcts_globals.mcts_globals_time_limit < 0.0)
          {
            best_score = SCORE_PLUS_INFINITY;
            *arg_best_pv = INVALID;
    
            goto label_return;
          }
    
          //return best result until now
          goto label_break;
        }
  
        best_score = temp_score;
        *arg_best_pv = jmove;
  
        if (best_score < beta) break;
  
        window *= 2;
      }//while(TRUE)

      if ((arg_nply == 0) and arg_verbose)
      {
        move2bstring(arg_moves_list, *arg_best_pv, bmove_string);

        my_printf(object->S_my_printf, "depth=%d move=%s best_score==%d\n",
                  depth, bdata(bmove_string), best_score);
      }

      undo_move(&(object->S_board), jmove, arg_moves_list, TRUE);
    }//for (int imove

    if ((arg_nply == 0) and arg_verbose)
    {
      move2bstring(arg_moves_list, *arg_best_pv, bmove_string);

      my_printf(object->S_my_printf, "depth=%d move=%s best_score===%d\n",
                depth, bdata(bmove_string), best_score);
    }

    if (depth >= arg_mcts_depth)
    {
      if (best_score >= 0) break;

      if (ntrouble > 1) break;

      ntrouble++;
    }

    ++depth;
  }//depth

  label_break:

  if ((arg_nply == 0) and arg_verbose)
  {
    move2bstring(arg_moves_list, *arg_best_pv, bmove_string);

    my_printf(object->S_my_printf, "depth=%d move=%s best_score====%d\n",
              depth, bdata(bmove_string), best_score);
  }

  label_return:

  BDESTROY(bmove_string)

  POP_NAME(__FUNC__)

  return(best_score);
}

local int mcts_shoot_out(search_t *object, int arg_nply, int arg_mcts_depth)
{
  if (object->S_board.B_inode > 200)
  {
    print_board(&(object->S_board));

    my_printf(object->S_my_printf, "WARNING: VERY DEEP SEARCH\n");
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(object->S_board), &moves_list);

  //local time limit

  if (mcts_globals.mcts_globals_time_limit > 0.0)
    reset_my_timer(&(object->S_timer));

  int best_score = SCORE_PLUS_INFINITY;
  int best_pv = INVALID;

  best_score = mcts_search(object, 0, arg_nply, arg_mcts_depth,
    &moves_list, &best_pv, TRUE);

  if ((best_pv == INVALID) or (arg_nply >= MCTS_PLY_MAX)) goto label_return;

  HARDBUG(best_pv >= moves_list.ML_nmoves)

  do_move(&(object->S_board), best_pv, &moves_list, TRUE);

  best_score = -mcts_shoot_out(object, arg_nply + 1, arg_mcts_depth);

  undo_move(&(object->S_board), best_pv, &moves_list, TRUE);

  if (best_score == SCORE_MINUS_INFINITY) best_score = SCORE_PLUS_INFINITY;

  label_return:

  if (best_score == SCORE_PLUS_INFINITY)
  {
    HARDBUG(mcts_globals.mcts_globals_time_limit >= 0.0)
  }

  return(best_score);
}

double mcts_shoot_outs(search_t *object, int arg_nply,
  int *arg_nshoot_outs, int arg_mcts_depth, double arg_mcts_time_limit,
  int *arg_nwon, int *arg_ndraw, int *arg_nlost)
{
  options.material_only = TRUE;

  mcts_globals.mcts_globals_time_limit = arg_mcts_time_limit;

  HARDBUG(*arg_nshoot_outs < 1)

  my_timer_t mcts_timer;
  
  construct_my_timer(&mcts_timer, "mcts_timer", STDOUT, FALSE);

  reset_my_timer(&mcts_timer);

  //global time limit

  if (arg_mcts_time_limit < 0.0) reset_my_timer(&(object->S_timer));

  double result = 0.0;

  int nwon_local = 0;
  int ndraw_local = 0;
  int nlost_local = 0;

  if (arg_nwon == NULL) arg_nwon = &nwon_local;
  if (arg_ndraw == NULL) arg_ndraw = &ndraw_local;
  if (arg_nlost == NULL) arg_nlost = &nlost_local;

  for (int ishoot_out = 0; ishoot_out < *arg_nshoot_outs; ishoot_out++)
  {
    int temp_score = mcts_shoot_out(object, arg_nply, arg_mcts_depth);

    if (temp_score == SCORE_PLUS_INFINITY)
    {
      HARDBUG(arg_mcts_time_limit >= 0.0)

      my_printf(object->S_my_printf,
                "time limit in mcts_shoot_outs ishoot_out=%d!\n", ishoot_out);

      *arg_nshoot_outs = ishoot_out;

      break;
    }

    if (temp_score >= MCTS_THRESHOLD_WON)
      (*arg_nwon)++;
    else if (temp_score <= MCTS_THRESHOLD_LOST)
      (*arg_nlost)++;
    else
      (*arg_ndraw)++;
  }

  if (*arg_nshoot_outs == 0)
    my_printf(object->S_my_printf, "WARNING: *nshoot_outs == 0\n");

  if ((*arg_nwon + *arg_nlost) > 0)
    result = (double) (*arg_nwon - *arg_nlost) / (*arg_nwon + *arg_nlost);

  my_printf(object->S_my_printf, "*nshoot_outs=%d time=%.2f"
            " *nwon=%d *nlost=%d *ndraw=%d result=%.6f"
            " (%.2f shoot_outs/second)\n",
         *arg_nshoot_outs, return_my_timer(&mcts_timer, FALSE),
         *arg_nwon, *arg_nlost, *arg_ndraw, result,
         *arg_nshoot_outs / return_my_timer(&mcts_timer, FALSE));

  return(result);
}

void init_mcts(void)
{
  if (my_mpi_globals.MMG_nslaves == 1)
    construct_my_random(&mcts_random, 0);
  else
    construct_my_random(&mcts_random, INVALID);

  mcts_globals.mcts_globals_time_limit = 100000.0;
  mcts_globals.mcts_globals_nodes = 0;
}
