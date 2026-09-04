// SCU REVISION 8.12462 vr  4 sep 2026  7:33:55 CEST
#include "globals.h"

#define NAG '$'

#define RESULT_WON "1-0"
#define RESULT_DRAW "1/2-1/2"
#define RESULT_LOST "0-1"
#define RESULT_UNKNOWN "*"

#define TOKEN_SKIP 0
#define TOKEN_MOVE 1
#define TOKEN_WON 2
#define TOKEN_DRAW 3
#define TOKEN_LOST 4
#define TOKEN_UNKNOWN 5

#define NPLY_PAUSE 10

local int
get_next_token(bstring arg_bstring, int *arg_ichar, bstring arg_btoken)
{
  int result = INVALID;

  HARDBUG(bassigncstr(arg_btoken, "NULL") == BSTR_ERR)

  while ((*arg_ichar < blength(arg_bstring)) and
         (bchar(arg_bstring, *arg_ichar) == ' '))
    (*arg_ichar)++;

  if (*arg_ichar >= blength(arg_bstring))
    return (result);

  int jchar = *arg_ichar;

  if (strncmp(bdata(arg_bstring) + jchar, RESULT_WON, strlen(RESULT_WON)) == 0)
  {
    jchar += strlen("1-0");

    result = TOKEN_WON;
  }
  else if (strncmp(bdata(arg_bstring) + jchar,
                   RESULT_DRAW,
                   strlen(RESULT_DRAW)) == 0)
  {
    jchar += strlen("1/2-1/2");

    result = TOKEN_DRAW;
  }
  else if (strncmp(bdata(arg_bstring) + jchar,
                   RESULT_LOST,
                   strlen(RESULT_LOST)) == 0)
  {
    jchar += strlen("0-1");

    result = TOKEN_LOST;
  }
  else if (strncmp(bdata(arg_bstring) + jchar,
                   RESULT_UNKNOWN,
                   strlen(RESULT_UNKNOWN)) == 0)
  {
    jchar += strlen("*");

    result = TOKEN_UNKNOWN;
  }
  else if ((bchar(arg_bstring, jchar) == '"') or
           (bchar(arg_bstring, jchar) == '[') or
           (bchar(arg_bstring, jchar) == '(') or
           (bchar(arg_bstring, jchar) == '{'))
  {
    char bopen = bchar(arg_bstring, jchar);

    char bclose = '"';

    if (bopen == '[')
      bclose = ']';

    if (bopen == '(')
      bclose = ')';

    if (bopen == '{')
      bclose = '}';

    jchar++;

    int nest = 1;

    while (jchar < blength(arg_bstring))
    {
      if (bchar(arg_bstring, jchar) == bopen)
        ++nest;

      if (bchar(arg_bstring, jchar) == bclose)
      {
        if (--nest == 0)
          break;
      }

      jchar++;
    }
    HARDBUG(nest != 0)

    jchar++;

    result = TOKEN_SKIP;
  }
  else if (bchar(arg_bstring, jchar) == NAG)
  {
    jchar++;

    while ((jchar < blength(arg_bstring)) and
           isdigit(bchar(arg_bstring, jchar)))
      jchar++;

    result = TOKEN_SKIP;
  }
  else
  {
    // move number or move

    // read until space or '.'

    while (jchar < blength(arg_bstring))
    {
      if (bchar(arg_bstring, jchar) == ' ')
        break;
      if (bchar(arg_bstring, jchar) == '.')
        break;

      jchar++;
    }

    result = TOKEN_MOVE;

    if ((jchar < blength(arg_bstring)) and (bchar(arg_bstring, jchar) == '.'))
    {
      jchar++;

      result = TOKEN_SKIP;
    }
  }

  HARDBUG(jchar <= *arg_ichar)

  HARDBUG(
    bassignmidstr(arg_btoken, arg_bstring, *arg_ichar, jchar - *arg_ichar) ==
    BSTR_ERR)

  *arg_ichar = jchar;

  return (result);
}

void read_games(my_sqlite3_t *arg_db, char *arg_pdn_name)
{
  BSTRING(string);

  file2bstring(arg_pdn_name, string);

  board_t board;

  construct_board(&board, STDOUT, NETWORK0_ID);

  board_t *with_board = &board;

  bucket_t bucket_nman;

  bucket_t bucket_rows[NPIECES_MAX + 1];

  bucket_t bucket_kings[NPIECES_MAX + 1];

  bucket_t bucket_nmoves[NPIECES_MAX + 1];

  construct_bucket(&bucket_nman,
                   "bucket_nman",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)
    construct_bucket(bucket_rows + npieces,
                     "bucket_rows",
                     1,
                     0,
                     9,
                     BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)
    construct_bucket(bucket_kings + npieces,
                     "bucket_kings",
                     1,
                     0,
                     NPIECES_MAX,
                     BUCKET_LINEAR);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)
    construct_bucket(bucket_nmoves + npieces,
                     "bucket_nmoves",
                     1,
                     0,
                     NMOVES_MAX,
                     BUCKET_LINEAR);

  i64_t ngames = 0;
  i64_t nerrors = 0;

  game_state_t game_state;

  int ichar = 0;

  BSTRING(btoken)

  BSTRING(bmove_string)

  BSTRING(bfen)

  BSTRING(bmove)

  construct_game_state(&game_state);

  while (TRUE)
  {
    int itoken = INVALID;

    // read game into game_state

    game_state.clear_moves(&game_state);

    while (TRUE)
    {
      // read tokens until result

      itoken = get_next_token(string, &ichar, btoken);

      if (itoken == INVALID)
        break;

      // PRINTF("itoken=%d btoken=%s\n", itoken, bdata(btoken));

      if (itoken == TOKEN_SKIP)
        continue;

      if (itoken != TOKEN_MOVE)
        break;

      game_state.push_move(&game_state, bdata(btoken), "");
    }

    if (itoken == INVALID)
      break;

    if (itoken == TOKEN_UNKNOWN)
      continue;

    ++ngames;

    if ((ngames % 1000) == 0)
      PRINTF("ngames=%lld\n", ngames);

    // replay the game

    fen2board(with_board, game_state.get_starting_position(&game_state));

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_state.get_moves(&game_state))
    {
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(with_board, &moves_list);

      // update statistics

      update_bucket(&bucket_nman, BIT_COUNT(with_board->B_black_man_bb));

      for (int i = 1; i <= 50; ++i)
      {
        int ifield = square2field[i];

        if (with_board->B_black_man_bb & BITULL(ifield))
        {
          update_bucket(bucket_rows + BIT_COUNT(with_board->B_black_man_bb),
                        (i - 1) / 5);
        }
      }

      update_bucket(bucket_kings + BIT_COUNT(with_board->B_black_man_bb),
                    BIT_COUNT(with_board->B_black_king_bb));

      if (IS_WHITE(with_board->B_colour2move))
      {
        update_bucket(bucket_nmoves + BIT_COUNT(with_board->B_white_man_bb),
                      moves_list.ML_nmoves);
      }
      else
      {
        update_bucket(bucket_nmoves + BIT_COUNT(with_board->B_black_man_bb),
                      moves_list.ML_nmoves);
      }

      // update book

#ifdef DEBUG
      check_moves(with_board, &moves_list);
#endif

      cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(move_string))

      HARDBUG(bassigncstr(bmove_string, cJSON_GetStringValue(move_string)) ==
              BSTR_ERR)

      int imove = INVALID;

      if ((imove = search_move(&moves_list, bmove_string)) == INVALID)
      {
        print_board(with_board);

        PRINTF("move not found: %s\n", bdata(bmove_string));

        fprintf_moves_list(&moves_list, STDOUT, TRUE);

        ++nerrors;

        break;
      }

      if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
      {
        board2fen(with_board, bfen, FALSE);

        char *column;

        if ((IS_WHITE(with_board->B_colour2move) and (itoken == TOKEN_WON)) or
            (IS_BLACK(with_board->B_colour2move) and (itoken == TOKEN_LOST)))
        {
          column = "nwon";
        }
        else if ((IS_WHITE(with_board->B_colour2move) and
                  (itoken == TOKEN_LOST)) or
                 (IS_BLACK(with_board->B_colour2move) and
                  (itoken == TOKEN_WON)))
        {
          column = "nlost";
        }
        else
        {
          HARDBUG(itoken != TOKEN_DRAW)

          column = "ndraw";
        }

        int material = return_material_score(with_board);

        append_sql_buffer(
          arg_db,
          "INSERT INTO positions (fen, material, frequency, %s) "
          "VALUES ('%s', %d, 1, 1) "
          "ON CONFLICT(fen) DO UPDATE SET "
          "frequency = frequency + 1, %s = %s + 1;",
          column,
          bdata(bfen),
          material,
          column,
          column);

        move2bstring(&moves_list, imove, bmove);

        append_sql_buffer(arg_db,
                          "INSERT INTO moves "
                          "(position_id, move, frequency, %s) "
                          "VALUES ("
                          "(SELECT id FROM positions WHERE fen = '%s'), "
                          "'%s', 1, 1"
                          ") "
                          "ON CONFLICT(position_id, move) DO UPDATE SET "
                          "frequency = frequency + 1, %s = %s + 1;",
                          column,
                          bdata(bfen),
                          bdata(bmove),
                          column,
                          column);
      }

      do_move(with_board, imove, &moves_list);
    }
  }

  flush_sql_buffer(arg_db, 0);

  destroy_game_state(&game_state);

  BDESTROY(bmove)

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  BDESTROY(btoken)

  BDESTROY(string)

  PRINTF("cleaning up..\n");

  const char *sql = "DELETE FROM positions WHERE frequency = 1;";

  HARDBUG(execute_sql(arg_db->MSQL_db, sql, FALSE) != SQLITE_OK)

  HARDBUG(execute_sql(arg_db->MSQL_db, "VACUUM;", FALSE) != SQLITE_OK)

  PRINTF("..done\n");

  HARDBUG(execute_sql(arg_db->MSQL_db,
                      "INSERT INTO evaluations (move_id, evaluation) "
                      "SELECT "
                      "m.id, "
                      "CAST(ROUND(1000.0 * "
                      "CASE WHEN (m.nwon + m.nlost) > 0 "
                      "THEN (m.nwon - m.nlost) * 1.0 / (m.nwon + m.nlost) "
                      "ELSE 0.0 "
                      "END"
                      ") AS INTEGER) "
                      "FROM moves m WHERE 1 "
                      "ON CONFLICT(move_id) DO UPDATE SET "
                      "evaluation = excluded.evaluation;",
                      FALSE) != SQLITE_OK)

  printf_bucket(&bucket_nman);

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)
  {
    PRINTF("npieces=%d\n", npieces);

    printf_bucket(bucket_rows + npieces);
  }

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)
  {
    PRINTF("npieces=%d\n", npieces);

    printf_bucket(bucket_kings + npieces);
  }

  for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)
  {
    PRINTF("npieces=%d\n", npieces);

    printf_bucket(bucket_nmoves + npieces);
  }

  PRINTF("ngames=%lld nerrors=%lld\n", ngames, nerrors);
}

void auto_play(char *arg_txt_name, i64_t arg_npositions)
{
#ifdef DEBUG
  arg_npositions = 1000;
#endif
  PRINTF("arg_txt_name=%s arg_npositions=%lld\n", arg_txt_name, arg_npositions);

  local my_random_t auto_play_random;

  if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_lead_slave)
    construct_my_random(&auto_play_random, NULL);
  else
  {
    ui64_t seed;

    construct_my_random(&auto_play_random, &seed);

    PRINTF("auto_play seed=%llX\n", seed);
  }

  BSTRING(btxt_name);

  HARDBUG(bassigncstr(btxt_name, arg_txt_name) == BSTR_ERR)

  fbuffer_t ftxt;

  construct_fbuffer(&ftxt, btxt_name, NULL, TRUE);

  my_timer_t timer;

  construct_my_timer(&timer, "auto_play", STDOUT, FALSE);

  reset_my_timer(&timer);

  search_t search0;

  construct_search(&search0, STDOUT, NULL, NETWORK0_ID);

  HARDBUG(networks[NETWORK1_ID].N_network_id == INVALID)

  search_t search1;

  construct_search(&search1, STDOUT, NULL, NETWORK1_ID);

  progress_t progress;

  construct_progress(&progress, "auto_play", arg_npositions, 10);

  i64_t ngames = 0;
  i64_t ngames_failed = 0;

  BSTRING(bmove_string)

  BSTRING(bfen)

  game_state_t game_state;

  construct_game_state(&game_state);

  bucket_t bucket_result0;

  construct_bucket(&bucket_result0,
                   "bucket_result0",
                   1.0,
                   -1.0,
                   1.0,
                   BUCKET_LINEAR);

  while (return_ntodo(&progress) < arg_npositions)
  {
    ++ngames;

    PRINTF("ngames=%lld\n", ngames);

    fen2board(&(search0.S_board),
              game_state.get_starting_position(&game_state));

    fen2board(&(search1.S_board),
              game_state.get_starting_position(&game_state));

    game_state.clear_moves(&game_state);

    search_t *with_search = &search0;

    if ((ngames % 2) == 0)
      with_search = &search1;

    int nply = 0;

    int egtb_mate = ENDGAME_UNKNOWN;

    PRINTF("auto play..\n");

    while (TRUE)
    {
      if (with_search == &search0)
        PRINTF("search0 turn..\n");
      else if (with_search == &search1)
      {
        PRINTF("search1 turn..\n");

        options.material_only = TRUE;
      }

      else
        FATAL("with_search error", EXIT_FAILURE)

      egtb_mate = read_endgame(with_search);

      if (egtb_mate != ENDGAME_UNKNOWN)
        break;

      if (nply >= MCTS_PLY_MAX)
        break;

      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with_search->S_board), &moves_list);

      if (moves_list.ML_nmoves == 0)
        break;

      reset_my_timer(&(with_search->S_timer));

      if (nply > NPLY_PAUSE)
        pause_my_printf(with_search->S_my_printf);

      do_search(with_search,
                &moves_list,
                INVALID,
                INVALID,
                SCORE_MINUS_INFINITY,
                &auto_play_random);

      move2bstring(&moves_list, with_search->S_best_move, bmove_string);

      resume_my_printf(with_search->S_my_printf);

      print_board(&(with_search->S_board));

      PRINTF("nply=%d"
             " with_search->S_best_move=%s"
             " with_search->S_root_score=" PRINTF_SCORE_T
             " with_search->S_best_score=" PRINTF_SCORE_T "\n",
             nply,
             bdata(bmove_string),
             SCORE2PRINTF(with_search->S_root_score),
             SCORE2PRINTF(with_search->S_best_score));

      game_state.push_move(&game_state, bdata(bmove_string), "");

      ++nply;

      do_move(&(search0.S_board), with_search->S_best_move, &moves_list);
      do_move(&(search1.S_board), with_search->S_best_move, &moves_list);

      options.material_only = FALSE;

      if (with_search == &search0)
        with_search = &search1;
      else if (with_search == &search1)
        with_search = &search0;
      else
        FATAL("with_search error", EXIT_FAILURE)
    }

    resume_my_printf(with_search->S_my_printf);

    if (egtb_mate == ENDGAME_UNKNOWN)
    {
      ++ngames_failed;

      PRINTF("ngames=%lld ngames_failed=%lld (%.2f%%)\n",
             ngames,
             ngames_failed,
             ngames_failed * 100.0 / ngames);

      continue;
    }

    double result;

    if (egtb_mate == INVALID)
      result = 0.0;
    else if (egtb_mate > 0)
      result = 1.0;
    else
      result = -1.0;

    if (with_search == &search0)
    {
      PRINTF("search0 result=%.6f\n", result);

      update_bucket(&bucket_result0, result);
    }
    else if (with_search == &search1)
    {
      PRINTF("search1 result=%.6f\n", result);

      update_bucket(&bucket_result0, -result);
    }
    else
      FATAL("with_search error", EXIT_FAILURE)

    PRINTF("replay..\n");

    // result as seen from white

    if (IS_BLACK(with_search->S_board.B_colour2move))
      result = -result;

    // loop over all moves

    fen2board(&(search0.S_board),
              game_state.get_starting_position(&game_state));

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_state.get_moves(&game_state))
    {
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(search0.S_board), &moves_list);

      if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
      {
        board2fen(&(search0.S_board), bfen, FALSE);

        append_fbuffer_fmt(&ftxt, "%s {%.6f}\n", bdata(bfen), result);

        (void)make_progress(&progress, 0);
      }

      cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(move_string))

      HARDBUG(bassigncstr(bmove_string, cJSON_GetStringValue(move_string)) ==
              BSTR_ERR)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bmove_string)) == INVALID)

      do_move(&(search0.S_board), imove, &moves_list);
    }
  }

  PRINTF("generating %lld positions took %.2f seconds\n",
         return_ndone(&progress),
         return_my_timer(&timer, FALSE));

  finalize_progress(&progress);

  destroy_progress(&progress);

  flush_fbuffer(&ftxt, 0);

  destroy_game_state(&game_state);

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  my_mpi_barrier_v3("after auto_play",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_auto_play_barrier,
                    TRUE);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &ngames,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &ngames_failed,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_comm_slaves);

  PRINTF("my_mpi_all_reduce(ngames)=%lld\n", ngames);
  PRINTF("my_mpi_all_reduce(ngames_failed)=%lld\n", ngames_failed);

  my_mpi_bucket_aggregate(&bucket_result0, my_mpi_globals.MMG_comm_slaves);

  printf_bucket(&bucket_result0);
}

void gen_db(my_sqlite3_t *arg_db, int arg_mcts_depth, i64_t arg_npositions)
{
#ifdef DEBUG
  arg_npositions = 1000;
#endif
  PRINTF("arg_npositions=%lld\n", arg_npositions);

  my_timer_t timer;

  construct_my_timer(&timer, "gen_db", STDOUT, FALSE);

  reset_my_timer(&timer);

  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  search_t *with_search = &search;

  progress_t progress;

  construct_progress(&progress, "gen_db", arg_npositions, 10);

  i64_t ngames = 0;
  i64_t ngames_failed = 0;

  BSTRING(bmove_string)

  BSTRING(bfen)

  game_state_t game_state;

  construct_game_state(&game_state);

  while (return_ndone(&progress) < return_ntodo(&progress))
  {
    ++ngames;

    fen2board(&(with_search->S_board),
              game_state.get_starting_position(&game_state));

    game_state.clear_moves(&game_state);

    PRINTF("auto play\n");

    // auto play
    // no time limit

    mcts_globals.mcts_globals_time_limit = 0.0;

    int nply = 0;

    while (TRUE)
    {
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with_search->S_board), &moves_list);

      if (moves_list.ML_nmoves == 0)
        break;

      int sort[NMOVES_MAX];

      reset_my_timer(&(with_search->S_timer));

      score_t root_score = return_material_score(&(with_search->S_board));

      if (nply > NPLY_PAUSE)
        pause_my_printf(with_search->S_my_printf);

      score_t best_score;

      mcts_search(with_search,
                  arg_mcts_depth,
                  nply,
                  &moves_list,
                  sort,
                  &best_score,
                  FALSE);

      if (sort[0] == INVALID)
        break;

      if (nply >= MCTS_PLY_MAX)
        break;

      HARDBUG(sort[0] >= moves_list.ML_nmoves)

      move2bstring(&moves_list, sort[0], bmove_string);

      print_board(&(with_search->S_board));

      PRINTF("nply=%d"
             " sort[0]=%s"
             " root_score=" PRINTF_SCORE_T " best_score=" PRINTF_SCORE_T "\n",
             nply,
             bdata(bmove_string),
             SCORE2PRINTF(root_score),
             SCORE2PRINTF(best_score));

      game_state.push_move(&game_state, bdata(bmove_string), "");

      ++nply;

      do_move(&(with_search->S_board), sort[0], &moves_list);
    }

    resume_my_printf(with_search->S_my_printf);

    int egtb_mate = read_endgame(with_search);

    if (egtb_mate == ENDGAME_UNKNOWN)
    {
      ++ngames_failed;

      PRINTF("ngames=%lld ngames_failed=%lld (%.2f%%)\n",
             ngames,
             ngames_failed,
             ngames_failed * 100.0 / ngames);

      continue;
    }

    int result = INVALID;

    if (egtb_mate == INVALID)
      result = 1;
    else if (egtb_mate > 0)
      result = 2;
    else
      result = 0;

    HARDBUG(result == INVALID)

    // result as seen from white

    if (IS_BLACK(with_search->S_board.B_colour2move))
      result = 2 - result;

    PRINTF("replay\n");

    // loop over all moves

    fen2board(&(with_search->S_board),
              game_state.get_starting_position(&game_state));

    cJSON *game_move;

    cJSON_ArrayForEach(game_move, game_state.get_moves(&game_state))
    {
      moves_list_t moves_list;

      construct_moves_list(&moves_list);

      gen_moves(&(with_search->S_board), &moves_list);

      if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
      {
        board2fen(&(with_search->S_board), bfen, FALSE);

        if ((IS_WHITE(with_search->S_board.B_colour2move) and (result == 2)) or
            (IS_BLACK(with_search->S_board.B_colour2move) and (result == 0)))
        {
          append_sql_buffer(arg_db,
                            "INSERT INTO positions (fen, frequency, nwon) "
                            "VALUES ('%s', 1, 1) "
                            "ON CONFLICT(fen) DO UPDATE SET "
                            "frequency = frequency + 1, nwon = nwon + 1;",
                            bdata(bfen));
        }
        else if ((IS_WHITE(with_search->S_board.B_colour2move) and
                  (result == 0)) or
                 (IS_BLACK(with_search->S_board.B_colour2move) and
                  (result == 2)))
        {
          append_sql_buffer(arg_db,
                            "INSERT INTO positions (fen, frequency, nlost) "
                            "VALUES ('%s', 1, 1) "
                            "ON CONFLICT(fen) DO UPDATE SET "
                            "frequency = frequency + 1, nlost = nlost + 1;",
                            bdata(bfen));
        }
        else
        {
          HARDBUG(result != 1)

          append_sql_buffer(arg_db,
                            "INSERT INTO positions (fen, frequency, ndraw) "
                            "VALUES ('%s', 1, 1) "
                            "ON CONFLICT(fen) DO UPDATE SET "
                            "frequency = frequency + 1, ndraw = ndraw + 1;",
                            bdata(bfen));
        }

        (void)make_progress(&progress, 0);
      }

      cJSON *move_string = cJSON_GetObjectItem(game_move, CJSON_MOVE_STRING_ID);

      HARDBUG(!cJSON_IsString(move_string))

      HARDBUG(bassigncstr(bmove_string, cJSON_GetStringValue(move_string)) ==
              BSTR_ERR)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bmove_string)) == INVALID)

      do_move(&(with_search->S_board), imove, &moves_list);
    }
  }

  PRINTF("generating %lld positions took %.2f seconds\n",
         return_ndone(&progress),
         return_my_timer(&timer, FALSE));

  finalize_progress(&progress);

  destroy_progress(&progress);

  flush_sql_buffer(arg_db, 0);

  destroy_game_state(&game_state);

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  my_mpi_barrier_v3("after gen",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_gen_db_barrier,
                    TRUE);
}

#define IDS_NAME "ids.bin"

void update_db(my_sqlite3_t *arg_db, int arg_mcts_depth, int arg_frequency)
{
  mcts_globals.mcts_globals_time_limit = 0.0;

  FILE *fids;

  i64_t npositions = 0;

  if (my_mpi_globals.MMG_is_lead_slave)
  {
    HARDBUG((fids = fopen(IDS_NAME, "wb")) == NULL)

    char *sql = "SELECT id FROM positions WHERE"
                " frequency < ?"
                " ORDER BY id ASC;";

    sqlite3_stmt *stmt;

    my_sqlite3_prepare_v2(arg_db->MSQL_db, sql, &stmt);

    HARDBUG(my_sqlite3_bind_int(stmt, 1, arg_frequency) != SQLITE_OK)

    int rc;

    while ((rc = execute_sql(arg_db->MSQL_db, stmt, TRUE)) == SQLITE_ROW)
    {
      i64_t id = my_sqlite3_column_int64(stmt, 0);

      FWRITE(fids, &id, i64_t, 1)

      npositions++;
    }

    HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

    FCLOSE(fids)
  }

  my_mpi_barrier_v3("after query ids",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_update_db_barrier,
                    FALSE);

  MPI_Bcast(&npositions, 1, MPI_INT64_T, 0, my_mpi_globals.MMG_comm_slaves);

  PRINTF("npositions=%lld\n", npositions);

  my_timer_t timer;

  construct_my_timer(&timer, "update_db", STDOUT, FALSE);

  reset_my_timer(&timer);

  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  search_t *with_search = &search;

  progress_t progress;

  construct_progress(&progress, "update_db", npositions, 10);

  BSTRING(bmove_string)

  BSTRING(bfen)

  HARDBUG((fids = fopen(IDS_NAME, "rb")) == NULL)

  for (i64_t iposition = 0; iposition < npositions; iposition++)
  {
    i64_t id;

    FREAD(fids, &id, i64_t, 1)

    if (!make_progress(&progress, iposition))
      continue;

    HARDBUG(id < 1)

    PRINTF("id=%lld\n", id);

    char *sql = "SELECT fen, frequency FROM positions WHERE id = ?;";

    sqlite3_stmt *stmt;

    my_sqlite3_prepare_v2(arg_db->MSQL_db, sql, &stmt);

    HARDBUG(my_sqlite3_bind_int64(stmt, 1, id) != SQLITE_OK)

    HARDBUG(execute_sql(arg_db->MSQL_db, stmt, TRUE) != SQLITE_ROW)

    const unsigned char *fen = my_sqlite3_column_text(stmt, 0);

    HARDBUG(bassigncstr(bfen, (char *)fen) == BSTR_ERR)

    int frequency = my_sqlite3_column_int(stmt, 1);

    HARDBUG(frequency >= arg_frequency)

    HARDBUG(my_sqlite3_finalize(stmt) != SQLITE_OK)

    PRINTF("iposition=%lld id=%lld frequency=%d fen=%s\n$",
           iposition,
           id,
           frequency,
           bdata(bfen));

    fen2board(&(with_search->S_board), bdata(bfen));

    int nshoot_outs = arg_frequency - frequency;

    int nwon = 0;
    int ndraw = 0;
    int nlost = 0;

    double mcts_result = mcts_shoot_outs(with_search,
                                         arg_mcts_depth,
                                         0,
                                         0.0,
                                         &nshoot_outs,
                                         &nwon,
                                         &ndraw,
                                         &nlost,
                                         NULL,
                                         NULL,
                                         FALSE);

    nshoot_outs = nwon + ndraw + nlost;

    PRINTF("nshoot_outs=%d"
           " nwon=%d"
           " ndraw=%d"
           " nlost=%d"
           " mcts_result=%.6f\n$",
           nshoot_outs,
           nwon,
           ndraw,
           nlost,
           mcts_result);

    HARDBUG(nshoot_outs > (arg_frequency - frequency))

    if (nshoot_outs < (arg_frequency - frequency))
    {
      print_board(&(with_search->S_board));

      PRINTF("WARNING: nshoot_outs < (arg_frequency - frequency)\n");
    }

    append_sql_buffer(
      arg_db,
      "UPDATE positions SET "
      "frequency = frequency + %d,"
      "nwon = nwon + %d, ndraw = ndraw + %d, nlost = nlost + %d "
      "WHERE fen = '%s';",
      nshoot_outs,
      nwon,
      ndraw,
      nlost,
      bdata(bfen));
  }

  FCLOSE(fids)

  finalize_progress(&progress);

  destroy_progress(&progress);

  flush_sql_buffer(arg_db, 0);

  BDESTROY(bfen)

  BDESTROY(bmove_string)

  my_mpi_barrier_v3("after update_db",
                    my_mpi_globals.MMG_comm_slaves,
                    my_mpi_globals.MMG_semkey_update_db_barrier,
                    TRUE);
}
