//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#ifndef BookH
#define BookH

void init_book(void);

void open_book(void);

void return_book_move(board_t *, moves_list_t *, bstring);

void add_evaluations2book(my_sqlite3_t *, int, int);

void expand_book(my_sqlite3_t *, char *, int, int);

void walk_book(char *, char *);

#endif

