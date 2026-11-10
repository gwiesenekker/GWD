// SCU REVISION 8.1262 di 10 nov 2026 15:00:45 CET
#ifndef EndgameH
#define EndgameH

#define USE_COMPRESSED_WDL

#define ENDGAME_UNKNOWN 32767

extern GwdegtbWdlProbe *wdl_probe_alpha_beta_master;

void init_endgame(void);
int read_endgame(search_t *, const char **);
score_t return_endgame_score(search_t *);
void fin_endgame(void);

#endif
