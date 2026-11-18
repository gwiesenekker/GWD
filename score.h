// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef ScoreH
#define ScoreH

#include "common.h"
#include "search.h"

enum
{
  SCORE_MAN = 100,
  SCORE_KING = 333
};

enum
{
  SCORE_WON = 10000,
  SCORE_LOST = (-SCORE_WON),
  SCORE_WON_ABSOLUTE = 20000,
  SCORE_LOST_ABSOLUTE = (-SCORE_WON_ABSOLUTE),
  SCORE_PLUS_INFINITY = 30000,
  SCORE_MINUS_INFINITY = (-SCORE_PLUS_INFINITY)
};

#define SCORE_NOT_A_NUMBER SCORE(40000)

static inline int is_score_won(score_t arg_score)
{
  return (arg_score >= SCORE(SCORE_WON - NODE_MAX));
}

static inline int is_score_lost(score_t arg_score)
{
  return (arg_score <= SCORE(SCORE_LOST + NODE_MAX));
}

static inline int is_score_nan(score_t arg_score)
{
  return (arg_score == SCORE_NOT_A_NUMBER);
}

score_t return_score_from_board(board_t *arg_with_board,
                                double_t *arg_network_score_scaled);
score_t return_score(search_t *arg_with_search);

int return_npieces(const board_t *arg_with_board);
score_t return_material_score(const board_t *arg_with_board);

void init_score(void);

#endif
