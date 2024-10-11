//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

mcts_globals_t mcts_globals;

local int return_scaled_float_score(int best_score, ui64_t key)
{
  int result = best_score * SCALED_FLOAT_FACTOR;

  if (best_score == SCORE_WON)
  { 
    result -= key % (SCALED_FLOAT_FACTOR / 10);
  }
  else if (best_score == SCORE_LOST)
  {
    result += key % (SCALED_FLOAT_FACTOR / 10);
  }
  else
  {
    if ((key % 2) == 0)
      result += key % (SCALED_FLOAT_FACTOR / 10);
    else
      result -= key % (SCALED_FLOAT_FACTOR / 10);
  }
  return(result);
}

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

local int mcts_shoot_out(board_t *with, int nply, int alpha_beta_depth)
{
  int result = 0;

  if (with->board_inode > 200)
  {
    print_board(with->board_id);
    PRINTF("WARNING: VERY DEEP SEARCH\n");
  }

  moves_list_t moves_list;

  create_moves_list(&moves_list);

  gen_moves(with, &moves_list, FALSE);

  for (int imove = moves_list.nmoves - 1; imove >= 1; --imove)
  {
    int jmove = randull(0) % (imove + 1);

    if (jmove != imove)
    {
      move_t temp = moves_list.moves[imove];

      moves_list.moves[imove] = moves_list.moves[jmove];

      moves_list.moves[jmove] = temp;
    }
  }

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

  if (best_pv == INVALID)
  {
    result = best_score;

    goto label_return;
  }

  nply++;

  HARDBUG(best_pv >= moves_list.nmoves)

  do_move(with, best_pv, &moves_list);

  result = -mcts_shoot_out(with, nply, alpha_beta_depth);

  undo_move(with, best_pv, &moves_list);

  label_return:

  return(result);
}

double mcts_shoot_outs(board_t *with, int nply,
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

  PRINTF("nwon=%d nlost=%d ndraw=%d\n", nwon, nlost, ndraw);

  result = 1.0 * (nwon - nlost) / nshoot_outs;

  return(result);
}

int mcts_quiescence(board_t *with, int nply, int alpha, int beta, 
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  if (IS_WHITE(with->board_colour2move))
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

int mcts_alpha_beta(board_t *with, int nply,
  int alpha, int beta, int depth,
  int node_type, moves_list_t *moves_list, int *best_pv)
{
  if (IS_WHITE(with->board_colour2move))
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

void init_mcts(void)
{
  construct_my_timer(&(mcts_globals.mcts_globals_timer), "mcts",
    STDOUT, FALSE);
  mcts_globals.mcts_globals_time_limit = 2.0;
  mcts_globals.mcts_globals_nodes = 0;
}
