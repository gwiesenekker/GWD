//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#ifndef BookH
#define BookH

void init_book(void);

void open_book(void);

void return_book_move(board_t *with, moves_list_t *, bstring);

void gen_book(int, int);

void walk_book(int);

void count_book(int);

#endif

