// SCU REVISION 8.1273 wo 11 nov 2026 19:28:10 CET
#ifndef PdnH
#define PdnH

void read_games(my_sqlite3_t *, char *);
void auto_play(char *, i64_t);
void gen_db(my_sqlite3_t *, int, i64_t);
void update_db(my_sqlite3_t *, int, int);

#endif
