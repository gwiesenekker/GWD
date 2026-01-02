//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef ScoreH
#define ScoreH

#define SCORE_MAN            100
#define SCORE_KING           333

#define SCORE_WON            10000
#define SCORE_LOST           (-SCORE_WON)
#define SCORE_WON_ABSOLUTE   20000
#define SCORE_LOST_ABSOLUTE  (-SCORE_WON_ABSOLUTE)
#define SCORE_PLUS_INFINITY  30000
#define SCORE_MINUS_INFINITY (-SCORE_PLUS_INFINITY)

int return_score_from_board(board_t *, double_t *);
int return_score(search_t *);

int return_npieces(board_t *);
int return_material_score(board_t *);

void init_score(void);

#endif

