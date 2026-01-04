//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
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

int return_score_from_board(board_t *, double_t *);
int return_score(search_t *);

int return_npieces(board_t *);
int return_material_score(board_t *);

void init_score(void);

#endif
