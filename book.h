//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#ifndef BookH
#define BookH

void init_book(void);

void open_book(void);

void return_book_move(board_t *with, moves_list_t *, bstring);

void gen_book(my_sqlite3_t *, int);

void walk_book(char *, char *);

void add2book(my_sqlite3_t *, char *, int, int, int);

void gen_lmr(char *, int);

#endif

