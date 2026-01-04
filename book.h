//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef BookH
#define BookH

void init_book(void);

void open_book(void);

void return_book_move(board_t *, moves_list_t *, bstring);

void add_evaluations2book(my_sqlite3_t *, int, int);

void expand_book(my_sqlite3_t *, char *, int, int);

void walk_book(char *, char *);

#endif
