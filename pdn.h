// SCU REVISION 8.1236 vr  1 mei 2026  7:13:53 CEST
#ifndef PdnH
#define PdnH

void read_games(my_sqlite3_t *, char *);
void auto_play(char *, i64_t);
void gen_db(my_sqlite3_t *, int, i64_t);
void update_db(my_sqlite3_t *, int, int);

#endif
