//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef MctsH
#define MctsH

#define SCALED_FLOAT_FACTOR 1000

#define FLOAT2SCALED(D) (round((F) * SCALED_FLOAT_FACTOR))
#define SCALED2FLOAT(S) ((double) (S) / SCALED_FLOAT_FACTOR)

#define MCTS_SCORE_WON  (1.0)
#define MCTS_SCORE_DRAW (0.0)
#define MCTS_SCORE_LOST (-1.0)

#define MCTS_THRESHOLD_LOST (-2 * SCORE_MAN)
#define MCTS_THRESHOLD_WON  (2 * SCORE_MAN)

#define MCTS_PLY_MAX 300

#define the_mcts_quiescence(X) cat2(X, _mcts_quiescence)
#define my_mcts_quiescence     the_mcts_quiescence(my_colour)
#define your_mcts_quiescence   the_mcts_quiescence(your_colour)

#define the_mcts_alpha_beta(X) cat2(X, _mcts_alpha_beta)
#define my_mcts_alpha_beta     the_mcts_alpha_beta(my_colour)
#define your_mcts_alpha_beta   the_mcts_alpha_beta(your_colour)

typedef struct
{
  my_timer_t mcts_globals_timer;
  double mcts_globals_time_limit;
  i64_t mcts_globals_nodes;
} mcts_globals_t;

extern mcts_globals_t mcts_globals;

int white_mcts_quiescence(board_t *, int, int, int, int, moves_list_t *,
  int *);
int black_mcts_quiescence(board_t *, int, int, int, int, moves_list_t *,
  int *);

int white_mcts_alpha_beta(board_t *, int, int, int, int, int, moves_list_t *,
  int *);
int black_mcts_alpha_beta(board_t *, int, int, int, int, int, moves_list_t *,
  int *);

double mcts_shoot_outs(board_t *, int, int, int);

int mcts_quiescence(board_t *, int, int, int, int, moves_list_t *,
  int *);
int mcts_alpha_beta(board_t *, int, int, int, int, int, moves_list_t *,
  int *);

void init_mcts(void);

#endif

