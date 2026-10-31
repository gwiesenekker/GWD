// SCU REVISION 8.1255 za 31 okt 2026  9:15:42 CET
#ifndef MctsH
#define MctsH

#define MCTS_THRESHOLD_LOST (-2 * SCORE_MAN)
#define MCTS_THRESHOLD_WON (2 * SCORE_MAN)

#define MCTS_PLY_MAX 150

typedef struct
{
  double mcts_globals_time_limit;
  i64_t mcts_globals_nodes;
} mcts_globals_t;

extern mcts_globals_t mcts_globals;

void mcts_search(search_t *, int, int, moves_list_t *, int *, score_t *, int);

double mcts_shoot_outs(search_t *,
                       int,
                       int,
                       double,
                       int *,
                       int *,
                       int *,
                       int *,
                       double *,
                       double *,
                       int);

void init_mcts(void);

#endif
