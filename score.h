// SCU REVISION 8.12462 vr  4 sep 2026  7:33:55 CEST
#ifndef ScoreH
#define ScoreH

#define SCORE_MAN 100
#define SCORE_KING 333

#define SCORE_WON 10000
#define SCORE_LOST (-SCORE_WON)
#define SCORE_WON_ABSOLUTE 20000
#define SCORE_LOST_ABSOLUTE (-SCORE_WON_ABSOLUTE)
#define SCORE_PLUS_INFINITY 30000
#define SCORE_MINUS_INFINITY (-SCORE_PLUS_INFINITY)

#define SCORE_NOT_A_NUMBER SCORE(40000)

#define IS_SCORE_WON(S) ((S) >= SCORE(SCORE_WON - NODE_MAX))
#define IS_SCORE_LOST(S) ((S) <= SCORE(SCORE_LOST + NODE_MAX))
#define IS_SCORE_NAN(S) ((S) == SCORE_NOT_A_NUMBER)

score_t return_score_from_board(board_t *, double_t *);
score_t return_score(search_t *);

int return_npieces(board_t *);
score_t return_material_score(board_t *);

void init_score(void);

#endif
