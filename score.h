//SCU REVISION 7.700 zo  3 nov 2024 10:44:36 CET
#ifndef ScoreH
#define ScoreH

#define SCORE_MAN            100
#define SCORE_CROWN          333

#define return_the_score(X) cat3(return_, X, _score)
#define return_my_score     return_the_score(my_colour)
#define return_your_score   return_the_score(your_colour)

int return_white_score(board_t *);
int return_black_score(board_t *);

#endif

