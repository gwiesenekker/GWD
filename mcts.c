//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
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
  moves_list_t *, int *);
local int black_mcts_quiescence(search_t *, int, int, int, int,
  moves_list_t *, int *);

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

local int mcts_quiescence(search_t *with, int nply, int alpha, int beta, 
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  if (IS_WHITE(with->S_board.board_colour2move))
  {
    return(white_mcts_quiescence(with, nply,
      alpha, beta, node_type, moves_list, best_pv));
  }
  else
  {
    return(black_mcts_quiescence(with, nply,
      alpha, beta, node_type, moves_list, best_pv));
  }
}

local int mcts_alpha_beta(search_t *with, int nply,
  int alpha, int beta, int depth,
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  if (IS_WHITE(with->S_board.board_colour2move))
  {
    return(white_mcts_alpha_beta(with, nply,
      alpha, beta, depth,
      node_type, moves_list, best_pv));
  }
  else
  {
    return(black_mcts_alpha_beta(with, nply,
      alpha, beta, depth,
      node_type, moves_list, best_pv));
  }
}

local int mcts_shoot_out(search_t *with, int nply, int alpha_beta_depth)
{
  int result = 0;

  if (with->S_board.board_inode > 200)
  {
    print_board(&(with->S_board));

    PRINTF("WARNING: VERY DEEP SEARCH\n");
  }

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  gen_moves(&(with->S_board), &moves_list, FALSE);

  int best_pv;

  int best_score;

  if (alpha_beta_depth != INVALID)
  {
    best_score = mcts_alpha_beta(with, nply,
      SCORE_MINUS_INFINITY, SCORE_PLUS_INFINITY, alpha_beta_depth,
      PV_BIT, &moves_list, &best_pv);
  }
  else
  {
    best_score = mcts_quiescence(with, nply,
      SCORE_MINUS_INFINITY, SCORE_PLUS_INFINITY,
      PV_BIT, &moves_list, &best_pv);
  }

  if ((best_pv == INVALID) or (nply >= MCTS_PLY_MAX))
  {
    result = best_score;

    goto label_return;
  }

  nply++;

  HARDBUG(best_pv >= moves_list.nmoves)

  do_move(&(with->S_board), best_pv, &moves_list);

  result = -mcts_shoot_out(with, nply, alpha_beta_depth);

  undo_move(&(with->S_board), best_pv, &moves_list);

  label_return:

  return(result);
}

double mcts_shoot_outs(search_t *with, int nply,
  int nshoot_outs, int alpha_beta_depth)
{
  HARDBUG(nshoot_outs < 1)

  double result = 0.0;

  int nwon = 0;
  int nlost = 0;
  int ndraw = 0;

  for (int ishoot_out = 0; ishoot_out < nshoot_outs; ishoot_out++)
  {
    int temp_score = mcts_shoot_out(with, nply, alpha_beta_depth);

    if (temp_score >= MCTS_THRESHOLD_WON)
      nwon++;
    else if (temp_score <= MCTS_THRESHOLD_LOST)
      nlost++;
    else
      ndraw++;
  }

  result = (double) (nwon - nlost) / nshoot_outs;

  PRINTF("nwon=%d nlost=%d ndraw=%d result=%.6f\n",
         nwon, nlost, ndraw, result);

  return(result);
}

void init_mcts(void)
{
  if (my_mpi_globals.MY_MPIG_nslaves == 1)
    construct_my_random(&mcts_random, 0);
  else
    construct_my_random(&mcts_random, INVALID);

  mcts_globals.mcts_globals_time_limit = 2.0;
  mcts_globals.mcts_globals_nodes = 0;
}
