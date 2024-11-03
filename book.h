//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#ifndef BookH
#define BookH

void init_book(void);
void open_book(void);
void return_book_move(board_t *with, moves_list_t *, char[MY_LINE_MAX]);

void gen_book(int, int);
void walk_book(int);
void count_book(int);

#endif

