//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#include "globals.h"

mcts_globals_t mcts_globals;

local my_random_t mcts_random;

#define the_mcts_quiescence(X) cat2(X, _mcts_quiescence)
#define my_mcts_quiescence     the_mcts_quiescence(my_colour)
#define your_mcts_quiescence   the_mcts_quiescence(your_colour)

#define the_mcts_alpha_beta(X) cat2(X, _mcts_alpha_beta)
#define my_mcts_alpha_beta     the_mcts_alpha_beta(my_colour)
#define your_mcts_alpha_beta   the_mcts_alpha_beta(your_colour)

local int white_mcts_quiescence(search_t *, int, int, int, int,
  moves_list_t *, int *, int);
local int black_mcts_quiescence(search_t *, int, int, int, int,
  moves_list_t *, int *, int);

local int white_mcts_alpha_beta(search_t *, int, int, int, int, int,
  moves_list_t *, int *);
local int black_mcts_alpha_beta(search_t *, int, int, int, int, int,
  moves_list_t *, int *);

#define MY_BIT      WHITE_BIT
#define YOUR_BIT    BLACK_BIT
#define my_colour   white
#define your_colour black

#include "mcts.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

#define MY_BIT      BLACK_BIT
#define YOUR_BIT    WHITE_BIT
#define my_colour   black
#define your_colour white

#include "mcts.d"

#undef MY_BIT
#undef YOUR_BIT
#undef my_colour
#undef your_colour

local int mcts_alpha_beta(search_t *object, int arg_nply,
  int arg_alpha, int arg_beta, int arg_depth,
  int arg_node_type, moves_list_t *arg_moves_list, int *arg_best_pv)
{
  if (IS_WHITE(object->S_board.board_colour2move))
  {
    return(white_mcts_alpha_beta(object, arg_nply,
      arg_alpha, arg_beta, arg_depth,
      arg_node_type, arg_moves_list, arg_best_pv));
  }
  else
  {
    return(black_mcts_alpha_beta(object, arg_nply,
      arg_alpha, arg_beta, arg_depth,
      arg_node_type, arg_moves_list, arg_best_pv));
  }
}

//mcts_search returns SCORE_PLUS_INFINITY in case of time limit
//returns INVALID for best_pv in case of time limit, EGTB hit,
//draw by repetition or lost because of no moves

int mcts_search(search_t *object, int arg_root_score, int arg_nply,
  int arg_mcts_depth, moves_list_t *arg_moves_list, int *arg_best_pv, int arg_verbose)
{
  int best_score = SCORE_PLUS_INFINITY;
  *arg_best_pv = INVALID;

  BSTRING(bmove_string)

  //check endgame

  int wdl;

  int temp_mate = read_endgame(object, object->S_board.board_colour2move, &wdl);

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

    gen_moves(&(object->S_board), &temp_moves_list, TRUE);

    arg_moves_list = &temp_moves_list;
  }

  if (arg_moves_list->nmoves == 0)
  {
    best_score = SCORE_LOST_ABSOLUTE;

    goto label_return;
  }

  int sort[MOVES_MAX];

  shuffle(sort, arg_moves_list->nmoves, &mcts_random);

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

    do_move(&(object->S_board), jmove, arg_moves_list);

    int temp_score = SCORE_PLUS_INFINITY;
    int temp_pv = INVALID;

    temp_score = -mcts_alpha_beta(object, arg_nply + 1,
      -beta, -alpha, depth - 1, 
      PV_BIT, NULL, &temp_pv);

    if (temp_score == SCORE_MINUS_INFINITY) temp_score = SCORE_PLUS_INFINITY;

    if (temp_score == SCORE_PLUS_INFINITY)
    {
      HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

      undo_move(&(object->S_board), jmove, arg_moves_list);

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

          undo_move(&(object->S_board), jmove, arg_moves_list);
    
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

          undo_move(&(object->S_board), jmove, arg_moves_list);
    
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

    undo_move(&(object->S_board), *arg_best_pv, arg_moves_list);

    if ((arg_nply == 0) and arg_verbose)
    { 
      move2bstring(arg_moves_list, *arg_best_pv, bmove_string);

      PRINTF("depth=%d move=%s best_score=%d\n",
             depth, bdata(bmove_string), best_score);
    }

    for (int imove = 1; imove < arg_moves_list->nmoves; imove++)
    {
      jmove = sort[imove];

      alpha = best_score;
      beta = best_score + 1;

      do_move(&(object->S_board), jmove, arg_moves_list);

      temp_score = -mcts_alpha_beta(object, arg_nply + 1,
        -beta, -alpha, depth - 1, 
        MINIMAL_WINDOW_BIT, NULL, &temp_pv);
  
      if (temp_score == SCORE_MINUS_INFINITY) temp_score = SCORE_PLUS_INFINITY;
  
      if (temp_score == SCORE_PLUS_INFINITY)
      {
        HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

        undo_move(&(object->S_board), jmove, arg_moves_list);
  
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
        undo_move(&(object->S_board), jmove, arg_moves_list);

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

        PRINTF("depth=%d move=%s best_score>=%d\n",
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

          undo_move(&(object->S_board), jmove, arg_moves_list);
    
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

        PRINTF("depth=%d move=%s best_score==%d\n",
               depth, bdata(bmove_string), best_score);
      }

      undo_move(&(object->S_board), jmove, arg_moves_list);
    }//for (int imove

    if ((arg_nply == 0) and arg_verbose)
    {
      move2bstring(arg_moves_list, *arg_best_pv, bmove_string);

      PRINTF("depth=%d move=%s best_score===%d\n",
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

    PRINTF("depth=%d move=%s best_score====%d\n",
           depth, bdata(bmove_string), best_score);
  }

  label_return:

  BDESTROY(bmove_string)

  return(best_score);
}

local int mcts_shoot_out(search_t *object, int arg_nply, int arg_mcts_depth)
{
  if (object->S_board.board_inode > 200)
  {
    print_board(&(object->S_board));

    PRINTF("WARNING: VERY DEEP SEARCH\n");
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(object->S_board), &moves_list, FALSE);

  //local time limit

  if (mcts_globals.mcts_globals_time_limit > 0.0)
    reset_my_timer(&(object->S_timer));

  int best_score = SCORE_PLUS_INFINITY;
  int best_pv = INVALID;

  best_score = mcts_search(object, 0, arg_nply, arg_mcts_depth,
    &moves_list, &best_pv, TRUE);

  if ((best_pv == INVALID) or (arg_nply >= MCTS_PLY_MAX)) goto label_return;

  HARDBUG(best_pv >= moves_list.nmoves)

  do_move(&(object->S_board), best_pv, &moves_list);

  best_score = -mcts_shoot_out(object, arg_nply + 1, arg_mcts_depth);

  undo_move(&(object->S_board), best_pv, &moves_list);

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

      PRINTF("time limit in mcts_shoot_outs ishoot_out=%d!\n", ishoot_out);

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

  if (*arg_nshoot_outs == 0) PRINTF("WARNING: *nshoot_outs == 0\n");

  if ((*arg_nwon + *arg_nlost) > 0)
    result = (double) (*arg_nwon - *arg_nlost) / (*arg_nwon + *arg_nlost);

  PRINTF("*nshoot_outs=%d time=%.2f"
         " *nwon=%d *nlost=%d *ndraw=%d result=%.6f"
         " (%.2f shoot_outs/second)\n",
         *arg_nshoot_outs, return_my_timer(&mcts_timer, FALSE),
         *arg_nwon, *arg_nlost, *arg_ndraw, result,
         *arg_nshoot_outs / return_my_timer(&mcts_timer, FALSE));

  return(result);
}

void init_mcts(void)
{
  if (my_mpi_globals.MY_MPIG_nslaves == 1)
    construct_my_random(&mcts_random, 0);
  else
    construct_my_random(&mcts_random, INVALID);

  mcts_globals.mcts_globals_time_limit = 100000.0;
  mcts_globals.mcts_globals_nodes = 0;
}
