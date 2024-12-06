//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
local void my_endgame_pv(search_t *with, int mate, bstring bpv_string)
{
  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_my_moves(&(with->S_board), &moves_list, FALSE);

  if (moves_list.nmoves == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
    }
    else
    {
      HARDBUG(bcatcstr(bpv_string, " #") != BSTR_OK)
    }

    return;
  }
  if (mate == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
    }
    else
    {
      HARDBUG(bcatcstr(bpv_string, " mate==0?") != BSTR_OK)
    }

    return;
  }
  
  int ibest_move = INVALID;  
  int best_mate = ENDGAME_UNKNOWN;

  for (int imove = 0; imove < moves_list.nmoves; imove++)
  {
    do_my_move(&(with->S_board), imove, &moves_list);

    int your_mate = read_endgame(with, YOUR_BIT, TRUE);

    undo_my_move(&(with->S_board), imove, &moves_list);

    //may be this endgame is not loaded
    if (your_mate == ENDGAME_UNKNOWN)
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
      }
      else
      {
        HARDBUG(bcatcstr(bpv_string, " not loaded") != BSTR_OK)
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

  if (best_mate != mate)
  {
    print_board(&(with->S_board));

    my_printf(with->S_my_printf, "best_mate=%d mate=%d\n",
      best_mate, mate);

    for (int imove = 0; imove < moves_list.nmoves; imove++)
    {
      do_my_move(&(with->S_board), imove, &moves_list);

      int your_mate = read_endgame(with, YOUR_BIT, TRUE);

      undo_my_move(&(with->S_board), imove, &moves_list);
    
      BSTRING(bmove_string)

      move2bstring(&moves_list, imove, bmove_string);

      my_printf(with->S_my_printf, "move=%s your_mate=%d\n",
        bdata(bmove_string), your_mate);

      BDESTROY(bmove_string)
    }
    my_printf(with->S_my_printf, "");

    FATAL("best_mate != mate", EXIT_FAILURE)
  }

  HARDBUG(bcatcstr(bpv_string, " ") != BSTR_OK)

  BSTRING(bmove_string)

  move2bstring(&moves_list, ibest_move, bmove_string);

  bconcat(bpv_string, bmove_string);

  BDESTROY(bmove_string)

  do_my_move(&(with->S_board), ibest_move, &moves_list);

  int temp_mate;

  if (mate > 0)
    temp_mate = -mate + 1;
  else
    temp_mate = -mate - 1;

  your_endgame_pv(with, temp_mate, bpv_string);

  undo_my_move(&(with->S_board), ibest_move, &moves_list);
}

local int my_quiescence(search_t *with, int my_alpha, int my_beta, 
  int node_type, moves_list_t *moves_list, pv_t *best_pv, int *best_depth,
  int all_moves)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(my_alpha >= my_beta)

  SOFTBUG(node_type == 0)

  SOFTBUG(IS_MINIMAL_WINDOW(node_type) and IS_PV(node_type))

  SOFTBUG(IS_MINIMAL_WINDOW(node_type) and ((my_beta - my_alpha) != 1))

  if ((with->S_board.board_inode - with->S_board.board_root_inode) >= 128)
  {
    my_printf(with->S_my_printf, "quiescence inode=%d\n",
      with->S_board.board_inode - with->S_board.board_root_inode);
    print_board(&(with->S_board));
    my_printf(with->S_my_printf, 
      "my_alpha=%d my_beta=%d node_type=%d\n",
      my_alpha, my_beta, node_type);
  }

  *best_pv = INVALID;

  *best_depth = INVALID;

  int best_score = SCORE_MINUS_INFINITY;

  ++(with->S_total_nodes);

  ++(with->S_total_quiescence_nodes);

  if ((with->S_total_quiescence_nodes % UPDATE_NODES) == 0)
  {
    //only thread[0] should check the time limit

    if (with->S_thread == thread_alpha_beta_master)
    {
      //Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      //struct timespec tv;

      //HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      //check for input

      if (poll_queue(return_thread_queue(with->S_thread)) != INVALID)
      {
        my_printf(with->S_my_printf, "got message\n");

        with->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }

      if (return_my_timer(&(with->S_timer), FALSE) >=
          options.time_limit) 
      {
        if ((with->S_best_score < 0) and
            ((with->S_best_score - with->S_root_score) <
             (-SCORE_MAN / 4)))
        { 
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(with->S_my_printf,
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
      if (return_my_timer(&(with->S_timer), FALSE) >=
          options.time_limit) 
      {
        with->S_interrupt = BOARD_INTERRUPT_TIME;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
    else
    {
      SOFTBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(with->S_thread)) != INVALID)
      {
        with->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(with->S_board), FALSE))
  {
    best_score = 0;

    *best_depth = 0;

    goto label_return;
  }

  //check endgame

  int temp_mate = read_endgame(with, MY_BIT, FALSE);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    *best_depth = DEPTH_MAX;

    if (temp_mate == INVALID)
    {
      best_score = 0;
 
      *best_depth = 0;
    }
    else if (temp_mate > 0)
      best_score = SCORE_WON - with->S_board.board_inode - temp_mate;
    else
      best_score = SCORE_LOST + with->S_board.board_inode - temp_mate;

    goto label_return;
  }

  moves_list_t moves_list_temp;

  if (moves_list == NULL)
  {
    construct_moves_list(&moves_list_temp);
  
    gen_my_moves(&(with->S_board), &moves_list_temp, TRUE);

    moves_list = &moves_list_temp;
  }

  if (moves_list->nmoves == 0)
  {
    best_score = SCORE_LOST + with->S_board.board_inode;

    *best_depth = DEPTH_MAX;

    goto label_return;
  }

  if (!all_moves)
  {
    if (moves_list->ncaptx > 0)
    {
      ++(with->S_total_quiescence_all_moves_captures_only);
  
      all_moves = TRUE;
    } 
    else if ((moves_list->nmoves <= 2) and !move_repetition(&(with->S_board)))
    {
      ++(with->S_total_quiescence_all_moves_le2_moves);
  
      all_moves = TRUE;
    }
    else
    {
      int nextend = 0;
  
      for (int imove = 0; imove < moves_list->nmoves; imove++)
      {
        if (MOVE_EXTEND_IN_QUIESCENCE(moves_list->moves_flag[imove]))
          nextend++;
      }
  
      if (nextend == moves_list->nmoves)
      {
        ++(with->S_total_quiescence_all_moves_extended);
  
        all_moves = TRUE;
      }
    }
  }

  if (!all_moves and IS_PV(node_type) and 
      (my_alpha >= (SCORE_LOST + NODE_MAX)) and
      (options.quiescence_extension_search_delta > 0))
  {
    with->S_total_quiescence_extension_searches++;

    int reduced_alpha = my_alpha - options.quiescence_extension_search_delta;
    int reduced_beta = reduced_alpha + 1;
   
    int temp_score;

    pv_t temp_pv[PV_MAX];

    int temp_depth;

    temp_score = my_quiescence(with, reduced_alpha, reduced_beta,
      MINIMAL_WINDOW_BIT, moves_list, temp_pv, &temp_depth, TRUE);

    if (with->S_interrupt != 0)
    {
      best_score = SCORE_PLUS_INFINITY;
    
      goto label_return;
    }

    if (temp_score <= reduced_alpha)
    {
      with->S_total_quiescence_extension_searches_le_alpha++;

      all_moves = TRUE;
    }
    else
    {
      with->S_total_quiescence_extension_searches_ge_beta++;
    }
  }

  if (!all_moves)
  {
    best_score = return_my_score(&(with->S_board), moves_list);

    *best_depth = 0;

    if (best_score >= my_beta) goto label_return;
  }


  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < moves_list->nmoves; imove++)
    moves_weight[imove] = moves_list->moves_weight[imove];

  for (int imove = 0; imove < moves_list->nmoves; imove++)
  {
    int jmove = 0;
   
    for (int kmove = 1; kmove < moves_list->nmoves; kmove++)
    {
      if (moves_weight[kmove] == L_MIN) continue;
      
      if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
    }

    SOFTBUG(moves_weight[jmove] == L_MIN)
  
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

    int temp_score = SCORE_MINUS_INFINITY;

    pv_t temp_pv[PV_MAX];

    int temp_depth;

    if (IS_MINIMAL_WINDOW(node_type))
    {
      do_my_move(&(with->S_board), jmove, moves_list);

      temp_score = -your_quiescence(with, -my_beta, -temp_alpha,
        node_type, NULL, temp_pv + 1, &temp_depth, FALSE);

      undo_my_move(&(with->S_board), jmove, moves_list);
    }
    else
    {
      if (imove == 0)
      {
        do_my_move(&(with->S_board), jmove, moves_list);

        temp_score = -your_quiescence(with, -my_beta, -temp_alpha,
          node_type, NULL, temp_pv + 1, &temp_depth, FALSE);

        undo_my_move(&(with->S_board), jmove, moves_list);
      }
      else
      {
        int temp_beta = temp_alpha + 1;

        do_my_move(&(with->S_board), jmove, moves_list);

        temp_score = -your_quiescence(with, -temp_beta, -temp_alpha,
          MINIMAL_WINDOW_BIT, NULL, temp_pv + 1, &temp_depth, FALSE);

        undo_my_move(&(with->S_board), jmove, moves_list);
  
        if ((with->S_interrupt == 0) and
            (temp_score >= temp_beta) and (temp_score < my_beta))
        {
          do_my_move(&(with->S_board), jmove, moves_list);

          temp_score = -your_quiescence(with, -my_beta, -temp_score,
            node_type, NULL, temp_pv + 1, &temp_depth, FALSE);

          undo_my_move(&(with->S_board), jmove, moves_list);
        }
      }
    }

    if (with->S_interrupt != 0)
    {
       best_score = SCORE_PLUS_INFINITY;
  
       goto label_return;
    }

    SOFTBUG(temp_score == SCORE_MINUS_INFINITY)

    if (temp_score > best_score)
    {
      best_score = temp_score;
  
      *best_pv = jmove;

      for (int ipv = 1; (best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);

      *best_depth = temp_depth;
    }
  
    if (best_score >= my_beta) break;
  } //imove

  //we always searched move 0

  SOFTBUG(best_score == SCORE_MINUS_INFINITY)
   
  SOFTBUG(*best_depth == INVALID)

  label_return:

  END_BLOCK

  if (best_score < my_alpha) best_score = my_alpha;
  return(best_score);
}

local int my_alpha_beta(search_t *with,
  int my_alpha, int my_beta, int my_depth, int node_type,
  int reduction_depth_root, int my_tweaks,
  pv_t *best_pv, int *best_depth)
{
  BEGIN_BLOCK(__FUNC__)

  SOFTBUG(my_alpha >= my_beta)

  SOFTBUG(my_depth < 0)

  SOFTBUG(node_type == 0)

  SOFTBUG(IS_MINIMAL_WINDOW(node_type) and IS_PV(node_type))

  SOFTBUG(IS_MINIMAL_WINDOW(node_type) and ((my_beta - my_alpha) != 1))

  if (with->S_thread == thread_alpha_beta_master)
  {
    update_bucket(&bucket_depth, my_depth);
  }

  if ((with->S_board.board_inode - with->S_board.board_root_inode) >= 128)
  {
    my_printf(with->S_my_printf, "alpha_beta inode=%d\n",
      with->S_board.board_inode - with->S_board.board_root_inode);
    print_board(&(with->S_board));
    my_printf(with->S_my_printf, 
      "my_alpha=%d my_beta=%d my_depth=%d node_type=%d\n",
      my_alpha, my_beta, my_depth, node_type);
  }

  *best_pv = INVALID;

  *best_depth = INVALID;

  int best_score = SCORE_PLUS_INFINITY;

  int best_move = INVALID;

  ++(with->S_total_nodes);

  ++(with->S_total_alpha_beta_nodes);

  if (IS_MINIMAL_WINDOW(node_type))
    ++(with->S_total_minimal_window_nodes);
  else if (IS_PV(node_type))
    ++(with->S_total_pv_nodes);
  else
    FATAL("unknown node_type", EXIT_FAILURE)

  if ((with->S_total_alpha_beta_nodes % UPDATE_NODES) == 0)
  {
    //only thread[0] should check the time limit

    if (with->S_thread == thread_alpha_beta_master)
    {
      //Make sure that CLOCK_THREAD_CPUTIME_ID works in the search

      //struct timespec tv;

      //HARDBUG(clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)

      //check for input

      if (poll_queue(return_thread_queue(with->S_thread)) != INVALID)
      {
        my_printf(with->S_my_printf, "got message\n");

        with->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }

      if (return_my_timer(&(with->S_timer), FALSE) >=
          options.time_limit) 
      {
        if ((with->S_best_score < 0) and
            ((with->S_best_score - with->S_root_score) <
             (-SCORE_MAN / 4)))
        { 
          for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
          {
            if (options.time_trouble[itrouble] > 0.0)
            {
              my_printf(with->S_my_printf,
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
      if (return_my_timer(&(with->S_timer), FALSE) >=
          options.time_limit) 
      {
        with->S_interrupt = BOARD_INTERRUPT_TIME;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
    else
    {
      SOFTBUG(options.nthreads <= 1)

      if (poll_queue(return_thread_queue(with->S_thread)) != INVALID)
      {
        with->S_interrupt = BOARD_INTERRUPT_MESSAGE;

        best_score = SCORE_PLUS_INFINITY;

        goto label_return;
      }
    }
  }

  if (draw_by_repetition(&(with->S_board), FALSE))
  {
    best_score = 0;

    *best_depth = 0;

    goto label_return;
  }

  //check endgame

  int temp_mate = read_endgame(with, MY_BIT, FALSE);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    *best_depth = DEPTH_MAX;

    if (temp_mate == INVALID)
    {
      best_score = 0;

      *best_depth = 0;
    }
    else if (temp_mate > 0)
      best_score = SCORE_WON - with->S_board.board_inode - temp_mate;
    else
      best_score = SCORE_LOST + with->S_board.board_inode - temp_mate;

    goto label_return;
  }

  int alpha_beta_cache_hit = FALSE;

  alpha_beta_cache_entry_t alpha_beta_cache_entry = 
    alpha_beta_cache_entry_default;

  alpha_beta_cache_slot_t *alpha_beta_cache_slot;

  if (options.alpha_beta_cache_size > 0)
  {
    alpha_beta_cache_hit = probe_alpha_beta_cache(with, node_type, FALSE,
      my_alpha_beta_cache, &alpha_beta_cache_entry, &alpha_beta_cache_slot);

    if (alpha_beta_cache_hit)
    {
      ++(with->S_total_alpha_beta_cache_hits);

      if (!IS_PV(node_type) and 
          (alpha_beta_cache_slot->ABCS_depth >= my_depth))
      {
        ++(with->S_total_alpha_beta_cache_depth_hits);

        if (alpha_beta_cache_slot->ABCS_flags & LE_ALPHA_BIT)
        {
          ++(with->S_total_alpha_beta_cache_le_alpha_hits);
  
          if (alpha_beta_cache_slot->ABCS_score <= my_alpha)
          {
            ++(with->S_total_alpha_beta_cache_le_alpha_cutoffs);
  
            best_score = alpha_beta_cache_slot->ABCS_score;

            best_pv[0] = alpha_beta_cache_slot->ABCS_moves[0];

            best_pv[1] = INVALID;
  
            *best_depth = alpha_beta_cache_slot->ABCS_depth;
   
            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot->ABCS_flags & GE_BETA_BIT)
        {
          ++(with->S_total_alpha_beta_cache_ge_beta_hits);
  
          if (alpha_beta_cache_slot->ABCS_score >= my_beta)
          {
            ++(with->S_total_alpha_beta_cache_ge_beta_cutoffs);
  
            best_score = alpha_beta_cache_slot->ABCS_score;
  
            best_pv[0] = alpha_beta_cache_slot->ABCS_moves[0];

            best_pv[1] = INVALID;

            *best_depth = alpha_beta_cache_slot->ABCS_depth;
    
            goto label_return;
          }
        }
        else if (alpha_beta_cache_slot->ABCS_flags & TRUE_SCORE_BIT)
        {
          ++(with->S_total_alpha_beta_cache_true_score_hits);
  
          best_score = alpha_beta_cache_slot->ABCS_score;
  
          best_pv[0] = alpha_beta_cache_slot->ABCS_moves[0];

          best_pv[1] = INVALID;

          *best_depth = alpha_beta_cache_slot->ABCS_depth;
  
          goto label_return;
        } 
      }
    }
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_my_moves(&(with->S_board), &moves_list, FALSE);

  if (moves_list.nmoves == 0)
  {
    best_score = SCORE_LOST + with->S_board.board_inode;

    *best_depth = DEPTH_MAX;

    goto label_return;
  }

  int moves_weight[MOVES_MAX];

  for (int imove = 0; imove < moves_list.nmoves; imove++)
    moves_weight[imove] = moves_list.moves_weight[imove];
  
  if (alpha_beta_cache_hit)
  {
    for (int imove = 0; imove < NMOVES; imove++)
    {
      int jmove = alpha_beta_cache_slot->ABCS_moves[imove];

      if (jmove == INVALID) break;

      if (jmove >= moves_list.nmoves) 
        with->S_total_alpha_beta_cache_nmoves_errors++;
      else
        moves_weight[jmove] = L_MAX - imove;
    }
  }

  best_score = SCORE_MINUS_INFINITY;

  best_move = INVALID;

  SOFTBUG(*best_depth != INVALID)

  int your_tweaks = 0;
 
  if (((moves_list.nmoves <= 2) and !move_repetition(&(with->S_board))) or
      (options.captures_are_transparent and (moves_list.ncaptx > 0)))
  {
    for (int imove = 0; imove < moves_list.nmoves; imove++)
    {
      int jmove = 0;
   
      for (int kmove = 1; kmove < moves_list.nmoves; kmove++)
      {
        if (moves_weight[kmove] == L_MIN) continue;
        
        if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
      }
  
      SOFTBUG(moves_weight[jmove] == L_MIN)
  
      moves_weight[jmove] = L_MIN;

      int temp_alpha;
      pv_t temp_pv[PV_MAX];
      int temp_depth;
  
      if (best_score < my_alpha)
        temp_alpha = my_alpha;
      else
        temp_alpha = best_score;
  
      int temp_score = SCORE_MINUS_INFINITY;

      if (IS_MINIMAL_WINDOW(node_type))
      {
        do_my_move(&(with->S_board), jmove, &moves_list);

        temp_score = -your_alpha_beta(with,
          -my_beta, -temp_alpha, my_depth, node_type,
          reduction_depth_root,
          your_tweaks,
          temp_pv + 1, &temp_depth);

        undo_my_move(&(with->S_board), jmove, &moves_list);
      }
      else
      {
        if (imove == 0)
        {
          do_my_move(&(with->S_board), jmove, &moves_list);

          temp_score = -your_alpha_beta(with,
            -my_beta, -temp_alpha, my_depth, node_type,
            reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);

          undo_my_move(&(with->S_board), jmove, &moves_list);
        } 
        else
        {
          int temp_beta = temp_alpha + 1;

          do_my_move(&(with->S_board), jmove, &moves_list);

          temp_score = -your_alpha_beta(with,
            -temp_beta, -temp_alpha, my_depth, MINIMAL_WINDOW_BIT,
            reduction_depth_root,
            your_tweaks, 
            temp_pv + 1, &temp_depth);

          undo_my_move(&(with->S_board), jmove, &moves_list);
  
          if ((with->S_interrupt == 0) and
              (temp_score >= temp_beta) and (temp_score < my_beta))
          {
            do_my_move(&(with->S_board), jmove, &moves_list);

            temp_score = -your_alpha_beta(with,
              -my_beta, -temp_score, my_depth, node_type,
              reduction_depth_root,
              your_tweaks,
              temp_pv + 1, &temp_depth);

            undo_my_move(&(with->S_board), jmove, &moves_list);
          }
        }
      }

      if (with->S_interrupt != 0)
      {
        best_score = SCORE_PLUS_INFINITY;
    
        goto label_return;
      }

      if (temp_score > best_score)
      {
        best_score = temp_score;
    
        best_move = jmove;

        *best_pv = jmove;

        for (int ipv = 1; (best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
    
        *best_depth = temp_depth;

        if (options.alpha_beta_cache_size > 0)
          update_alpha_beta_cache_slot_moves(alpha_beta_cache_slot, jmove);
    
        if (best_score >= my_beta) break;
      }
    }
    goto label_break;
  }

  SOFTBUG(((moves_list.nmoves <= 2) and !move_repetition(&(with->S_board))) or
          (options.captures_are_transparent and (moves_list.ncaptx > 0)))

  if (reduction_depth_root > 0) reduction_depth_root--;

  int your_depth = my_depth - 1;

  if (your_depth < 0)
  {
    best_score = my_quiescence(with, my_alpha, my_beta,
      node_type, &moves_list, best_pv, best_depth, FALSE);

    if (with->S_interrupt != 0) best_score = SCORE_PLUS_INFINITY;

    goto label_return;
  }

  int npassed_min = 1;

  if (IS_MINIMAL_WINDOW(node_type))
  {
    int allow_reductions = 
      options.use_reductions and
      !(my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
      !(my_tweaks & TWEAK_PREVIOUS_MOVE_NULL_BIT) and
      !(my_tweaks & TWEAK_PREVIOUS_MOVE_REDUCED_BIT) and
      !(my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
      (moves_list.ncaptx == 0) and
      (your_depth >= options.reduction_depth_leaf) and
      (reduction_depth_root == 0) and
      (my_alpha >= (SCORE_LOST + NODE_MAX)) and
      (my_beta <= (SCORE_WON - NODE_MAX));

    float factor = 1.0;

    if (allow_reductions and (options.reduction_mean > 0))
    {
      int delta = BIT_COUNT(with->S_board.my_man_bb |
                            with->S_board.your_man_bb |
                            with->S_board.my_king_bb |
                            with->S_board.your_king_bb) -
                  options.reduction_mean;

      float gamma = options.reduction_sigma;

      factor = (gamma * gamma) / (delta * delta + gamma * gamma);
//PRINTF("factor=%.6f\n", factor);
    }

    int pass[MOVES_MAX];

    for (int imove = 0; imove < moves_list.nmoves; imove++)
    {
      pass[imove] = INVALID;

      if (MOVE_DO_NOT_REDUCE(moves_list.moves_flag[imove]))
        pass[imove] = 0;
   
      //backward compatability

      pass[imove] = 0;
    }

    int npassed = 0;

    for (int ipass = -1; ipass <= 0; ++ipass)
    {
      for (int imove = 0; imove < moves_list.nmoves; imove++)
      {
        int jmove = INVALID;

        for (jmove = 0; jmove < moves_list.nmoves; jmove++)
          if (pass[jmove] == ipass) break;

        if (jmove >= moves_list.nmoves) break;

        for (int kmove = jmove + 1; kmove < moves_list.nmoves; kmove++)
        {
          if (pass[kmove] != ipass) continue;

          if (moves_weight[kmove] > moves_weight[jmove]) jmove = kmove;
        }

        SOFTBUG(jmove == INVALID)

        HARDBUG(pass[jmove] != ipass)

        int temp_score;

        pv_t temp_pv[PV_MAX];
  
        int temp_depth;

        do_my_move(&(with->S_board), jmove, &moves_list);

        if (allow_reductions and
            !(MOVE_DO_NOT_REDUCE(moves_list.moves_flag[jmove])) and
            (best_score >= (SCORE_LOST + NODE_MAX)) and
            (npassed >= npassed_min))
        {
          ++(with->S_total_reductions_delta);

          int reduced_alpha = my_alpha - options.reduction_delta;
          int reduced_beta = reduced_alpha + 1;

          int reduced_depth = 
            roundf((100 - options.reduction_max) / 100.0f * your_depth);

          HARDBUG(reduced_depth == your_depth)

          temp_score = -your_alpha_beta(with,
            -reduced_beta, -reduced_alpha, reduced_depth,
            node_type,
            reduction_depth_root,
            your_tweaks | TWEAK_PREVIOUS_MOVE_REDUCED_BIT,
            temp_pv + 1, &temp_depth);

          if (with->S_interrupt != 0)
          {
            undo_my_move(&(with->S_board), jmove, &moves_list);

            best_score = SCORE_PLUS_INFINITY;
              
            goto label_return;
          }
  
          if (temp_score < (SCORE_LOST + NODE_MAX))
          {
            ++(with->S_total_reductions_delta_lost);

            pass[jmove] = 1;
          }
          else
          {
            with->S_total_reductions++;

            int percentage = INVALID;

            if (temp_score <= reduced_alpha)
            {
              ++(with->S_total_reductions_delta_le_alpha);

              percentage = roundf(options.reduction_strong * factor);
            }
            else
            { 
              SOFTBUG(!(temp_score >= reduced_beta))

              ++(with->S_total_reductions_delta_ge_beta);

              percentage = roundf(options.reduction_weak * factor);
            }

            if (percentage < options.reduction_min)
              percentage = options.reduction_min;

            reduced_depth = roundf((100 - percentage) / 100.0f * your_depth);

            if (reduced_depth < your_depth)
            {
              temp_score = -your_alpha_beta(with,
                -my_beta, -my_alpha, reduced_depth,
                node_type,
                reduction_depth_root,
                your_tweaks | TWEAK_PREVIOUS_MOVE_REDUCED_BIT,
                temp_pv + 1, &temp_depth);

              if (with->S_interrupt != 0)
              {
                undo_my_move(&(with->S_board), jmove, &moves_list);
    
                best_score = SCORE_PLUS_INFINITY;
                  
                goto label_return;
              }

              if (temp_score <= best_score)
              {
                with->S_total_reductions_le_alpha++;

                pass[jmove] = 1;
              }
              else
              {
                with->S_total_reductions_ge_beta++;
              }
            }
          }//if (temp_score < (SCORE_LOST + NODE+MAX))
        }//if (!allow_reduction   

        if (pass[jmove] != 1)
        {
          temp_score = -your_alpha_beta(with,
            -my_beta, -my_alpha, your_depth,
            node_type,
            reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);
 
          pass[jmove] = 1;

          npassed++;
        }

        undo_my_move(&(with->S_board), jmove, &moves_list);

        if (with->S_interrupt != 0)
        {
          best_score = SCORE_PLUS_INFINITY;
        
          goto label_return;
        }

        SOFTBUG(temp_score == SCORE_MINUS_INFINITY)

        if (temp_score > best_score)
        {
          best_score = temp_score;
        
          best_move = jmove;
        
          *best_pv = jmove;
  
          for (int ipv = 1; (best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
          if ((options.returned_depth_includes_captures or
              (moves_list.ncaptx == 0)) and (temp_depth < (DEPTH_MAX - 1)))
          {
            temp_depth++;
          }
  
          *best_depth = temp_depth;

          if (options.alpha_beta_cache_size > 0)
            update_alpha_beta_cache_slot_moves(alpha_beta_cache_slot, jmove);

          if (best_score >= my_beta) goto label_break;
        }
      }//for (int imove
    }//for (int ipass
  }
  else //IS_MINIMAL_WINDOW(node_type) NEW
  {
    //check if position should be extended

    //HARDBUG(my_tweaks & TWEAK_PREVIOUS_MOVE_NULL_BIT)
    HARDBUG(my_tweaks & TWEAK_PREVIOUS_MOVE_REDUCED_BIT)

    if (!(my_tweaks & TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT) and
        !(my_tweaks & TWEAK_PREVIOUS_MOVE_EXTENDED_BIT) and
        (options.pv_extension_search_delta > 0) and
        (my_alpha >= (SCORE_LOST + NODE_MAX)))
    {
      with->S_total_pv_extension_searches++;

      int reduced_alpha = my_alpha - options.pv_extension_search_delta;
      int reduced_beta = reduced_alpha + 1;
  
      int reduced_depth = your_depth;

      pv_t temp_pv[PV_MAX];
  
      int temp_depth;
      
      int temp_score = my_alpha_beta(with,
        reduced_alpha, reduced_beta, reduced_depth, MINIMAL_WINDOW_BIT,
        options.reduction_depth_root,
        my_tweaks | TWEAK_PREVIOUS_SEARCH_EXTENDED_BIT,
        temp_pv, &temp_depth);
  
      if (with->S_interrupt != 0)
      {
        best_score = SCORE_PLUS_INFINITY;
      
        goto label_return;
      }
  
      if (temp_score <= reduced_alpha)
      {
        with->S_total_pv_extension_searches_le_alpha++;

        your_depth = your_depth + 1;

        your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;
      }
      else
      {
        with->S_total_pv_extension_searches_ge_beta++;
      }
    }

    for (int ipass = 1; ipass <= 2; ++ipass)
    {
      int nsingle_replies = 0;

      for (int imove = 0; imove < moves_list.nmoves; imove++)
      {
        int jmove = 0;
      
        for (int kmove = 1; kmove < moves_list.nmoves; kmove++)
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
      
        if (best_score < my_alpha)
          temp_alpha = my_alpha;
        else
          temp_alpha = best_score;

        int temp_score = SCORE_MINUS_INFINITY;
  
        pv_t temp_pv[PV_MAX];
  
        int temp_depth;
  
        if (imove == 0)
        {
          do_my_move(&(with->S_board), jmove, &moves_list);
  
          temp_score = -your_alpha_beta(with,
            -my_beta, -temp_alpha, your_depth, node_type,
            reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);
  
          undo_my_move(&(with->S_board), jmove, &moves_list);
        }
        else
        {
          int temp_beta = temp_alpha + 1;
  
          do_my_move(&(with->S_board), jmove, &moves_list);
  
          temp_score = -your_alpha_beta(with,
            -temp_beta, -temp_alpha, your_depth,
            MINIMAL_WINDOW_BIT,
            reduction_depth_root,
            your_tweaks,
            temp_pv + 1, &temp_depth);
  
          undo_my_move(&(with->S_board), jmove, &moves_list);
  
          if ((with->S_interrupt == 0) and
              (temp_score >= temp_beta) and (temp_score < my_beta))
          {
            do_my_move(&(with->S_board), jmove, &moves_list);

            temp_score = -your_alpha_beta(with,
              -my_beta, -temp_score, your_depth, node_type,
              reduction_depth_root,
              your_tweaks,
              temp_pv + 1, &temp_depth);
  
            undo_my_move(&(with->S_board), jmove, &moves_list);
          }
        }
  
        if (with->S_interrupt != 0)
        {
          best_score = SCORE_PLUS_INFINITY;
    
          goto label_return;
        }
    
        SOFTBUG(temp_score == SCORE_MINUS_INFINITY)
  
        if (temp_score > my_alpha)
        {
          SOFTBUG(!IS_PV(node_type))
  
          nsingle_replies++;
        }
      
        if (temp_score > best_score)
        {
          best_score = temp_score;
      
          best_move = jmove;
      
          *best_pv = jmove;

          for (int ipv = 1; (best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
          if ((options.returned_depth_includes_captures or
              (moves_list.ncaptx == 0)) and (temp_depth < (DEPTH_MAX - 1)))
          {
            temp_depth++;
          }
  
          *best_depth = temp_depth;
        
          if (options.alpha_beta_cache_size > 0)
            update_alpha_beta_cache_slot_moves(alpha_beta_cache_slot, jmove);
  
          if (best_score >= my_beta) goto label_break;
        }
      } //imove

      if ((ipass == 1) and
          (options.use_single_reply_extensions) and
          (nsingle_replies == 1))
      {
        with->S_total_single_reply_extensions++;

        *best_pv = INVALID;

        *best_depth = INVALID;

        best_score = SCORE_MINUS_INFINITY;

        your_depth = your_depth + 1;

        your_tweaks |= TWEAK_PREVIOUS_MOVE_EXTENDED_BIT;

        for (int imove = 0; imove < moves_list.nmoves; imove++)
          moves_weight[imove] = moves_list.moves_weight[imove];

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
   
  SOFTBUG(*best_depth == INVALID)

  SOFTBUG(*best_depth > DEPTH_MAX)

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

    alpha_beta_cache_slot->ABCS_depth = *best_depth;
  
    if (best_score <= my_alpha)
    {
      ++(with->S_total_alpha_beta_cache_le_alpha_stored);

      if (best_score >= (SCORE_LOST + NODE_MAX))
        alpha_beta_cache_slot->ABCS_score = my_alpha;

      alpha_beta_cache_slot->ABCS_flags = LE_ALPHA_BIT;
    }
    else if (best_score >= my_beta)
    {
      ++(with->S_total_alpha_beta_cache_ge_beta_stored);

      if (best_score <= (SCORE_WON - NODE_MAX))
        alpha_beta_cache_slot->ABCS_score = my_beta;

      alpha_beta_cache_slot->ABCS_flags = GE_BETA_BIT;
    }
    else
    {
      ++(with->S_total_alpha_beta_cache_true_score_stored);

      alpha_beta_cache_slot->ABCS_flags = TRUE_SCORE_BIT;
    }
 
    alpha_beta_cache_slot->ABCS_key = with->S_board.board_key;

    ui32_t crc32 = 0xFFFFFFFF;
    crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_key);
    crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_data);
    alpha_beta_cache_slot->ABCS_crc32 = ~crc32;
  
    if (IS_PV(node_type))
    {
      my_alpha_beta_cache[with->S_board.board_key %
                          nalpha_beta_pv_cache_entries] =
        alpha_beta_cache_entry;
    }
    else
    {
      my_alpha_beta_cache[nalpha_beta_pv_cache_entries + 
                          with->S_board.board_key %
                          nalpha_beta_cache_entries] = 
        alpha_beta_cache_entry;
    }
  }

  label_return:

  END_BLOCK

  if (best_score < my_alpha) best_score = my_alpha;
  return(best_score);
}

void my_pv(search_t *with, pv_t *best_pv, int pv_score, bstring bpv_string)
{
  if (draw_by_repetition(&(with->S_board), FALSE))
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
    }
    else 
    {
      HARDBUG(bcatcstr(bpv_string, " rep") != BSTR_OK)
    }

    return;
  }
  
  int temp_mate = read_endgame(with, MY_BIT, FALSE);

  if (temp_mate != ENDGAME_UNKNOWN)
  {
    temp_mate = read_endgame(with, MY_BIT, TRUE);

    if (temp_mate == INVALID)
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
      }
      else
      {
        HARDBUG(bcatcstr(bpv_string, " draw") != BSTR_OK)
      }
    }
    else if (temp_mate > 0)
    {
      if (!options.hub)
      {
        HARDBUG(bformata(bpv_string, " won in %d", temp_mate) != BSTR_OK)
      }
      my_endgame_pv(with, temp_mate, bpv_string);
    }
    else
    {
      if (!options.hub)
      {
        HARDBUG(bformata(bpv_string, " lost in %d", -temp_mate) != BSTR_OK)
      }
      my_endgame_pv(with, temp_mate, bpv_string);
    }
    return;
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_my_moves(&(with->S_board), &moves_list, FALSE);

  if (moves_list.nmoves == 0)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
    }
    else
    {
      HARDBUG(bcatcstr(bpv_string, " #") != BSTR_OK)
    }

    return;
  }

  int jmove = *best_pv;

  if (jmove == INVALID)
  {
    if (options.hub)
    {
      HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
    }
    else
    {
      int temp_score;

      temp_score = return_my_score(&(with->S_board), &moves_list);
    
      HARDBUG(bformata(bpv_string, " %d", temp_score) != BSTR_OK)

      if (temp_score != pv_score)
      {
        HARDBUG(bcatcstr(bpv_string, " PV ERROR") != BSTR_OK)
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

      (void) probe_alpha_beta_cache(with, PV_BIT, TRUE,
        my_alpha_beta_cache, &alpha_beta_cache_entry, &alpha_beta_cache_slot);

      *alpha_beta_cache_slot = alpha_beta_cache_slot_default;

      alpha_beta_cache_slot->ABCS_moves[0] = jmove;

      alpha_beta_cache_slot->ABCS_key = with->S_board.board_key;

      ui32_t crc32 = 0xFFFFFFFF;
      crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_key);
      crc32 = _mm_crc32_u64(crc32, alpha_beta_cache_slot->ABCS_data);
      alpha_beta_cache_slot->ABCS_crc32 = ~crc32;

      my_alpha_beta_cache[with->S_board.board_key % 
                          nalpha_beta_pv_cache_entries] =
        alpha_beta_cache_entry;
    }

    BSTRING(bmove_string)

    move2bstring(&moves_list, jmove, bmove_string);

    HARDBUG(bcatcstr(bpv_string, " ") != BSTR_OK)

    HARDBUG(bconcat(bpv_string, bmove_string) != BSTR_OK)
  
    BDESTROY(bmove_string)

    do_my_move(&(with->S_board), jmove, &moves_list);
  
    your_pv(with, best_pv + 1, -pv_score, bpv_string);

    undo_my_move(&(with->S_board), jmove, &moves_list);
  }
}

local void print_my_pv(search_t *with, int my_depth,
  int imove, int jmove, moves_list_t *moves_list,
  int move_score, pv_t *best_pv, char *move_status)
{
  BSTRING(bmove_string)

  move2bstring(moves_list, jmove, bmove_string);

  BSTRING(bpv_string)

  if (options.hub)
  {
    if (options.nthreads == INVALID)
      global_nodes = with->S_total_nodes;
    else
    {
      global_nodes = 0;

      for (int ithread = 0; ithread < options.nthreads; ithread++)
      {
        thread_t *object = threads + ithread;

        global_nodes += object->thread_search.S_total_nodes;
      }
    }
    bformata(bpv_string, 
      "depth=%d score=%.3f time=%.2f nps=%.2f pv=\"%s",
      my_depth,
      (float) move_score / (float) SCORE_MAN,
      return_my_timer(&(with->S_timer), FALSE),
      global_nodes / return_my_timer(&(with->S_timer), TRUE) / 1000000.0,
      bdata(bmove_string));
  }
  else
  {
    bformata(bpv_string,
      "d=%d i=%d j=%d m=%s s=%d (%.2f) r=%d t=%.2f n=%lld/%lld/%lld x=%s pv=%s",
      my_depth,
      imove,
      jmove,
      bdata(bmove_string),
      move_score,
      (float) move_score / (float) SCORE_MAN,
      with->S_root_score,
      return_my_timer(&(with->S_timer), FALSE),
      with->S_total_nodes,
      with->S_total_alpha_beta_nodes,
      with->S_total_quiescence_nodes,
      move_status,
      bdata(bmove_string));
  }

  do_my_move(&(with->S_board), jmove, moves_list);

  your_pv(with, best_pv + 1, -move_score, bpv_string);

  undo_my_move(&(with->S_board), jmove, moves_list);

  my_printf(with->S_my_printf, "%s\n", bdata(bpv_string));

  //flush

  if (with->S_thread == NULL) my_printf(with->S_my_printf, "");

  if (options.mode == GAME_MODE)
  {
    if ((with->S_thread != NULL) and
        (with->S_thread == thread_alpha_beta_master))
      enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string));
  }

  BDESTROY(bpv_string)

  BDESTROY(bmove_string)
}

void my_search(search_t *with, moves_list_t *moves_list,
  int depth_min, int depth_max, int root_score,
  my_random_t *shuffle)
{
  BEGIN_BLOCK(__FUNC__)

  my_printf(with->S_my_printf, "time_limit=%.2f\n", options.time_limit);

  my_printf(with->S_my_printf, "time_ntrouble=%d\n", 
    options.time_ntrouble);

  for (int itrouble = 0; itrouble < options.time_ntrouble; itrouble++)
    my_printf(with->S_my_printf, "itrouble=%d time_trouble=%.2f\n",
            itrouble, options.time_trouble[itrouble]);

  if (with->S_thread == thread_alpha_beta_master)
  {
    clear_bucket(&bucket_depth);
  }

  int my_depth = INVALID;

  SOFTBUG(moves_list->nmoves == 0)

  if (depth_min == INVALID) depth_min = 1;
  if (depth_max == INVALID) depth_max = options.depth_limit;

  with->S_interrupt = 0;

  with->S_board.board_root_inode = with->S_board.board_inode;

  clear_totals(with);

  reset_my_timer(&(with->S_timer));

  int sort[MOVES_MAX];

  for (int imove = 0; imove < moves_list->nmoves; imove++)
    sort[imove] = imove;

  if (shuffle != NULL)
  { 
    for (int imove = moves_list->nmoves - 1; imove >= 1; --imove)
    {
      int jmove = return_my_random(shuffle) % (imove + 1);
      if (jmove != imove)
      {
        int temp = sort[imove];
        sort[imove] = sort[jmove];
        sort[jmove] = temp;
      }
    }
    for (int idebug = 0; idebug < moves_list->nmoves; idebug++)
    {
      int ndebug = 0;
      for (int jdebug = 0; jdebug < moves_list->nmoves; jdebug++)
        if (sort[jdebug] == idebug) ++ndebug;
      HARDBUG(ndebug != 1)
    }

    my_printf(with->S_my_printf, "shuffle nmoves=%d",
      moves_list->nmoves);

    for (int imove = 0; imove < moves_list->nmoves; imove++)
      my_printf(with->S_my_printf, " %d", sort[imove]);

    my_printf(with->S_my_printf, "\n");
  }

  //check for known endgame

  int egtb_mate = read_endgame(with, MY_BIT, TRUE);

  if (egtb_mate != ENDGAME_UNKNOWN)
  {
    print_board(&(with->S_board));

    my_printf(with->S_my_printf, "known endgame egtb_mate=%d\n",
      egtb_mate);

    with->S_best_score = SCORE_MINUS_INFINITY;

    int your_mate = INVALID;

    for (int imove = 0; imove < moves_list->nmoves; imove++)
    {
      int jmove = sort[imove];

      do_my_move(&(with->S_board), jmove, moves_list);

      ++(with->S_total_nodes);

      int temp_mate = read_endgame(with, YOUR_BIT, TRUE);

      SOFTBUG(temp_mate == ENDGAME_UNKNOWN)

      int temp_score;

      if (temp_mate == INVALID)
        temp_score = 0;
      else if (temp_mate > 0)
        temp_score = SCORE_WON - with->S_board.board_inode - temp_mate;
      else
        temp_score = SCORE_LOST + with->S_board.board_inode - temp_mate;

      undo_my_move(&(with->S_board), jmove, moves_list);

      int move_score = -temp_score;

      BSTRING(bmove_string)
  
      move2bstring(moves_list, imove, bmove_string);

      my_printf(with->S_my_printf, "%s %d\n", bdata(bmove_string),
                move_score);

      BDESTROY(bmove_string)

      if (move_score > with->S_best_score)
      {
        for (int kmove = imove; kmove > 0; kmove--)
          sort[kmove] = sort[kmove - 1];

        sort[0] = jmove;

        with->S_best_score = move_score;

        with->S_best_move = jmove;

        with->S_best_pv[0] = jmove;
        with->S_best_pv[1] = INVALID;

        your_mate = temp_mate;
      }
#ifdef DEBUG
      for (int idebug = 0; idebug < moves_list->nmoves; idebug++)
      {
        int ndebug = 0;
        for (int jdebug = 0; jdebug < moves_list->nmoves; jdebug++)
          if (sort[jdebug] == idebug) ++ndebug;
        HARDBUG(ndebug != 1)
      }
#endif
    }

    SOFTBUG(with->S_best_score == SCORE_MINUS_INFINITY)

    with->S_best_score_kind = SEARCH_BEST_SCORE_EGTB;

    with->S_best_depth = 0;

    BSTRING(bmove_string)
    
    move2bstring(moves_list, sort[0], bmove_string);

    BSTRING(bpv_string)

    if (options.hub)
    {
      HARDBUG(bformata(bpv_string, "depth=127 score=%d nps=0.00 pv=\"%s",
        with->S_best_score, bdata(bmove_string)) != BSTR_OK)
    }
    else
    {
      HARDBUG(bformata(bpv_string, "s=%d (%d) x=egtb pv=%s ",
        with->S_best_score,
        egtb_mate, bdata(bmove_string)) != BSTR_OK)
    }

    if (your_mate != INVALID)
    {
      do_my_move(&(with->S_board), sort[0], moves_list);

      your_endgame_pv(with, your_mate, bpv_string);

      undo_my_move(&(with->S_board), sort[0], moves_list);
    } 
    else
    {
      if (options.hub)
      {
        HARDBUG(bcatcstr(bpv_string, "\"") != BSTR_OK)
      }
    }

    my_printf(with->S_my_printf, "%s\n", bdata(bpv_string));

    if (options.mode == GAME_MODE)
    {
      if ((with->S_thread != NULL) and
          (with->S_thread == thread_alpha_beta_master))
        enqueue(&main_queue, MESSAGE_INFO, bdata(bpv_string)); 
    }

    BDESTROY(bpv_string)

    BDESTROY(bmove_string)

    goto label_limit;
  }

  int your_tweaks = 0;
 
  //RE-INITIALIZE ACCUMULATOR

  return_network_score_scaled(&(with->S_board.board_network), FALSE, FALSE);

  with->S_root_simple_score = return_my_score(&(with->S_board), moves_list);

  pv_t temp_pv[PV_MAX];

  int temp_depth;

  if (FALSE and (moves_list->ncaptx == 0) and (moves_list->nmoves > 2))
  { 
    do_my_move(&(with->S_board), INVALID, NULL);

    with->S_root_score = -your_alpha_beta(with,
      SCORE_MINUS_INFINITY, SCORE_PLUS_INFINITY, 1, PV_BIT,
      options.reduction_depth_root,
      TWEAK_PREVIOUS_MOVE_NULL_BIT, temp_pv + 1, &temp_depth);

    undo_my_move(&(with->S_board), INVALID, NULL);
  }
  else
  {
    with->S_root_score = my_alpha_beta(with,
      SCORE_MINUS_INFINITY, SCORE_PLUS_INFINITY, 1, PV_BIT,
      options.reduction_depth_root,
      0, temp_pv, &temp_depth);
  }

  if (with->S_interrupt != 0)
  {
    with->S_root_score = return_my_score(&(with->S_board), moves_list);

    with->S_best_score_kind = SEARCH_BEST_SCORE_AB;

    with->S_best_move = 0;

    with->S_best_depth = INVALID;

    with->S_best_pv[0] = 0;
    with->S_best_pv[1] = INVALID;

    goto label_limit;
  }

  my_printf(with->S_my_printf, "root_simple_score=%d root_score=%d\n",
    with->S_root_simple_score, with->S_root_score);

  with->S_best_score_kind = SEARCH_BEST_SCORE_AB;

  with->S_best_move = 0;

  with->S_best_depth = 0;

  with->S_best_pv[0] = 0;
  with->S_best_pv[1] = INVALID;

  if (root_score == SCORE_MINUS_INFINITY)
    with->S_best_score = with->S_root_score;
  else
    with->S_best_score = root_score;

  //publish work

  if ((with->S_thread != NULL) and
      (with->S_thread == thread_alpha_beta_master))
  {
    BSTRING(bmove_string)

    move2bstring(moves_list, 0, bmove_string);
  
    BSTRING(bmessage)

    HARDBUG(bformata(bmessage, "%s %d %d %d %d",
      bdata(bmove_string), depth_min, depth_max, 
      with->S_best_score, FALSE) != BSTR_OK)
        
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

  for (my_depth = depth_min; my_depth <= depth_max;
       ++my_depth)
  {
    int your_depth = my_depth - 1;

    int imove = 0;

    int jmove = sort[0];

    int my_alpha = with->S_best_score - SEARCH_WINDOW;
    int my_beta = with->S_best_score + SEARCH_WINDOW;

    do_my_move(&(with->S_board), jmove, moves_list);

    int move_score = -your_alpha_beta(with,
      -my_beta, -my_alpha, your_depth, PV_BIT,
      options.reduction_depth_root,
      your_tweaks, temp_pv + 1, &temp_depth);

    undo_my_move(&(with->S_board), jmove, moves_list);

    if (with->S_interrupt != 0) goto label_limit;

    //update search best score so that trouble can kick in

    with->S_best_score = move_score;

    with->S_best_move = jmove;

    //with->S_best_depth = temp_depth + my_depth - your_depth;
    with->S_best_depth = my_depth;

    with->S_best_pv[0] = jmove;
    for (int ipv = 1;
         (with->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
    
    //handle fail low or fail high

    if (move_score <= my_alpha)
    {
      int window = SEARCH_WINDOW;

      while((move_score <= my_alpha) and
            (move_score >= (SCORE_LOST + NODE_MAX)))
      {
        print_my_pv(with, my_depth, 0, jmove, moves_list, move_score,
                    with->S_best_pv, "<=");

        my_beta = move_score;
        my_alpha = my_beta - window;
        if (my_alpha < SCORE_MINUS_INFINITY) my_alpha = SCORE_MINUS_INFINITY;

        my_printf(with->S_my_printf,
          "PV0 fail-low my_depth=%d my_alpha=%d my_beta=%d window=%d\n",
          my_depth, my_alpha, my_beta, window);

        do_my_move(&(with->S_board), jmove, moves_list);

        move_score = -your_alpha_beta(with,
          -my_beta, -my_alpha, your_depth + 1, PV_BIT,
          options.reduction_depth_root,
          your_tweaks | TWEAK_PREVIOUS_MOVE_EXTENDED_BIT,
          temp_pv + 1, &temp_depth);

        undo_my_move(&(with->S_board), jmove, moves_list);

        if (with->S_interrupt != 0) goto label_limit;

        with->S_best_score = move_score;

        with->S_best_move = jmove;

        //with->S_best_depth = temp_depth + my_depth - your_depth;
        with->S_best_depth = my_depth;

        with->S_best_pv[0] = jmove;
        for (int ipv = 1;
          (with->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
        window *= 2;
      }
    }
    else if (move_score >= my_beta)
    {
      int window = SEARCH_WINDOW;

      while((move_score >= my_beta) and
            (move_score <= (SCORE_WON - NODE_MAX)))
      {
        print_my_pv(with, my_depth, 0, jmove, moves_list, move_score,
                    with->S_best_pv, ">=");

        my_alpha = move_score;
        my_beta = my_alpha + window;
        if (my_beta > SCORE_PLUS_INFINITY) my_beta = SCORE_PLUS_INFINITY;
  
        my_printf(with->S_my_printf,
          "PV0 fail-high my_alpha=%d my_beta=%d window=%d\n",
          my_alpha, my_beta, window);

        do_my_move(&(with->S_board), jmove, moves_list);
  
        move_score = -your_alpha_beta(with,
          -my_beta, -my_alpha, your_depth, PV_BIT,
          options.reduction_depth_root,
          your_tweaks, temp_pv + 1, &temp_depth);
  
        undo_my_move(&(with->S_board), jmove, moves_list);

        if (with->S_interrupt != 0) goto label_limit;

        with->S_best_score = move_score;
        with->S_best_move = jmove;
        //with->S_best_depth = temp_depth + my_depth - your_depth;
        with->S_best_depth = my_depth;

        with->S_best_pv[0] = jmove;

        for (int ipv = 1;
          (with->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);
  
        window *= 2;
      }
    }

    if (with->S_interrupt != 0) goto label_limit;

    print_my_pv(with, my_depth, 0, jmove, moves_list, move_score,
                with->S_best_pv, "==");

    //now search remaining moves

    imove = 1;

    while(TRUE)
    {
      if (imove >= moves_list->nmoves) break;

      jmove = sort[imove];

      my_alpha = with->S_best_score;
      my_beta = with->S_best_score + 1;
  
      do_my_move(&(with->S_board), jmove, moves_list);

      move_score = -your_alpha_beta(with,
        -my_beta, -my_alpha, your_depth, MINIMAL_WINDOW_BIT,
        options.reduction_depth_root,
        your_tweaks, temp_pv + 1, &temp_depth);

      undo_my_move(&(with->S_board), jmove, moves_list);

      if (with->S_interrupt != 0) goto label_limit;

      if (move_score >= my_beta)
      {
        for (int kmove = imove; kmove > 0; kmove--)
          sort[kmove] = sort[kmove - 1];

        sort[0] = jmove;

        if ((with->S_thread != NULL) and
            (with->S_thread == thread_alpha_beta_master) and
            (options.lazy_smp_policy > 0))
        {
          BSTRING(bmove_string)
    
          move2bstring(moves_list, jmove, bmove_string);
      
          BSTRING(bmessage)

          HARDBUG(bformata(bmessage, "%s %d %d %d %d",
            bdata(bmove_string), depth_min, depth_max, 
            with->S_best_score, FALSE) != BSTR_OK)

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

        with->S_best_score = move_score;
        with->S_best_move = jmove;
        //with->S_best_depth = temp_depth + my_depth - your_depth;
        with->S_best_depth = my_depth;

        with->S_best_pv[0] = jmove;
        for (int ipv = 1;
          (with->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);

        int window = SEARCH_WINDOW;

        while((move_score >= my_beta) and
              (move_score <= (SCORE_WON - NODE_MAX)))
        {
          print_my_pv(with, my_depth, imove, jmove, moves_list, move_score,
                      with->S_best_pv, ">=");

          my_alpha = move_score;
          my_beta = my_alpha + window;
          if (my_beta > SCORE_PLUS_INFINITY) my_beta = SCORE_PLUS_INFINITY;
  
          my_printf(with->S_my_printf,
            "PV1 fail-high my_alpha=%d my_beta=%d window=%d\n",
            my_alpha, my_beta, window);

          do_my_move(&(with->S_board), jmove, moves_list);

          move_score = -your_alpha_beta(with,
            -my_beta, -my_alpha, your_depth, PV_BIT,
            options.reduction_depth_root,
            your_tweaks, temp_pv + 1, &temp_depth);

          undo_my_move(&(with->S_board), jmove, moves_list);

          if (with->S_interrupt != 0) goto label_limit;

          with->S_best_score = move_score;

          with->S_best_move = jmove;

          //with->S_best_depth = temp_depth + my_depth - your_depth;
          with->S_best_depth = my_depth;

          with->S_best_pv[0] = jmove;
          for (int ipv = 1;
           (with->S_best_pv[ipv] = temp_pv[ipv]) != INVALID; ipv++);

          window *= 2;
        }
        print_my_pv(with, my_depth, imove, jmove, moves_list, move_score,
                    with->S_best_pv, "==");
      }

      for (int idebug = 0; idebug < moves_list->nmoves; idebug++)
      {
        int ndebug = 0;
        for (int jdebug = 0; jdebug < moves_list->nmoves; jdebug++)
          if (sort[jdebug] == idebug) ++ndebug;
        HARDBUG(ndebug != 1)
      }

      imove++;
    }

    if (with->S_best_score > (SCORE_WON - NODE_MAX)) break;
    if (with->S_best_score < (SCORE_LOST + NODE_MAX)) break;
  }
  label_limit:

  if (with->S_thread == thread_alpha_beta_master)
  {
    if (FALSE and (with->S_best_score < -(SCORE_MAN / 4)))
    {
      BSTRING(bfen)

      board2fen(with->S_board.board_colour2move,
        with->S_board.board_white_man_bb,
        with->S_board.board_black_man_bb,
        with->S_board.board_white_king_bb,
        with->S_board.board_black_king_bb,
        bfen, TRUE);

      BSTRING(bmove_string)

      move2bstring(moves_list, with->S_best_move, bmove_string);

      my_printf(with->S_my_printf, "%s bs=%d bm=%s md=%d bd=%d\n",
        bdata(bfen),
        with->S_best_score,
        bdata(bmove_string),
        my_depth,
        with->S_best_depth);

      char stamp[MY_LINE_MAX];

      time_t t = time(NULL);

      HARDBUG(strftime(stamp, MY_LINE_MAX, "%d%b%Y-%H%M%S",
                       localtime(&t)) == 0)
 
      FILE *fbugs;
    
      HARDBUG((fbugs = fopen("bugs.txt", "a")) == NULL)
 
      fprintf(fbugs, "%s %s %s bs=%d bm=%s md=%d bd=%d\n",
        bdata(bfen), stamp, REVISION,
        with->S_best_score,
        bdata(bmove_string),
        my_depth,
        with->S_best_depth);

      FCLOSE(fbugs)

      BDESTROY(bmove_string)

      BDESTROY(bfen)
    }
  }

  stop_my_timer(&(with->S_timer));

  BSTRING(bmove_string)

  move2bstring(moves_list, with->S_best_move, bmove_string),

  my_printf(with->S_my_printf, "best_move=%s best_score=%d\n",
    bdata(bmove_string),
    with->S_best_score);

  BDESTROY(bmove_string)

  my_printf(with->S_my_printf,
    "%lld nodes in %.2f seconds\n"
    "%.0f nodes/second\n",
    with->S_total_nodes, return_my_timer(&(with->S_timer), FALSE),
    with->S_total_nodes / return_my_timer(&(with->S_timer), FALSE));

  if (options.verbose > 0) print_totals(with);

  if ((with->S_thread != NULL) and
      (with->S_thread == thread_alpha_beta_master))
  {
    global_nodes = 0;

    for (int ithread = 0; ithread < options.nthreads; ithread++)
    {
      thread_t *object = threads + ithread;

      global_nodes += object->thread_search.S_total_nodes;
    }

    my_printf(with->S_my_printf,
      "%lld global_nodes in %.2f seconds\n"
      "%.0f global_nodes/second\n",
      global_nodes, return_my_timer(&(with->S_timer), FALSE),
      global_nodes / return_my_timer(&(with->S_timer), FALSE));
  }

  if (with->S_thread == thread_alpha_beta_master)
  {
    printf_bucket(&bucket_depth);
  }
  END_BLOCK
}
