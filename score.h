//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef ScoreH
#define ScoreH

#define SCORE_MAN            100
#define SCORE_KING          333

#define return_the_score(X) cat3(return_, X, _score)
#define return_my_score     return_the_score(my_colour)
#define return_your_score   return_the_score(your_colour)

int return_white_score(board_t *, moves_list_t *);
int return_black_score(board_t *, moves_list_t *);

#endif

