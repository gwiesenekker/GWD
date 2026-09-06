// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST
#ifndef BookH
#define BookH

void init_book(void);

void open_book(void);

void return_book_move(board_t *, moves_list_t *, bstring);

void add_evaluations2book(my_sqlite3_t *, int, int);

void expand_book(my_sqlite3_t *, char *, int);

void walk_book(char *, char *);

#endif
