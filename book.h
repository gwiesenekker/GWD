// SCU REVISION 8.1181 zo  8 feb 2026 10:04:41 CET
#ifndef BookH
#define BookH

void init_book(void);

void open_book(void);

void return_book_move(board_t *, moves_list_t *, bstring);

void add_evaluations2book(my_sqlite3_t *, int, int);

void expand_book(my_sqlite3_t *, char *, int);

void walk_book(char *, char *);

#endif
