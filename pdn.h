// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef PdnH
#define PdnH

#include "common.h"
#include "my_sqlite3.h"

void read_games(my_sqlite3_t *arg_db, const char *arg_pdn_name);
void auto_play(const char *arg_txt_name, my_int64_t arg_npositions);
void gen_db(my_sqlite3_t *arg_db,
            int arg_mcts_depth,
            my_int64_t arg_npositions);
void update_db(my_sqlite3_t *arg_db, int arg_mcts_depth, int arg_frequency);

#endif
