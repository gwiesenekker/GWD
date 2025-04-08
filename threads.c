//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

#define THREAD_ALPHA_BETA_MASTER 0
#define THREAD_ALPHA_BETA_SLAVE  1

thread_t threads[NTHREADS_MAX];

thread_t *thread_alpha_beta_master = NULL;

local void solve_problems(void *self, char *arg_name)
{ 
  thread_t *object = self;

  FILE *ftmp;
  
  HARDBUG((ftmp = fopen("tmp.fen", "w")) == NULL)

  my_bstream_t my_bstream;

  construct_my_bstream(&my_bstream, arg_name);

  int nproblems = 0;
  int nsolved = 0;

  while(my_bstream_readln(&my_bstream, TRUE) == BSTR_OK)
  {
    bstring bline = my_bstream.BS_bline;

    if (bchar(bline, 0) == '*') break;
    if (bchar(bline, 0) == '#') continue;

    BSTRING(bfen)

    CSTRING(cfen, blength(bline))

    HARDBUG(sscanf(bdata(bline), "%[^\n]", cfen) != 1)

    HARDBUG(bassigncstr(bfen, cfen) == BSTR_ERR)

    CDESTROY(cfen)

    ++nproblems;

    BSTRING(btext)
    
    bformata(btext, "problem #%d", nproblems);

    enqueue(&main_queue, MESSAGE_INFO, bdata(btext));

    BDESTROY(btext)

    //send FEN and GO to other threads

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread), MESSAGE_FEN,
                bdata(bfen));

      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread), MESSAGE_GO,
          "threads/solve_problems");
    }

    my_printf(&(object->thread_my_printf), "problem #%d\n", nproblems);

    fen2board(&(object->thread_search.S_board), bdata(bfen), TRUE);

    board_t *with_board = &(object->thread_search.S_board);

    print_board(&(object->thread_search.S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(with_board, &moves_list, FALSE);

    check_moves(with_board, &moves_list);

    if (moves_list.ML_nmoves > 0)
    {
      do_search(&(object->thread_search), &moves_list,
             INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

      if ((object->thread_search.S_best_score -
           object->thread_search.S_root_simple_score) > (SCORE_MAN / 2))
      {
        ++nsolved;
        my_printf(&(object->thread_my_printf),
          "solved problem #%d\n", nproblems);
      }
      else
      {
        my_printf(&(object->thread_my_printf),
          "did not solve problem #%d\n", nproblems);

        HARDBUG(fprintf(ftmp, "%s\n", bdata(bfen)) < 0)
      }
    }

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                   MESSAGE_ABORT_SEARCH, "threads/solve_problems");
    }
    BDESTROY(bfen)
  }

  destroy_my_bstream(&my_bstream);

  FCLOSE(ftmp)

  BSTRING(btext)

  HARDBUG(bformata(btext, "solved %d out of %d problems",
                   nsolved, nproblems) == BSTR_ERR)

  enqueue(&main_queue, MESSAGE_INFO, bdata(btext));

  BDESTROY(btext)
}

local void thread_func_alpha_beta_master(void *self)
{
  thread_t *object = self;

  //wait for message
    
  my_printf(&(object->thread_my_printf), "alpha_beta master thread\n");
  my_printf(&(object->thread_my_printf), "");
  while(TRUE)
  { 
    message_t message;
    
    if (dequeue(&(object->thread_queue), &message) != INVALID)
    {
      if (message.message_id == MESSAGE_SOLVE)
      {
        my_printf(&(object->thread_my_printf), "got SOLVE message %s\n",
          bdata(message.message_text));

        stop_my_timer(&(object->thread_idle_timer));
        start_my_timer(&(object->thread_busy_timer));

        solve_problems(object, bdata(message.message_text));

        stop_my_timer(&(object->thread_busy_timer));
        start_my_timer(&(object->thread_idle_timer));

        enqueue(&main_queue, MESSAGE_READY,
          "threads/thread_func_alpha_beta_master");
      }
      else if (message.message_id == MESSAGE_EXIT_THREAD)
      {
        my_printf(&(object->thread_my_printf), "got EXIT THREAD message %s\n",
          bdata(message.message_text));

        stop_my_timer(&(object->thread_idle_timer));

        my_printf(&(object->thread_my_printf),
          "%.2f seconds idle\n%.2f seconds busy\n",
          return_my_timer(&(object->thread_idle_timer), FALSE),
          return_my_timer(&(object->thread_busy_timer), FALSE));

        object->thread_idle =
          return_my_timer(&(object->thread_idle_timer), FALSE);

        break;
      }
      else if (message.message_id == MESSAGE_ABORT_SEARCH)
      {
        my_printf(&(object->thread_my_printf), "got ABORT SEARCH message %s\n",
          bdata(message.message_text));

        //send abort also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads + ithread),
              MESSAGE_ABORT_SEARCH, bdata(message.message_text));
        }
      }
      else if (message.message_id == MESSAGE_STATE)
      {
        my_printf(&(object->thread_my_printf), "got STATE message %s\n",
          bdata(message.message_text));

        //send board also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads + ithread),
              MESSAGE_STATE, bdata(message.message_text));
        }

        state_t game_state;

        construct_state(&game_state);

        game_state.set_state(&game_state, bdata(message.message_text));

        board_t *with_board = &(object->thread_search.S_board);

        state2board(with_board, &game_state);

        destroy_state(&game_state);
      }
      else if (message.message_id == MESSAGE_GO)
      {
        my_printf(&(object->thread_my_printf), "got GO message %s\n",
          bdata(message.message_text));

        //send GO also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads + ithread),
              MESSAGE_GO, "threads/thread_alha_beta_master");
        }

        board_t *with_board = &(object->thread_search.S_board);

        print_board(&(object->thread_search.S_board));

        moves_list_t moves_list;

        construct_moves_list(&moves_list);

        gen_moves(with_board, &moves_list, FALSE);

        HARDBUG(moves_list.ML_nmoves == 0)

        check_moves(with_board, &moves_list);

        stop_my_timer(&(object->thread_idle_timer));
        start_my_timer(&(object->thread_busy_timer));

        do_search(&(object->thread_search), &moves_list,
               INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

        stop_my_timer(&(object->thread_busy_timer));
        start_my_timer(&(object->thread_idle_timer));

        //message_t *sent[NTHREADS_MAX];

        BSTRING(btext)

        BSTRING(bmove_string)

        move2bstring(&moves_list, object->thread_search.S_best_move,
                     bmove_string);

        HARDBUG(bformata(btext, "%s %d %d",
                         bdata(bmove_string),
                         object->thread_search.S_best_score,
                         object->thread_search.S_best_depth) == BSTR_ERR)
      
        enqueue(&main_queue, MESSAGE_RESULT, bdata(btext));

        BDESTROY(bmove_string)

        BDESTROY(btext)

        //send abort to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads + ithread),
              MESSAGE_ABORT_SEARCH, bdata(message.message_text));
        }
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }
    compat_sleep(CENTI_SECOND);
  }
}

local void thread_func_alpha_beta_slave(thread_t *object)
{
  BEGIN_BLOCK(__FUNC__)

  my_printf(&(object->thread_my_printf), "alpha_beta slave thread\n");
  my_printf(&(object->thread_my_printf), "");

  while(TRUE)
  { 
    message_t message;
    
    if (dequeue(&(object->thread_queue), &message) != INVALID)
    {
      if (message.message_id == MESSAGE_FEN)
      {
        my_printf(&(object->thread_my_printf), "got FEN message %s\n",
          bdata(message.message_text));

        fen2board(&(object->thread_search.S_board),
                  bdata(message.message_text), TRUE);

        print_board(&(object->thread_search.S_board));
      }
      else if (message.message_id == MESSAGE_EXIT_THREAD)
      {
        my_printf(&(object->thread_my_printf), "got EXIT THREAD message %s\n",
          bdata(message.message_text));

        stop_my_timer(&(object->thread_idle_timer));

        my_printf(&(object->thread_my_printf),
          "%.2f seconds idle\n%.2f seconds busy\n",
          return_my_timer(&(object->thread_idle_timer), FALSE),
          return_my_timer(&(object->thread_busy_timer), FALSE));

        object->thread_idle =
          return_my_timer(&(object->thread_idle_timer), FALSE);

        break;
      }
      else if (message.message_id == MESSAGE_ABORT_SEARCH)
      {
        my_printf(&(object->thread_my_printf), "got ABORT SEARCH message %s\n",
          bdata(message.message_text));
      }
      else if (message.message_id == MESSAGE_STATE)
      {
        my_printf(&(object->thread_my_printf), "got STATE message %s\n",
          bdata(message.message_text));

        state_t game_state;

        construct_state(&game_state);

        game_state.set_state(&game_state, bdata(message.message_text));

        board_t *with_board = &(object->thread_search.S_board);

        state2board(with_board, &game_state);

        destroy_state(&game_state);
      }
      else if (message.message_id == MESSAGE_GO)
      {
        my_printf(&(object->thread_my_printf), "got GO message %s\n",
          bdata(message.message_text));

        while(TRUE)
        {
          if (dequeue(&(object->thread_queue), &message) == INVALID)
          {
            compat_sleep(CENTI_SECOND);
            continue;
          }

          if (message.message_id == MESSAGE_ABORT_SEARCH) break;

          BSTRING(bmove_string)

          CSTRING(cmove_string, blength(message.message_text))

          int depth_min;
          int depth_max;
          int root_score;
          int minimal_window;

          HARDBUG(sscanf(bdata(message.message_text), "%s%d%d%d%d",
                         cmove_string,
                         &depth_min, &depth_max,
                         &root_score, &minimal_window) != 5)

          HARDBUG(bassigncstr(bmove_string, cmove_string) == BSTR_ERR)

          CDESTROY(cmove_string)
            
          board_t *with_board = &(object->thread_search.S_board);

          moves_list_t moves_list;

          construct_moves_list(&moves_list);

          gen_moves(with_board, &moves_list, FALSE);

          HARDBUG(moves_list.ML_nmoves == 0)
    
          check_moves(with_board, &moves_list);
    
          int imove;

          if ((imove = search_move(&moves_list, bmove_string)) ==
              INVALID)
          {  
            print_board(&(object->thread_search.S_board));
  
            my_printf(&(object->thread_my_printf), "move=%s\n",
              bdata(bmove_string));
    
            fprintf_moves_list(&moves_list, &(object->thread_my_printf),
                               TRUE);
    
            FATAL("move not found", EXIT_FAILURE)
          }
    
          if (message.message_id == MESSAGE_SEARCH_FIRST)
          {
            my_printf(&(object->thread_my_printf),
              "got SEARCH FIRST message %s\n", bdata(message.message_text));

            my_printf(&(object->thread_my_printf), "%s %d %d %d %d\n",
              bdata(bmove_string), depth_min, depth_max,
              root_score, minimal_window);

            clear_totals(&(object->thread_search));

            stop_my_timer(&(object->thread_idle_timer));
            start_my_timer(&(object->thread_busy_timer));

            do_search(&(object->thread_search), &moves_list,
                   depth_min, depth_max, root_score, FALSE);

            stop_my_timer(&(object->thread_busy_timer));
            start_my_timer(&(object->thread_idle_timer));
  
            print_totals(&(object->thread_search));
          }
          else if (message.message_id == MESSAGE_SEARCH_AHEAD)
          {
            my_printf(&(object->thread_my_printf),
              "got SEARCH AHEAD message %s\n", bdata(message.message_text));

            my_printf(&(object->thread_my_printf), "%s %d %d %d %d\n",
              bdata(bmove_string), depth_min, depth_max,
              root_score, minimal_window);

            do_move(with_board, imove, &moves_list);

            moves_list_t your_moves_list;
  
            construct_moves_list(&your_moves_list);
  
            gen_moves(with_board, &your_moves_list, FALSE);
  
            if (your_moves_list.ML_nmoves > 0)
            {
              clear_totals(&(object->thread_search));

              stop_my_timer(&(object->thread_idle_timer));
              start_my_timer(&(object->thread_busy_timer));

              do_search(&(object->thread_search), &your_moves_list,
                     depth_min, depth_max, -root_score, FALSE);

              stop_my_timer(&(object->thread_busy_timer));
              start_my_timer(&(object->thread_idle_timer));
   
              print_totals(&(object->thread_search));
            }

            undo_move(with_board, imove, &moves_list);
          }
          else if (message.message_id == MESSAGE_SEARCH_SECOND)
          {
            my_printf(&(object->thread_my_printf),
              "got SEARCH SECOND message %s\n", bdata(message.message_text));

            my_printf(&(object->thread_my_printf), "%s %d %d %d %d\n",
              bdata(bmove_string), depth_min, depth_max,
              root_score, minimal_window);

            if (moves_list.ML_nmoves > 1)
            {
              //remove 

              for (int jmove = imove; jmove < (moves_list.ML_nmoves - 1); jmove++)
                moves_list.ML_moves[jmove] = moves_list.ML_moves[jmove + 1];

              moves_list.ML_nmoves--;
            }

            clear_totals(&(object->thread_search));

            stop_my_timer(&(object->thread_idle_timer));
            start_my_timer(&(object->thread_busy_timer));

            do_search(&(object->thread_search), &moves_list,
                   depth_min, depth_max, root_score, FALSE);
    
            stop_my_timer(&(object->thread_busy_timer));
            start_my_timer(&(object->thread_idle_timer));

            print_totals(&(object->thread_search));
          }
          else
            FATAL("unknown message_id", EXIT_FAILURE)

          BDESTROY(bmove_string)
        }
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }

    compat_sleep(CENTI_SECOND);
  }

  END_BLOCK
}

local void *thread_func(void *self)
{
  thread_t *object = self;

  my_printf(&(object->thread_my_printf), "thread=%p pthread_self=%#lX\n",
    object, compat_pthread_self());

  BEGIN_BLOCK("main-thread")

  BSTRING(bname)

  HARDBUG(bformata(bname, "thread-%#lX", compat_pthread_self()) == BSTR_ERR)

  construct_my_timer(&(object->thread_idle_timer), bdata(bname),
    &(object->thread_my_printf), FALSE);

  construct_my_timer(&(object->thread_busy_timer), bdata(bname),
    &(object->thread_my_printf), FALSE);

  BDESTROY(bname)

  object->thread_idle = 0.0;

  reset_my_timer(&(object->thread_idle_timer));

  reset_my_timer(&(object->thread_busy_timer));

  stop_my_timer(&(object->thread_busy_timer));

  if (object->thread_role == THREAD_ALPHA_BETA_MASTER)
  {
    thread_func_alpha_beta_master(object);
  }
  else if (object->thread_role == THREAD_ALPHA_BETA_SLAVE)
  {
    thread_func_alpha_beta_slave(object);
  }
  else
    FATAL("unknown thread_role", EXIT_FAILURE);

  //solve_random(object);

  END_BLOCK

  DUMP_PROFILE(1)

  return(NULL);
}

local void create_thread(void *self, int arg_role)
{
  thread_t *object = self;

  object->thread_role = arg_role;

  construct_my_printf(&(object->thread_my_printf), "log", FALSE);

  construct_my_random(&(object->thread_random), INVALID);

  BSTRING(queue_name)

  HARDBUG(bformata(queue_name, "thread-%p", self) == BSTR_ERR)

  construct_queue(&(object->thread_queue), bdata(queue_name),
                  &(object->thread_my_printf));

  BDESTROY(queue_name)

  construct_search(&(object->thread_search),
    &(object->thread_my_printf), object);

  compat_thread_create(&(object->thread), thread_func, object);
}

queue_t *return_thread_queue(void *self)
{
  thread_t *object = self;

  if (object == NULL) return(NULL);

  return(&(object->thread_queue));
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
    thread_t *object = threads + ithread;

    compat_thread_join(object->thread);
    
    idle += object->thread_idle;
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
    thread_t *object = test + itest;

    compat_thread_join(object->thread);
  }
}

