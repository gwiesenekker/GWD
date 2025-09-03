//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#ifndef PdnH
#define PdnH

void read_games(char *, char *);
void gen_db(my_sqlite3_t *, i64_t, int);
void update_db(my_sqlite3_t *, int);
void update_db_v2(my_sqlite3_t *, int, int);

#endif

