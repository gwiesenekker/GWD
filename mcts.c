// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#include "mcts.h"

#include "boards.h"
#include "bstrlib.h"
#include "common.h"
#include "endgame.h"
#include "main.h"
#include "moves.h"
#include "my_malloc.h"
#include "my_printf.h"
#include "my_random.h"
#include "my_threads.h"
#include "profile.h"
#include "queues.h"
#include "score.h"
#include "search.h"
#include "timers.h"

mcts_globals_t mcts_globals;

local void inline flip_score(score_t *arg_score)
{
  if (!is_score_nan(*arg_score))
    *arg_score = -*arg_score;
}

local void mcts_alpha_beta(search_t *arg_with_search,
                           score_t arg_my_alpha,
                           score_t arg_my_beta,
                           int arg_my_depth,
                           int arg_nply,
                           int arg_node_type,
                           moves_list_t *arg_moves_list,
                           int arg_all_moves,
                           score_t *arg_best_score)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  HARDBUG(arg_my_alpha >= arg_my_beta)

  HARDBUG(arg_node_type == 0)

  HARDBUG(is_minimal_window(arg_node_type) and is_pv(arg_node_type))

  HARDBUG(is_minimal_window(arg_node_type) and
          ((arg_my_beta - arg_my_alpha) != 1))

  *arg_best_score = SCORE_NOT_A_NUMBER;

  if (mcts_globals.mcts_globals_time_limit != 0.0)
  {
    if ((mcts_globals.mcts_globals_nnodes++ % 100) == 0)
    {
      if (poll_queue(return_thread_queue(arg_with_search->S_thread)) != INVALID)
      {
        my_printf(arg_with_search->S_my_printf, "got message\n");

        goto label_return;
      }

      if (return_my_timer(&(arg_with_search->S_timer), FALSE) >=
          fabs(mcts_globals.mcts_globals_time_limit))
      {
        goto label_return;
      }
    }
  }

  if (arg_all_moves and ((arg_my_depth == 0) or (arg_nply >= MCTS_PLY_MAX)))
  {
    mcts_alpha_beta(arg_with_search,
                    arg_my_alpha,
                    arg_my_beta,
                    -1,
                    arg_nply,
                    arg_node_type,
                    NULL,
                    FALSE,
                    arg_best_score);

    goto label_return;
  }

  if (draw_by_repetition(&(arg_with_search->S_board), FALSE))
  {
    *arg_best_score = SCORE(0);

    goto label_return;
  }

  // check endgame

  score_t endgame_score = return_endgame_score(arg_with_search);

  if (!is_score_nan(endgame_score))
  {
    *arg_best_score = endgame_score;

    goto label_return;
  }

  moves_list_t my_moves_list;

  if (arg_moves_list == NULL)
  {
    construct_moves_list(&my_moves_list);

    gen_moves(&(arg_with_search->S_board), &my_moves_list);

    arg_moves_list = &my_moves_list;
  }

  if (arg_moves_list->ML_nmoves == 0)
  {
    *arg_best_score =
      SCORE(SCORE_LOST_ABSOLUTE + arg_with_search->S_board.B_inode);

    goto label_return;
  }

  int all_moves = arg_all_moves;

  if (!all_moves)
  {
    if (arg_moves_list->ML_ncaptx > 0)
    {
      all_moves = TRUE;
    }
    else if (arg_moves_list->ML_nmoves <= 2)
    {
      all_moves = TRUE;
    }
    else
    {
      int nextend = 0;

      for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
      {
        if (move_extend_in_quiescence(arg_moves_list->ML_move_flags[imove]))
          nextend++;
      }

      if (nextend == arg_moves_list->ML_nmoves)
      {
        all_moves = TRUE;
      }
    }
  }

  *arg_best_score = SCORE_MINUS_INFINITY;

  if (!all_moves)
  {
    *arg_best_score = return_material_score(&(arg_with_search->S_board));

    if (*arg_best_score >= arg_my_beta)
      goto label_return;
  }

  int sort[NMOVES_MAX];

  shuffle(sort,
          arg_moves_list->ML_nmoves,
          &(arg_with_search->S_random),
          INVALID);

  arg_nply++;

  int your_depth = arg_my_depth;

  if (arg_my_depth > 0)
  {
    if (arg_moves_list->ML_ncaptx == 0)
    {
      if ((arg_moves_list->ML_nmoves > 2) or
          move_repetition(&(arg_with_search->S_board)))
      {
        your_depth = arg_my_depth - 1;
      }
    }
  }

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
  {
    int jmove = sort[imove];

    if (!all_moves)
    {
      if (!move_extend_in_quiescence(arg_moves_list->ML_move_flags[jmove]))
        continue;
    }

    score_t temp_alpha;

    if (*arg_best_score < arg_my_alpha)
      temp_alpha = arg_my_alpha;
    else
      temp_alpha = *arg_best_score;

    score_t temp_beta = arg_my_beta;

    uint32_t node_type = arg_node_type;

    if (is_pv(arg_node_type) and (imove > 0))
    {
      temp_beta = temp_alpha + 1;

      node_type = MINIMAL_WINDOW_BIT;
    }

    score_t temp_score = SCORE(SCORE_MINUS_INFINITY);

    do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    for (int ipass = 1; ipass <= 2; ++ipass)
    {
      mcts_alpha_beta(arg_with_search,
                      -temp_beta,
                      -temp_alpha,
                      your_depth,
                      arg_nply,
                      node_type,
                      NULL,
                      arg_all_moves,
                      &temp_score);

      flip_score(&temp_score);

      HARDBUG(temp_score == SCORE(SCORE_MINUS_INFINITY))

      if (is_score_nan(temp_score))
      {
        undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

        *arg_best_score = temp_score;

        goto label_return;
      }

      if (ipass == 2)
        break;

      if (is_minimal_window(arg_node_type) or (temp_score < temp_beta) or
          (temp_score >= arg_my_beta))
        break;

      temp_beta = arg_my_beta;

      node_type = PV_BIT;
    }

    undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

    if (temp_score > *arg_best_score)
    {
      *arg_best_score = temp_score;

      if (*arg_best_score >= arg_my_beta)
        break;
    }
  } // imove

  // we always assigned *arg_best_score
  //*best_pv can be invalid, for example if (!all_moves) and
  // move scores are below alpha

  HARDBUG(*arg_best_score == SCORE_MINUS_INFINITY)

label_return:;

  END_BLOCK

  POP_NAME(__FUNC__)
}

void mcts_search(search_t *arg_with_search,
                 int arg_mcts_depth,
                 int arg_nply,
                 moves_list_t *arg_moves_list,
                 int arg_sort[NMOVES_MAX],
                 score_t *arg_best_score,
                 int arg_verbose)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  BSTRING(bmove_string)

  HARDBUG(arg_mcts_depth < 1)

  HARDBUG(arg_moves_list == NULL)

  HARDBUG(arg_moves_list->ML_nmoves == 0)

  for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    arg_sort[imove] = INVALID;

  // check endgame

  score_t endgame_score = return_endgame_score(arg_with_search);

  if (!is_score_nan(endgame_score))
  {
    *arg_best_score = endgame_score;

    goto label_return;
  }

  shuffle(arg_sort,
          arg_moves_list->ML_nmoves,
          &(arg_with_search->S_random),
          INVALID);

  *arg_best_score = return_material_score(&(arg_with_search->S_board));

  int my_depth = 1;

  int ntrouble = 0;

  while (TRUE)
  {
    int your_depth = my_depth - 1;

    for (int imove = 0; imove < arg_moves_list->ML_nmoves; imove++)
    {
      int jmove = arg_sort[imove];

      score_t my_alpha = *arg_best_score;

      score_t my_beta = my_alpha + 1;

      uint32_t node_type = MINIMAL_WINDOW_BIT;

      if (imove == 0)
      {
        my_alpha = *arg_best_score - SCORE_MAN / 4;

        my_beta = *arg_best_score + SCORE_MAN / 4;

        node_type = PV_BIT;
      }

      score_t temp_score = SCORE(SCORE_MINUS_INFINITY);

      do_move(&(arg_with_search->S_board), jmove, arg_moves_list);

      for (int ipass = 1; ipass <= 2; ++ipass)
      {
        mcts_alpha_beta(arg_with_search,
                        -my_beta,
                        -my_alpha,
                        your_depth,
                        arg_nply + 1,
                        node_type,
                        NULL,
                        TRUE,
                        &temp_score);

        flip_score(&temp_score);

        if (is_score_nan(temp_score))
        {
          HARDBUG(mcts_globals.mcts_globals_time_limit == 0.0)

          undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

          // if global time limit we return NAN

          if (mcts_globals.mcts_globals_time_limit < 0.0)
          {
            *arg_best_score = temp_score;

            goto label_return;
          }

          // if time limit per move we return the best score until now

          goto label_break;
        }

        if (ipass == 2)
          break;

        if (temp_score <= my_alpha)
        {
          if (is_minimal_window(node_type))
            break;

          my_beta = my_alpha;

          my_alpha = SCORE_MINUS_INFINITY;
        }
        else if (temp_score >= my_beta)
        {
          my_alpha = my_beta;

          my_beta = SCORE_PLUS_INFINITY;

          node_type = PV_BIT;
        }
        else
        {
          // true score

          break;
        }
      } // for (int ipass

      undo_move(&(arg_with_search->S_board), jmove, arg_moves_list);

      if ((imove == 0) or (temp_score > *arg_best_score))
      {
        HARDBUG(!is_pv(node_type))

        if (imove > 0)
        {
          for (int kmove = imove; kmove > 0; kmove--)
            arg_sort[kmove] = arg_sort[kmove - 1];

          arg_sort[0] = jmove;
        }

        *arg_best_score = temp_score;

        if ((arg_nply == 0) and arg_verbose)
        {
          move2bstring(arg_moves_list, jmove, bmove_string);

          my_printf(arg_with_search->S_my_printf,
                    " d=%d"
                    " i=%d"
                    " j=%s"
                    " s=" PRINTF_SCORE_T " n=%lld"
                    " t=%.2f"
                    " a=" PRINTF_SCORE_T " b=" PRINTF_SCORE_T "\n",
                    my_depth,
                    imove,
                    bdata(bmove_string),
                    SCORE2PRINTF(*arg_best_score),
                    mcts_globals.mcts_globals_nnodes,
                    return_my_timer(&(arg_with_search->S_timer), FALSE),
                    SCORE2PRINTF(my_alpha),
                    SCORE2PRINTF(my_beta));
        }
      }
    } // for (int imove

    // #ifdef DEBUG
    for (int idebug = 0; idebug < arg_moves_list->ML_nmoves; idebug++)
    {
      int ndebug = 0;

      for (int jdebug = 0; jdebug < arg_moves_list->ML_nmoves; jdebug++)
        if (arg_sort[jdebug] == idebug)
          ++ndebug;

      HARDBUG(ndebug != 1)
    }
    // #endif

    if (is_score_won(*arg_best_score))
      goto label_break;

    if (is_score_lost(*arg_best_score))
      goto label_break;

    if (my_depth >= arg_mcts_depth)
    {
      if (*arg_best_score >= 0)
        break;

      ++ntrouble;

      if (ntrouble > 1)
        break;
    }

    ++my_depth;
  } // while ((my_depth

label_break:

  if ((arg_nply == 0) and arg_verbose)
  {
    move2bstring(arg_moves_list, arg_sort[0], bmove_string);

    my_printf(arg_with_search->S_my_printf,
              " my_depth=%d"
              " arg_sort[0]=%s"
              " *arg_best_score=" PRINTF_SCORE_T "\n",
              my_depth,
              bdata(bmove_string),
              SCORE2PRINTF(*arg_best_score));
  }

label_return:

  BDESTROY(bmove_string)

  END_BLOCK

  POP_NAME(__FUNC__)
}

local void mcts_shoot_out(search_t *arg_with_search,
                          int arg_mcts_depth,
                          int arg_nply,
                          score_t *arg_best_score,
                          int arg_verbose)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  HARDBUG(arg_mcts_depth < 1)

  if (arg_with_search->S_board.B_inode > 200)
  {
    print_board(&(arg_with_search->S_board));

    my_printf(arg_with_search->S_my_printf, "WARNING: VERY DEEP SEARCH\n");
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(arg_with_search->S_board), &moves_list);

  if (moves_list.ML_nmoves == 0)
  {
    *arg_best_score =
      SCORE(SCORE_LOST_ABSOLUTE + arg_with_search->S_board.B_inode);

    goto label_return;
  }

  // local time limit

  if (mcts_globals.mcts_globals_time_limit > 0.0)
    reset_my_timer(&(arg_with_search->S_timer));

  int arg_sort[NMOVES_MAX];

  *arg_best_score = SCORE_MINUS_INFINITY;

  mcts_search(arg_with_search,
              arg_mcts_depth,
              arg_nply,
              &moves_list,
              arg_sort,
              arg_best_score,
              arg_verbose);

  if (is_score_nan(*arg_best_score))
    goto label_return;

  int jmove = arg_sort[0];

  if ((jmove == INVALID) or (arg_nply >= MCTS_PLY_MAX))
    goto label_return;

  HARDBUG(jmove >= moves_list.ML_nmoves)

  do_move(&(arg_with_search->S_board), jmove, &moves_list);

  mcts_shoot_out(arg_with_search,
                 arg_mcts_depth,
                 arg_nply + 1,
                 arg_best_score,
                 arg_verbose);

  flip_score(arg_best_score);

  undo_move(&(arg_with_search->S_board), jmove, &moves_list);

label_return:;

  END_BLOCK

  POP_NAME(__FUNC__)
}

local const double wilson_z = 1.96;

local double return_wilson_lcb(int arg_nsuccesses, int arg_ntotal)
{
  if (arg_ntotal == 0)
    return (0.0);

  double nsamples = (double)arg_ntotal;
  double success_fraction = (double)arg_nsuccesses / nsamples;
  double squared_z = wilson_z * wilson_z;

  double denom = 1.0 + squared_z / nsamples;
  double center = success_fraction + squared_z / (2.0 * nsamples);
  double margin = wilson_z * sqrt((success_fraction * (1.0 - success_fraction) +
                                   squared_z / (4.0 * nsamples)) /
                                  nsamples);

  double lcb = (center - margin) / denom;

  if (lcb < 0.0)
    lcb = 0.0;

  if (lcb > 1.0)
    lcb = 1.0;

  return (lcb);
}

static double return_wilson_ucb(int arg_nsuccesses, int arg_ntotal)
{
  if (arg_ntotal == 0)
    return (1.0);

  double nsamples = (double)arg_ntotal;
  double success_fraction = (double)arg_nsuccesses / nsamples;
  double squared_z = wilson_z * wilson_z;

  double denom = 1.0 + squared_z / nsamples;
  double center = success_fraction + squared_z / (2.0 * nsamples);
  double margin = wilson_z * sqrt((success_fraction * (1.0 - success_fraction) +
                                   squared_z / (4.0 * nsamples)) /
                                  nsamples);

  double ucb = (center + margin) / denom;

  if (ucb < 0.0)
    ucb = 0.0;

  if (ucb > 1.0)
    ucb = 1.0;

  return (ucb);
}

double mcts_shoot_outs(search_t *arg_with_search,
                       int arg_mcts_depth,
                       int arg_nply,
                       double arg_mcts_time_limit,
                       int *arg_nshoot_outs,
                       int *arg_nwon,
                       int *arg_ndraw,
                       int *arg_nlost,
                       double *arg_wilson_lcb,
                       double *arg_wilson_ucb,
                       int arg_verbose)
{
  PUSH_NAME(__FUNC__)

  BEGIN_BLOCK(__FUNC__)

  HARDBUG(arg_mcts_depth < 1)

  options.material_only = TRUE;

  mcts_globals.mcts_globals_time_limit = arg_mcts_time_limit;

  HARDBUG(*arg_nshoot_outs < 1)

  my_timer_t mcts_timer;

  construct_my_timer(&mcts_timer, "mcts_timer", STDOUT, FALSE);

  reset_my_timer(&mcts_timer);

  // global time limit

  if (arg_mcts_time_limit < 0.0)
    reset_my_timer(&(arg_with_search->S_timer));

  double result = 0.0;

  int nwon_local = 0;
  int ndraw_local = 0;
  int nlost_local = 0;

  if (arg_nwon == NULL)
    arg_nwon = &nwon_local;

  if (arg_ndraw == NULL)
    arg_ndraw = &ndraw_local;

  if (arg_nlost == NULL)
    arg_nlost = &nlost_local;

  for (int ishoot_out = 1; ishoot_out <= *arg_nshoot_outs; ++ishoot_out)
  {
    score_t temp_score;

    mcts_shoot_out(arg_with_search,
                   arg_mcts_depth,
                   arg_nply,
                   &temp_score,
                   arg_verbose);

    if (is_score_nan(temp_score))
    {
      HARDBUG(arg_mcts_time_limit >= 0.0)

      my_printf(arg_with_search->S_my_printf,
                "time limit in mcts_shoot_outs ishoot_out=%d!\n",
                ishoot_out);

      *arg_nshoot_outs = ishoot_out;

      break;
    }

    if (temp_score >= MCTS_THRESHOLD_WON)
      (*arg_nwon)++;
    else if (temp_score <= MCTS_THRESHOLD_LOST)
      (*arg_nlost)++;
    else
      (*arg_ndraw)++;

    if ((ishoot_out % 100) == 0)
    {
      BSTRING(bline)

      HARDBUG(bformata(bline,
                       "nshoot_outs=%d"
                       " nwon=%d"
                       " ndraw=%d"
                       " nlost=%d",
                       *arg_nshoot_outs,
                       *arg_nwon,
                       *arg_ndraw,
                       *arg_nlost) == BSTR_ERR)

      PRINTF("%s\n", bdata(bline));

      if ((arg_with_search->S_thread != NULL) and
          (arg_with_search->S_thread == thread_alpha_beta_master))
        enqueue(&main_queue, MESSAGE_INFO, bdata(bline));

      BDESTROY(bline)
    }
  }

  if (*arg_nshoot_outs == 0)
    my_printf(arg_with_search->S_my_printf, "WARNING: *nshoot_outs == 0\n");

  double wilson_lcb = return_wilson_lcb(*arg_nwon, *arg_nshoot_outs);

  double wilson_ucb = return_wilson_ucb(*arg_nlost, *arg_nshoot_outs);

  if ((*arg_nwon + *arg_nlost) > 0)
    result = (double)(*arg_nwon - *arg_nlost) / (*arg_nwon + *arg_nlost);

  my_printf(arg_with_search->S_my_printf,
            "*arg_nshoot_outs=%d"
            " time=%.2f"
            " *arg_nwon=%d"
            " *arg_nlost=%d"
            " *arg_ndraw=%d"
            " result=%.6f"
            " lcb=%.6f"
            " ucb=%.6f"
            " lcb - ucb=%.6f"
            " (%.2f shoot_outs/second)\n",
            *arg_nshoot_outs,
            return_my_timer(&mcts_timer, FALSE),
            *arg_nwon,
            *arg_nlost,
            *arg_ndraw,
            result,
            wilson_lcb,
            wilson_ucb,
            wilson_lcb - wilson_ucb,
            *arg_nshoot_outs / return_my_timer(&mcts_timer, FALSE));

  END_BLOCK

  POP_NAME(__FUNC__)

  if (arg_wilson_lcb != NULL)
    *arg_wilson_lcb = wilson_lcb;

  if (arg_wilson_ucb != NULL)
    *arg_wilson_ucb = wilson_ucb;

  return (result);
}

void init_mcts(void)
{
  mcts_globals.mcts_globals_time_limit = 100000.0;
  mcts_globals.mcts_globals_nnodes = 0;
}
