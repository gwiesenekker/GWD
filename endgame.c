// SCU REVISION 8.1273 wo 11 nov 2026 19:28:10 CET

#include "globals.h"
#include "material.h" // GWDEGTB's EGTB_MAX_PIECES

GwdegtbWdlProbe *wdl_probe_alpha_beta_master;

local my_mutex_t dtm_probe_mutex;

void init_endgame(void)
{
  PRINTF("init_endgame..\n");

  HARDBUG(compat_mutex_init(&dtm_probe_mutex) != 0)

  size_t egtb_ram = 0;

  cJSON *egtbs = cJSON_GetObjectItem(gwd_json, options.egtb_ram_wdl);

  HARDBUG(!cJSON_IsArray(egtbs))

  cJSON *egtb;

  cJSON_ArrayForEach(egtb, egtbs)
  {
    HARDBUG(!cJSON_IsString(egtb))

    char *endgame_name = cJSON_GetStringValue(egtb);

    HARDBUG(endgame_name == NULL)

    size_t egtb_size;

#ifdef USE_COMPRESSED_WDL
    HARDBUG(!gwdegtb_wdl_compressed_info_threads(options.egtb_dir_wdl,
                                                 endgame_name,
                                                 &egtb_size,
                                                 4));

    if (egtb_size == 0)
    {
      PRINTF("skipping unavaible EGTB %s\n", endgame_name);

      continue;
    }
#else
    uint64_t maximum_index;

    HARDBUG(!gwdegtb_wdl_info(endgame_name, &maximum_index, &egtb_size))
#endif

    PRINTF("loading EGTB %s..\n", endgame_name);

    void *pendgame;
    MPI_Win wendgame;

    my_mpi_allocate_shared(my_mpi_globals.MMG_comm_global,
                           sizeof(ui8_t),
                           egtb_size,
                           &pendgame,
                           &wendgame);

    if (my_mpi_globals.MMG_is_master)
    {
#ifdef USE_COMPRESSED_WDL
      HARDBUG(!gwdegtb_wdl_compressed_load_threads(options.egtb_dir_wdl,
                                                   endgame_name,
                                                   pendgame,
                                                   egtb_size,
                                                   4));
#else
      HARDBUG(!gwdegtb_wdl_decompress_threads(options.egtb_dir_wdl,
                                              endgame_name,
                                              pendgame,
                                              egtb_size,
                                              4))
#endif
    }

    /*
    my_mpi_barrier_v3("gwdegtb_wdl_decompress",
                      my_mpi_globals.MMG_comm_global,
                      my_mpi_globals.MMG_semkey_init_endgame_barrier,
                      TRUE);
    */

    my_mpi_sync_shared(my_mpi_globals.MMG_comm_global, wendgame, "WDL EGTB");

#ifdef USE_COMPRESSED_WDL
    HARDBUG(!gwdegtb_wdl_compressed_attach(endgame_name, pendgame, egtb_size));

#else
    HARDBUG(!gwdegtb_wdl_attach(endgame_name, pendgame, egtb_size))
#endif
    PRINTF("..loaded %zu bytes\n", egtb_size);

    egtb_ram += egtb_size;

  } // cJSON_ArrayForEach(egtb, egtbs)

#ifdef USE_COMPRESSED_WDL
  size_t wdl_probe_cache = 256 * MBYTE;

  HARDBUG(
    !gwdegtb_wdl_probe_create(wdl_probe_cache, &wdl_probe_alpha_beta_master));

  PRINTF("EGTB probe cache is %zu MBYTE\n", wdl_probe_cache / MBYTE + 1);

  egtb_ram += wdl_probe_cache;
#endif

  PRINTF("total EGTB ram is %zu MBYTE\n", egtb_ram / MBYTE + 1);

  PRINTF("..done\n");
}

int read_endgame(search_t *arg_with_search, const char **arg_dtm_missing_name)
{
  if (arg_dtm_missing_name != NULL)
    *arg_dtm_missing_name = NULL;

  board_t *with_board = &(arg_with_search->S_board);

  HARDBUG(compat_mutex_lock(&dtm_probe_mutex) != 0)

  int result = gwdegtb_dtm_lookup(options.egtb_dir,
                                  16 * MBYTE,
                                  with_board->B_white_king_bb,
                                  with_board->B_white_man_bb,
                                  with_board->B_black_king_bb,
                                  with_board->B_black_man_bb,
                                  with_board->B_colour2move == WHITE_ENUM
                                    ? GWDEGTB_WHITE_TO_MOVE
                                    : GWDEGTB_BLACK_TO_MOVE);

  HARDBUG(compat_mutex_unlock(&dtm_probe_mutex) != 0)

  if (result == GWDEGTB_DTM_UNAVAILABLE)
  {
    if (arg_dtm_missing_name != NULL)
      *arg_dtm_missing_name = gwdegtb_dtm_missing_name();

    result = ENDGAME_UNKNOWN;
  }

  return (result);
}

score_t return_endgame_score(search_t *arg_with_search)
{
  score_t result = SCORE_NOT_A_NUMBER;

  board_t *with_board = &(arg_with_search->S_board);

  if (BIT_COUNT(with_board->B_white_king_bb | with_board->B_white_man_bb |
                with_board->B_black_king_bb | with_board->B_black_man_bb) >
      EGTB_MAX_PIECES)
    return (result);

  int temp_mate = ENDGAME_UNKNOWN;

#ifdef USE_COMPRESSED_WDL
  GwdegtbWdlProbe *wdl_probe;

  if (arg_with_search->S_thread == thread_alpha_beta_master)
    wdl_probe = wdl_probe_alpha_beta_master;
  else
    wdl_probe = arg_with_search->S_wdl_probe;

  temp_mate = gwdegtb_wdl_lookup_probe(wdl_probe,
                                       with_board->B_white_king_bb,
                                       with_board->B_white_man_bb,
                                       with_board->B_black_king_bb,
                                       with_board->B_black_man_bb,
                                       with_board->B_colour2move == WHITE_ENUM
                                         ? GWDEGTB_WHITE_TO_MOVE
                                         : GWDEGTB_BLACK_TO_MOVE);
#else
  temp_mate = gwdegtb_wdl_lookup(with_board->B_white_king_bb,
                                 with_board->B_white_man_bb,
                                 with_board->B_black_king_bb,
                                 with_board->B_black_man_bb,
                                 with_board->B_colour2move == WHITE_ENUM
                                   ? GWDEGTB_WHITE_TO_MOVE
                                   : GWDEGTB_BLACK_TO_MOVE);
#endif

  if (temp_mate == GWDEGTB_WDL_UNAVAILABLE)
    temp_mate = ENDGAME_UNKNOWN;
  else if (temp_mate == GWDEGTB_WDL_DRAW)
    temp_mate = INVALID;
  else if (temp_mate == GWDEGTB_WDL_WIN)
    temp_mate = 1;
  else if (temp_mate == GWDEGTB_WDL_LOSS)
    temp_mate = -2;
  else
    FATAL("unknown temp_mate", EXIT_FAILURE)

  if (temp_mate == ENDGAME_UNKNOWN)
    return (result);

  if (temp_mate == INVALID)
  {
    result = SCORE(0);
  }
  else
  {
    int inode = 0;

    if (arg_with_search != NULL)
      inode = arg_with_search->S_board.B_inode;

    if (temp_mate > 0)
      result = SCORE(SCORE_WON - inode - temp_mate);
    else
      result = SCORE(SCORE_LOST + inode - temp_mate);
  }

  return (result);
}

void fin_endgame(void)
{
  GwdegtbWdlProbeStatistics statistics;

  gwdegtb_wdl_probe_statistics(wdl_probe_alpha_beta_master, &statistics);

  PRINTF("WDL probe cache:\n");
  PRINTF(
    "  requested       %llu MiB\n",
    (unsigned long long)(statistics.requested_cache_bytes / (1024 * 1024)));
  PRINTF(
    "  allocated       %llu MiB\n",
    (unsigned long long)(statistics.allocated_cache_bytes / (1024 * 1024)));
  PRINTF("  entries         %llu\n",
         (unsigned long long)statistics.cache_entries);
  PRINTF("  lookups         %llu\n", (unsigned long long)statistics.lookups);
  PRINTF("  hits            %llu\n", (unsigned long long)statistics.hits);
  PRINTF("  misses          %llu\n", (unsigned long long)statistics.misses);
  PRINTF("  hit rate        %.2f%%\n",
         statistics.lookups ? 100.0 * statistics.hits / statistics.lookups
                            : 0.0);
  PRINTF("  decompressions  %llu\n",
         (unsigned long long)statistics.decompressions);
}
