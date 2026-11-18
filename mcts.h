// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef MctsH
#define MctsH

#include "common.h"
#include "search.h"
#include "score.h"

#define MCTS_THRESHOLD_LOST (-2 * SCORE_MAN)
#define MCTS_THRESHOLD_WON (2 * SCORE_MAN)

enum
{
  MCTS_PLY_MAX = 150
};

typedef struct
{
  double mcts_globals_time_limit;
  my_int64_t mcts_globals_nnodes;
} mcts_globals_t;

extern mcts_globals_t mcts_globals;

void mcts_search(search_t *arg_with_search,
                 int arg_mcts_depth,
                 int arg_nply,
                 moves_list_t *arg_moves_list,
                 int *arg_sort,
                 score_t *arg_best_score,
                 int arg_verbose);

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
                       int arg_verbose);

void init_mcts(void);

#endif
