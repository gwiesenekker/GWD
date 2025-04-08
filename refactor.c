//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
local int add_man_random(int arg_nwhite_man, int arg_nblack_man,
  int arg_shape, int arg_scale, my_random_t *arg_random,
  search_t *arg_with, char *arg_s, int *arg_best_score)
{
  int result = FALSE;

  int ntry = 0;

  my_timer_t timer;

  construct_my_timer(&timer, "timer", STDOUT, FALSE);

  reset_my_timer(&timer);

  while(TRUE)
  {
    ++ntry;

    for (int i = 1; i <= 50; ++i) arg_s[i] = *nn;

    for (int iwhite_man = 1; iwhite_man <= arg_nwhite_man;
         ++iwhite_man)
    {
      int i = INVALID;

      while(TRUE)
      {
        double probability = return_my_random(arg_random) /
                             (double) ULL_MAX;

        int irow = 9 - weibull_sample_row(probability, arg_shape, arg_scale);
        int icol = return_my_random(arg_random) % 5;

        i = irow * 5 + icol + 1;

        HARDBUG(i > 50)

        if (s[i] == *nn) break;
      }
 
      HARDBUG(i == INVALID)

      s[i] = *wO;
    }
  
    for (int iblack_man = 1; iblack_man <= arg_nblack_man;
         ++iblack_man)
    {
      int i = INVALID;

      while(TRUE)
      {
        double probability = (return_my_random(arg_random) /
                             (double) ULL_MAX);

        int irow = weibull_sample_row(probability, arg_shape, arg_scale);
        int icol = return_my_random(arg_random) % 5;
 
        i = irow * 5 + icol + 1;

        HARDBUG(i > 50)

        if (s[i] == *nn) break;
      }
 
      HARDBUG(i == INVALID)

      s[i] = *bO;
    }

    arg_s[51] = '\0';
  
    string2board(&(with->S_board), s, FALSE);
  
    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with->S_board), &moves_list, FALSE);

    if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
    {
      if (best_score == NULL)
        result = TRUE;
      else
      {
        int root_score = return_material_score(&(with->S_board));

        reset_my_timer(&(with->S_timer));
    
        int best_pv;
    
        *best_score = mcts_search(with, root_score, 0, MCTS_PLY_MAX, 
                                  &moves_list, &best_pv, FALSE);
    
        if (abs(root_score - *best_score) <= (SCORE_MAN / 4))
        { 
          result = TRUE;
        }
        else
        { 
          if (ntry <= 10)
          {
            PRINTF("add_man_random rejected ntry=%d root_score=%d *best_score=%d\n",
                   ntry, root_score, *best_score);
          }
        }
      }
    }
    else
    {
      if (ntry <= 10)
      {
        PRINTF("add_man_random rejected ntry=%d ncaptx=%d nmoves=%d\n",
               ntry, moves_list.ML_ncaptx, moves_list.ML_nmoves);
      }
    }

    if (result)
    {
      PRINTF("add_man_random returned result ntry=%d\n", ntry);
  
      break;
    }

    if (return_my_timer(&timer, FALSE) > 2.0)
    {
      PRINTF("time limit in add_man_random ntry=%d\n", ntry);

      break;
    }
  }

  return(result);
}

local int add_kings_random(int arg_nwhite_kings, int arg_nblack_kings,
  my_random_t *arg_random, search_t *arg_with, char *s, int *arg_best_score)
{
  char t[MY_LINE_MAX];

  for (int i = 1; i <= 50; ++i) t[i] = s[i];
 
  int result = FALSE;

  int ntry = 0;

  my_timer_t timer;

  construct_my_timer(&timer, "timer", STDOUT, FALSE);

  reset_my_timer(&timer);

  while(TRUE)
  {
    ++ntry;

    for (int i = 1; i <= 50; ++i) s[i] = t[i];

    for (int iwhite_king = 1; iwhite_king <= arg_nwhite_kings;
         ++iwhite_king)
    {
      int i = INVALID;

      while(TRUE)
      {
        int irow = return_my_random(arg_random) % 10;
        int icol = return_my_random(arg_random) % 5;

        i = irow * 5 + icol + 1;

        if (s[i] == *nn) break;
      }
      s[i] = *wX;
    }

    for (int iblack_king = 1; iblack_king <= arg_nblack_kings;
         ++iblack_king)
    {
      int i = INVALID;

      while(TRUE)
      {
        int irow = return_my_random(arg_random) % 10;
        int icol = return_my_random(arg_random) % 5;
 
        i = irow * 5 + icol + 1;

        if (s[i] == *nn) break;
      }
      s[i] = *bX;
    }

    s[51] = '\0';
  
    string2board(&(arg_with->S_board), s, FALSE);

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(arg_with->S_board), &moves_list, FALSE);

    if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
    {
      if (arg_best_score == NULL)
        result = TRUE;
      else
      {
        int root_score = return_material_score(&(arg_with->S_board));

        reset_my_timer(&(arg_with->S_timer));
    
        int best_pv;
    
        *arg_best_score = mcts_search(arg_with, root_score, 0, MCTS_PLY_MAX, 
                                      &moves_list, &best_pv, FALSE);
    
        if (abs(root_score - *arg_best_score) <= (SCORE_MAN / 4))
        {
          result = TRUE;
        }
        else
        {
          if (ntry <= 10)
          {
            PRINTF("add_kings_random rejected ntry=%d"
                   " root_score=%d *arg_best_score=%d\n",
                   ntry, root_score, *arg_best_score);
          }
        }
      }
    }
    else
    {
      if (ntry <= 10)
      {
        PRINTF("add_kings_random rejected ntry=%d ncaptx=%d nmoves=%d\n",
               ntry, moves_list.ML_ncaptx, moves_list.ML_nmoves);
      }
    }

    if (result)
    {
      PRINTF("add_kings_random returned result ntry=%d\n", ntry);

      break;
    }

    if (return_my_timer(&timer, FALSE) > 2.0)
    {
      PRINTF("time limit in add_kings_random ntry=%d\n", ntry);

      break;
    }
  }

  return(result);
}

