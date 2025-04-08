#include "globals.h"
//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
local void my_endgame_pv(search_t *object, int arg_mate, bstring arg_bpv_string)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_my_moves(&(object->S_board), &moves_list, FALSE);

  if (moves_list.ML_nmoves == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      HARDBUG(bcatcstr(arg_bpv_string, " #") == BSTR_ERR)
    }

    return;
  }
  if (arg_mate == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      HARDBUG(bcatcstr(arg_bpv_string, " mate==0?") == BSTR_ERR)
    }

    return;
  }
  
  int ibest_move = INVALID;  
  int best_mate = ENDGAME_UNKNOWN;

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
  {
    do_my_move(&(object->S_board), imove, &moves_list, FALSE);

    int your_mate = read_endgame(object, YOUR_BIT, NULL);

    undo_my_move(&(object->S_board), imove, &moves_list, FALSE);

    //may be this endgame is not loaded
    if (your_mate == ENDGAME_UNKNOWN)
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
      }
      else
      {
        HARDBUG(bcatcstr(arg_bpv_string, " not loaded") == BSTR_ERR)
      }

      return;
    }

    if (your_mate == INVALID)
    {
      if (best_mate == ENDGAME_UNKNOWN)
      {
        best_mate = INVALID;
        ibest_move = imove;
      }
      else if (best_mate <= 0)
      {
        best_mate = INVALID;
        ibest_move = imove;
      }
    }
    else //(your_mate != INVALID)
    {
      int temp_mate;

      if (your_mate > 0)
        temp_mate = -your_mate - 1;
      else
        temp_mate = -your_mate + 1;
      if (best_mate == ENDGAME_UNKNOWN)
      {
        best_mate = temp_mate;
        ibest_move = imove;
      }
      else if (best_mate == INVALID)
      {
        if (temp_mate > 0)
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
      }
      else if (best_mate > 0)
      {
        if ((temp_mate > 0) and (temp_mate < best_mate))
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
      }
      else //(best_mate <= 0)
      {
        if (temp_mate > 0)  
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
        else if (temp_mate < best_mate)
        {
          best_mate = temp_mate;
          ibest_move = imove;
        }
      }
    }
  }
  SOFTBUG(ibest_move == INVALID)

  SOFTBUG(best_mate == ENDGAME_UNKNOWN)

  if (best_mate != arg_mate)
  {
    print_board(&(object->S_board));

    my_printf(object->S_my_printf, "best_mate=%d mate=%d\n",
      best_mate, arg_mate);

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      do_my_move(&(object->S_board), imove, &moves_list, FALSE);

      int your_mate = read_endgame(object, YOUR_BIT, NULL);

      undo_my_move(&(object->S_board), imove, &moves_list, FALSE);
    
      BSTRING(bmove_string)

      move2bstring(&moves_list, imove, bmove_string);

      my_printf(object->S_my_printf, "move=%s your_mate=%d\n",
        bdata(bmove_string), your_mate);

      BDESTROY(bmove_string)
    }
    my_printf(object->S_my_printf, "");

    FATAL("best_mate != mate", EXIT_FAILURE)
  }

  HARDBUG(bcatcstr(arg_bpv_string, " ") == BSTR_ERR)

  BSTRING(bmove_string)

  move2bstring(&moves_list, ibest_move, bmove_string);

  bconcat(arg_bpv_string, bmove_string);

  BDESTROY(bmove_string)

  do_my_move(&(object->S_board), ibest_move, &moves_list, FALSE);

  int temp_mate;

  if (arg_mate > 0)
    temp_mate = -arg_mate + 1;
  else
    temp_mate = -arg_mate - 1;

  your_endgame_pv(object, temp_mate, arg_bpv_string);

  undo_my_move(&(object->S_board), ibest_move, &moves_list, FALSE);
}

local int my_quiescence(search_t *object, int arg_my_alpha, int arg_my_beta, 
  int arg_node_type, moves_list_t *arg_moves_list, pv_t *arg_best_pv, int *arg_best_depth,
  int arg_all_moves)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(arg_my_alpha >= arg_my_beta)

  SOFTBUG(arg_node_type == 0)

  SOFTBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  SOFTBUG(IS_MINIMAL_WINDOW(arg_node_type) and ((arg_my_beta - arg_my_alpha) != 1))

  if ((object->S_board.B_inode - object->S_board.B_root_inode) >= 128)
  {
    my_printf(object->S_my_printf, "quiescence inode=%d\n",
      object->S_board.B_inode - object->S_board.B_root_inode);
    print_board(&(object->S_board));
    my_printf(object->S_my_printf, 
      "my_alpha=%d my_beta=%d node_type=%d\n",
      arg_my_alpha, arg_my_beta, arg_node_type);
  }

  *arg_best_pv = INVALID;

  *arg_best_depth = INVALID;

  int best_score = SCORE_MINUS_INFINITY;

  ++(object->S_total_nodes);

  ++(object->S_total_quiescence_nodes);

  if ((object->S_total_quiescence_nodes % UPDATE_NODES) == 0)
  {
    //only thread[0] should check the time limit

    if (object->S_thread == thread_alpha_beta_master)
    {
      //Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      //struct timespec tv;

      //HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      //check for input

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        my_printf(object->S_my_printf, "got message\n");

        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }

      if (return_my_timer(&(object->S_timer), FALSE) >=
          options.time_limit) 
      {
        if ((object->S_best_score < 0) and
            ((object->S_best_score - object->S_root_score) <
             (-SCORE_MAN / 4)))
        { 
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(object->S_my_printf,
                "problems.. itrouble=%d time_trouble=%.2f\n",
                itrouble, options.time_trouble[itrouble]);

              options.time_limit += options.time_trouble[itrouble];

              options.time_trouble[itrouble] = 0.0;
           
              break;
            }
          }
        }
      }
      //check time_limit again
      if (return_my_timer(&(object->S_timer), FALSE) >=
          options.time_limit) 
      {
        object->S_interrupt = BOARD_INTERRUPT_TIME;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
    else
    {
      SOFTBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    best_score = 0;

    *arg_best_depth = 0;

    goto label_return;
  }

  //check endgame

  int wdl;

  int temp_mate = read_endgame(object, MY_BIT, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    *arg_best_depth = DEPTH_MAX;

    if (temp_mate == INVALID)
    {
      best_score = 0;
 
      *arg_best_depth = 0;
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        best_score = SCORE_WON - object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_WON_ABSOLUTE - object->S_board.B_inode -
                     temp_mate;
    }
    else  
    {
      if (wdl)
        best_score = SCORE_LOST + object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode -
                     temp_mate;
    }

    goto label_return;
  }

  moves_list_t moves_list_temp;

  if (arg_moves_list == NULL)
  {
    construct_moves_list(&moves_list_temp);
  
    gen_my_moves(&(object->S_board), &moves_list_temp, TRUE);

    arg_moves_list = &moves_list_temp;
  }

  if (arg_moves_list->ML_nmoves == 0)
  {
    best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode;

    *arg_best_depth = DEPTH_MAX;

    goto label_return;
  }

  if (!arg_all_moves)
  {
    if (arg_moves_list->ML_ncaptx > 0)
    {
      ++(object->S_total_quiescence_all_moves_captures_only);
  
      arg_all_moves = TRUE;
    } 
    else if ((arg_moves_list->ML_nmoves <= 2) and !move_repetition(&(object->S_board)))
    {
      ++(object->S_total_quiescence_all_moves_le2_moves);
  
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
        ++(object->S_total_quiescence_all_moves_extended);
  
        arg_all_moves = TRUE;
      }
    }
  }

  if (!arg_all_moves and IS_PV(arg_node_type) and 
      (arg_my_alpha >= (SCORE_LOST + NODE_MAX)) and
      (options.quiescence_extension_search_delta > 0))
  {
    object->S_total_quiescence_extension_searches++;

    int reduced_alpha = arg_my_alpha - options.quiescence_extension_search_delta;
    int reduced_beta = reduced_alpha + 1;
   
    int temp_score;

    pv_t temp_pv[PV_MAX];

    int temp_depth;

    temp_score = my_quiescence(object, reduced_alpha, reduced_beta,
      MINIMAL_WINDOW_BIT, arg_moves_list, temp_pv, &temp_depth, TRUE);

    if (object->S_interrupt != 0)
    {
      best_score = SCORE_PLUS_INFINITY;
    
      goto label_return;
    }

    if (temp_score <= reduced_alpha)
    {
      object->S_total_quiescence_extension_searches_le_alpha++;

      arg_all_moves = TRUE;
    }
    else
    {
      object->S_total_quiescence_extension_searches_ge_beta++;
    }
  }

  if (!arg_all_moves)
  {
    best_score = return_my_score(&(object->S_board), arg_moves_list);

    *arg_best_depth = 0;

    //if ((with->S_board.my_king_bb | with->S_board.your_king_bb) != 0)
    //  goto label_return;

    if (best_score >= arg_my_beta) goto label_return;
  }


  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    moves_weight[imove] = arg_moves_list->ML_move_weights[imove];

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = 0;
   
    for (int kmove = 1; kmove < arg_moves_list->ML_nmoves; kmove++)
    {
      if (moves_weight[kmove] == L_MIN) continue;
      
      if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
    }

    SOFTBUG(moves_weight[jmove] == L_MIN)
  
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

    int temp_score = SCORE_MINUS_INFINITY;

    pv_t temp_pv[PV_MAX];

    int temp_depth;

    if (IS_MINIMAL_WINDOW(arg_node_type))
    {
      do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      temp_score = -your_quiescence(object, -arg_my_beta, -temp_alpha,
        arg_node_type, NULL, temp_pv + 1, &temp_depth, FALSE);

      undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        temp_score = -your_quiescence(object, -arg_my_beta, -temp_alpha,
          arg_node_type, NULL, temp_pv + 1, &temp_depth, FALSE);

        undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        temp_score = -your_quiescence(object, -temp_beta, -temp_alpha,
          MINIMAL_WINDOW_BIT, NULL, temp_pv + 1, &temp_depth, FALSE);

        if ((object->S_interrupt == 0) and
            (temp_score >= temp_beta) and (temp_score < arg_my_beta))
        {
          temp_score = -your_quiescence(object, -arg_my_beta, -temp_score,
            arg_node_type, NULL, temp_pv + 1, &temp_depth, FALSE);
        }

        undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);
      }
    }

    if (object->S_interrupt != 0)
    {
       best_score = SCORE_PLUS_INFINITY;
  
       goto label_return;
    }

    SOFTBUG(temp_score == SCORE_MINUS_INFINITY)

    if (temp_score > best_score)
    {
      best_score = temp_score;
  
      *arg_best_pv = jmove;

      for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);

      *arg_best_depth = temp_depth;
    }
  
    if (best_score >= arg_my_beta) break;
  } //imove

  //we always searched move 0

  SOFTBUG(best_score == SCORE_MINUS_INFINITY)
   
  SOFTBUG(*arg_best_depth == INVALID)

  label_return:

  END_BLOCK

  if (*arg_best_depth == DEPTH_MAX)
  {
    HARDBUG((best_score < (SCORE_WON - NODE_MAX)) and
            (best_score > (SCORE_LOST + NODE_MAX)) and 
            (best_score != 0))
  }

  if (best_score < arg_my_alpha)
  {
    best_score = arg_my_alpha;
    *arg_best_depth = 0;
  }
  return(best_score);
}

local int my_alpha_beta(search_t *object,
  int arg_my_alpha, int arg_my_beta, int arg_my_depth, int arg_node_type,
  int arg_reduction_depth_root, int arg_my_tweaks,
  pv_t *arg_best_pv, int *arg_best_depth)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(arg_my_alpha >= arg_my_beta)

  SOFTBUG(arg_my_depth < 0)

  SOFTBUG(arg_node_type == 0)

  SOFTBUG(IS_MINIMAL_WINDOW(arg_node_type) and IS_PV(arg_node_type))

  SOFTBUG(IS_MINIMAL_WINDOW(arg_node_type) and ((arg_my_beta - arg_my_alpha) != 1))

  if (object->S_thread == thread_alpha_beta_master)
  {
    update_bucket(&bucket_depth, arg_my_depth);
  }

  if ((object->S_board.B_inode - object->S_board.B_root_inode) >= 128)
  {
    my_printf(object->S_my_printf, "alpha_beta inode=%d\n",
      object->S_board.B_inode - object->S_board.B_root_inode);
    print_board(&(object->S_board));
    my_printf(object->S_my_printf, 
      "my_alpha=%d my_beta=%d my_depth=%d node_type=%d\n",
      arg_my_alpha, arg_my_beta, arg_my_depth, arg_node_type);
  }

  *arg_best_pv = INVALID;

  *arg_best_depth = INVALID;

  int best_score = SCORE_PLUS_INFINITY;

  int best_move = INVALID;

  ++(object->S_total_nodes);

  ++(object->S_total_alpha_beta_nodes);

  if (IS_MINIMAL_WINDOW(arg_node_type))
    ++(object->S_total_minimal_window_nodes);
  else if (IS_PV(arg_node_type))
    ++(object->S_total_pv_nodes);
  else
    FATAL("unknown node_type", EXIT_FAILURE)

  if ((object->S_total_alpha_beta_nodes % UPDATE_NODES) == 0)
  {
    //only thread[0] should check the time limit

    if (object->S_thread == thread_alpha_beta_master)
    {
      //Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      //struct timespec tv;

      //HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      //check for input

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        my_printf(object->S_my_printf, "got message\n");

        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }

      if (return_my_timer(&(object->S_timer), FALSE) >=
          options.time_limit) 
      {
        if ((object->S_best_score < 0) and
            ((object->S_best_score - object->S_root_score) <
             (-SCORE_MAN / 4)))
        { 
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(object->S_my_printf,
                "problems.. itrouble=%d time_trouble=%.2f\n",
                itrouble, options.time_trouble[itrouble]);

              options.time_limit += options.time_trouble[itrouble];

              options.time_trouble[itrouble] = 0.0;
           
              break;
            }
          }
        }
      }
      //check time_limit again
      if (return_my_timer(&(object->S_timer), FALSE) >=
          options.time_limit) 
      {
        object->S_interrupt = BOARD_INTERRUPT_TIME;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
    else
    {
      SOFTBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(object->S_thread)) != INVALID)
      {
        object->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    best_score = 0;

    *arg_best_depth = 0;

    goto label_return;
  }

  //check endgame

  int wdl;

  int temp_mate = read_endgame(object, MY_BIT, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    *arg_best_depth = DEPTH_MAX;

    if (temp_mate == INVALID)
    {
      best_score = 0;

      *arg_best_depth = 0;
    }
    else if (temp_mate > 0)
    {
      if (wdl)
        best_score = SCORE_WON - object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_WON_ABSOLUTE - object->S_board.B_inode -
                     temp_mate;
    }
    else
    {
      if (wdl)
        best_score = SCORE_LOST + object->S_board.B_inode - temp_mate;
      else
        best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode -
                     temp_mate;
    }

    goto label_return;
  }

  int alpha_beta_cache_hit = FALSE;

  alpha_beta_cache_entry_t alpha_beta_cache_entry = 
    alpha_beta_cache_entry_default;

  alpha_beta_cache_slot_t *alpha_beta_cache_slot;

  if (options.alpha_beta_cache_size > 0)
  {
    alpha_beta_cache_hit = probe_alpha_beta_cache(object, arg_node_type, FALSE,
      my_alpha_beta_cache, &alpha_beta_cache_entry, &alpha_beta_cache_slot);

    if (alpha_beta_cache_hit)
    {
      ++(object->S_total_alpha_beta_cache_hits);

      if (!IS_PV(arg_node_type) and 
          (alpha_beta_cache_slot->ABCS_depth >= arg_my_depth))
      {
        ++(object->S_total_alpha_beta_cache_depth_hits);

        if (alpha_beta_cache_slot->ABCS_flags & LE_ALPHA_BIT)
        {
          ++(object->S_total_alpha_beta_cache_le_alpha_hits);
  
          if (alpha_beta_cache_slot->ABCS_score <= arg_my_alpha)
          {
            ++(object->S_total_alpha_beta_cache_le_alpha_cutoffs);
  
            best_score = alpha_beta_cache_slot->ABCS_score;

            arg_best_pv[0] = alpha_beta_cache_slot->ABCS_moves[0];

            arg_best_pv[1] = INVALID;
  
            *arg_best_depth = alpha_beta_cache_slot->ABCS_depth;
   
            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot->ABCS_flags & GE_BETA_BIT)
        {
          ++(object->S_total_alpha_beta_cache_ge_beta_hits);
  
          if (alpha_beta_cache_slot->ABCS_score >= arg_my_beta)
          {
            ++(object->S_total_alpha_beta_cache_ge_beta_cutoffs);
  
            best_score = alpha_beta_cache_slot->ABCS_score;
  
            arg_best_pv[0] = alpha_beta_cache_slot->ABCS_moves[0];

            arg_best_pv[1] = INVALID;

            *arg_best_depth = alpha_beta_cache_slot->ABCS_depth;
    
            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot->ABCS_flags & TRUE_SCORE_BIT)
        {
          ++(object->S_total_alpha_beta_cache_true_score_hits);
  
          best_score = alpha_beta_cache_slot->ABCS_score;
  
          arg_best_pv[0] = alpha_beta_cache_slot->ABCS_moves[0];

          arg_best_pv[1] = INVALID;

          *arg_best_depth = alpha_beta_cache_slot->ABCS_depth;
  
          goto label_return;
        } 
      }
    }
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_my_moves(&(object->S_board), &moves_list, FALSE);

  if (moves_list.ML_nmoves == 0)
  {
    best_score = SCORE_LOST_ABSOLUTE + object->S_board.B_inode;

    *arg_best_depth = DEPTH_MAX;

    goto label_return;
  }

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    moves_weight[imove] = moves_list.ML_move_weights[imove];
  
  if (alpha_beta_cache_hit)
  {
    for (int imove = 0; imove < NMOVES; imove++)
    {
      int jmove = alpha_beta_cache_slot->ABCS_moves[imove];

      if (jmove == INVALID) break;

      if (jmove >= moves_list.ML_nmoves) 
        object->S_total_alpha_beta_cache_nmoves_errors++;
      else
        moves_weight[jmove] = L_MAX - imove;
    }
  }

  best_score = SCORE_MINUS_INFINITY;

  best_move = INVALID;

  SOFTBUG(*arg_best_depth != INVALID)

  int your_tweaks = 0;
 
  if (((moves_list.ML_nmoves <= 2) and !move_repetition(&(object->S_board))) or
      (options.captures_are_transparent and (moves_list.ML_ncaptx > 0)))
  {
    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      int jmove = 0;
   
      for (int kmove = 1; kmove < moves_list.ML_nmoves; kmove++)
      {
        if (moves_weight[kmove] == L_MIN) continue;
        
        if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
      }
  
      SOFTBUG(moves_weight[jmove] == L_MIN)
  
      moves_weight[jmove] = L_MIN;

      int temp_alpha;
      pv_t temp_pv[PV_MAX];
      int temp_depth;
  
      if (best_score < arg_my_alpha)
        temp_alpha = arg_my_alpha;
      else
        temp_alpha = best_score;
  
      int temp_score = SCORE_MINUS_INFINITY;

      if (IS_MINIMAL_WINDOW(arg_node_type))
      {
        do_my_move(&(object->S_board), jmove, &moves_list, FALSE);

        temp_score = -your_alpha_beta(object,
          -arg_my_beta, -temp_alpha, arg_my_depth, arg_node_type,
          arg_reduction_depth_root,
          your_tweaks,
          temp_pv + 1, &temp_depth);

        undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
      }
      else
      {
        if (imove == 0)
        {
          do_my_move(&(object->S_board), jmove, &moves_list, FALSE);

          temp_score = -your_alpha_beta(object,
            -arg_my_beta, -temp_alpha, arg_my_depth, arg_node_type,
            arg_reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);

          undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
        } 
        else
        {
          int temp_beta = temp_alpha + 1;

          do_my_move(&(object->S_board), jmove, &moves_list, FALSE);

          temp_score = -your_alpha_beta(object,
            -temp_beta, -temp_alpha, arg_my_depth, MINIMAL_WINDOW_BIT,
            arg_reduction_depth_root,
            your_tweaks, 
            temp_pv + 1, &temp_depth);

          if ((object->S_interrupt == 0) and
              (temp_score >= temp_beta) and (temp_score < arg_my_beta))
          {
            temp_score = -your_alpha_beta(object,
              -arg_my_beta, -temp_score, arg_my_depth, arg_node_type,
              arg_reduction_depth_root,
              your_tweaks,
              temp_pv + 1, &temp_depth);
          }

          undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
        }
      }

      if (object->S_interrupt != 0)
      {
        best_score = SCORE_PLUS_INFINITY;
    
        goto label_return;
      }

      if (temp_score > best_score)
      {
        best_score = temp_score;
    
        best_move = jmove;

        *arg_best_pv = jmove;

        for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
    
        *arg_best_depth = temp_depth;

        if (options.alpha_beta_cache_size > 0)
          update_alpha_beta_cache_slot_moves(alpha_beta_cache_slot, jmove);
    
        if (best_score >= arg_my_beta) break;
      }
    }
    goto label_break;
  }

  SOFTBUG(((moves_list.ML_nmoves <= 2) and !move_repetition(&(object->S_board))) or
          (options.captures_are_transparent and (moves_list.ML_ncaptx > 0)))

  if (arg_reduction_depth_root > 0) arg_reduction_depth_root--;

  int your_depth = arg_my_depth - 1;

  if (your_depth < 0)
  {
    best_score = my_quiescence(object, arg_my_alpha, arg_my_beta,
      arg_node_type, &moves_list, arg_best_pv, arg_best_depth, FALSE);

    if (object->S_interrupt != 0) best_score = SCORE_PLUS_INFINITY;

    goto label_return;
  }

  int npassed_min = 1;

  if (IS_MINIMAL_WINDOW(arg_node_type))
  {
    int allow_reductions = 
      options.use_reductions and
      !(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_NULL_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_REDUCED_BIT) and
      !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
      (moves_list.ML_ncaptx == 0) and
      (your_depth >= options.reduction_depth_leaf) and
      (arg_reduction_depth_root == 0) and
      (arg_my_alpha >= (SCORE_LOST + NODE_MAX)) and
      (arg_my_beta <= (SCORE_WON - NODE_MAX));

    float factor = 1.0;

    if (allow_reductions and (options.reduction_mean > 0))
    {
      int delta = BIT_COUNT(object->S_board.my_man_bb |
                            object->S_board.your_man_bb |
                            object->S_board.my_king_bb |
                            object->S_board.your_king_bb) -
                  options.reduction_mean;

      float gamma = options.reduction_sigma;

      factor = (gamma * gamma) / (delta * delta + gamma * gamma);
//PRINTF("factor=%.6f\n", factor);
    }

    int pass[MOVES_MAX];

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      pass[imove] = INVALID;

      if (MOVE_DO_NOT_REDUCE(moves_list.ML_move_flags[imove]))
        pass[imove] = 0;
   
      //backward compatability

      //pass[imove] = 0;
    }

    int npassed = 0;

    for (int ipass = -1; ipass <= 0; ++ipass)
    {
      for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
      {
        int jmove = INVALID;

        for (jmove = 0; jmove < moves_list.ML_nmoves; jmove++)
          if (pass[jmove] == ipass) break;

        if (jmove >= moves_list.ML_nmoves) break;

        for (int kmove = jmove + 1; kmove < moves_list.ML_nmoves; kmove++)
        {
          if (pass[kmove] != ipass) continue;

          if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
        }

        SOFTBUG(jmove == INVALID)

        HARDBUG(pass[jmove] != ipass)

        int temp_score;

        pv_t temp_pv[PV_MAX];
  
        int temp_depth;

        do_my_move(&(object->S_board), jmove, &moves_list, FALSE);

        if (allow_reductions and
            !(MOVE_DO_NOT_REDUCE(moves_list.ML_move_flags[jmove])) and
            (best_score >= (SCORE_LOST + NODE_MAX)) and
            (npassed >= npassed_min))
        {
          ++(object->S_total_reductions_delta);

          int reduced_alpha = arg_my_alpha - options.reduction_delta;
          int reduced_beta = reduced_alpha + 1;

          int reduced_depth = 
            roundf((100 - options.reduction_max) / 100.0f * your_depth);

          HARDBUG(reduced_depth == your_depth)

          temp_score = -your_alpha_beta(object,
            -reduced_beta, -reduced_alpha, reduced_depth,
            arg_node_type,
            arg_reduction_depth_root,
            your_tweaks | TWEAK_PREVIOUS_MOVE_REDUCED_BIT,
            temp_pv + 1, &temp_depth);

          if (object->S_interrupt != 0)
          {
            undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);

            best_score = SCORE_PLUS_INFINITY;
              
            goto label_return;
          }
  
          if (temp_score < (SCORE_LOST + NODE_MAX))
          {
            ++(object->S_total_reductions_delta_lost);

            pass[jmove] = 1;
          }
          else
          {
            object->S_total_reductions++;

            int percentage = INVALID;

            if (temp_score <= reduced_alpha)
            {
              ++(object->S_total_reductions_delta_le_alpha);

              percentage = roundf(options.reduction_strong * factor);
            }
            else
            { 
              SOFTBUG(!(temp_score >= reduced_beta))

              ++(object->S_total_reductions_delta_ge_beta);

              percentage = roundf(options.reduction_weak * factor);
            }

            if (percentage < options.reduction_min)
              percentage = options.reduction_min;

            reduced_depth = roundf((100 - percentage) / 100.0f * your_depth);

            if (reduced_depth < your_depth)
            {
              temp_score = -your_alpha_beta(object,
                -arg_my_beta, -arg_my_alpha, reduced_depth,
                arg_node_type,
                arg_reduction_depth_root,
                your_tweaks | TWEAK_PREVIOUS_MOVE_REDUCED_BIT,
                temp_pv + 1, &temp_depth);

              if (object->S_interrupt != 0)
              {
                undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
    
                best_score = SCORE_PLUS_INFINITY;
                  
                goto label_return;
              }

              if (temp_score <= best_score)
              {
                object->S_total_reductions_le_alpha++;

                pass[jmove] = 1;
              }
              else
              {
                object->S_total_reductions_ge_beta++;
              }
            }
          }//if (temp_score < (SCORE_LOST + NODE+MAX))
        }//if (!allow_reduction   

        if (pass[jmove] != 1)
        {
          temp_score = -your_alpha_beta(object,
            -arg_my_beta, -arg_my_alpha, your_depth,
            arg_node_type,
            arg_reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);
 
          pass[jmove] = 1;

          npassed++;
        }

        undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);

        if (object->S_interrupt != 0)
        {
          best_score = SCORE_PLUS_INFINITY;
        
          goto label_return;
        }

        SOFTBUG(temp_score == SCORE_MINUS_INFINITY)

        if (temp_score > best_score)
        {
          best_score = temp_score;
        
          best_move = jmove;
        
          *arg_best_pv = jmove;
  
          for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
          if ((options.returned_depth_includes_captures or
              (moves_list.ML_ncaptx == 0)) and (temp_depth < (DEPTH_MAX - 1)))
          {
            temp_depth++;
          }
  
          *arg_best_depth = temp_depth;

          if (options.alpha_beta_cache_size > 0)
            update_alpha_beta_cache_slot_moves(alpha_beta_cache_slot, jmove);

          if (best_score >= arg_my_beta) goto label_break;
        }
      }//for (int imove
    }//for (int ipass
  }
  else //IS_MINIMAL_WINDOW(node_type) NEW
  {
    //check if position should be extended

    //HARDBUG(my_tweaks & TWEAK_PREVIOUS_MOVE_NULL_BIT)
    HARDBUG(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_REDUCED_BIT)

    if (!(arg_my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
        !(arg_my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
        (options.pv_extension_search_delta > 0) and
        (arg_my_alpha >= (SCORE_LOST + NODE_MAX)))
    {
      object->S_total_pv_extension_searches++;

      int reduced_alpha = arg_my_alpha - options.pv_extension_search_delta;
      int reduced_beta = reduced_alpha + 1;
  
      int reduced_depth = your_depth;

      pv_t temp_pv[PV_MAX];
  
      int temp_depth;
      
      int temp_score = my_alpha_beta(object,
        reduced_alpha, reduced_beta, reduced_depth, MINIMAL_WINDOW_BIT,
        options.reduction_depth_root,
        arg_my_tweaks | TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT,
        temp_pv, &temp_depth);
  
      if (object->S_interrupt != 0)
      {
        best_score = SCORE_PLUS_INFINITY;
      
        goto label_return;
      }
  
      if (temp_score <= reduced_alpha)
      {
        object->S_total_pv_extension_searches_le_alpha++;

        your_depth = your_depth + 1;

        your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;
      }
      else
      {
        object->S_total_pv_extension_searches_ge_beta++;
      }
    }

    for (int ipass = 1; ipass <= 2; ++ipass)
    {
      int nsingle_replies = 0;

      for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
      {
        int jmove = 0;
      
        for (int kmove = 1; kmove < moves_list.ML_nmoves; kmove++)
        {
          if (moves_weight[kmove] == L_MIN) continue;
          
          if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
        }
  
        if (moves_weight[jmove] == L_MIN)
        {
          if (ipass == 1)
            HARDBUG(moves_weight[jmove] == L_MIN)
          else
            break;
        }
      
        moves_weight[jmove] = L_MIN;
  
        int temp_alpha;
      
        if (best_score < arg_my_alpha)
          temp_alpha = arg_my_alpha;
        else
          temp_alpha = best_score;

        int temp_score = SCORE_MINUS_INFINITY;
  
        pv_t temp_pv[PV_MAX];
  
        int temp_depth;
  
        if (imove == 0)
        {
          do_my_move(&(object->S_board), jmove, &moves_list, FALSE);
  
          temp_score = -your_alpha_beta(object,
            -arg_my_beta, -temp_alpha, your_depth, arg_node_type,
            arg_reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);
  
          undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
        }
        else
        {
          int temp_beta = temp_alpha + 1;
  
          do_my_move(&(object->S_board), jmove, &moves_list, FALSE);
  
          temp_score = -your_alpha_beta(object,
            -temp_beta, -temp_alpha, your_depth,
            MINIMAL_WINDOW_BIT,
            arg_reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);
  
          if ((object->S_interrupt == 0) and
              (temp_score >= temp_beta) and (temp_score < arg_my_beta))
          {
            temp_score = -your_alpha_beta(object,
              -arg_my_beta, -temp_score, your_depth, arg_node_type,
              arg_reduction_depth_root,
              your_tweaks,
              temp_pv + 1, &temp_depth);
          }

          undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
        }
  
        if (object->S_interrupt != 0)
        {
          best_score = SCORE_PLUS_INFINITY;
    
          goto label_return;
        }
    
        SOFTBUG(temp_score == SCORE_MINUS_INFINITY)
  
        if (temp_score > arg_my_alpha)
        {
          SOFTBUG(!IS_PV(arg_node_type))
  
          nsingle_replies++;
        }
      
        if (temp_score > best_score)
        {
          best_score = temp_score;
      
          best_move = jmove;
      
          *arg_best_pv = jmove;

          for (int ipv = 1; (arg_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
          if ((options.returned_depth_includes_captures or
              (moves_list.ML_ncaptx == 0)) and (temp_depth < (DEPTH_MAX - 1)))
          {
            temp_depth++;
          }
  
          *arg_best_depth = temp_depth;
        
          if (options.alpha_beta_cache_size > 0)
            update_alpha_beta_cache_slot_moves(alpha_beta_cache_slot, jmove);
  
          if (best_score >= arg_my_beta) goto label_break;
        }
      } //imove

      if ((ipass == 1) and
          (options.use_single_reply_extensions) and
          (nsingle_replies == 1))
      {
        object->S_total_single_reply_extensions++;

        *arg_best_pv = INVALID;

        *arg_best_depth = INVALID;

        best_score = SCORE_MINUS_INFINITY;

        your_depth = your_depth + 1;

        your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;

        for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
          moves_weight[imove] = moves_list.ML_move_weights[imove];

        moves_weight[best_move] = L_MAX;

        continue;
      }

      break;
    }//ipass
  }//IS_MINIMAL_WINDOW(node_type) NEW

  label_break:
  
  //we always searched move 0

  SOFTBUG(best_move == INVALID)

  SOFTBUG(best_score == SCORE_MINUS_INFINITY)
   
  SOFTBUG(*arg_best_depth == INVALID)

  SOFTBUG(*arg_best_depth > DEPTH_MAX)

  //if (best_score == 0) goto label_return;

#ifdef DEBUG
  if (options.alpha_beta_cache_size > 0)
  {
    for (int idebug = 0; idebug < NMOVES; idebug++)
    {
      if (alpha_beta_cache_slot->ABCS_moves[idebug] == INVALID)
      {  
        for (int jdebug = idebug + 1; jdebug < NMOVES; jdebug++)
          HARDBUG(alpha_beta_cache_slot->ABCS_moves[jdebug] != INVALID)
  
        break;
      }
  
      int ndebug = 0;
      
      for (int jdebug = 0; jdebug < NMOVES; jdebug++)
      {
        if (alpha_beta_cache_slot->ABCS_moves[jdebug] == 
            alpha_beta_cache_slot->ABCS_moves[idebug]) ndebug++;
      }
      HARDBUG(ndebug != 1)
    }
  }
#endif

  if (options.alpha_beta_cache_size > 0)
  {
    SOFTBUG(best_score > SCORE_PLUS_INFINITY)
    SOFTBUG(best_score < SCORE_MINUS_INFINITY)

    alpha_beta_cache_slot->ABCS_score = best_score;

    alpha_beta_cache_slot->ABCS_depth = *arg_best_depth;
  
    if (best_score <= arg_my_alpha)
    {
      ++(object->S_total_alpha_beta_cache_le_alpha_stored);

      if (best_score >= (SCORE_LOST + NODE_MAX))
        alpha_beta_cache_slot->ABCS_score = arg_my_alpha;

      alpha_beta_cache_slot->ABCS_flags = LE_ALPHA_BIT;
    }
    else if (best_score >= arg_my_beta)
    {
      ++(object->S_total_alpha_beta_cache_ge_beta_stored);

      if (best_score <= (SCORE_WON - NODE_MAX))
        alpha_beta_cache_slot->ABCS_score = arg_my_beta;

      alpha_beta_cache_slot->ABCS_flags = GE_BETA_BIT;
    }
    else
    {
      ++(object->S_total_alpha_beta_cache_true_score_stored);

      alpha_beta_cache_slot->ABCS_flags = TRUE_SCORE_BIT;
    }
 
    alpha_beta_cache_slot->ABCS_key = object->S_board.B_key;

    ui32_t crc32 = 0xFFFFFFFF;
    crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_key);
    crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_data);
    alpha_beta_cache_slot->ABCS_crc32 = ~crc32;
  
    if (IS_PV(arg_node_type))
    {
      my_alpha_beta_cache[object->S_board.B_key %
                          nalpha_beta_pv_cache_entries] =
        alpha_beta_cache_entry;
    }
    else
    {
      my_alpha_beta_cache[nalpha_beta_pv_cache_entries + 
                          object->S_board.B_key %
                          nalpha_beta_cache_entries] = 
        alpha_beta_cache_entry;
    }
  }

  label_return:

  END_BLOCK

  if (*arg_best_depth == DEPTH_MAX)
  {
    HARDBUG((best_score < (SCORE_WON - NODE_MAX)) and
            (best_score > (SCORE_LOST + NODE_MAX)) and 
            (best_score != 0))
  }

  if (best_score < arg_my_alpha)
  {
    best_score = arg_my_alpha;
    *arg_best_depth = arg_my_depth;
  }
  return(best_score);
}

void my_pv(search_t *object, pv_t *arg_best_pv, int arg_pv_score, bstring arg_bpv_string)
{
  if (draw_by_repetition(&(object->S_board), FALSE))
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else 
    {
      HARDBUG(bcatcstr(arg_bpv_string, " rep") == BSTR_ERR)
    }

    return;
  }
  
  int wdl;

  int temp_mate = read_endgame(object, MY_BIT, &wdl);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    temp_mate = read_endgame(object, MY_BIT, NULL);

    if (temp_mate == INVALID)
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
      }
      else
      {
        HARDBUG(bcatcstr(arg_bpv_string, " draw") == BSTR_ERR)
      }
    }
    else if (temp_mate > 0)
    {
      if (!options.hub)
      {
        HARDBUG(bformata(arg_bpv_string, " won in %d", temp_mate) == BSTR_ERR)
      }
      my_endgame_pv(object, temp_mate, arg_bpv_string);
    }
    else
    {
      if (!options.hub)
      {
        HARDBUG(bformata(arg_bpv_string, " lost in %d", -temp_mate) == BSTR_ERR)
      }
      my_endgame_pv(object, temp_mate, arg_bpv_string);
    }
    return;
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_my_moves(&(object->S_board), &moves_list, FALSE);

  if (moves_list.ML_nmoves == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      HARDBUG(bcatcstr(arg_bpv_string, " #") == BSTR_ERR)
    }

    return;
  }

  int jmove = *arg_best_pv;

  if (jmove == INVALID)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(arg_bpv_string, "\"") == BSTR_ERR)
    }
    else
    {
      int temp_score;

      temp_score = return_my_score(&(object->S_board), &moves_list);
    
      HARDBUG(bformata(arg_bpv_string, " %d", temp_score) == BSTR_ERR)

      if (temp_score != arg_pv_score)
      {
        HARDBUG(bcatcstr(arg_bpv_string, " PV ERROR") == BSTR_ERR)
      }
    }
  }
  else
  {
    //reload pv 

    if (options.alpha_beta_cache_size > 0)
    {
      alpha_beta_cache_entry_t alpha_beta_cache_entry;

      alpha_beta_cache_slot_t *alpha_beta_cache_slot;

      (void) probe_alpha_beta_cache(object, PV_BIT, TRUE,
        my_alpha_beta_cache, &alpha_beta_cache_entry, &alpha_beta_cache_slot);

      *alpha_beta_cache_slot = alpha_beta_cache_slot_default;

      alpha_beta_cache_slot->ABCS_moves[0] = jmove;

      alpha_beta_cache_slot->ABCS_key = object->S_board.B_key;

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_key);
      crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_data);
      alpha_beta_cache_slot->ABCS_crc32 = ~crc32;

      my_alpha_beta_cache[object->S_board.B_key % 
                          nalpha_beta_pv_cache_entries] =
        alpha_beta_cache_entry;
    }

    BSTRING(bmove_string)

    move2bstring(&moves_list, jmove, bmove_string);

    HARDBUG(bcatcstr(arg_bpv_string, " ") == BSTR_ERR)

    HARDBUG(bconcat(arg_bpv_string, bmove_string) == BSTR_ERR)
  
    BDESTROY(bmove_string)

    do_my_move(&(object->S_board), jmove, &moves_list, FALSE);
  
    your_pv(object, arg_best_pv + 1, -arg_pv_score, arg_bpv_string);

    undo_my_move(&(object->S_board), jmove, &moves_list, FALSE);
  }
}

local void print_my_pv(search_t *object, int arg_my_depth,
  int arg_imove, int arg_jmove, moves_list_t *arg_moves_list,
  int arg_move_score, pv_t *arg_best_pv, char *arg_move_status)
{
  BSTRING(bmove_string)

  move2bstring(arg_moves_list, arg_jmove, bmove_string);

  BSTRING(bpv_string)

  if (options.hub)
  {
    if (options.nthreads == INVALID)
      global_nodes = object->S_total_nodes;
    else
    {
      global_nodes = 0;

      for (int ithread = 0; ithread < options.nthreads; ithread++)
      {
        thread_t *with_thread = threads + ithread;

        global_nodes += with_thread->thread_search.S_total_nodes;
      }
    }
    bformata(bpv_string, 
      "depth=%d score=%.3f time=%.2f nps=%.2f pv=\"%s",
      arg_my_depth,
      (float) arg_move_score / (float) SCORE_MAN,
      return_my_timer(&(object->S_timer), FALSE),
      global_nodes / return_my_timer(&(object->S_timer), TRUE) / 1000000.0,
      bdata(bmove_string));
  }
  else
  {
    bformata(bpv_string,
      "d=%d i=%d j=%d m=%s s=%d (%.2f) r=%d t=%.2f n=%lld/%lld/%lld x=%s pv=%s",
      arg_my_depth,
      arg_imove,
      arg_jmove,
      bdata(bmove_string),
      arg_move_score,
      (float) arg_move_score / (float) SCORE_MAN,
      object->S_root_score,
      return_my_timer(&(object->S_timer), FALSE),
      object->S_total_nodes,
      object->S_total_alpha_beta_nodes,
      object->S_total_quiescence_nodes,
      arg_move_status,
      bdata(bmove_string));
  }

  do_my_move(&(object->S_board), arg_jmove, arg_moves_list, FALSE);

  your_pv(object, arg_best_pv + 1, -arg_move_score, bpv_string);

  undo_my_move(&(object->S_board), arg_jmove, arg_moves_list, FALSE);

  my_printf(object->S_my_printf, "%s\n", bdata(bpv_string));

  //flush

  if (object->S_thread == NULL) my_printf(object->S_my_printf, "");

  if (options.mode == GAME_MODE)
  {
    if ((object->S_thread != NULL) and
        (object->S_thread == thread_alpha_beta_master))
      enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string));
  }

  BDESTROY(bpv_string)

  BDESTROY(bmove_string)
}

void my_search(search_t *object, moves_list_t *arg_moves_list,
  int arg_depth_min, int arg_depth_max, int arg_root_score,
  my_random_t *arg_shuffle)
{
  BEGIN_BLOCK(__FUNC__)

  my_printf(object->S_my_printf, "time_limit=%.2f\n", options.time_limit);

  my_printf(object->S_my_printf, "time_ntrouble=%d\n", 
    options.time_ntrouble);

  for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
    my_printf(object->S_my_printf, "itrouble=%d time_trouble=%.2f\n",
            itrouble, options.time_trouble[itrouble]);

  if (object->S_thread == thread_alpha_beta_master)
  {
    clear_bucket(&bucket_depth);
  }

  int my_depth = INVALID;

  SOFTBUG(arg_moves_list->ML_nmoves == 0)

  if (arg_depth_min == INVALID) arg_depth_min = 1;
  if (arg_depth_max == INVALID) arg_depth_max = options.depth_limit;

  object->S_interrupt = 0;

  object->S_board.B_root_inode = object->S_board.B_inode;

  clear_totals(object);

  reset_my_timer(&(object->S_timer));

  int sort[MOVES_MAX];

  shuffle(sort, arg_moves_list->ML_nmoves, arg_shuffle);

  //check for known endgame

  int egtb_mate = read_endgame(object, MY_BIT, NULL);

  if (egtb_mate != ENDGAME_UNKNOWN)
  {
    print_board(&(object->S_board));

    my_printf(object->S_my_printf, "known endgame egtb_mate=%d\n",
      egtb_mate);

    object->S_best_score = SCORE_MINUS_INFINITY;

    int your_mate = INVALID;

    for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    {
      int jmove = sort[imove];

      do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      ++(object->S_total_nodes);

      int temp_mate = read_endgame(object, YOUR_BIT, NULL);

      SOFTBUG(temp_mate == ENDGAME_UNKNOWN)

      int temp_score;

      if (temp_mate == INVALID)
        temp_score = 0;
      else if (temp_mate > 0)
        temp_score = SCORE_WON - object->S_board.B_inode - temp_mate;
      else
        temp_score = SCORE_LOST + object->S_board.B_inode - temp_mate;

      undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      int move_score = -temp_score;

      BSTRING(bmove_string)
  
      move2bstring(arg_moves_list, imove, bmove_string);

      my_printf(object->S_my_printf, "%s %d\n", bdata(bmove_string),
                move_score);

      BDESTROY(bmove_string)

      if (move_score > object->S_best_score)
      {
        for (int kmove = imove; kmove > 0; kmove--)
          sort[kmove] = sort[kmove - 1];

        sort[0] = jmove;

        object->S_best_score = move_score;

        object->S_best_move = jmove;

        object->S_best_pv[0] = jmove;
        object->S_best_pv[1] = INVALID;

        your_mate = temp_mate;
      }
#ifdef DEBUG
      for (int idebug = 0; idebug < arg_moves_list->ML_nmoves; idebug++)
      {
        int ndebug = 0;
        for (int jdebug = 0; jdebug < arg_moves_list->ML_nmoves; jdebug++)
          if (sort[jdebug] == idebug) ++ndebug;
        HARDBUG(ndebug != 1)
      }
#endif
    }

    SOFTBUG(object->S_best_score == SCORE_MINUS_INFINITY)

    object->S_best_score_kind = SEARCH_BEST_SCORE_EGTB;

    object->S_best_depth = 0;

    BSTRING(bmove_string)
    
    move2bstring(arg_moves_list, sort[0], bmove_string);

    BSTRING(bpv_string)

    if (options.hub)
    {
      HARDBUG(bformata(bpv_string, "depth=127 score=%d nps=0.00 pv=\"%s",
        object->S_best_score, bdata(bmove_string)) == BSTR_ERR)
    }
    else
    {
      HARDBUG(bformata(bpv_string, "s=%d (%d) x=egtb pv=%s ",
        object->S_best_score,
        egtb_mate, bdata(bmove_string)) == BSTR_ERR)
    }

    if (your_mate != INVALID)
    {
      do_my_move(&(object->S_board), sort[0], arg_moves_list, FALSE);

      your_endgame_pv(object, your_mate, bpv_string);

      undo_my_move(&(object->S_board), sort[0], arg_moves_list, FALSE);
    } 
    else
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(bpv_string, "\"") == BSTR_ERR)
      }
    }

    my_printf(object->S_my_printf, "%s\n", bdata(bpv_string));

    if (options.mode == GAME_MODE)
    {
      if ((object->S_thread != NULL) and
          (object->S_thread == thread_alpha_beta_master))
        enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string)); 
    }

    BDESTROY(bpv_string)

    BDESTROY(bmove_string)

    goto label_limit;
  }

  int your_tweaks = 0;
 
  //RE-INITIALIZE ACCUMULATOR

  return_network_score_scaled(&(object->S_board.B_network), FALSE, FALSE);

  object->S_root_simple_score = return_my_score(&(object->S_board), arg_moves_list);

  pv_t temp_pv[PV_MAX];

  int temp_depth;

  if (FALSE and (arg_moves_list->ML_ncaptx == 0) and (arg_moves_list->ML_nmoves > 2))
  { 
    do_my_move(&(object->S_board), INVALID, NULL, FALSE);

    object->S_root_score = -your_alpha_beta(object,
      SCORE_MINUS_INFINITY, SCORE_PLUS_INFINITY, 1, PV_BIT,
      options.reduction_depth_root,
      TWEAK_PREVIOUS_MOVE_NULL_BIT, temp_pv + 1, &temp_depth);

    undo_my_move(&(object->S_board), INVALID, NULL, FALSE);
  }
  else
  {
    object->S_root_score = my_alpha_beta(object,
      SCORE_MINUS_INFINITY, SCORE_PLUS_INFINITY, 1, PV_BIT,
      options.reduction_depth_root,
      0, temp_pv, &temp_depth);
  }

  if (object->S_interrupt != 0)
  {
    object->S_root_score = return_my_score(&(object->S_board), arg_moves_list);

    object->S_best_score_kind = SEARCH_BEST_SCORE_AB;

    object->S_best_move = 0;

    object->S_best_depth = INVALID;

    object->S_best_pv[0] = 0;
    object->S_best_pv[1] = INVALID;

    goto label_limit;
  }

  my_printf(object->S_my_printf, "root_simple_score=%d root_score=%d\n",
    object->S_root_simple_score, object->S_root_score);

  object->S_best_score_kind = SEARCH_BEST_SCORE_AB;

  object->S_best_move = 0;

  object->S_best_depth = 0;

  object->S_best_pv[0] = 0;
  object->S_best_pv[1] = INVALID;

  if (arg_root_score == SCORE_MINUS_INFINITY)
    object->S_best_score = object->S_root_score;
  else
    object->S_best_score = arg_root_score;

  //publish work

  if ((object->S_thread != NULL) and
      (object->S_thread == thread_alpha_beta_master))
  {
    BSTRING(bmove_string)

    move2bstring(arg_moves_list, 0, bmove_string);
  
    BSTRING(bmessage)

    HARDBUG(bformata(bmessage, "%s %d %d %d %d",
      bdata(bmove_string), arg_depth_min, arg_depth_max, 
      object->S_best_score, FALSE) == BSTR_ERR)
        
    if (options.lazy_smp_policy == 0)
    {
      for (int ithread = 1; ithread < options.nthreads_alpha_beta; ithread++)
      {
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_SEARCH_FIRST, bdata(bmessage));
      }
    }
    else
    {
      for (int ithread = 1; ithread < options.nthreads_alpha_beta; ithread++)
      {
        if ((ithread % 3) == 1)
        {
          enqueue(return_thread_queue(threads + ithread),
                  MESSAGE_SEARCH_FIRST, bdata(bmessage));
        }
        else if ((ithread % 3) == 2)
        {
          enqueue(return_thread_queue(threads + ithread),
                  MESSAGE_SEARCH_AHEAD, bdata(bmessage));
        }
        else
        {
          enqueue(return_thread_queue(threads + ithread),
                  MESSAGE_SEARCH_SECOND, bdata(bmessage));
        }
      }
    }
    BDESTROY(bmessage)

    BDESTROY(bmove_string)
  }

  for (my_depth = arg_depth_min; my_depth <= arg_depth_max;
       ++my_depth)
  {
    int your_depth = my_depth - 1;

    int imove = 0;

    int jmove = sort[0];

    int my_alpha = object->S_best_score - SEARCH_WINDOW;
    int my_beta = object->S_best_score + SEARCH_WINDOW;

    do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

    int move_score = -your_alpha_beta(object,
      -my_beta, -my_alpha, your_depth, PV_BIT,
      options.reduction_depth_root,
      your_tweaks, temp_pv + 1, &temp_depth);

    undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

    if (object->S_interrupt != 0) goto label_limit;

    //update search best score so that trouble can kick in

    object->S_best_score = move_score;

    object->S_best_move = jmove;

    //with->S_best_depth = temp_depth + my_depth - your_depth;
    object->S_best_depth = my_depth;

    object->S_best_pv[0] = jmove;
    for (int ipv = 1;
         (object->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
    
    //handle fail low or fail high

    if (move_score <= my_alpha)
    {
      int window = SEARCH_WINDOW;

      while((move_score <= my_alpha) and
            (move_score >= (SCORE_LOST + NODE_MAX)))
      {
        print_my_pv(object, my_depth, 0, jmove, arg_moves_list, move_score,
                    object->S_best_pv, "<=");

        my_beta = move_score;
        my_alpha = my_beta - window;
        if (my_alpha < SCORE_MINUS_INFINITY) my_alpha = SCORE_MINUS_INFINITY;

        my_printf(object->S_my_printf,
          "PV0 fail-low my_depth=%d my_alpha=%d my_beta=%d window=%d\n",
          my_depth, my_alpha, my_beta, window);

        do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        move_score = -your_alpha_beta(object,
          -my_beta, -my_alpha, your_depth + 1, PV_BIT,
          options.reduction_depth_root,
          your_tweaks | TWEAK_PREVIOUS_MOVE_EXTENDED_BIT,
          temp_pv + 1, &temp_depth);

        undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        if (object->S_interrupt != 0) goto label_limit;

        object->S_best_score = move_score;

        object->S_best_move = jmove;

        //with->S_best_depth = temp_depth + my_depth - your_depth;
        object->S_best_depth = my_depth;

        object->S_best_pv[0] = jmove;
        for (int ipv = 1;
          (object->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
        window *= 2;
      }
    }
    else if (move_score >= my_beta)
    {
      int window = SEARCH_WINDOW;

      while((move_score >= my_beta) and
            (move_score <= (SCORE_WON - NODE_MAX)))
      {
        print_my_pv(object, my_depth, 0, jmove, arg_moves_list, move_score,
                    object->S_best_pv, ">=");

        my_alpha = move_score;
        my_beta = my_alpha + window;
        if (my_beta > SCORE_PLUS_INFINITY) my_beta = SCORE_PLUS_INFINITY;
  
        my_printf(object->S_my_printf,
          "PV0 fail-high my_alpha=%d my_beta=%d window=%d\n",
          my_alpha, my_beta, window);

        do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);
  
        move_score = -your_alpha_beta(object,
          -my_beta, -my_alpha, your_depth, PV_BIT,
          options.reduction_depth_root,
          your_tweaks, temp_pv + 1, &temp_depth);
  
        undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

        if (object->S_interrupt != 0) goto label_limit;

        object->S_best_score = move_score;
        object->S_best_move = jmove;
        //with->S_best_depth = temp_depth + my_depth - your_depth;
        object->S_best_depth = my_depth;

        object->S_best_pv[0] = jmove;

        for (int ipv = 1;
          (object->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
        window *= 2;
      }
    }

    if (object->S_interrupt != 0) goto label_limit;

    print_my_pv(object, my_depth, 0, jmove, arg_moves_list, move_score,
                object->S_best_pv, "==");

    //now search remaining moves

    imove = 1;

    while(TRUE)
    {
      if (imove >= arg_moves_list->ML_nmoves) break;

      jmove = sort[imove];

      my_alpha = object->S_best_score;
      my_beta = object->S_best_score + 1;
  
      do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      move_score = -your_alpha_beta(object,
        -my_beta, -my_alpha, your_depth, MINIMAL_WINDOW_BIT,
        options.reduction_depth_root,
        your_tweaks, temp_pv + 1, &temp_depth);

      undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

      if (object->S_interrupt != 0) goto label_limit;

      if (move_score >= my_beta)
      {
        for (int kmove = imove; kmove > 0; kmove--)
          sort[kmove] = sort[kmove - 1];

        sort[0] = jmove;

        if ((object->S_thread != NULL) and
            (object->S_thread == thread_alpha_beta_master) and
            (options.lazy_smp_policy > 0))
        {
          BSTRING(bmove_string)
    
          move2bstring(arg_moves_list, jmove, bmove_string);
      
          BSTRING(bmessage)

          HARDBUG(bformata(bmessage, "%s %d %d %d %d",
            bdata(bmove_string), arg_depth_min, arg_depth_max, 
            object->S_best_score, FALSE) == BSTR_ERR)

          for (int ithread = 1; ithread < options.nthreads_alpha_beta;
               ithread++)
          {
            if ((ithread % 3) == 2)
            {
              enqueue(return_thread_queue(threads + ithread),
                      MESSAGE_SEARCH_AHEAD, bdata(bmessage));
            }
            else if ((ithread % 3) == 0)
            {
              enqueue(return_thread_queue(threads + ithread),
                      MESSAGE_SEARCH_SECOND, bdata(bmessage));
            }
          }

          BDESTROY(bmessage)

          BDESTROY(bmove_string)
        }

        object->S_best_score = move_score;
        object->S_best_move = jmove;
        //with->S_best_depth = temp_depth + my_depth - your_depth;
        object->S_best_depth = my_depth;

        object->S_best_pv[0] = jmove;
        for (int ipv = 1;
          (object->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);

        int window = SEARCH_WINDOW;

        while((move_score >= my_beta) and
              (move_score <= (SCORE_WON - NODE_MAX)))
        {
          print_my_pv(object, my_depth, imove, jmove, arg_moves_list, move_score,
                      object->S_best_pv, ">=");

          my_alpha = move_score;
          my_beta = my_alpha + window;
          if (my_beta > SCORE_PLUS_INFINITY) my_beta = SCORE_PLUS_INFINITY;
  
          my_printf(object->S_my_printf,
            "PV1 fail-high my_alpha=%d my_beta=%d window=%d\n",
            my_alpha, my_beta, window);

          do_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

          move_score = -your_alpha_beta(object,
            -my_beta, -my_alpha, your_depth, PV_BIT,
            options.reduction_depth_root,
            your_tweaks, temp_pv + 1, &temp_depth);

          undo_my_move(&(object->S_board), jmove, arg_moves_list, FALSE);

          if (object->S_interrupt != 0) goto label_limit;

          object->S_best_score = move_score;

          object->S_best_move = jmove;

          //with->S_best_depth = temp_depth + my_depth - your_depth;
          object->S_best_depth = my_depth;

          object->S_best_pv[0] = jmove;
          for (int ipv = 1;
           (object->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);

          window *= 2;
        }
        print_my_pv(object, my_depth, imove, jmove, arg_moves_list, move_score,
                    object->S_best_pv, "==");
      }

      for (int idebug = 0; idebug < arg_moves_list->ML_nmoves; idebug++)
      {
        int ndebug = 0;
        for (int jdebug = 0; jdebug < arg_moves_list->ML_nmoves; jdebug++)
          if (sort[jdebug] == idebug) ++ndebug;
        HARDBUG(ndebug != 1)
      }

      imove++;
    }

    if (object->S_best_score > (SCORE_WON - NODE_MAX)) break;
    if (object->S_best_score < (SCORE_LOST + NODE_MAX)) break;
  }
  label_limit:

  if (object->S_thread == thread_alpha_beta_master)
  {
    if (FALSE and (object->S_best_score < -(SCORE_MAN / 4)))
    {
      BSTRING(bfen)

      board2fen(&(object->S_board), bfen, TRUE);

      BSTRING(bmove_string)

      move2bstring(arg_moves_list, object->S_best_move, bmove_string);

      my_printf(object->S_my_printf, "%s bs=%d bm=%s md=%d bd=%d\n",
        bdata(bfen),
        object->S_best_score,
        bdata(bmove_string),
        my_depth,
        object->S_best_depth);

      char stamp[MY_LINE_MAX];

      time_t t = time(NULL);

      HARDBUG(strftime(stamp, MY_LINE_MAX, "%d%b%Y-%H%M%S",
                       localtime(&t)) == 0)
 
      FILE *fbugs;
    
      HARDBUG((fbugs = fopen("bugs.txt", "a")) == NULL)
 
      fprintf(fbugs, "%s %s %s bs=%d bm=%s md=%d bd=%d\n",
        bdata(bfen), stamp, REVISION,
        object->S_best_score,
        bdata(bmove_string),
        my_depth,
        object->S_best_depth);

      FCLOSE(fbugs)

      BDESTROY(bmove_string)

      BDESTROY(bfen)
    }
  }

  stop_my_timer(&(object->S_timer));

  BSTRING(bmove_string)

  move2bstring(arg_moves_list, object->S_best_move, bmove_string),

  my_printf(object->S_my_printf, "best_move=%s best_score=%d\n",
    bdata(bmove_string),
    object->S_best_score);

  BDESTROY(bmove_string)

  my_printf(object->S_my_printf,
    "%lld nodes in %.2f seconds\n"
    "%.0f nodes/second\n",
    object->S_total_nodes, return_my_timer(&(object->S_timer), FALSE),
    object->S_total_nodes / return_my_timer(&(object->S_timer), FALSE));

  if (options.verbose > 0) print_totals(object);

  if ((object->S_thread != NULL) and
      (object->S_thread == thread_alpha_beta_master))
  {
    global_nodes = 0;

    for (int ithread = 0; ithread < options.nthreads; ithread++)
    {
      thread_t *with_thread = threads + ithread;

      global_nodes += with_thread->thread_search.S_total_nodes;
    }

    my_printf(object->S_my_printf,
      "%lld global_nodes in %.2f seconds\n"
      "%.0f global_nodes/second\n",
      global_nodes, return_my_timer(&(object->S_timer), FALSE),
      global_nodes / return_my_timer(&(object->S_timer), FALSE));
  }

  if (object->S_thread == thread_alpha_beta_master)
  {
    printf_bucket(&bucket_depth);
  }
  END_BLOCK
}
