//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef MctsH
#define MctsH

#define MCTS_SCORE_WON  (1.0)
#define MCTS_SCORE_DRAW (0.0)
#define MCTS_SCORE_LOST (-1.0)

#define MCTS_THRESHOLD_LOST (-2 * SCORE_MAN)
#define MCTS_THRESHOLD_WON  (2 * SCORE_MAN)

#define MCTS_PLY_MAX 150

typedef struct
{
  double mcts_globals_time_limit;
  i64_t mcts_globals_nodes;
} mcts_globals_t;

extern mcts_globals_t mcts_globals;

int mcts_search(search_t *, int, int, int, moves_list_t *, int *, int);

double mcts_shoot_outs(search_t *, int, int *, int, double,
  int *, int *, int *);

void init_mcts(void);

#endif

