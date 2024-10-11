//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef BookH
#define BookH

void init_book(void);
void open_book(void);
void return_book_move(board_t *with, moves_list_t *, char[MY_LINE_MAX]);

void gen_book(int, int);
void walk_book(int);
void count_book(int);

#endif

