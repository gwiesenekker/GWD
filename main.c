// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "main.h"

#include "boards.h"
#include "book.h"
#include "bstrlib.h"
#include "buckets.h"
#include "cJSON.h"
#include "caches.h"
#include "classes.h"
#include "common.h"
#include "dxp.h"
#include "endgame.h"
#include "fbuffer.h"
#include "hub.h"
#include "mcts.h"
#include "moves.h"
#include "my_bstreams.h"
#include "my_cjson.h"
#include "my_malloc.h"
#include "my_mpi.h"
#include "my_printf.h"
#include "my_random.h"
#include "my_safec.h"
#include "my_sqlite3.h"
#include "my_threads.h"
#include "my_valgrind.h"
#include "networks.h"
#include "pdn.h"
#include "profile.h"
#include "queues.h"
#include "records.h"
#include "revision.h"
#include "score.h"
#include "search.h"
#include "states.h"
#include "stats.h"
#include "timers.h"
#include "utils.h"

#define GWD_JSON "gwd.json"
#define OVERRIDES_JSON "overrides.json"

enum
{
  NMAN_MIN = 4,
  NPIECES_MIN = 7
};

local const double PROBABILITY_EQUAL = 0.5;

void *STDOUT;

my_random_t main_random;

queue_t main_queue;

// options

options_t options;

local void fen2score(const char *arg_name)
{
  BEGIN_BLOCK(__FUNC__)

  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  board_t *with_board = &(search.S_board);

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  BSTRING(bfen)

  int nproblems = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '*')
      break;

    if (bchar(bline, 0) == '#')
      continue;

    if (strncmp(bdata(bline), "//", 2) == 0)
      continue;

    CSTRING(cfen, blength(bline))

    HARDBUG(my_sscanf(bdata(bline), "%s", cfen) != 1)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(&(search.S_board), bdata(bfen));

    CDESTROY(cfen)

    ++nproblems;

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with_board, &moves_list);

    check_moves(with_board, &moves_list);

    int material_score = return_material_score(with_board);

    int positional_score = return_score_from_board(with_board, NULL);

    /*
        if ((abs(material_score) >= (3 * SCORE_MAN)) and
              ((material_score > 0) and (positional_score < 0)) or
            ((material_score < 0) and (positional_score > 0)))
    */
    {
      print_board(with_board);

      PRINTF("material_score=%d positional_score=%d\n",
             material_score,
             positional_score);
    }
  }

  destroy_my_bstream(&my_bstream);

  BDESTROY(bfen)

  PRINTF("nproblems=%d\n", nproblems);

  END_BLOCK
}

local void solve_problems(const char *arg_name)
{
  BEGIN_BLOCK(__FUNC__)

  FILE *ftmp;

  HARDBUG((ftmp = fopen("tmp.fen", "w")) == NULL)

  search_t search0;

  construct_search(&search0, STDOUT, NULL, NETWORK0_ID);

  search_t *with_search0 = &search0;

  search_t search1;

  search_t *with_search1 = NULL;

  if (networks[NETWORK1_ID].N_network_id != INVALID)
  {
    construct_search(&search1, STDOUT, NULL, NETWORK1_ID);

    with_search1 = &search1;
  }

  // print_board(&(with_search->S_board));

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  BSTRING(bfen)

  int nproblems = 0;

  int nsolved = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '*')
      break;

    if (bchar(bline, 0) == '#')
      continue;

    if (strncmp(bdata(bline), "//", 2) == 0)
      continue;

    CSTRING(cfen, blength(bline))

    HARDBUG(my_sscanf(bdata(bline), "%s", cfen) != 1)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(&(with_search0->S_board), bdata(bfen));

    CDESTROY(cfen)

    ++nproblems;

    print_board(&(with_search0->S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with_search0->S_board), &moves_list);

    check_moves(&(with_search0->S_board), &moves_list);

    fprintf_moves_list(&moves_list, STDOUT, FALSE);

    BSTRING(book_move)

    if (options.use_book)
      return_book_move(&(with_search0->S_board), &moves_list, book_move);
    else
      PRINTF("WARNING: book is not enabled!\n");

    BDESTROY(book_move)

    if (moves_list.ML_nmoves > 0)
    {
      // options.hub=1;

      do_search(with_search0,
                &moves_list,
                INVALID,
                INVALID,
                SCORE_MINUS_INFINITY,
                NULL);

      if ((with_search0->S_best_score - with_search0->S_root_simple_score) >
          (SCORE_MAN / 2))
      {
        ++nsolved;
        PRINTF("solved problem #%d\n", nproblems);
      }
      else
      {
        PRINTF("did not solve problem #%d\n", nproblems);

        HARDBUG(fprintf(ftmp, "%s\n", bdata(bfen)) < 0)
      }

      if (with_search1 != NULL)
      {
        fen2board(&(with_search1->S_board), bdata(bfen));

        print_board(&(with_search1->S_board));

        gen_moves(&(with_search1->S_board), &moves_list);

        check_moves(&(with_search1->S_board), &moves_list);

        fprintf_moves_list(&moves_list, STDOUT, FALSE);

        do_search(with_search1,
                  &moves_list,
                  INVALID,
                  INVALID,
                  SCORE_MINUS_INFINITY,
                  NULL);
      }
    }
  }

  destroy_my_bstream(&my_bstream);

  BDESTROY(bfen)

  PRINTF("solved %d out of %d problems\n", nsolved, nproblems);

  END_BLOCK
}

local void solve_problems_mcts(const char *arg_name,
                               int arg_mcts_depth,
                               double arg_mcts_time_limit,
                               int arg_nshoot_outs)
{
  BEGIN_BLOCK(__FUNC__)

  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  search_t *with_search = &search;

  // print_board(&(with_search->S_board));

  fbuffer_t fcsv;

  BSTRING(barg_name)

  HARDBUG(bassigncstr(barg_name, arg_name) == BSTR_ERR)

  construct_fbuffer(&fcsv, barg_name, ".csv", TRUE);

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  my_int64_t nfen = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '*')
      break;

    if (bchar(bline, 0) == '#')
      continue;

    nfen++;
  }

  PRINTF("nfen=%lld\n", nfen);

  destroy_my_bstream(&my_bstream);

  progress_t progress;

  construct_progress(&progress, "solve_problems_mcts", nfen, 10);

  construct_my_bstream(&my_bstream, arg_name);

  BSTRING(bfen)

  for (my_int64_t ifen = 0; ifen < nfen; ifen++)
  {
    HARDBUG(my_bstream_readln(&my_bstream, TRUE) == BSTR_ERR)

    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '*')
      break;

    if (bchar(bline, 0) == '#')
      continue;

    if (!make_progress(&progress, ifen))
      continue;

    CSTRING(cfen, blength(bline))

    double result;

    HARDBUG(my_sscanf(bdata(bline), "%s {%lf}", cfen, &result) != 2)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    fen2board(&(with_search->S_board), bdata(bfen));

    CDESTROY(cfen)

    if (is_black(with_search->S_board.B_colour2move))
      result = -result;

    print_board(&(with_search->S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with_search->S_board), &moves_list);

    check_moves(&(with_search->S_board), &moves_list);

    fprintf_moves_list(&moves_list, STDOUT, FALSE);

    if (moves_list.ML_nmoves > 0)
    {
      double mcts_result = 0.0;

      if (arg_mcts_depth > 0)
      {
        int nshoot_outs = arg_nshoot_outs;

        mcts_result = mcts_shoot_outs(with_search,
                                      arg_mcts_depth,
                                      0,
                                      arg_mcts_time_limit,
                                      &nshoot_outs,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      NULL,
                                      FALSE);
      }
      else
      {
        mcts_globals.mcts_globals_time_limit = arg_mcts_time_limit;

        mcts_globals.mcts_globals_nnodes = 0;

        reset_my_timer(&(with_search->S_timer));

        int sort[NMOVES_MAX];

        score_t best_score;

        mcts_search(with_search,
                    -arg_mcts_depth,
                    0,
                    &moves_list,
                    sort,
                    &best_score,
                    TRUE);

        mcts_result = best_score;
      }

      PRINTF("mcts_result=%.6f\n", mcts_result);

      append_fbuffer_fmt(&fcsv, "%.6f,%.6f\n", result, mcts_result);
    }
  }

  BDESTROY(bfen)

  finalize_progress(&progress);

  destroy_progress(&progress);

  destroy_my_bstream(&my_bstream);

  flush_fbuffer(&fcsv, 0);

  BDESTROY(barg_name)

  END_BLOCK
}

local void solve_problems_multi_threaded(const char *arg_name)
{
  enqueue(return_thread_queue(thread_alpha_beta_master),
          MESSAGE_SOLVE,
          arg_name);

  while (TRUE)
  {
    message_t message;

    if (dequeue(&main_queue, &message) != INVALID)
    {
      if (message.message_id == MESSAGE_INFO)
      {
        PRINTF("got info %s\n", bdata(message.message_text));
      }
      else if (message.message_id == MESSAGE_READY)
      {
        break;
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }

    HARDBUG(compat_sleep(CENTI_SECOND) != 0)
  }
}

local char *secs2string(int arg_seconds, bstring arg_string)
{
  int hours;
  int minutes;
  int seconds;

  hours = arg_seconds / 3600;
  minutes = (arg_seconds - hours * 3600) / 60;
  seconds = arg_seconds - hours * 3600 - minutes * 60;

  HARDBUG((hours * 3600 + minutes * 60 + seconds) != arg_seconds)

  HARDBUG(
    bassignformat(arg_string, "%02d:%02d:%02d", hours, minutes, seconds) ==
    BSTR_ERR)

  return (bdata(arg_string));
}

enum
{
  GAME_MOVES = 40
};

void play_game(const char *arg_name,
               const char *arg_colour,
               int arg_game_minutes,
               int arg_used_minutes)
{
  my_random_t play_random;

  construct_my_random(&play_random, NULL);

  game_state_t game_state;

  construct_game_state(&game_state);

  BSTRING(btime)
  BSTRING(line)
  BSTRING(comment)
  BSTRING(bbest_move)
  BSTRING(banswer)

  if (fexists(arg_name))
  {
    game_state.load(&game_state, arg_name);
  }
  else
  {
    if (*arg_colour == 'w')
    {
      game_state.set_white(&game_state, "Human");
      game_state.set_black(&game_state, "GWD");
    }
    else
    {
      game_state.set_white(&game_state, "GWD");
      game_state.set_black(&game_state, "Human");
    }
  }

  colour_enum human_colour;

  if (*arg_colour == 'w')
    human_colour = WHITE_ENUM;
  else
    human_colour = BLACK_ENUM;

  double game_time = arg_game_minutes * 60.0;
  double game_time_used = arg_used_minutes * 60.0;

  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  search_t *with_search = &search;

  int automatic = FALSE;

  int ndone = 0;

  while (TRUE)
  {
    enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_STATE,
            game_state.get_game_state(&game_state));

    state2board(&(with_search->S_board), &game_state);

    print_board(&(with_search->S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with_search->S_board), &moves_list);

    if (moves_list.ML_nmoves == 0)
    {
      PRINTF("mate!\n");

      goto label_return;
    }

    if (with_search->S_board.B_colour2move == human_colour)
    {
      // configure thread for pondering

      if (options.ponder)
      {
        options.time_limit = 10000.0;
        options.time_ntrouble = 0;

        PRINTF("\nPondering..\n");

        enqueue(return_thread_queue(thread_alpha_beta_master),
                MESSAGE_GO,
                "main/play_game");
      }

      if (!automatic and options.sync_clock and ((ndone % 9) == 0))
      {
        PRINTF("\nMinutes remaining "
               " (press <ENTER> to accept the default (%s)?\n",
               secs2string(round(game_time - game_time_used), btime));

        if (!my_breadline(line, stdin))
          goto label_return;

        int remaining_minutes;

        if (my_sscanf(bdata(line), "%d", &remaining_minutes) == 1)
        {
          game_time_used = game_time - remaining_minutes * 60.0;

          PRINTF("remaining %s minutes\n",
                 secs2string(round(game_time - game_time_used), btime));
        }
      }

      double start_time = compat_time();

      while (TRUE)
      {

        if (automatic)
        {
          HARDBUG(bassigncstr(banswer, ".") == BSTR_ERR)
        }
        else
        {
          PRINTF("\nEnter your move or any one of the following options:\n\n"
                 "'?': show the list of legal moves;\n"
                 "'b': show the book moves;\n"
                 "'u': undo a move;\n"
                 "'q': quit;\n"
                 "'.': play a random move;\n"
                 "'a': play random moves automatically from now on.\n\n");

          if (!my_breadline(line, stdin))
            goto label_return;

          HARDBUG(bassign(banswer, line) == BSTR_ERR)
          HARDBUG(bltrimws(banswer) == BSTR_ERR)

          int nanswer = 0;

          while ((nanswer < blength(banswer)) and
                 !isspace((unsigned char)bchar(banswer, nanswer)))
            ++nanswer;

          HARDBUG(btrunc(banswer, nanswer) == BSTR_ERR)

          if (nanswer == 0)
          {
            PRINTF("\neh?\n");

            continue;
          }
        }

        if (bchar(banswer, 0) == '?')
        {
          fprintf_moves_list(&moves_list, STDOUT, FALSE);

          continue;
        }

        if (bchar(banswer, 0) == 'b')
        {
          BSTRING(book_move)

          if (options.use_book)
            return_book_move(&(with_search->S_board), &moves_list, book_move);
          else
            PRINTF("WARNING: book is not enabled!\n");

          BDESTROY(book_move)

          continue;
        }

        if (bchar(banswer, 0) == 'u')
        {
          game_state.pop_move(&game_state);

          goto label_undo;
        }

        if (bchar(banswer, 0) == 'q')
        {
          enqueue(return_thread_queue(thread_alpha_beta_master),
                  MESSAGE_ABORT_SEARCH,
                  "main/play_game");

          goto label_return;
        }

        if (bchar(banswer, 0) == '.')
        {
          int imove = return_my_random(&play_random) % moves_list.ML_nmoves;

          move2bstring(&moves_list, imove, bbest_move);

          PRINTF("randomly selected move %s\n", bdata(bbest_move));

          break;
        }

        if (bchar(banswer, 0) == 'a')
        {
          automatic = TRUE;

          continue;
        }

        int imove = search_move(&moves_list, banswer);

        if (imove == INVALID)
        {
          fprintf_moves_list(&moves_list, STDOUT, FALSE);

          PRINTF("eh?\n\n");

          continue;
        }

        move2bstring(&moves_list, imove, bbest_move);

        break;
      }

      double end_time = compat_time();

      double wait_time = end_time - start_time;

      PRINTF("waited %s\n", secs2string(round(wait_time), btime));

      HARDBUG(bassignformat(comment, "%d", 0) == BSTR_ERR)

      game_state.push_move(&game_state, bdata(bbest_move), bdata(comment));

      ++ndone;

      // cancel pondering and dequeue messages

    label_undo:

      if (options.ponder)
      {
        enqueue(return_thread_queue(thread_alpha_beta_master),
                MESSAGE_ABORT_SEARCH,
                "main/play_game");

        while (TRUE)
        {
          message_t message;

          if (dequeue(&main_queue, &message) != INVALID)
          {
            if (message.message_id == MESSAGE_RESULT)
            {
              PRINTF("got hint %s\n", bdata(message.message_text));

              break;
            }
          }

          HARDBUG(compat_sleep(CENTI_SECOND) != 0)
        }
      }
    }
    else
    {

      int best_score;

      double start_time = compat_time();

      if (moves_list.ML_nmoves == 1)
      {
        move2bstring(&moves_list, 0, bbest_move);

        best_score = 0;

        goto label_break;
      }

      // check book move

      HARDBUG(bassigncstr(bbest_move, "NULL") == BSTR_ERR)

      if (options.use_book)
        return_book_move(&(with_search->S_board), &moves_list, bbest_move);

      if (compat_strcasecmp(bdata(bbest_move), "NULL") != 0)
      {
        PRINTF("book move=%s\n", bdata(bbest_move));

        best_score = 0;

        goto label_break;
      }

      // configure thread for search

      options.time_limit = (game_time - game_time_used) / GAME_MOVES;

      if (options.time_limit <= 0.1)
        options.time_limit = 0.1;

      options.time_trouble[0] =
        (game_time - game_time_used - options.time_limit) / GAME_MOVES;

      if (options.time_trouble[0] <= 0.1)
        options.time_trouble[0] = 0.1;

      options.time_trouble[1] = (game_time - game_time_used -
                                 options.time_limit - options.time_trouble[0]) /
                                GAME_MOVES;

      if (options.time_trouble[1] <= 0.1)
        options.time_trouble[1] = 0.1;

      options.time_ntrouble = 2;

      enqueue(return_thread_queue(thread_alpha_beta_master),
              MESSAGE_GO,
              "main/play_game");

      while (TRUE)
      {
        message_t message;

        if (dequeue(&main_queue, &message) != INVALID)
        {
          if (message.message_id == MESSAGE_INFO)
          {
            PRINTF("got info %s\n", bdata(message.message_text));
          }
          else if (message.message_id == MESSAGE_RESULT)
          {
            PRINTF("\ngot result %s\n", bdata(message.message_text));

            CSTRING(cbest_move, blength(message.message_text))

            HARDBUG(my_sscanf(bdata(message.message_text),
                              "%s%d",
                              cbest_move,
                              &best_score) != 2)

            HARDBUG(bassigncstr(bbest_move, cbest_move) == BSTR_ERR)

            CDESTROY(cbest_move)

            goto label_break;
          }
          else
            FATAL("message.message_id error", EXIT_FAILURE)
        }

        HARDBUG(compat_sleep(CENTI_SECOND) != 0)
      }

      double end_time;

    label_break:

      end_time = compat_time();

      double move_time = end_time - start_time;

      game_time_used += move_time;

      PRINTF("used   %s\n", secs2string(round(move_time), btime));

      PRINTF("total  %s\n", secs2string(round(game_time_used), btime));

      PRINTF("remain %s\n",
             secs2string(round(game_time - game_time_used), btime));

      PRINTF("\n* * * * * * * * * * %s %d\n\n", bdata(bbest_move), best_score);

      HARDBUG(bassignformat(comment, "%d", best_score) == BSTR_ERR)

      game_state.push_move(&game_state, bdata(bbest_move), bdata(comment));
    }
  }
label_return:

  game_state.save(&game_state, arg_name);

  game_state.save2pdn(&game_state, "play.pdn");

  BDESTROY(banswer)

  BDESTROY(bbest_move)

  BDESTROY(comment)

  BDESTROY(line)

  BDESTROY(btime)

  destroy_game_state(&game_state);
}

double weibull_shape(int arg_npieces)
{
  double amplitude = 0.66;
  double mean = 7.65;
  double sigma = 3.57;
  double offset = 1.87;

  return (amplitude * exp(-((arg_npieces - mean) * (arg_npieces - mean)) /
                          (2 * sigma * sigma)) +
          offset);
}

double weibull_scale(int arg_npieces)
{
  double amplitude = 3.66;
  double decay = 0.12;
  double offset = 2.74;

  return (amplitude * exp(-decay * arg_npieces) + offset);
}

int weibull_sample_row(double arg_probability,
                       double arg_shape,
                       double arg_scale)
{
  int result = INVALID;

  if (arg_probability <= 0.0)
    result = 1;
  else if (arg_probability >= 1.0)
    result = 9;
  else
  {
    result =
      round(arg_scale * pow(-log(1.0 - arg_probability), 1.0 / arg_shape));

    if (result < 1)
      result = 1;

    if (result > 9)
      result = 9;
  }

  return (result - 1);
}

local void test_weibull(void)
{
  for (int npieces = 1; npieces <= 20; npieces++)
  {
    double shape = weibull_shape(npieces);
    double scale = weibull_scale(npieces);

    for (int percentage = 1; percentage <= 99; ++percentage)
    {
      int row = weibull_sample_row(percentage / 100.0, shape, scale);

      PRINTF("npieces=%d percentage=%d row=%d\n", npieces, percentage, row + 1);
    }
  }
}

// 10 pieces 25%
local double probability_one_king(int arg_npieces)
{
  double amplitude = 0.83;
  double exponent = -0.12;

  return (amplitude * exp(exponent * arg_npieces));
}

local int add_man_random(int arg_nwhite_men,
                         int arg_nblack_men,
                         int arg_shape,
                         int arg_scale,
                         my_random_t *arg_random,
                         search_t *arg_with_search,
                         char *arg_position,
                         score_t *arg_best_score)
{
  int result = FALSE;

  int ntry = 0;

  my_timer_t timer;

  construct_my_timer(&timer, "timer", STDOUT, FALSE);

  reset_my_timer(&timer);

  while (TRUE)
  {
    ++ntry;

    for (int isquare = 1; isquare <= 50; ++isquare)
      arg_position[isquare] = *nn;

    for (int iwhite_man = 1; iwhite_man <= arg_nwhite_men; ++iwhite_man)
    {
      int isquare = INVALID;

      while (TRUE)
      {
        double probability = return_my_random(arg_random) / (double)ULLONG_MAX;

        int irow = 9 - weibull_sample_row(probability, arg_shape, arg_scale);
        int icol = return_my_random(arg_random) % 5;

        isquare = irow * 5 + icol + 1;

        HARDBUG(isquare > 50)

        if (arg_position[isquare] == *nn)
          break;
      }

      HARDBUG(isquare == INVALID)

      arg_position[isquare] = *wO;
    }

    for (int iblack_man = 1; iblack_man <= arg_nblack_men; ++iblack_man)
    {
      int isquare = INVALID;

      while (TRUE)
      {
        double probability =
          (return_my_random(arg_random) / (double)ULLONG_MAX);

        int irow = weibull_sample_row(probability, arg_shape, arg_scale);
        int icol = return_my_random(arg_random) % 5;

        isquare = irow * 5 + icol + 1;

        HARDBUG(isquare > 50)

        if (arg_position[isquare] == *nn)
          break;
      }

      HARDBUG(isquare == INVALID)

      arg_position[isquare] = *bO;
    }

    arg_position[51] = '\0';

    string2board(&(arg_with_search->S_board), arg_position);

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(arg_with_search->S_board), &moves_list);

    if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
    {
      if (arg_best_score == NULL)
        result = TRUE;
      else
      {
        reset_my_timer(&(arg_with_search->S_timer));

        int sort[NMOVES_MAX];

        mcts_search(arg_with_search,
                    MCTS_PLY_MAX,
                    0,
                    &moves_list,
                    sort,
                    arg_best_score,
                    FALSE);

        score_t root_score = return_material_score(&(arg_with_search->S_board));

        if (ABS_SCORE_T(root_score - *arg_best_score) <= SCORE(SCORE_MAN / 4))
        {
          result = TRUE;
        }
        else
        {
          if (ntry <= 10)
          {
            PRINTF("add_man_random rejected"
                   " ntry=%d root_score=" PRINTF_SCORE_T
                   " *arg_best_score=" PRINTF_SCORE_T "\n",
                   ntry,
                   SCORE2PRINTF(root_score),
                   SCORE2PRINTF(*arg_best_score));
          }
        }
      }
    }
    else
    {
      if (ntry <= 10)
      {
        PRINTF("add_man_random rejected ntry=%d ncaptx=%d nmoves=%d\n",
               ntry,
               moves_list.ML_ncaptx,
               moves_list.ML_nmoves);
      }
    }

    if (result)
    {
      PRINTF("add_man_random accepted ntry=%d\n", ntry);

      break;
    }

    if (return_my_timer(&timer, FALSE) > 2.0)
    {
      PRINTF("time limit in add_man_random ntry=%d\n", ntry);

      break;
    }
  }

  return (result);
}

int return_random_set_bit(my_random_t *arg_random, uint64_t arg_mask)
{
  int nbits = BIT_COUNT(arg_mask);

  HARDBUG(nbits == 0)

  int ibit = return_my_random(arg_random) % nbits;

  for (int jbit = 0; jbit < ibit; jbit++)
    arg_mask &= arg_mask - 1;

  return (BIT_CTZ(arg_mask));
}

local int add_kings_random(int arg_nwhite_kings,
                           int arg_nblack_kings,
                           my_random_t *arg_random,
                           search_t *arg_with_search,
                           my_uint64_t *arg_white_kings_mask,
                           my_uint64_t *arg_black_kings_mask,
                           char *arg_position,
                           score_t *arg_best_score)
{
  HARDBUG((arg_nwhite_kings + arg_nblack_kings) == 0)

  // just to be sure

  arg_position[51] = '\0';

  int result = FALSE;

  int ntry = 0;

  my_timer_t timer;

  construct_my_timer(&timer, "timer", STDOUT, FALSE);

  reset_my_timer(&timer);

  my_uint64_t empty = 0;

  for (int isquare = 1; isquare <= 50; ++isquare)
    if (arg_position[isquare] == *nn)
      empty |= BITULL(isquare);

  HARDBUG(empty == 0)

  while (TRUE)
  {
    ++ntry;

    int nwhite_kings = 0;
    int nblack_kings = 0;

    int squares[NPIECES_MAX];
    int nsquares = 0;

    string2board(&(arg_with_search->S_board), arg_position);

    /*
        PRINTF("before while\n");
        print_board(&(arg_with_search->S_board));
        PRINTF("white mask:");
        for (int i = 1; i <= 50; ++i)
          if (*arg_white_kings_mask & BITULL(i)) PRINTF(" %d", i);
        PRINTF("\n");
        PRINTF("black mask:");
        for (int i = 1; i <= 50; ++i)
          if (*arg_black_kings_mask & BITULL(i)) PRINTF(" %d", i);
        PRINTF("\n");
    */

    while (TRUE)
    {
      int icolour = return_my_random(arg_random) % 2;

      if (icolour == 0)
      {
        if ((nwhite_kings >= arg_nwhite_kings) or
            ((empty & ~(*arg_white_kings_mask)) == 0))
        {
          icolour = 1;

          if ((nblack_kings >= arg_nblack_kings) or
              ((empty & ~(*arg_black_kings_mask)) == 0))
            break;
        }
      }
      else
      {
        if ((nblack_kings >= arg_nblack_kings) or
            ((empty & ~(*arg_black_kings_mask)) == 0))
        {
          icolour = 0;

          if ((nwhite_kings >= arg_nwhite_kings) or
              ((empty & ~(*arg_white_kings_mask)) == 0))
            break;
        }
      }

      // at least one empty square is available

      if (icolour == 0)
      {
        int isquare =
          return_random_set_bit(arg_random, empty & ~(*arg_white_kings_mask));

        HARDBUG(arg_position[isquare] != *nn)

        HARDBUG(!(empty & BITULL(isquare)))

        arg_position[isquare] = *wX;

        empty &= ~(BITULL(isquare));

        HARDBUG(nsquares >= NPIECES_MAX)

        squares[nsquares++] = isquare;

        ++nwhite_kings;

        /*
                PRINTF("added white king at %d\n", isquare);
        */
      }
      else
      {
        int isquare =
          return_random_set_bit(arg_random, empty & ~(*arg_black_kings_mask));

        HARDBUG(arg_position[isquare] != *nn)

        HARDBUG(!(empty & BITULL(isquare)))

        arg_position[isquare] = *bX;

        empty &= ~(BITULL(isquare));

        HARDBUG(nsquares >= NPIECES_MAX)

        squares[nsquares++] = isquare;

        ++nblack_kings;

        /*
                PRINTF("added black king at %d\n", isquare);
        */
      }

      if ((nwhite_kings >= arg_nwhite_kings) and
          (nblack_kings >= arg_nblack_kings))
        break;

      if (empty == 0)
        break;
    }

    /*
        PRINTF("after while\n");
        string2board(&(arg_with_search->S_board), arg_position);
        print_board(&(arg_with_search->S_board));
    */

    if (nsquares == 0)
    {
      PRINTF("add_kings_random rejected no more squares ntry=%d\n", ntry);

      return (result);
    }

    HARDBUG((nwhite_kings + nblack_kings) != nsquares)

    string2board(&(arg_with_search->S_board), arg_position);

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(arg_with_search->S_board), &moves_list);

    if ((moves_list.ML_ncaptx == 0) and (moves_list.ML_nmoves > 1))
    {
      if (arg_best_score == NULL)
        result = TRUE;
      else
      {
        reset_my_timer(&(arg_with_search->S_timer));

        int sort[NMOVES_MAX];

        mcts_search(arg_with_search,
                    MCTS_PLY_MAX,
                    0,
                    &moves_list,
                    sort,
                    arg_best_score,
                    FALSE);

        score_t root_score = return_material_score(&(arg_with_search->S_board));

        if (ABS_SCORE_T(root_score - *arg_best_score) <= SCORE(SCORE_MAN / 4))
        {
          result = TRUE;
        }
        else
        {
          if (ntry <= 10)
          {
            PRINTF("add_kings_random rejected"
                   " ntry=%d"
                   " root_score=" PRINTF_SCORE_T
                   " *arg_best_score=" PRINTF_SCORE_T "\n",
                   ntry,
                   SCORE2PRINTF(root_score),
                   SCORE2PRINTF(*arg_best_score));
          }
        }
      }
    }
    else
    {
      if (ntry <= 10)
      {
        PRINTF("add_kings_random rejected ntry=%d ncaptx=%d nmoves=%d\n",
               ntry,
               moves_list.ML_ncaptx,
               moves_list.ML_nmoves);
      }
    }

    // mark squares and restore

    for (int isquare = 0; isquare < nsquares; isquare++)
    {
      if (arg_position[squares[isquare]] == *wX)
      {
        *arg_white_kings_mask |= BITULL(squares[isquare]);
      }
      else if (arg_position[squares[isquare]] == *bX)
      {
        *arg_black_kings_mask |= BITULL(squares[isquare]);
      }
      else
        FATAL("invalid arg_position", EXIT_FAILURE)

      arg_position[squares[isquare]] = *nn;
    }

    if (result)
    {
      PRINTF("add_kings_random accepted ntry=%d\n", ntry);

      break;
    }

    if (return_my_timer(&timer, FALSE) > 2.0)
    {
      PRINTF("time limit in add_kings_random ntry=%d\n", ntry);

      break;
    }
  }

  return (result);
}

local void gen_random(const char *arg_fen_name,
                      my_int64_t arg_npositions_max,
                      int arg_nkings_max,
                      double arg_time_limit)
{
  my_random_t temp_random;

  if (my_mpi_globals.MMG_is_single)
    construct_my_random(&temp_random, NULL);
  else
  {
    my_uint64_t seed;

    construct_my_random(&temp_random, &seed);

    PRINTF("gen_random seed=%llX\n", seed);
  }

  my_timer_t timer;

  construct_my_timer(&timer, "timer", STDOUT, FALSE);

  reset_my_timer(&timer);

  BSTRING(position)

  HARDBUG(bassigncstr(position, "?") == BSTR_ERR)
  HARDBUG(bpattern(position, 51) == BSTR_ERR)

  BSTRING(bfen_name)

  HARDBUG(bassigncstr(bfen_name, arg_fen_name) == BSTR_ERR)

  fbuffer_t ffen;

  construct_fbuffer(&ffen, bfen_name, NULL, TRUE);

  search_t search;

  construct_search(&search, STDOUT, NULL, NETWORK0_ID);

  moves_list_t moves_list;

  construct_moves_list(&moves_list);

  mcts_globals.mcts_globals_time_limit = arg_time_limit;

  search_t *with_search = &search;

  my_int64_t npositions = arg_npositions_max;

  if (my_mpi_globals.MMG_has_slaves)
  {
    npositions = arg_npositions_max / my_mpi_globals.MMG_nslaves;

    if (npositions * my_mpi_globals.MMG_nslaves < arg_npositions_max)
      ++npositions;
  }

  HARDBUG(npositions < 1)

  PRINTF("npositions=%lld\n", npositions);

  my_int64_t iposition = 0;
  my_int64_t iposition_prev = 0;

  bucket_t bucket_root_score;

  construct_bucket(&bucket_root_score,
                   "bucket_root_score",
                   SCORE_MAN,
                   SCORE_LOST,
                   SCORE_WON,
                   BUCKET_LINEAR);

  bucket_t bucket_npieces;

  construct_bucket(&bucket_npieces,
                   "bucket_npieces",
                   1,
                   0,
                   2 * NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_ndelta;

  construct_bucket(&bucket_ndelta,
                   "bucket_ndelta",
                   1,
                   -NMAN_MIN,
                   NMAN_MIN,
                   BUCKET_LINEAR);

  bucket_t bucket_nwhite_men;

  construct_bucket(&bucket_nwhite_men,
                   "bucket_nwhite_man",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nblack_men;

  construct_bucket(&bucket_nblack_men,
                   "bucket_nblack_man",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nwhite_kings;

  construct_bucket(&bucket_nwhite_kings,
                   "bucket_nwhite_kings",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nblack_kings;

  construct_bucket(&bucket_nblack_kings,
                   "bucket_nblack_kings",
                   1,
                   0,
                   NPIECES_MAX,
                   BUCKET_LINEAR);

  bucket_t bucket_nkings;

  construct_bucket(&bucket_nkings,
                   "bucket_nkings",
                   1,
                   0,
                   2 * NPIECES_MAX,
                   BUCKET_LINEAR);

  while (iposition < npositions)
  {
    double probability = return_my_random(&temp_random) / (double)ULLONG_MAX;

    int npieces = INVALID;
    int nwhite = INVALID;
    int nblack = INVALID;
    int nwhite_men = INVALID;
    int nwhite_kings = INVALID;
    int nblack_men = INVALID;
    int nblack_kings = INVALID;

    while (TRUE)
    {
      nwhite_men =
        return_my_random(&temp_random) % (20 - arg_nkings_max - NMAN_MIN + 1) +
        NMAN_MIN;

      nblack_men = nwhite_men;

      int ndelta = 0;

      double cumulative_probability = PROBABILITY_EQUAL;

      double delta_probability = (1.0 - PROBABILITY_EQUAL) / NMAN_MIN;

      while (TRUE)
      {
        if (probability < cumulative_probability)
          break;

        ++ndelta;

        if (ndelta >= NMAN_MIN)
          break;

        cumulative_probability += delta_probability;
      }

      if ((return_my_random(&temp_random) % 2) == 0)
      {
        nwhite_men -= ndelta;
      }
      else
      {
        nblack_men -= ndelta;
      }

      nwhite_kings = 0;
      nblack_kings = 0;

      if (arg_nkings_max > 0)
      {
        while (TRUE)
        {
          probability = return_my_random(&temp_random) / (double)ULLONG_MAX;

          cumulative_probability = 1.0 - probability_one_king(nwhite_men);

          delta_probability = (1.0 - cumulative_probability) / arg_nkings_max;

          while (TRUE)
          {
            if (probability < cumulative_probability)
              break;

            ++nwhite_kings;

            if (nwhite_kings >= arg_nkings_max)
              break;

            cumulative_probability += delta_probability;
          }

          probability = return_my_random(&temp_random) / (double)ULLONG_MAX;

          cumulative_probability = 1.0 - probability_one_king(nblack_men);

          delta_probability = (1.0 - cumulative_probability) / arg_nkings_max;

          while (TRUE)
          {
            if (probability < cumulative_probability)
              break;

            ++nblack_kings;

            if (nblack_kings >= arg_nkings_max)
              break;

            cumulative_probability += delta_probability;
          }

          if ((nwhite_kings + nblack_kings) > 0)
            break;
        }
      }

      nwhite = nwhite_men + nwhite_kings;
      nblack = nblack_men + nblack_kings;

      npieces = nwhite + nblack;

      if (npieces >= NPIECES_MIN)
        break;
    }

    HARDBUG(npieces == INVALID)
    HARDBUG(nwhite == INVALID)
    HARDBUG(nblack == INVALID)
    HARDBUG(nwhite_men == INVALID)
    HARDBUG(nwhite_kings == INVALID)
    HARDBUG(nblack_men == INVALID)
    HARDBUG(nblack_kings == INVALID)

    PRINTF("trying iposition=%lld"
           " npieces=%d nwhite=%d nblack=%d"
           " nwhite_man=%d nblack_man=%d"
           " nwhite_kings=%d nblack_kings=%d\n",
           iposition,
           npieces,
           nwhite,
           nblack,
           nwhite_men,
           nblack_men,
           nwhite_kings,
           nblack_kings);

    double shape = weibull_shape(npieces);
    double scale = weibull_scale(npieces);

    PRINTF("shape=%.2f scale=%.2f\n", shape, scale);

    if ((return_my_random(&temp_random) % 2) == 0)
      bdata(position)[0] = 'w';
    else
      bdata(position)[0] = 'b';

    score_t best_score = SCORE_PLUS_INFINITY;

    int result = INVALID;

    if ((nwhite_kings == 0) and (nblack_kings == 0))
    {
      result = add_man_random(nwhite_men,
                              nblack_men,
                              shape,
                              scale,
                              &temp_random,
                              with_search,
                              bdata(position),
                              &best_score);
    }
    else
    {
      result = add_man_random(nwhite_men,
                              nblack_men,
                              shape,
                              scale,
                              &temp_random,
                              with_search,
                              bdata(position),
                              NULL);
    }

    if (!result)
    {
      PRINTF("add_man_random failed\n");

      continue;
    }

    my_uint64_t white_kings_mask = 0;
    my_uint64_t black_kings_mask = 0;

    int nsame = 0;

    while (TRUE)
    {
      if ((nwhite_kings > 0) or (nblack_kings > 0))
      {
        if (!add_kings_random(nwhite_kings,
                              nblack_kings,
                              &temp_random,
                              with_search,
                              &white_kings_mask,
                              &black_kings_mask,
                              bdata(position),
                              &best_score))
        {
          PRINTF("add_kings_random failed nsame=%d\n", nsame);

          break;
        }
      }

      ++nsame;

      HARDBUG(BIT_COUNT(with_search->S_board.B_white_men_bb) != nwhite_men)
      HARDBUG(BIT_COUNT(with_search->S_board.B_black_men_bb) != nblack_men)

      HARDBUG(best_score == SCORE_PLUS_INFINITY)

      int root_score = return_material_score(&(with_search->S_board));

      print_board(&(with_search->S_board));

      PRINTF("accepted iposition=%lld nsame=%d"
             " npieces=%d nwhite=%d nblack=%d"
             " nwhite_man=%d nblack_man=%d"
             " nwhite_kings=%d nblack_kings=%d"
             " root_score=%d best_score=" PRINTF_SCORE_T "\n",
             iposition,
             nsame,
             npieces,
             nwhite,
             nblack,
             BIT_COUNT(with_search->S_board.B_white_men_bb),
             BIT_COUNT(with_search->S_board.B_black_men_bb),
             BIT_COUNT(with_search->S_board.B_white_kings_bb),
             BIT_COUNT(with_search->S_board.B_black_kings_bb),
             root_score,
             SCORE2PRINTF(best_score));

      update_bucket(&bucket_root_score, root_score);

      update_bucket(&bucket_npieces, npieces);

      update_bucket(&bucket_ndelta, nwhite_men - nblack_men);

      update_bucket(&bucket_nwhite_men,
                    BIT_COUNT(with_search->S_board.B_white_men_bb));

      update_bucket(&bucket_nblack_men,
                    BIT_COUNT(with_search->S_board.B_black_men_bb));

      update_bucket(&bucket_nwhite_kings,
                    BIT_COUNT(with_search->S_board.B_white_kings_bb));

      update_bucket(&bucket_nblack_kings,
                    BIT_COUNT(with_search->S_board.B_black_kings_bb));

      update_bucket(&bucket_nkings,
                    BIT_COUNT(with_search->S_board.B_white_kings_bb) +
                      BIT_COUNT(with_search->S_board.B_black_kings_bb));

      BSTRING(bfen)

      board2fen(&(with_search->S_board), bfen, FALSE);

      append_fbuffer_fmt(&ffen, "%s {0.000000}\n", bdata(bfen));

      BDESTROY(bfen)

      ++iposition;

      if ((iposition - iposition_prev) > 100)
      {
        iposition_prev = iposition;

        PRINTF("iposition=%lld time=%.2f (%.2f positions/second)\n",
               iposition,
               return_my_timer(&timer, FALSE),
               iposition / return_my_timer(&timer, FALSE));
      }

      if ((nwhite_kings == 0) and (nblack_kings == 0))
        break;

    } // while(TRUE)
  }   // while(iposition < npositions

  flush_fbuffer(&ffen, 0);

  printf_bucket(&bucket_root_score);
  printf_bucket(&bucket_npieces);
  printf_bucket(&bucket_ndelta);
  printf_bucket(&bucket_nwhite_men);
  printf_bucket(&bucket_nblack_men);
  printf_bucket(&bucket_nwhite_kings);
  printf_bucket(&bucket_nblack_kings);
  printf_bucket(&bucket_nkings);

  my_mpi_barrier_v3("after gen",
                    my_mpi_globals.MMG_slave_comm,
                    my_mpi_globals.MMG_gen_random_barrier_semkey,
                    TRUE);

  my_mpi_allreduce(MPI_IN_PLACE,
                   &iposition,
                   1,
                   MPI_LONG_LONG_INT,
                   MPI_SUM,
                   my_mpi_globals.MMG_slave_comm);

  PRINTF("iposition=%lld\n", iposition);

  my_mpi_bucket_aggregate(&bucket_root_score, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_npieces, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_ndelta, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nwhite_men, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nblack_men, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nwhite_kings, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nblack_kings, my_mpi_globals.MMG_slave_comm);

  my_mpi_bucket_aggregate(&bucket_nkings, my_mpi_globals.MMG_slave_comm);

  printf_bucket(&bucket_root_score);

  printf_bucket(&bucket_npieces);

  printf_bucket(&bucket_ndelta);

  printf_bucket(&bucket_nwhite_men);

  printf_bucket(&bucket_nblack_men);

  printf_bucket(&bucket_nwhite_kings);

  printf_bucket(&bucket_nblack_kings);

  printf_bucket(&bucket_nkings);

  BDESTROY(bfen_name)

  BDESTROY(position)
}

local void test_cJSON(void)
{
  const unsigned int resolution_numbers[3][2] = {{1280, 720},
                                                 {1920, 1080},
                                                 {3840, 2160}};

  cJSON *monitor = cJSON_CreateObject();

  HARDBUG(monitor == NULL)

  HARDBUG(cJSON_AddStringToObject(monitor, "name", "Awesome 4K") == NULL)

  cJSON *resolutions = NULL;

  HARDBUG((resolutions = cJSON_AddArrayToObject(monitor, "resolutions")) ==
          NULL)

  for (size_t index = 0;
       index < (sizeof(resolution_numbers) / (2 * sizeof(int)));
       ++index)
  {
    cJSON *resolution = cJSON_CreateObject();

    HARDBUG(resolution == NULL)

    HARDBUG(cJSON_AddNumberToObject(resolution,
                                    "width",
                                    resolution_numbers[index][0]) == NULL)

    HARDBUG(cJSON_AddNumberToObject(resolution,
                                    "height",
                                    resolution_numbers[index][1]) == NULL)

    cJSON_AddItemToArray(resolutions, resolution);
  }

  BSTRING(string)

  char *serialized = cJSON_PrintUnformatted(monitor);

  HARDBUG(serialized == NULL)
  HARDBUG(bassigncstr(string, serialized) == BSTR_ERR)

  cJSON_free(serialized);

  cJSON_Delete(monitor);

  PRINTF("monitor=%s\n", bdata(string));

  BDESTROY(string)
}

#define ARGS_TXT "args.txt"

cJSON *gwd_json;

local void load_gwd_json(void)
{
  file2cjson(GWD_JSON, &gwd_json);

  // set defaults

  HARDBUG((options.hub_version = bfromcstr("NULL")) == NULL)

  HARDBUG((options.overrides = bfromcstr("NULL")) == NULL)

  options.verbose = 1;

  HARDBUG((options.network0 = bfromcstr("NULL")) == NULL)
  HARDBUG((options.network1 = bfromcstr("NULL")) == NULL)

  options.network_evaluation_min = -100;
  options.network_evaluation_max = -25;
  options.network_evaluation_time = 30;

  options.time_limit = 30.0;

  options.time_control_ntrouble = 3;
  options.time_control_mean = 30;
  options.time_control_sigma = 20;
  options.time_control_method = 0;

  // TO DO this should be moved out of options_t

  options.time_ntrouble = 0;

  options.wall_time_threshold = 5;

  options.depth_limit = 64;

  options.use_book = TRUE;

  HARDBUG((options.book_name = bfromcstr("book.db")) == NULL)

  options.book_randomness = 0;
  options.book_position_frequency = 5;
  options.book_move_frequency = 5;
  options.book_evaluation_time = 30;
  options.book_minimax = FALSE;

  options.ponder = TRUE;

  options.material_only = FALSE;

  options.mobility_moves_threshold = 2;

  options.best_score_margin = 0;
  options.aspiration_margin = 25;
  options.research_margin = 1;

  options.use_quiescence = FALSE;
  options.use_reductions = TRUE;
  options.reduction_moves_threshold = 2;
  options.reduction_depth_root = 2;
  options.reduction_depth_leaf = 2;
  options.reduction_full_min = 1;
  options.reduction_percentage = 50;
  options.reduction_margin = 100;

  options.prob_cut_depth = 0;
  options.prob_cut_percentage = 50;
  options.prob_cut_margin = 100;

  options.singular_probe_depth = 2;
  options.singular_moves_threshold = 2;
  options.singular_probe_percentage = 50;
  options.singular_probe_margin = 1;
  options.singular_reduction = 0;

  options.sync_clock = FALSE;

  options.alpha_beta_cache_size = 1024;
  options.cache_draw_results = FALSE;

  options.score_cache_size = 256;

  options.nthreads_alpha_beta = 4;
  options.lazy_smp_policy = 1;

  HARDBUG((options.egtb_dir = bfromcstr("NULL")) == NULL)
  HARDBUG((options.egtb_dir_wdl = bfromcstr("NULL")) == NULL)

  HARDBUG((options.egtb_ram = bfromcstr("egtb56")) == NULL)
  HARDBUG((options.egtb_ram_wdl = bfromcstr("egtb3")) == NULL)

  options.egtb_entry_cache_size = 4;

  HARDBUG((options.dxp_server_ip = bfromcstr("127.0.0.1")) == NULL)

  options.dxp_port = 27531;
  options.dxp_initiator = TRUE;
  options.dxp_game_time = 10;
  options.dxp_game_increment = 0.0;
  options.dxp_game_moves = 75;
  options.dxp_games = 1000;

  HARDBUG((options.dxp_ballot = bfromcstr("NULL")) == NULL)

  options.dxp_annotate_level = 1;
  options.dxp_strict_gameend = FALSE;

  HARDBUG((options.dxp_tag = bfromcstr("NULL")) == NULL)

  options.nthreads = INVALID;
  options.mode = INVALID;

  options.hub = FALSE;
}

local void parse_parameters(void)
{
  cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);

  HARDBUG(!cJSON_IsArray(parameters))

  // loop over parameters

  cJSON *parameter;

  cJSON_ArrayForEach(parameter, parameters)
  {
    //{"name": "time_limit", "type": "int", "value": 30, "ui": "false"},

    cJSON *cjson_name = cJSON_GetObjectItem(parameter, "name");

    char *parameter_name = cJSON_GetStringValue(cjson_name);

    HARDBUG(parameter_name == NULL)

    cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");

    char *parameter_type = cJSON_GetStringValue(cjson_type);

    HARDBUG(parameter_type == NULL)

    if (compat_strcasecmp(parameter_type, "int") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      int ivalue = round(cJSON_GetNumberValue(cjson_value));

      if (compat_strcasecmp(parameter_name, "verbose") == 0)
        options.verbose = ivalue;

      else if (compat_strcasecmp(parameter_name, "network_evaluation_min") == 0)
        options.network_evaluation_min = ivalue;

      else if (compat_strcasecmp(parameter_name, "network_evaluation_max") == 0)
        options.network_evaluation_max = ivalue;

      else if (compat_strcasecmp(parameter_name, "network_evaluation_time") ==
               0)
        options.network_evaluation_time = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_ntrouble") == 0)
        options.time_control_ntrouble = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_mean") == 0)
        options.time_control_mean = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_sigma") == 0)
        options.time_control_sigma = ivalue;

      else if (compat_strcasecmp(parameter_name, "time_control_method") == 0)
        options.time_control_method = ivalue;

      else if (compat_strcasecmp(parameter_name, "wall_time_threshold") == 0)
        options.wall_time_threshold = ivalue;

      else if (compat_strcasecmp(parameter_name, "depth_limit") == 0)
        options.depth_limit = ivalue;

      else if (compat_strcasecmp(parameter_name, "book_randomness") == 0)
        options.book_randomness = ivalue;

      else if (compat_strcasecmp(parameter_name, "book_position_frequency") ==
               0)
        options.book_position_frequency = ivalue;

      else if (compat_strcasecmp(parameter_name, "book_move_frequency") == 0)
        options.book_move_frequency = ivalue;

      else if (compat_strcasecmp(parameter_name, "book_evaluation_time") == 0)
        options.book_evaluation_time = ivalue;

      else if (compat_strcasecmp(parameter_name, "mobility_moves_threshold") ==
               0)
        options.mobility_moves_threshold = ivalue;

      else if (compat_strcasecmp(parameter_name, "best_score_margin") == 0)
        options.best_score_margin = ivalue;

      else if (compat_strcasecmp(parameter_name, "aspiration_margin") == 0)
        options.aspiration_margin = ivalue;

      else if (compat_strcasecmp(parameter_name, "research_margin") == 0)
        options.research_margin = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_moves_threshold") ==
               0)
        options.reduction_moves_threshold = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_depth_root") == 0)
        options.reduction_depth_root = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_depth_leaf") == 0)
        options.reduction_depth_leaf = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_full_min") == 0)
        options.reduction_full_min = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_percentage") == 0)
        options.reduction_percentage = ivalue;

      else if (compat_strcasecmp(parameter_name, "reduction_margin") == 0)
        options.reduction_margin = ivalue;

      else if (compat_strcasecmp(parameter_name, "prob_cut_depth") == 0)
        options.prob_cut_depth = ivalue;

      else if (compat_strcasecmp(parameter_name, "prob_cut_percentage") == 0)
        options.prob_cut_percentage = ivalue;

      else if (compat_strcasecmp(parameter_name, "prob_cut_margin") == 0)
        options.prob_cut_margin = ivalue;

      else if (compat_strcasecmp(parameter_name, "singular_probe_depth") == 0)
        options.singular_probe_depth = ivalue;

      else if (compat_strcasecmp(parameter_name, "singular_moves_threshold") ==
               0)
        options.singular_moves_threshold = ivalue;

      else if (compat_strcasecmp(parameter_name, "singular_probe_percentage") ==
               0)
        options.singular_probe_percentage = ivalue;

      else if (compat_strcasecmp(parameter_name, "singular_probe_margin") == 0)
        options.singular_probe_margin = ivalue;
      else if (compat_strcasecmp(parameter_name, "singular_reduction") == 0)
        options.singular_reduction = ivalue;

      else if (compat_strcasecmp(parameter_name, "alpha_beta_cache_size") == 0)
        options.alpha_beta_cache_size = ivalue;

      else if (compat_strcasecmp(parameter_name, "score_cache_size") == 0)
        options.score_cache_size = ivalue;

      else if (compat_strcasecmp(parameter_name, "nthreads_alpha_beta") == 0)
        options.nthreads_alpha_beta = ivalue;

      else if (compat_strcasecmp(parameter_name, "lazy_smp_policy") == 0)
        options.lazy_smp_policy = ivalue;

      else if (compat_strcasecmp(parameter_name, "egtb_entry_cache_size") == 0)
        options.egtb_entry_cache_size = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_port") == 0)
        options.dxp_port = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_game_time") == 0)
        options.dxp_game_time = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_game_moves") == 0)
        options.dxp_game_moves = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_games") == 0)
        options.dxp_games = ivalue;

      else if (compat_strcasecmp(parameter_name, "dxp_annotate_level") == 0)
        options.dxp_annotate_level = ivalue;

      else
        PRINTF("ignoring int parameter %s\n", parameter_name);
    }
    else if (compat_strcasecmp(parameter_type, "double") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsNumber(cjson_value))

      double dvalue = cJSON_GetNumberValue(cjson_value);

      if (compat_strcasecmp(parameter_name, "time_limit") == 0)
        options.time_limit = dvalue;

      else if (compat_strcasecmp(parameter_name, "dxp_game_increment") == 0)
      {
        HARDBUG((!isfinite(dvalue)) or (dvalue < 0.0) or (dvalue > 999999.0))

        char field[7];

        if ((dvalue > 0.0) and (format_dxp_increment(dvalue, field) == 0.0))
          FATAL("dxp_game_increment rounds to zero in the DXP Fischer request",
                EXIT_FAILURE)
        options.dxp_game_increment = dvalue;
      }

      else
        PRINTF("ignoring double parameter %s with value %.2f\n",
               parameter_name,
               dvalue);
    }
    else if (compat_strcasecmp(parameter_type, "string") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsString(cjson_value))

      char *svalue = cJSON_GetStringValue(cjson_value);

      HARDBUG(svalue == NULL)

      if (compat_strcasecmp(parameter_name, "hub_version") == 0)
        HARDBUG(bassigncstr(options.hub_version, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "overrides") == 0)
        HARDBUG(bassigncstr(options.overrides, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "network0") == 0)
        HARDBUG(bassigncstr(options.network0, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "network1") == 0)
        HARDBUG(bassigncstr(options.network1, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "book_name") == 0)
        HARDBUG(bassigncstr(options.book_name, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "egtb_dir") == 0)
        HARDBUG(bassigncstr(options.egtb_dir, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "egtb_dir_wdl") == 0)
        HARDBUG(bassigncstr(options.egtb_dir_wdl, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "egtb_ram") == 0)
        HARDBUG(bassigncstr(options.egtb_ram, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "egtb_ram_wdl") == 0)
        HARDBUG(bassigncstr(options.egtb_ram_wdl, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "dxp_server_ip") == 0)
        HARDBUG(bassigncstr(options.dxp_server_ip, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "dxp_ballot") == 0)
        HARDBUG(bassigncstr(options.dxp_ballot, svalue) == BSTR_ERR)

      else if (compat_strcasecmp(parameter_name, "dxp_tag") == 0)
        HARDBUG(bassigncstr(options.dxp_tag, svalue) == BSTR_ERR)

      else
        PRINTF("ignoring string parameter %s\n", parameter_name);
    }
    else if (compat_strcasecmp(parameter_type, "bool") == 0)
    {
      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

      HARDBUG(!cJSON_IsBool(cjson_value))

      int bvalue = 0;

      if (cJSON_IsTrue(cjson_value))
        bvalue = 1;

      if (compat_strcasecmp(parameter_name, "use_book") == 0)
        options.use_book = bvalue;

      else if (compat_strcasecmp(parameter_name, "book_minimax") == 0)
        options.book_minimax = bvalue;

      else if (compat_strcasecmp(parameter_name, "ponder") == 0)
        options.ponder = bvalue;

      else if (compat_strcasecmp(parameter_name, "material_only") == 0)
        options.material_only = bvalue;

      else if (compat_strcasecmp(parameter_name, "use_quiescence") == 0)
        options.use_quiescence = bvalue;

      else if (compat_strcasecmp(parameter_name, "use_reductions") == 0)
        options.use_reductions = bvalue;

      else if (compat_strcasecmp(parameter_name, "sync_clock") == 0)
        options.sync_clock = bvalue;

      else if (compat_strcasecmp(parameter_name, "cache_draw_results") == 0)
        options.cache_draw_results = bvalue;

      else if (compat_strcasecmp(parameter_name, "dxp_initiator") == 0)
        options.dxp_initiator = bvalue;

      else if (compat_strcasecmp(parameter_name, "dxp_strict_gameend") == 0)
        options.dxp_strict_gameend = bvalue;

      else
        PRINTF("ignoring bool parameter %s\n", parameter_name);
    }
    else
    {
      PRINTF("unknown parameter_type %s\n", parameter_type);
      exit(EXIT_FAILURE);
    }
  }
}

cJSON *overrides_json;

local void load_overrides(void)
{
  file2cjson(OVERRIDES_JSON, &overrides_json);

  cJSON *overrides =
    cJSON_GetObjectItem(overrides_json, bdata(options.overrides));

  HARDBUG(!cJSON_IsArray(overrides))

  cJSON *override;

  cJSON_ArrayForEach(override, overrides)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(override, "name");

    char *override_name = cJSON_GetStringValue(cjson_name);

    HARDBUG(override_name == NULL)

    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);

    HARDBUG(!cJSON_IsArray(parameters))

    // loop over parameters

    cJSON *parameter;

    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      cjson_name = cJSON_GetObjectItem(parameter, "name");

      char *parameter_name = cJSON_GetStringValue(cjson_name);

      HARDBUG(parameter_name == NULL)

      if (compat_strcasecmp(parameter_name, override_name) != 0)
        continue;

      cJSON *cjson_value = cJSON_GetObjectItem(parameter, "cli");

      if (cJSON_IsTrue(cjson_value))
      {
        nfound = 1;

        break;
      }

      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");

      char *parameter_type = cJSON_GetStringValue(cjson_type);

      HARDBUG(parameter_type == NULL)

      if (compat_strcasecmp(parameter_type, "int") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        int ivalue = round(cJSON_GetNumberValue(cjson_value));

        cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, ivalue);

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "double") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        double dvalue = cJSON_GetNumberValue(cjson_value);

        cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        cJSON_SetNumberValue(cjson_value, dvalue);

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "string") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");

        char *svalue = cJSON_GetStringValue(cjson_value);

        HARDBUG(svalue == NULL)

        cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsString(cjson_value))

        cJSON_SetValuestring(cjson_value, svalue);

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "bool") == 0)
      {
        cjson_value = cJSON_GetObjectItem(override, "value");

        HARDBUG(!cJSON_IsBool(cjson_value))

        int bvalue;

        if (cJSON_IsFalse(cjson_value))
          bvalue = 0;
        else
          bvalue = 1;

        cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsBool(cjson_value))

        cJSON_SetBoolValue(cjson_value, bvalue);

        nfound++;
      }
      else
        FATAL("unknown parameter_type", EXIT_FAILURE)
    }

    if (nfound != 1)
    {
      PRINTF("unknown override %s\n", override_name);
      exit(EXIT_FAILURE);
    }
  }

  // check CJSON_CSV_ID

  cJSON *cjson_csv = cJSON_GetObjectItem(overrides_json, CJSON_CSV_ID);

  HARDBUG(cjson_csv == NULL)

  cJSON *cjson_path = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_PATH_ID);

  HARDBUG(!cJSON_IsString(cjson_path))

  char *path = cJSON_GetStringValue(cjson_path);

  HARDBUG(path == NULL)

  cJSON *columns = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_COLUMNS_ID);

  HARDBUG(!cJSON_IsArray(columns))

  cJSON *column;

  cJSON_ArrayForEach(column, columns)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(column, "name");

    HARDBUG(!cJSON_IsString(cjson_name))

    char *column_name = cJSON_GetStringValue(cjson_name);

    HARDBUG(column_name == NULL)

    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);

    HARDBUG(!cJSON_IsArray(parameters))

    // loop over parameters

    cJSON *parameter;

    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      cjson_name = cJSON_GetObjectItem(parameter, "name");

      char *parameter_name = cJSON_GetStringValue(cjson_name);

      HARDBUG(parameter_name == NULL)

      if (compat_strcasecmp(parameter_name, column_name) != 0)
        continue;

      nfound++;
    }

    if (nfound != 1)
    {
      PRINTF("unknown column %s\n", column_name);
      exit(EXIT_FAILURE);
    }
  }
}

void results2csv(int arg_nwon, int arg_ndraw, int arg_nlost, int arg_nunknown)
{
  BSTRING(header)
  BSTRING(line)
  BSTRING(stamp)
  BSTRING(newpath)

  cJSON *cjson_csv = cJSON_GetObjectItem(overrides_json, CJSON_CSV_ID);

  HARDBUG(cjson_csv == NULL)

  cJSON *cjson_path = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_PATH_ID);

  HARDBUG(!cJSON_IsString(cjson_path))

  char *path = cJSON_GetStringValue(cjson_path);

  HARDBUG(path == NULL)

  cJSON *columns = cJSON_GetObjectItem(cjson_csv, CJSON_CSV_COLUMNS_ID);

  HARDBUG(!cJSON_IsArray(columns))

  // header

  HARDBUG(bassigncstr(header, "version,date") == BSTR_ERR)

  cJSON *column;

  cJSON_ArrayForEach(column, columns)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(column, "name");

    HARDBUG(!cJSON_IsString(cjson_name))

    HARDBUG(bcatcstr(header, ",") == BSTR_ERR)
    HARDBUG(bcatcstr(header, cJSON_GetStringValue(cjson_name)) == BSTR_ERR)
  }

  HARDBUG(bcatcstr(header, ",nwon,ndraw,nlost,nunknown\n") == BSTR_ERR)

  PRINTF("header=%s", bdata(header));

  FILE *fcsv;

  if (!fexists(path))
  {
    if ((fcsv = fopen(path, "w")) == NULL)
    {
      PRINTF("WARNING: cannot open file %s!\n", path);

      goto label_return;
    }

    HARDBUG(fprintf(fcsv, "%s", bdata(header)) < 0)
  }
  else
  {
    // check if header has changed

    HARDBUG((fcsv = fopen(path, "r")) == NULL)

    HARDBUG(!my_breadline(line, fcsv))

    FCLOSE(fcsv)

    if (strcmp(bdata(line), bdata(header)) == 0)
    {
      HARDBUG((fcsv = fopen(path, "a")) == NULL)
    }
    else
    {
      PRINTF("header has changed!\n");

      time_t current_time = time(NULL);

      my_bstrftime(stamp, "%d%b%Y-%H%M%S", current_time);

      HARDBUG(bassignformat(newpath, "%s-%s", path, bdata(stamp)) == BSTR_ERR)

      remove(bdata(newpath));

      HARDBUG(rename(path, bdata(newpath)) == -1)

      HARDBUG((fcsv = fopen(path, "w")) == NULL)

      HARDBUG(fprintf(fcsv, "%s", bdata(header)) < 0)
    }
  }

  HARDBUG(fcsv == NULL)

  time_t current_time = time(NULL);

  my_bstrftime(stamp, "%d%b%Y-%H%M%S", current_time);

  HARDBUG(fprintf(fcsv, "\"%s\",\"%s\"", REVISION, bdata(stamp)) < 0)

  cJSON_ArrayForEach(column, columns)
  {
    cJSON *cjson_name = cJSON_GetObjectItem(column, "name");

    HARDBUG(!cJSON_IsString(cjson_name))

    char *column_name = cJSON_GetStringValue(cjson_name);

    HARDBUG(column_name == NULL)

    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);

    HARDBUG(!cJSON_IsArray(parameters))

    // loop over parameters

    cJSON *parameter;

    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      cjson_name = cJSON_GetObjectItem(parameter, "name");

      char *parameter_name = cJSON_GetStringValue(cjson_name);

      HARDBUG(parameter_name == NULL)

      if (compat_strcasecmp(parameter_name, column_name) != 0)
        continue;

      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");

      char *parameter_type = cJSON_GetStringValue(cjson_type);

      HARDBUG(parameter_type == NULL)

      if (compat_strcasecmp(parameter_type, "int") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        int ivalue = round(cJSON_GetNumberValue(cjson_value));

        HARDBUG(fprintf(fcsv, ",%d", ivalue) < 0)

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "double") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsNumber(cjson_value))

        double dvalue = cJSON_GetNumberValue(cjson_value);

        HARDBUG(fprintf(fcsv, ",%.2f", dvalue) < 0)

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "string") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        char *svalue = cJSON_GetStringValue(cjson_value);

        HARDBUG(svalue == NULL)

        HARDBUG(fprintf(fcsv, ",\"%s\"", svalue) < 0)

        nfound++;
      }
      else if (compat_strcasecmp(parameter_type, "bool") == 0)
      {
        cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

        HARDBUG(!cJSON_IsBool(cjson_value))

        int bvalue;

        if (cJSON_IsFalse(cjson_value))
          bvalue = 0;
        else
          bvalue = 1;

        HARDBUG(fprintf(fcsv, ",%d", bvalue) < 0)

        nfound++;
      }
      else
        FATAL("unknown parameter_type", EXIT_FAILURE)
    }

    if (nfound != 1)
    {
      PRINTF("unknown column %s\n", column_name);
      exit(EXIT_FAILURE);
    }
  }
  fprintf(fcsv, ",%d,%d,%d,%d\n", arg_nwon, arg_ndraw, arg_nlost, arg_nunknown);

  FCLOSE(fcsv)

label_return:
  BDESTROY(newpath)

  BDESTROY(stamp)

  BDESTROY(line)

  BDESTROY(header)
}

#define PRINTF_CFG_B(I) \
  PRINTF("options." #I "=%s\n", options.I ? "true" : "false");
#define PRINTF_CFG_I(I) PRINTF("options." #I "=%d\n", options.I);
#define PRINTF_CFG_D(D) PRINTF("options." #D "=%.2f\n", options.D);
#define PRINTF_CFG_I64(I64) PRINTF("options." #I64 "=%lld\n", options.I64);
#define PRINTF_CFG_S(S) PRINTF("options." #S "=%s\n", bdata(options.S));

int main(int argc, char **argv)
{
  FILE *farg;

  if ((farg = fopen(ARGS_TXT, "w")) == NULL)
  {
    PRINTF("could not open '%s'\n", ARGS_TXT);
    exit(EXIT_FAILURE);
  }

  for (int iarg = 0; iarg < argc; iarg++)
  {
    if (fprintf(farg, "%d %s\n", iarg, argv[iarg]) < 0)
    {
      PRINTF("could not fprintf '%s'\n", ARGS_TXT);
      exit(EXIT_FAILURE);
    }
  }

  if (fclose(farg) == EOF)
  {
    PRINTF("could not fclose '%s'\n", ARGS_TXT);
    exit(EXIT_FAILURE);
  }

  FILE *fversion;

  if ((fversion = fopen("version.txt", "w")) == NULL)
  {
    PRINTF("could not open 'version.txt'\n");
    exit(EXIT_FAILURE);
  }

  fprintf(fversion, "%s\n", REVISION);

  if (fclose(fversion) == EOF)
  {
    PRINTF("could not fclose 'version.txt'\n");
    exit(EXIT_FAILURE);
  }

  // special argument

  if ((argc > 1) and (compat_strcasecmp(argv[1], "--revision") == 0))
  {
    fprintf(stdout, "%s\n", REVISION);
    exit(EXIT_SUCCESS);
  }

  init_my_malloc();

  load_gwd_json();

  parse_parameters();

  if (compat_strcasecmp(bdata(options.hub_version), "NULL") == 0)
  {
    PRINTF("required parameter hub_version not found in gwd.json!\n");
    exit(EXIT_FAILURE);
  }

  if (compat_strcasecmp(bdata(options.overrides), "NULL") == 0)
  {
    PRINTF("required parameter overrides not found in gwd.json!\n");
    exit(EXIT_FAILURE);
  }

  options.mode = INVALID;

  // parse command line arguments

  int iarg = 1;

  int narg = 1;

  while (argv[narg] != NULL)
    ++narg;

  HARDBUG(narg != argc)

  while (argv[iarg] != NULL)
  {
    cJSON *parameters = cJSON_GetObjectItem(gwd_json, CJSON_PARAMETERS_ID);

    HARDBUG(!cJSON_IsArray(parameters))

    // loop over parameters

    cJSON *parameter;

    int nfound = 0;

    cJSON_ArrayForEach(parameter, parameters)
    {
      //{"name": "time_limit", "type": "int", "value": 30, "ui": "false"},
      // "args": ["--depth_limit", "-d"]}

      cJSON *cjson_args = cJSON_GetObjectItem(parameter, "args");

      if (cjson_args == NULL)
        continue;

      HARDBUG(!cJSON_IsArray(cjson_args))

      cJSON *cjson_type = cJSON_GetObjectItem(parameter, "type");

      char *parameter_type = cJSON_GetStringValue(cjson_type);

      HARDBUG(parameter_type == NULL)

      cJSON *cjson_arg;

      cJSON_ArrayForEach(cjson_arg, cjson_args)
      {
        BSTRING(format)
        BSTRING(svalue)

        HARDBUG(strlen(argv[iarg]) >= INT_MAX)
        HARDBUG(balloc(svalue, (int)strlen(argv[iarg]) + 1) == BSTR_ERR)

        char *arg = cJSON_GetStringValue(cjson_arg);

        HARDBUG(arg == NULL)

        if (compat_strcasecmp(parameter_type, "int") == 0)
        {

          HARDBUG(bassignformat(format, "%s=%%d", arg) == BSTR_ERR)

          int ivalue;

          if (my_sscanf(argv[iarg], bdata(format), &ivalue) != 1)
            goto label_next_arg;

          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

          HARDBUG(!cJSON_IsNumber(cjson_value))

          cJSON_SetNumberValue(cjson_value, ivalue);

          cJSON_AddBoolToObject(parameter, "cli", TRUE);

          nfound++;
        }
        else if (compat_strcasecmp(parameter_type, "double") == 0)
        {

          HARDBUG(bassignformat(format, "%s=%%lf", arg) == BSTR_ERR)

          double dvalue;

          if (my_sscanf(argv[iarg], bdata(format), &dvalue) != 1)
            goto label_next_arg;

          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

          HARDBUG(!cJSON_IsNumber(cjson_value))

          cJSON_SetNumberValue(cjson_value, dvalue);

          cJSON_AddBoolToObject(parameter, "cli", TRUE);

          nfound++;
        }
        else if (compat_strcasecmp(parameter_type, "string") == 0)
        {

          HARDBUG(bassignformat(format, "%s=%%s", arg) == BSTR_ERR)

          if (my_sscanf(argv[iarg], bdata(format), bdata(svalue)) != 1)
            goto label_next_arg;

          svalue->slen = (int)strlen(bdata(svalue));

          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

          HARDBUG(!cJSON_IsString(cjson_value))

          cJSON_SetValuestring(cjson_value, bdata(svalue));

          cJSON_AddBoolToObject(parameter, "cli", TRUE);

          nfound++;
        }
        else if (compat_strcasecmp(parameter_type, "bool") == 0)
        {

          HARDBUG(bassignformat(format, "%s=%%s", arg) == BSTR_ERR)

          if (my_sscanf(argv[iarg], bdata(format), bdata(svalue)) != 1)
            goto label_next_arg;

          svalue->slen = (int)strlen(bdata(svalue));

          cJSON *cjson_value = cJSON_GetObjectItem(parameter, "value");

          HARDBUG(!cJSON_IsBool(cjson_value))

          if ((compat_strcasecmp(bdata(svalue), "false") == 0) or
              (compat_strcasecmp(bdata(svalue), "off") == 0) or
              (compat_strcasecmp(bdata(svalue), "0") == 0))
            cJSON_SetBoolValue(cjson_value, 0);
          else
            cJSON_SetBoolValue(cjson_value, 1);

          cJSON_AddBoolToObject(parameter, "cli", TRUE);

          nfound++;
        }
      label_next_arg:
        BDESTROY(svalue)

        BDESTROY(format)
      }
    }

    HARDBUG(nfound > 1)

    if (nfound == 0)
      break;

    iarg++;
  }

  // we have to parse the parameters again
  // because overrides could have been set from the cli

  parse_parameters();

  load_overrides();

  // we have to parse the parameters again
  // because these could have been overridden

  parse_parameters();

  // query physical memory before MPI processes are started

  int physical_memory = return_physical_memory();
  int physical_cpus = return_physical_cpus();

  // MPI

  my_mpi_init(&narg, &argv, physical_memory);

  // now that we have the MPI context we can initialize

  init_my_printf();

  static my_printf_t my_stdout;

  construct_my_printf(&my_stdout, "stdout", TRUE);

  STDOUT = &my_stdout;

  options.hub = FALSE;

  if ((iarg == narg) or (compat_strcasecmp(argv[iarg], "hub") == 0))
  {
    options.hub = TRUE;

    init_hub();

    parse_parameters();
  }

  // modify options

  options.nthreads = options.nthreads_alpha_beta;

  PRINTF("Version is %s\n", REVISION);

  PRINTF("sizeof(int)=%lld\n", (my_int64_t)sizeof(int));
  PRINTF("sizeof(long)=%lld\n", (my_int64_t)sizeof(long));
  PRINTF("sizeof(long long)=%lld\n", (my_int64_t)sizeof(long long));

#if SCORE_T == SCORE_T_INT
  PRINTF("score_t is an INT\n");
#elif SCORE_T == SCORE_T_FLOAT
  PRINTF("score_t is a FLOAT\n");
#else
#error unknown SCORE_T
#endif

  PRINTF("physical_memory=%d MBYTE\n", physical_memory);

  PRINTF("physical_cpus=%d\n", physical_cpus);

  BSTRING(cpu_flags)

  my_bcpu_flags(cpu_flags);

  PRINTF("cpu flags=%s\n", bdata(cpu_flags));

  BDESTROY(cpu_flags)

#ifdef USE_HARDWARE_CRC32
  PRINTF("using HARDWARE crc32\n");
#else
  PRINTF("using SOFTWARE crc32\n");
#endif

#ifdef USE_HARDWARE_RAND
  PRINTF("using HARDWARE rand\n");
#else
  PRINTF("using SOFTWARE rand\n");
#endif

#ifdef USE_AVX2_INTRINSICS
  PRINTF("using __mm256 intrinsics for AVX2\n");
#else
  PRINTF("using GCC for AVX2\n");
#endif

  PRINTF_CFG_S(hub_version);
  PRINTF_CFG_S(overrides);

  PRINTF_CFG_I(verbose);

  PRINTF_CFG_S(network0);
  PRINTF_CFG_S(network1);
  PRINTF_CFG_I(network_evaluation_min);
  PRINTF_CFG_I(network_evaluation_max);
  PRINTF_CFG_I(network_evaluation_time);

  PRINTF_CFG_D(time_limit);

  PRINTF_CFG_I(time_control_ntrouble);
  PRINTF_CFG_I(time_control_mean);
  PRINTF_CFG_I(time_control_sigma);
  PRINTF_CFG_I(time_control_method);

  PRINTF_CFG_I(wall_time_threshold);

  PRINTF_CFG_I(depth_limit);

  PRINTF_CFG_B(use_book);
  PRINTF_CFG_S(book_name);
  PRINTF_CFG_I(book_randomness);
  PRINTF_CFG_I(book_position_frequency);
  PRINTF_CFG_I(book_move_frequency);
  PRINTF_CFG_I(book_evaluation_time);
  PRINTF_CFG_B(book_minimax);

  PRINTF_CFG_B(ponder);

  PRINTF_CFG_B(material_only);

  PRINTF_CFG_I(mobility_moves_threshold);

  PRINTF_CFG_I(best_score_margin);
  PRINTF_CFG_I(aspiration_margin);
  PRINTF_CFG_I(research_margin);

  PRINTF_CFG_B(use_quiescence);
  PRINTF_CFG_B(use_reductions);
  PRINTF_CFG_I(reduction_moves_threshold);
  PRINTF_CFG_I(reduction_depth_root);
  PRINTF_CFG_I(reduction_depth_leaf);
  PRINTF_CFG_I(reduction_full_min);
  PRINTF_CFG_I(reduction_percentage);
  PRINTF_CFG_I(reduction_margin);

  PRINTF_CFG_I(prob_cut_depth);
  PRINTF_CFG_I(prob_cut_percentage);
  PRINTF_CFG_I(prob_cut_margin);

  PRINTF_CFG_I(singular_probe_depth);
  PRINTF_CFG_I(singular_moves_threshold);
  PRINTF_CFG_I(singular_probe_percentage);
  PRINTF_CFG_I(singular_probe_margin);
  PRINTF_CFG_I(singular_reduction);

  PRINTF_CFG_B(sync_clock);

  PRINTF_CFG_I64(alpha_beta_cache_size);
  PRINTF_CFG_B(cache_draw_results);

  PRINTF_CFG_I64(score_cache_size);

  PRINTF_CFG_I(nthreads_alpha_beta);
  PRINTF_CFG_I(lazy_smp_policy);

  PRINTF_CFG_S(egtb_dir);
  PRINTF_CFG_S(egtb_dir_wdl);
  PRINTF_CFG_S(egtb_ram);
  PRINTF_CFG_S(egtb_ram_wdl);

  PRINTF_CFG_I64(egtb_entry_cache_size);

  PRINTF_CFG_S(dxp_server_ip);
  PRINTF_CFG_I(dxp_port);
  PRINTF_CFG_B(dxp_initiator);
  PRINTF_CFG_I(dxp_game_time);
  PRINTF_CFG_D(dxp_game_increment);
  PRINTF_CFG_I(dxp_game_moves);
  PRINTF_CFG_I(dxp_games);
  PRINTF_CFG_S(dxp_ballot);
  PRINTF_CFG_I(dxp_annotate_level);
  PRINTF_CFG_B(dxp_strict_gameend);
  PRINTF_CFG_S(dxp_tag);

  PRINTF_CFG_I(nthreads);

  PRINTF("my_mpi_globals.MMG_nslaves=%d"
         " my_mpi_globals.MMG_slave_id=%d\n",
         my_mpi_globals.MMG_nslaves,
         my_mpi_globals.MMG_slave_id);

  PRINTF("my_mpi_globals.MMG_nglobal=%d"
         " my_mpi_globals.MMG_global_id=%d\n",
         my_mpi_globals.MMG_nglobal,
         my_mpi_globals.MMG_global_id);

  if ((my_mpi_globals.MMG_is_master and my_mpi_globals.MMG_has_slaves) or
      RUNNING_ON_VALGRIND)
  {
    options.alpha_beta_cache_size = 128;

    options.score_cache_size = 32;

    PRINTF("WARNING: CACHE AND PRELOAD EGTB HAVE BEEN ADJUSTED!\n");
  }

  // hardware checks

  int memory = options.alpha_beta_cache_size + options.score_cache_size +
               options.egtb_entry_cache_size * options.nthreads;

  PRINTF("memory=%d MBYTE\n", memory);

  HARDBUG(memory >= physical_memory)

  HARDBUG(options.nthreads_alpha_beta > physical_cpus)

  // parameter checks

  HARDBUG(options.mobility_moves_threshold < 1)

  HARDBUG(options.best_score_margin < 0)
  HARDBUG(options.aspiration_margin < 1)
  HARDBUG(options.research_margin < 1)

  HARDBUG(options.reduction_moves_threshold < 1)
  HARDBUG(options.reduction_depth_root < 0)
  HARDBUG(options.reduction_depth_leaf < 0)
  HARDBUG(options.reduction_percentage < 1)
  HARDBUG(options.reduction_percentage > 99)
  HARDBUG(options.reduction_margin < 0)

  HARDBUG(options.prob_cut_depth < 0)
  HARDBUG(options.prob_cut_percentage < 1)
  HARDBUG(options.prob_cut_percentage > 99)
  HARDBUG(options.prob_cut_margin < 0)

  HARDBUG(options.singular_probe_depth < 0)
  HARDBUG(options.singular_moves_threshold < 1)
  HARDBUG(options.singular_probe_percentage < 1)
  HARDBUG(options.singular_probe_percentage > 99)
  HARDBUG(options.singular_probe_margin < 0)
  HARDBUG(options.singular_reduction < 0)

  HARDBUG(options.alpha_beta_cache_size < 0)

  INIT_PROFILE

  // BEGIN_BLOCK("main-init")

  if (my_mpi_globals.MMG_init)
    test_my_mpi();

  construct_my_random(&main_random, NULL);

  HARDBUG(compat_init() != 0)

  init_utils();

  // test_my_timers();
  // FATAL("test_my_timers", 1)

  construct_queue(&main_queue, "main_queue", STDOUT);

  init_book();

  init_boards();

  init_endgame();

  init_mcts();

  init_networks();

  init_score();

  init_search();

  check_my_malloc();

  // END_BLOCK

  // BEGIN_BLOCK("main-tests")

  // tests

  // test_my_printf();

  test_my_cjson();

  test_buckets();

#ifdef DEBUG
  test_my_random();
  // FATAL("test_my_random", EXIT_FAILURE)
#endif

  test_cJSON();

  test_my_object_class();

  if (options.use_book)
    open_book();
  // FATAL("test_book", EXIT_FAILURE)

  test_networks();

  test_utils();

  // test_my_timers();

  test_records();

  test_caches();

  // test_fbuffer();

  // test_dbase();
  // FATAL("test_dbase", EXIT_FAILURE)

  // test_network();

  // test_nmsimplex();

  // test_queues();
  // FATAL("test_queues", 1)

  test_states();

  test_stats();
  // FATAL("test_stats", EXIT_FAILURE)

  // test_threads();

  // test_compress();
  // FATAL("test_compress", EXIT_FAILURE)

  // END_BLOCK

  if (options.hub)
  {
    options.mode = GAME_MODE;

    start_threads();

    hub();

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_EXIT_THREAD,
                "main/hub");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_EXIT_THREAD,
            "main/hub");

    PRINTF("before join\n");
    join_threads();

    goto label_hub;
  }

  HARDBUG(argv[iarg] == NULL)

  BEGIN_BLOCK("main-thread")

  // main loop

  if (compat_strcasecmp(argv[iarg], "fen2score") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    fen2score(argv[iarg]);
  }
  else if (compat_strcasecmp(argv[iarg], "solve_problems") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    solve_problems(argv[iarg]);
  }
  else if (compat_strcasecmp(argv[iarg], "solve_problems_mcts") == 0)
  {
    iarg++;

    if (argv[iarg] == NULL)
      FATAL("<name> <mcts_depth> <mcts_time_limit> <nshoot_outs>", EXIT_FAILURE)

    char *name = argv[iarg];

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int mcts_depth;

    HARDBUG(my_sscanf(argv[iarg], "%d", &mcts_depth) != 1)

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    double mcts_time_limit;

    HARDBUG(my_sscanf(argv[iarg], "%lf", &mcts_time_limit) != 1)

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int nshoot_outs;

    HARDBUG(my_sscanf(argv[iarg], "%d", &nshoot_outs) != 1)

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      solve_problems_mcts(name, mcts_depth, mcts_time_limit, nshoot_outs);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "solve_problems_multi_threaded") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    start_threads();

    HARDBUG(thread_alpha_beta_master == NULL)

    solve_problems_multi_threaded(argv[iarg]);

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_EXIT_THREAD,
                "main/solve");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_EXIT_THREAD,
            "main/solve");

    join_threads();
  }
  else if (compat_strcasecmp(argv[iarg], "gen_random") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *fen_name;

    fen_name = argv[iarg];

    //

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    my_int64_t npositions_max;

    HARDBUG(my_sscanf(argv[iarg], "%lld", &npositions_max) != 1)

    //

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int nkings_max;

    HARDBUG(my_sscanf(argv[iarg], "%d", &nkings_max) != 1)

    //

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    double mcts_time_limit;

    HARDBUG(my_sscanf(argv[iarg], "%lf", &mcts_time_limit) != 1)

    HARDBUG(mcts_time_limit <= 0.0)

    int memory_slaves = memory * my_mpi_globals.MMG_nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    // int nthreads_slaves =
    //   options.nthreads_alpha_beta * my_mpi_globals.MMG_nslaves;
    // HARDBUG(nthreads_slaves > physical_cpus)

    test_weibull();

    for (int npieces = 0; npieces <= NPIECES_MAX; ++npieces)
      PRINTF("npieces=%d probability_one_king=%.6f\n",
             npieces,
             probability_one_king(npieces));

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
      gen_random(fen_name, npositions_max, nkings_max, mcts_time_limit);
  }
  else if (compat_strcasecmp(argv[iarg], "read_games") == 0)
  {
    iarg++;

    if (argv[iarg] == NULL)
      FATAL("db_name missing!", EXIT_FAILURE)

    char *db_name = argv[iarg];

    iarg++;

    if (argv[iarg] == NULL)
      FATAL("pdn_name missing!", EXIT_FAILURE)

    char *pdn_name = argv[iarg];

    my_sqlite3_t db;

    construct_my_sqlite3(&db, db_name, my_mpi_globals.MMG_slave_comm);

    db.MSQL_verbose = 0;

    read_games(&db, pdn_name);

    close_my_sqlite3(&db);
  }
  else if (compat_strcasecmp(argv[iarg], "add_evaluations2book") == 0)
  {
    iarg++;

    if (argv[iarg] == NULL)
      FATAL("db_name missing!", EXIT_FAILURE)

    char *db_name = argv[iarg];

    iarg++;

    int centi_seconds;

    HARDBUG(my_sscanf(argv[iarg], "%d", &centi_seconds) != 1)

    HARDBUG(centi_seconds < 1)

    int arg_add_moves = 0;

    if (argv[iarg] != NULL)
      HARDBUG(my_sscanf(argv[iarg], "%d", &arg_add_moves) != 1)

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      my_sqlite3_t db;

      construct_my_sqlite3(&db, db_name, my_mpi_globals.MMG_slave_comm);

      add_evaluations2book(&db, centi_seconds, arg_add_moves);

      close_my_sqlite3(&db);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "expand_book") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *db_name = argv[iarg];

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *fen = argv[iarg];

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int centi_seconds;

    HARDBUG(my_sscanf(argv[iarg], "%d", &centi_seconds) != 1)

    HARDBUG(centi_seconds < 1)

    my_sqlite3_t db;

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      construct_my_sqlite3(&db, db_name, my_mpi_globals.MMG_slave_comm);

      expand_book(&db, fen, centi_seconds);

      close_my_sqlite3(&db);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "walk_book") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *db_name = argv[iarg];

    iarg++;

    char *fen = argv[iarg];

    walk_book(db_name, fen);
  }
  else if (compat_strcasecmp(argv[iarg], "auto_play") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *txt_name;

    txt_name = argv[iarg];

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    my_int64_t npositions;

    HARDBUG(my_sscanf(argv[iarg], "%lld", &npositions) != 1)

    int memory_slaves = memory * my_mpi_globals.MMG_nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    // int nthreads_slaves =
    //   options.nthreads_alpha_beta * my_mpi_globals.MMG_nslaves;
    // HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      auto_play(txt_name, npositions);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "gen_db") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *db_name;

    db_name = argv[iarg];

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int mcts_depth;

    HARDBUG(my_sscanf(argv[iarg], "%d", &mcts_depth) != 1)

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    my_int64_t npositions;

    HARDBUG(my_sscanf(argv[iarg], "%lld", &npositions) != 1)

    int memory_slaves = memory * my_mpi_globals.MMG_nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    // int nthreads_slaves =
    //   options.nthreads_alpha_beta * my_mpi_globals.MMG_nslaves;
    // HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      my_sqlite3_t db;

      construct_my_sqlite3(&db, db_name, my_mpi_globals.MMG_slave_comm);

      gen_db(&db, mcts_depth, npositions);

      close_my_sqlite3(&db);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "update_db") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *db_name;

    db_name = argv[iarg];

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int mcts_depth;

    HARDBUG(my_sscanf(argv[iarg], "%d", &mcts_depth) != 1)

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int frequency;

    HARDBUG(my_sscanf(argv[iarg], "%d", &frequency) != 1)

    int memory_slaves = memory * my_mpi_globals.MMG_nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    // int nthreads_slaves =
    //   options.nthreads_alpha_beta * my_mpi_globals.MMG_nslaves;
    // HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      my_sqlite3_t db;

      construct_my_sqlite3(&db, db_name, my_mpi_globals.MMG_slave_comm);

      update_db(&db, mcts_depth, frequency);

      close_my_sqlite3(&db);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "play_game") == 0)
  {
    BSTRING(name)
    BSTRING(colour)
    BSTRING(line)
    BSTRING(oldpath)
    BSTRING(newpath)

    // name

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    HARDBUG(bassigncstr(name, argv[iarg]) == BSTR_ERR)

    // colour

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    HARDBUG(bassigncstr(colour, argv[iarg]) == BSTR_ERR)

    // game_minutes

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int game_minutes;

    HARDBUG(my_sscanf(argv[iarg], "%d", &game_minutes) != 1)

    // used_minutes

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    int used_minutes;

    HARDBUG(my_sscanf(argv[iarg], "%d", &used_minutes) != 1)

    options.mode = GAME_MODE;

    start_threads();

    while (TRUE)
    {

      if (fexists(bdata(name)))
      {
        PRINTF("\nDo you want to resume the game from %s (y/n)?\n",
               bdata(name));

        if (!my_breadline(line, stdin))
          break;

        CSTRING(canswer, strlen(bdata(line)))

        if (my_sscanf(bdata(line), "%s", canswer) != 1)
          strcpy(canswer, "y");

        if (*canswer == 'y')
        {
          PRINTF("Resuming game from %s\n", bdata(name));
        }
        else
        {
          PRINTF("New game %s\n", bdata(name));

          remove(bdata(name));
        }

        CDESTROY(canswer)
      }

      play_game(bdata(name), bdata(colour), game_minutes, used_minutes);

      PRINTF("\nDo you want to play another game (y/n)?\n");

      if (!my_breadline(line, stdin))
        break;

      CSTRING(canswer, strlen(bdata(line)))

      if (my_sscanf(bdata(line), "%s", canswer) != 1)
      {
        PRINTF("\neh?\n");

        CDESTROY(canswer)

        continue;
      }

      int finished = (*canswer == 'n');

      CDESTROY(canswer)

      if (finished)
        break;

      for (int ihistory = 10; ihistory > 1; --ihistory)
      {

        HARDBUG(bassignformat(oldpath, "%s-%d", bdata(name), ihistory - 1) ==
                BSTR_ERR)
        HARDBUG(bassignformat(newpath, "%s-%d", bdata(name), ihistory) ==
                BSTR_ERR)

        remove(bdata(newpath));

        rename(bdata(oldpath), bdata(newpath));
      }

      HARDBUG(bassignformat(newpath, "%s-1", bdata(name)) == BSTR_ERR)

      remove(bdata(newpath));

      rename(bdata(name), bdata(newpath));
    }

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_EXIT_THREAD,
                "main/play_game");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_EXIT_THREAD,
            "main/play_game");

    PRINTF("before join\n");
    join_threads();

    BDESTROY(newpath)

    BDESTROY(oldpath)

    BDESTROY(line)

    BDESTROY(colour)

    BDESTROY(name)
  }
  else if (compat_strcasecmp(argv[iarg], "fen2network") == 0)
  {
    iarg++;

    HARDBUG(argv[iarg] == NULL)

    char *fen_name;

    fen_name = argv[iarg];

    iarg++;

    HARDBUG(argv[iarg] == NULL)

    my_int64_t npositions;

    HARDBUG(my_sscanf(argv[iarg], "%lld", &npositions) != 1)

    int memory_slaves = memory * my_mpi_globals.MMG_nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    // int nthreads_slaves =
    //   options.nthreads_alpha_beta * my_mpi_globals.MMG_nslaves;
    // HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      fen2network(fen_name, npositions);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "fen2csv") == 0)
  {
    iarg++;

    if (argv[iarg] == NULL)
      FATAL("npieces_min argument missing", EXIT_FAILURE)

    int npieces_min;

    HARDBUG(my_sscanf(argv[iarg], "%d", &npieces_min) != 1)

    iarg++;

    if (argv[iarg] == NULL)
      FATAL("npieces_max argument missing", EXIT_FAILURE)

    int npieces_max;

    HARDBUG(my_sscanf(argv[iarg], "%d", &npieces_max) != 1)

    //

    iarg++;

    if (argv[iarg] == NULL)
      FATAL("file argument missing", EXIT_FAILURE)

    int memory_slaves = memory * my_mpi_globals.MMG_nslaves;

    HARDBUG(memory_slaves >= physical_memory)

    // int nthreads_slaves =
    //   options.nthreads_alpha_beta * my_mpi_globals.MMG_nslaves;
    // HARDBUG(nthreads_slaves > physical_cpus)

    if (my_mpi_globals.MMG_is_single or my_mpi_globals.MMG_is_slave)
    {
      fen2csv(argv[iarg], npieces_min, npieces_max);
    }
  }
  else if (compat_strcasecmp(argv[iarg], "dxp_server") == 0)
  {
    HARDBUG(memory >= (physical_memory / 2))

    if (options.ponder)
      HARDBUG(options.nthreads_alpha_beta > (physical_cpus / 2))
    else
      HARDBUG(options.nthreads_alpha_beta > physical_cpus)

    start_threads();

    dxp_server();

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_EXIT_THREAD,
                "main/dxp_server");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_EXIT_THREAD,
            "main/dxp_server");

    PRINTF("before join\n");
    join_threads();
  }
  else if (compat_strcasecmp(argv[iarg], "dxp_client") == 0)
  {
    HARDBUG(memory >= (physical_memory / 2))

    if (options.ponder)
    {
      if (options.nthreads_alpha_beta > (physical_cpus / 2))
        PRINTF("WARNING: options.nthreads_alpha_beta > (physical_cpus / 2)!");
    }
    else
    {
      if (options.nthreads_alpha_beta > physical_cpus)
        PRINTF("WARNING: options.nthreads_alpha_beta > physical_cpus!");
    }

    start_threads();

    dxp_client();

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_EXIT_THREAD,
                "main/dxp_client");
    }

    enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_EXIT_THREAD,
            "main/dxp_client");

    PRINTF("before join\n");
    join_threads();
  }
  else
  {
    PRINTF("unknown option or command %s\n", argv[iarg]);

    FATAL("unknown command", EXIT_FAILURE)
  }

label_hub:

  if (my_mpi_globals.MMG_init > 0)
    my_mpi_barrier_v3("main",
                      my_mpi_globals.MMG_global_comm,
                      my_mpi_globals.MMG_main_barrier_semkey,
                      TRUE);

  END_BLOCK

  // BEGIN_BLOCK("main-final")

  // finalization

  fin_endgame();

  fin_search();

  HARDBUG(bdestroy(options.hub_version) == BSTR_ERR)
  HARDBUG(bdestroy(options.overrides) == BSTR_ERR)
  HARDBUG(bdestroy(options.network0) == BSTR_ERR)
  HARDBUG(bdestroy(options.network1) == BSTR_ERR)
  HARDBUG(bdestroy(options.book_name) == BSTR_ERR)
  HARDBUG(bdestroy(options.egtb_dir) == BSTR_ERR)
  HARDBUG(bdestroy(options.egtb_dir_wdl) == BSTR_ERR)
  HARDBUG(bdestroy(options.egtb_ram) == BSTR_ERR)
  HARDBUG(bdestroy(options.egtb_ram_wdl) == BSTR_ERR)
  HARDBUG(bdestroy(options.dxp_server_ip) == BSTR_ERR)
  HARDBUG(bdestroy(options.dxp_ballot) == BSTR_ERR)
  HARDBUG(bdestroy(options.dxp_tag) == BSTR_ERR)

  fin_my_malloc(FALSE);

  PRINTF("$");

  // END_BLOCK

  DUMP_PROFILE(1)

  FATAL("OK", EXIT_SUCCESS)

  return (EXIT_SUCCESS);
}
