// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef BookH
#define BookH

#include "common.h"
#include "moves.h"
#include "my_sqlite3.h"

void init_book(void);

void open_book(void);

void return_book_move(board_t *arg_with_board,
                      moves_list_t *arg_moves_list,
                      bstring arg_bbook_move);

void add_evaluations2book(my_sqlite3_t *arg_db,
                          int arg_centi_seconds,
                          int arg_add_moves);

void expand_book(my_sqlite3_t *arg_db,
                 const char *arg_fen,
                 int arg_centi_seconds);

void walk_book(const char *arg_db_name, const char *arg_fen);

#endif
