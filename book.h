//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#ifndef BookH
#define BookH

void init_book(void);

void open_book(void);

void return_book_move(board_t *with, moves_list_t *, bstring);

void gen_book(char *, char *);

void walk_book(char *);

#endif

