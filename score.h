//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
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

#define return_the_score(X) cat3(return_, X, _score)
#define return_my_score     return_the_score(my_colour)
#define return_your_score   return_the_score(your_colour)

int return_white_score(search_t *, moves_list_t *);
int return_black_score(search_t *, moves_list_t *);

int return_npieces(board_t *);
int return_material_score(board_t *);

void init_score(void);

#endif

