//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef MctsH
#define MctsH

#define MCTS_SCORE_WON  (1.0)
#define MCTS_SCORE_DRAW (0.0)
#define MCTS_SCORE_LOST (-1.0)

#define MCTS_THRESHOLD_LOST (-2 * SCORE_MAN)
#define MCTS_THRESHOLD_WON  (2 * SCORE_MAN)

#define MCTS_PLY_MAX 128

typedef struct
{
  double mcts_globals_time_limit;
  i64_t mcts_globals_nodes;
} mcts_globals_t;

extern mcts_globals_t mcts_globals;

double mcts_shoot_outs(search_t *, int, int, int);

void init_mcts(void);

#endif

