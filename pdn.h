// SCU REVISION 8.1285 di 17 nov 2026 23:14:01 CET
#ifndef PdnH
#define PdnH

void read_games(my_sqlite3_t *, const char *);
void auto_play(const char *, i64_t);
void gen_db(my_sqlite3_t *, int, i64_t);
void update_db(my_sqlite3_t *, int, int);

#endif
