// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef ScoreH
#define ScoreH

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

#define IS_SCORE_WON(S) ((S) >= SCORE(SCORE_WON - NODE_MAX))
#define IS_SCORE_LOST(S) ((S) <= SCORE(SCORE_LOST + NODE_MAX))
#define IS_SCORE_NAN(S) ((S) == SCORE_NOT_A_NUMBER)

score_t return_score_from_board(board_t *, double_t *);
score_t return_score(search_t *);

int return_npieces(const board_t *);
score_t return_material_score(const board_t *);

void init_score(void);

#endif
