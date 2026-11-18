// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef EndgameH
#define EndgameH

#include "common.h"
#include "gwdegtb.h"
#include "search.h"

#define USE_COMPRESSED_WDL

enum
{
  ENDGAME_UNKNOWN = 32767
};

extern GwdegtbWdlProbe *wdl_probe_alpha_beta_master;

void init_endgame(void);
int read_endgame(search_t *arg_with_search, const char **arg_dtm_missing_name);
int read_endgame_if_open(search_t *arg_with_search);
score_t return_endgame_score(search_t *arg_with_search);
void fin_endgame(void);

#endif
