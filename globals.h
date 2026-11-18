// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef GlobalsH
#define GlobalsH

#include "revision.h"

#include "common.h"

// specific includes

#include "my_valgrind.h"

#ifdef USE_OPENMPI
#include <mpi.h>
#endif

#include "profile.h"

#include "bstrlib.h"
#include "cJSON.h"
#include "my_safec.h"
#include "classes.h"
#include "my_printf.h"
#include "my_bstreams.h"
#include "my_cjson.h"
#include "my_malloc.h"
#include "my_mpi.h"
#include "my_random.h"
#include "my_sqlite3.h"
#include "fbuffer.h"

#include "buckets.h"
#include "caches.h"
#include "compress.h"
#include "records.h"
#include "queues.h"
#include "states.h"
#include "stats.h"
#include "timers.h"
#include "utils.h"
#include "patterns.h"

#include "gwdegtb.h"

#include "networks.h"
#include "boards.h"
#include "moves.h"
#include "sort.h"
#include "search.h"
#include "my_threads.h"
#include "book.h"
#include "dxp.h"
#include "endgame.h"
#include "hub.h"
#include "main.h"
#include "mcts.h"
#include "pdn.h"
#include "score.h"

#endif
