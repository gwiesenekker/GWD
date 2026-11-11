// SCU REVISION 8.1266 wo 11 nov 2026  2:49:21 CET
#include "globals.h"

#define THREAD_ALPHA_BETA_MASTER 0
#define THREAD_ALPHA_BETA_SLAVE 1

thread_t threads[NTHREADS_MAX];

thread_t *thread_alpha_beta_master = NULL;

/* The previous search may still be unwinding. Acknowledge GO only after
 * its final publication, before any work for the new search is dispatched. */
local void start_search_workers(char *description)
{
  for (int i = 1; i < options.nthreads; i++)
  {
    atomic_store_explicit(&threads[i].T_nodes_ready,
                          FALSE,
                          memory_order_relaxed);
    enqueue(return_thread_queue(threads + i), MESSAGE_GO, description);
  }
  for (int i = 1; i < options.nthreads; i++)
    while (
      !atomic_load_explicit(&threads[i].T_nodes_ready, memory_order_acquire))
      HARDBUG(compat_sleep(CENTI_SECOND) != 0)
}

local void solve_problems(void *self, char *arg_name)
{
  thread_t *arg_with_thread = self;

  FILE *ftmp;

  HARDBUG((ftmp = fopen("tmp.fen", "w")) == NULL)

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  int nproblems = 0;
  int nsolved = 0;

  while (my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '*')
      break;
    if (bchar(bline, 0) == '#')
      continue;

    BSTRING(bfen)

    CSTRING(cfen, blength(bline))

    HARDBUG(my_sscanf(bdata(bline), "%[^\n]", cfen) != 1)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    ++nproblems;

    BSTRING(btext)

    bformata(btext, "problem #%d", nproblems);

    enqueue(&main_queue, MESSAGE_INFO, bdata(btext));

    BDESTROY(btext)

    // send FEN and GO to other threads

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_FEN,
                bdata(bfen));

      start_search_workers("threads/solve_problems");
    }

    my_printf(&(arg_with_thread->T_my_printf), "problem #%d\n", nproblems);

    fen2board(&(arg_with_thread->T_search.S_board), bdata(bfen));

    board_t *with_board = &(arg_with_thread->T_search.S_board);

    print_board(&(arg_with_thread->T_search.S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with_board, &moves_list);

    check_moves(with_board, &moves_list);

    if (moves_list.ML_nmoves > 0)
    {
      do_search(&(arg_with_thread->T_search),
                &moves_list,
                INVALID,
                INVALID,
                SCORE(SCORE_MINUS_INFINITY),
                NULL);

      if ((arg_with_thread->T_search.S_best_score -
           arg_with_thread->T_search.S_root_simple_score) > (SCORE_MAN / 2))
      {
        ++nsolved;
        my_printf(&(arg_with_thread->T_my_printf),
                  "solved problem #%d\n",
                  nproblems);
      }
      else
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "did not solve problem #%d\n",
                  nproblems);

        HARDBUG(fprintf(ftmp, "%s\n", bdata(bfen)) < 0)
      }
    }

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                MESSAGE_ABORT_SEARCH,
                "threads/solve_problems");
    }
    BDESTROY(bfen)
  }

  destroy_my_bstream(&my_bstream);

  FCLOSE(ftmp)

  BSTRING(btext)

  HARDBUG(bformata(btext, "solved %d out of %d problems", nsolved, nproblems) ==
          BSTR_ERR)

  enqueue(&main_queue, MESSAGE_INFO, bdata(btext));

  BDESTROY(btext)
}

local void thread_func_alpha_beta_master(void *self)
{
  thread_t *arg_with_thread = self;

  // wait for message

  my_printf(&(arg_with_thread->T_my_printf), "alpha_beta master thread\n");
  my_printf(&(arg_with_thread->T_my_printf), "$");
  while (TRUE)
  {
    message_t message;

    if (dequeue(&(arg_with_thread->T_queue), &message) != INVALID)
    {
      if (message.message_id == MESSAGE_SOLVE)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got SOLVE message %s\n",
                  bdata(message.message_text));

        stop_my_timer(&(arg_with_thread->T_idle_timer));
        start_my_timer(&(arg_with_thread->T_busy_timer));

        solve_problems(arg_with_thread, bdata(message.message_text));

        stop_my_timer(&(arg_with_thread->T_busy_timer));
        start_my_timer(&(arg_with_thread->T_idle_timer));

        enqueue(&main_queue,
                MESSAGE_READY,
                "threads/thread_func_alpha_beta_master");
      }
      else if (message.message_id == MESSAGE_EXIT_THREAD)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got EXIT THREAD message %s\n",
                  bdata(message.message_text));

        stop_my_timer(&(arg_with_thread->T_idle_timer));

        my_printf(&(arg_with_thread->T_my_printf),
                  "%.2f seconds idle\n%.2f seconds busy\n",
                  return_my_timer(&(arg_with_thread->T_idle_timer), FALSE),
                  return_my_timer(&(arg_with_thread->T_busy_timer), FALSE));

        arg_with_thread->T_idle =
          return_my_timer(&(arg_with_thread->T_idle_timer), FALSE);

        break;
      }
      else if (message.message_id == MESSAGE_ABORT_SEARCH)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got ABORT SEARCH message %s\n",
                  bdata(message.message_text));

        // send abort also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads + ithread),
                    MESSAGE_ABORT_SEARCH,
                    bdata(message.message_text));
        }
      }
      else if (message.message_id == MESSAGE_STATE)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got STATE message %s\n",
                  bdata(message.message_text));

        // send board also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads + ithread),
                    MESSAGE_STATE,
                    bdata(message.message_text));
        }

        game_state_t game_state;

        construct_game_state(&game_state);

        game_state.set_game_state(&game_state, bdata(message.message_text));

        board_t *with_board = &(arg_with_thread->T_search.S_board);

        state2board(with_board, &game_state);

        destroy_game_state(&game_state);
      }
      else if (message.message_id == MESSAGE_GO)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got GO message %s\n",
                  bdata(message.message_text));

        // send GO also to other threads

        if (options.nthreads > 1)
        {
          start_search_workers("threads/thread_alha_beta_master");
        }

        board_t *with_board = &(arg_with_thread->T_search.S_board);

        print_board(&(arg_with_thread->T_search.S_board));

        moves_list_t moves_list;

        construct_moves_list(&moves_list);

        gen_moves(with_board, &moves_list);

        HARDBUG(moves_list.ML_nmoves == 0)

        check_moves(with_board, &moves_list);

        stop_my_timer(&(arg_with_thread->T_idle_timer));
        start_my_timer(&(arg_with_thread->T_busy_timer));

        do_search(&(arg_with_thread->T_search),
                  &moves_list,
                  INVALID,
                  INVALID,
                  SCORE(SCORE_MINUS_INFINITY),
                  NULL);

        stop_my_timer(&(arg_with_thread->T_busy_timer));
        start_my_timer(&(arg_with_thread->T_idle_timer));

        // message_t *sent[NTHREADS_MAX];

        BSTRING(btext)

        BSTRING(bmove_string)

        move2bstring(&moves_list,
                     arg_with_thread->T_search.S_best_move,
                     bmove_string);

        HARDBUG(bformata(btext,
                         "%s " PRINTF_SCORE_T_RAW " %d",
                         bdata(bmove_string),
                         arg_with_thread->T_search.S_best_score,
                         arg_with_thread->T_search.S_best_depth) == BSTR_ERR)

        enqueue(&main_queue, MESSAGE_RESULT, bdata(btext));

        BDESTROY(bmove_string)

        BDESTROY(btext)

        // send abort to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads + ithread),
                    MESSAGE_ABORT_SEARCH,
                    bdata(message.message_text));
        }
      }
      else if (message.message_id == MESSAGE_MCTS)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got MCTS message %s\n",
                  bdata(message.message_text));

        // single threaded

        board_t *with_board = &(arg_with_thread->T_search.S_board);

        print_board(&(arg_with_thread->T_search.S_board));

        moves_list_t moves_list;

        construct_moves_list(&moves_list);

        gen_moves(with_board, &moves_list);

        HARDBUG(moves_list.ML_nmoves == 0)

        check_moves(with_board, &moves_list);

        stop_my_timer(&(arg_with_thread->T_idle_timer));
        start_my_timer(&(arg_with_thread->T_busy_timer));

        int nshoot_outs = 1000000;

        double wilson_lcb;
        double wilson_ucb;

        double result = mcts_shoot_outs(&(arg_with_thread->T_search),
                                        2,
                                        0,
                                        -options.time_limit,
                                        &nshoot_outs,
                                        NULL,
                                        NULL,
                                        NULL,
                                        &wilson_lcb,
                                        &wilson_ucb,
                                        FALSE);

        stop_my_timer(&(arg_with_thread->T_busy_timer));
        start_my_timer(&(arg_with_thread->T_idle_timer));

        // message_t *sent[NTHREADS_MAX];

        BSTRING(btext)

        HARDBUG(
          bformata(btext, "%.6f %.6f %.6f", result, wilson_lcb, wilson_ucb) ==
          BSTR_ERR)

        enqueue(&main_queue, MESSAGE_RESULT, bdata(btext));

        BDESTROY(btext)
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }
    HARDBUG(compat_sleep(CENTI_SECOND) != 0)
  }
}

local void thread_func_alpha_beta_slave(thread_t *arg_with_thread)
{
  BEGIN_BLOCK(__FUNC__)

  my_printf(&(arg_with_thread->T_my_printf), "alpha_beta slave thread\n");
  my_printf(&(arg_with_thread->T_my_printf), "$");

  while (TRUE)
  {
    message_t message;

    if (dequeue(&(arg_with_thread->T_queue), &message) != INVALID)
    {
      if (message.message_id == MESSAGE_FEN)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got FEN message %s\n",
                  bdata(message.message_text));

        fen2board(&(arg_with_thread->T_search.S_board),
                  bdata(message.message_text));

        print_board(&(arg_with_thread->T_search.S_board));
      }
      else if (message.message_id == MESSAGE_EXIT_THREAD)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got EXIT THREAD message %s\n",
                  bdata(message.message_text));

        stop_my_timer(&(arg_with_thread->T_idle_timer));

        my_printf(&(arg_with_thread->T_my_printf),
                  "%.2f seconds idle\n%.2f seconds busy\n",
                  return_my_timer(&(arg_with_thread->T_idle_timer), FALSE),
                  return_my_timer(&(arg_with_thread->T_busy_timer), FALSE));

        arg_with_thread->T_idle =
          return_my_timer(&(arg_with_thread->T_idle_timer), FALSE);

        break;
      }
      else if (message.message_id == MESSAGE_ABORT_SEARCH)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got ABORT SEARCH message %s\n",
                  bdata(message.message_text));
      }
      else if (message.message_id == MESSAGE_STATE)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got STATE message %s\n",
                  bdata(message.message_text));

        game_state_t game_state;

        construct_game_state(&game_state);

        game_state.set_game_state(&game_state, bdata(message.message_text));

        board_t *with_board = &(arg_with_thread->T_search.S_board);

        state2board(with_board, &game_state);

        destroy_game_state(&game_state);
      }
      else if (message.message_id == MESSAGE_GO)
      {
        my_printf(&(arg_with_thread->T_my_printf),
                  "got GO message %s\n",
                  bdata(message.message_text));

        atomic_store_explicit(&arg_with_thread->T_published_nodes,
                              0,
                              memory_order_relaxed);
        atomic_store_explicit(&arg_with_thread->T_nodes_ready,
                              TRUE,
                              memory_order_release);

        while (TRUE)
        {
          if (dequeue(&(arg_with_thread->T_queue), &message) == INVALID)
          {
            HARDBUG(compat_sleep(CENTI_SECOND) != 0)
            continue;
          }

          if (message.message_id == MESSAGE_ABORT_SEARCH)
            break;

          my_printf(&(arg_with_thread->T_my_printf),
                    "message.message_id=%d\n",
                    message.message_id);

          BSTRING(bmove_string)

          CSTRING(cmove_string, blength(message.message_text))

          int depth_min;
          int depth_max;
          score_t root_score;
          int minimal_window;

          HARDBUG(my_sscanf(bdata(message.message_text),
                            "%s%d%d" SCANF_SCORE_T_RAW "%d",
                            cmove_string,
                            &depth_min,
                            &depth_max,
                            &root_score,
                            &minimal_window) != 5)

          HARDBUG(bassigncstr(bmove_string, cmove_string) == BSTR_ERR)

          CDESTROY(cmove_string)

          board_t *with_board = &(arg_with_thread->T_search.S_board);

          moves_list_t moves_list;

          construct_moves_list(&moves_list);

          gen_moves(with_board, &moves_list);

          HARDBUG(moves_list.ML_nmoves == 0)

          check_moves(with_board, &moves_list);

          int imove;

          if ((imove = search_move(&moves_list, bmove_string)) == INVALID)
          {
            print_board(&(arg_with_thread->T_search.S_board));

            my_printf(&(arg_with_thread->T_my_printf),
                      "move=%s\n",
                      bdata(bmove_string));

            fprintf_moves_list(&moves_list,
                               &(arg_with_thread->T_my_printf),
                               TRUE);

            FATAL("move not found", EXIT_FAILURE)
          }

          if (message.message_id == MESSAGE_SEARCH_FIRST)
          {
            my_printf(&(arg_with_thread->T_my_printf),
                      "got SEARCH FIRST message %s\n",
                      bdata(message.message_text));

            my_printf(&(arg_with_thread->T_my_printf),
                      "%s %d %d " PRINTF_SCORE_T " %d\n",
                      bdata(bmove_string),
                      depth_min,
                      depth_max,
                      SCORE2PRINTF(root_score),
                      minimal_window);

            clear_totals(&(arg_with_thread->T_search));

            stop_my_timer(&(arg_with_thread->T_idle_timer));
            start_my_timer(&(arg_with_thread->T_busy_timer));

            do_search(&(arg_with_thread->T_search),
                      &moves_list,
                      depth_min,
                      depth_max,
                      root_score,
                      NULL);

            stop_my_timer(&(arg_with_thread->T_busy_timer));
            start_my_timer(&(arg_with_thread->T_idle_timer));

            print_totals(&(arg_with_thread->T_search));
          }
          else if (message.message_id == MESSAGE_SEARCH_AHEAD)
          {
            my_printf(&(arg_with_thread->T_my_printf),
                      "got SEARCH AHEAD message %s\n",
                      bdata(message.message_text));

            my_printf(&(arg_with_thread->T_my_printf),
                      "%s %d %d " PRINTF_SCORE_T " %d\n",
                      bdata(bmove_string),
                      depth_min,
                      depth_max,
                      SCORE2PRINTF(root_score),
                      minimal_window);

            do_move(with_board, imove, &moves_list);

            moves_list_t your_moves_list;

            construct_moves_list(&your_moves_list);

            gen_moves(with_board, &your_moves_list);

            if (your_moves_list.ML_nmoves > 0)
            {
              clear_totals(&(arg_with_thread->T_search));

              stop_my_timer(&(arg_with_thread->T_idle_timer));
              start_my_timer(&(arg_with_thread->T_busy_timer));

              do_search(&(arg_with_thread->T_search),
                        &your_moves_list,
                        depth_min,
                        depth_max,
                        -root_score,
                        NULL);

              stop_my_timer(&(arg_with_thread->T_busy_timer));
              start_my_timer(&(arg_with_thread->T_idle_timer));

              print_totals(&(arg_with_thread->T_search));
            }

            undo_move(with_board, imove, &moves_list);
          }
          else if (message.message_id == MESSAGE_SEARCH_SECOND)
          {
            my_printf(&(arg_with_thread->T_my_printf),
                      "got SEARCH SECOND message %s\n",
                      bdata(message.message_text));

            my_printf(&(arg_with_thread->T_my_printf),
                      "%s %d %d " PRINTF_SCORE_T " %d\n",
                      bdata(bmove_string),
                      depth_min,
                      depth_max,
                      SCORE2PRINTF(root_score),
                      minimal_window);

            if (moves_list.ML_nmoves > 1)
            {
              // remove

              for (int jmove = imove; jmove < (moves_list.ML_nmoves - 1);
                   jmove++)
                moves_list.ML_moves[jmove] = moves_list.ML_moves[jmove + 1];

              moves_list.ML_nmoves--;
            }

            clear_totals(&(arg_with_thread->T_search));

            stop_my_timer(&(arg_with_thread->T_idle_timer));
            start_my_timer(&(arg_with_thread->T_busy_timer));

            do_search(&(arg_with_thread->T_search),
                      &moves_list,
                      depth_min,
                      depth_max,
                      root_score,
                      NULL);

            stop_my_timer(&(arg_with_thread->T_busy_timer));
            start_my_timer(&(arg_with_thread->T_idle_timer));

            print_totals(&(arg_with_thread->T_search));
          }
          else
            FATAL("unknown message_id", EXIT_FAILURE)

          BDESTROY(bmove_string)
        }
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }

    HARDBUG(compat_sleep(CENTI_SECOND) != 0)
  }

  END_BLOCK
}

local void *thread_func(void *self)
{
  thread_t *arg_with_thread = self;

  my_printf(&(arg_with_thread->T_my_printf),
            "thread=%p pthread_self=%#lX\n",
            arg_with_thread,
            compat_pthread_self());

  BEGIN_BLOCK("main-thread")

  BSTRING(bname)

  HARDBUG(bformata(bname, "thread-%#lX", compat_pthread_self()) == BSTR_ERR)

  construct_my_timer(&(arg_with_thread->T_idle_timer),
                     bdata(bname),
                     &(arg_with_thread->T_my_printf),
                     FALSE);

  construct_my_timer(&(arg_with_thread->T_busy_timer),
                     bdata(bname),
                     &(arg_with_thread->T_my_printf),
                     FALSE);

  BDESTROY(bname)

  arg_with_thread->T_idle = 0.0;

  reset_my_timer(&(arg_with_thread->T_idle_timer));

  reset_my_timer(&(arg_with_thread->T_busy_timer));

  stop_my_timer(&(arg_with_thread->T_busy_timer));

  if (arg_with_thread->thread_role == THREAD_ALPHA_BETA_MASTER)
  {
    thread_func_alpha_beta_master(arg_with_thread);
  }
  else if (arg_with_thread->thread_role == THREAD_ALPHA_BETA_SLAVE)
  {
    thread_func_alpha_beta_slave(arg_with_thread);
  }
  else
    FATAL("unknown thread_role", EXIT_FAILURE);

  // solve_random(arg_with_thread);

  END_BLOCK

  DUMP_PROFILE(1)

  return (NULL);
}

local void create_thread(void *self, int arg_role)
{
  thread_t *arg_with_thread = self;

  arg_with_thread->thread_role = arg_role;

  atomic_init(&arg_with_thread->T_published_nodes, 0);
  atomic_init(&arg_with_thread->T_nodes_ready, FALSE);
  HARDBUG(!atomic_is_lock_free(&arg_with_thread->T_published_nodes))

  construct_my_printf(&(arg_with_thread->T_my_printf), "log", FALSE);

  BSTRING(queue_name)

  HARDBUG(bformata(queue_name, "thread-%p", self) == BSTR_ERR)

  construct_queue(&(arg_with_thread->T_queue),
                  bdata(queue_name),
                  &(arg_with_thread->T_my_printf));

  BDESTROY(queue_name)

  construct_search(&(arg_with_thread->T_search),
                   &(arg_with_thread->T_my_printf),
                   arg_with_thread,
                   NETWORK0_ID);

  HARDBUG(compat_thread_create(&(arg_with_thread->T_thread),
                               thread_func,
                               arg_with_thread) != 0)
}

queue_t *return_thread_queue(void *self)
{
  thread_t *arg_with_thread = self;

  if (arg_with_thread == NULL)
    return (NULL);

  return (&(arg_with_thread->T_queue));
}

void start_threads(void)
{
  if (options.nthreads_alpha_beta > 0)
  {
    create_thread(threads, THREAD_ALPHA_BETA_MASTER);

    thread_alpha_beta_master = threads;

    PRINTF("thread_alpha_beta_master=%p\n", thread_alpha_beta_master);

    for (int ithread = 1; ithread < options.nthreads_alpha_beta; ithread++)
      create_thread(threads + ithread, THREAD_ALPHA_BETA_SLAVE);
  }
}

void join_threads(void)
{
  double idle = 0.0;

  for (int ithread = 0; ithread < options.nthreads; ithread++)
  {
    thread_t *with_thread = threads + ithread;

    HARDBUG(compat_thread_join(with_thread->T_thread) != 0)

    idle += with_thread->T_idle;
  }
  PRINTF("total idle time=%.2f seconds\n", idle);
}

#define NTEST 4

void test_threads(void)
{
  thread_t test[NTEST];

  for (int itest = 0; itest < NTEST; itest++)
    create_thread(test + itest, INVALID);

  for (int itest = 0; itest < NTEST; itest++)
  {
    thread_t *with_thread = test + itest;

    HARDBUG(compat_thread_join(with_thread->T_thread) != 0)
  }
}
