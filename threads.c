//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#include "globals.h"

#define THREAD_ALPHA_BETA_MASTER 0
#define THREAD_ALPHA_BETA_SLAVE  1

thread_t threads[NTHREADS_MAX];

thread_t *thread_alpha_beta_master = NULL;

local void solve_problems(void *self, char *arg_name)
{ 
  thread_t *object = self;

  FILE *fname, *ftmp;
  
  HARDBUG((fname = fopen(arg_name, "r")) == NULL)
  HARDBUG((ftmp = fopen("tmp.fen", "w")) == NULL)

  char line[MY_LINE_MAX];

  int nproblems = 0;
  int nsolved = 0;

  while(fgets(line, MY_LINE_MAX, fname) != NULL)
  {
    if (*line == '*') break;
    if (*line == '#') continue;

    char fen[MY_LINE_MAX];

    HARDBUG(sscanf(line, "%[^\n]", fen) != 1)

    ++nproblems;

    char text[MY_LINE_MAX];
    
    snprintf(text, MY_LINE_MAX, "problem #%d", nproblems);

    enqueue(&main_queue, MESSAGE_INFO, text);

    //send FEN and GO to other threads

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread), MESSAGE_FEN, fen);

      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread), MESSAGE_GO,
          "threads/solve_problems");
    }

    my_printf(&(object->thread_my_printf), "problem #%d\n", nproblems);

    fen2board(fen, object->thread_board_id);

    board_t *with_board = return_with_board(object->thread_board_id);

    print_board(object->thread_board_id);

    moves_list_t moves_list;

    create_moves_list(&moves_list);

    gen_moves(with_board, &moves_list, FALSE);

    check_moves(with_board, &moves_list);

    if (moves_list.nmoves > 0)
    {
      search(with_board, &moves_list,
             INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

      if ((with_board->board_search_best_score -
           with_board->board_search_root_simple_score) > (SCORE_MAN / 2))
      {
        ++nsolved;
        my_printf(&(object->thread_my_printf),
          "solved problem #%d\n", nproblems);
      }
      else
      {
        my_printf(&(object->thread_my_printf),
          "did not solve problem #%d\n", nproblems);
        HARDBUG(fprintf(ftmp, "%s\n", fen) < 0)
      }
    }

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads + ithread),
                   MESSAGE_ABORT_SEARCH, "threads/solve_problems");
    }
  }
  FCLOSE(fname)
  FCLOSE(ftmp)

  char text[MY_LINE_MAX];

  snprintf(text, MY_LINE_MAX, "solved %d out of %d problems",
    nsolved, nproblems);

  enqueue(&main_queue, MESSAGE_INFO, text);
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

        board_t *with_board = return_with_board(object->thread_board_id);

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

        board_t *with_board = return_with_board(object->thread_board_id);

        print_board(object->thread_board_id);

        moves_list_t moves_list;

        create_moves_list(&moves_list);

        gen_moves(with_board, &moves_list, FALSE);

        HARDBUG(moves_list.nmoves == 0)

        check_moves(with_board, &moves_list);

        stop_my_timer(&(object->thread_idle_timer));
        start_my_timer(&(object->thread_busy_timer));

        search(with_board, &moves_list,
               INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

        stop_my_timer(&(object->thread_busy_timer));
        start_my_timer(&(object->thread_idle_timer));

        //message_t *sent[NTHREADS_MAX];

        char move_string[MY_LINE_MAX];

        strcpy(move_string,
               moves_list.move2string(&moves_list,
                                      with_board->board_search_best_move));

        char text[MY_LINE_MAX];

        snprintf(text, MY_LINE_MAX, "%s %d %d",
          move_string, with_board->board_search_best_score,
          with_board->board_search_best_depth);
      
        enqueue(&main_queue, MESSAGE_RESULT, text);

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

        fen2board(bdata(message.message_text), object->thread_board_id);

        print_board(object->thread_board_id);
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

        board_t *with_board = return_with_board(object->thread_board_id);

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

          char move_string[MY_LINE_MAX];
          int depth_min;
          int depth_max;
          int root_score;
          int minimal_window;

          HARDBUG(sscanf(bdata(message.message_text), "%s%d%d%d%d",
                     move_string,
                     &depth_min, &depth_max,
                     &root_score, &minimal_window) != 5)
            
          board_t *with_board = return_with_board(object->thread_board_id);

          moves_list_t moves_list;

          create_moves_list(&moves_list);

          gen_moves(with_board, &moves_list, FALSE);

          HARDBUG(moves_list.nmoves == 0)
    
          check_moves(with_board, &moves_list);
    
          int imove;

          if ((imove = search_move(&moves_list, move_string)) ==
              INVALID)
          {  
            print_board(object->thread_board_id);
  
            my_printf(&(object->thread_my_printf), "move=%s\n",
              move_string);
    
            moves_list.fprintf_moves(&(object->thread_my_printf),
              &moves_list, TRUE);
    
            FATAL("move not found", EXIT_FAILURE)
          }
    
          if (message.message_id == MESSAGE_SEARCH_FIRST)
          {
            my_printf(&(object->thread_my_printf),
              "got SEARCH FIRST message %s\n", bdata(message.message_text));

            my_printf(&(object->thread_my_printf), "%s %d %d %d %d\n",
              move_string, depth_min, depth_max,
              root_score, minimal_window);

            clear_totals(with_board);

            stop_my_timer(&(object->thread_idle_timer));
            start_my_timer(&(object->thread_busy_timer));

            search(with_board, &moves_list,
                   depth_min, depth_max, root_score, FALSE);

            stop_my_timer(&(object->thread_busy_timer));
            start_my_timer(&(object->thread_idle_timer));
  
            print_totals(with_board);
          }
          else if (message.message_id == MESSAGE_SEARCH_AHEAD)
          {
            my_printf(&(object->thread_my_printf),
              "got SEARCH AHEAD message %s\n", bdata(message.message_text));

            my_printf(&(object->thread_my_printf), "%s %d %d %d %d\n",
              move_string, depth_min, depth_max,
              root_score, minimal_window);

            do_move(with_board, imove, &moves_list);

            moves_list_t your_moves_list;
  
            create_moves_list(&your_moves_list);
  
            gen_moves(with_board, &your_moves_list, FALSE);
  
            if (your_moves_list.nmoves > 0)
            {
              clear_totals(with_board);


              stop_my_timer(&(object->thread_idle_timer));
              start_my_timer(&(object->thread_busy_timer));

              search(with_board, &your_moves_list,
                     depth_min, depth_max, -root_score, FALSE);

              stop_my_timer(&(object->thread_busy_timer));
              start_my_timer(&(object->thread_idle_timer));
   
              print_totals(with_board);
            }

            undo_move(with_board, imove, &moves_list);
          }
          else if (message.message_id == MESSAGE_SEARCH_SECOND)
          {
            my_printf(&(object->thread_my_printf),
              "got SEARCH SECOND message %s\n", bdata(message.message_text));

            my_printf(&(object->thread_my_printf), "%s %d %d %d %d\n",
              move_string, depth_min, depth_max,
              root_score, minimal_window);

            if (moves_list.nmoves > 1)
            {
              //remove 

              for (int jmove = imove; jmove < (moves_list.nmoves - 1); jmove++)
                moves_list.moves[jmove] = moves_list.moves[jmove + 1];

              moves_list.nmoves--;
            }

            clear_totals(with_board);


            stop_my_timer(&(object->thread_idle_timer));
            start_my_timer(&(object->thread_busy_timer));

            search(with_board, &moves_list,
                   depth_min, depth_max, root_score, FALSE);
    
            stop_my_timer(&(object->thread_busy_timer));
            start_my_timer(&(object->thread_idle_timer));

            print_totals(with_board);
          }
          else
            FATAL("unknown message_id", EXIT_FAILURE)
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

#if COMPAT_CSTD == COMPAT_CSTD_C11
  i64_t tid = thrd_current();
#else
  i64_t tid = INVALID;
#endif

  my_printf(&(object->thread_my_printf), "thread=%p tid=%lld\n",
    object, tid);

  BEGIN_BLOCK("main-thread")

  bstring bname = bfromcstr("thread");

  HARDBUG(bname == NULL)

  HARDBUG(bformata(bname, "-%p", object) != BSTR_OK)

  construct_my_timer(&(object->thread_idle_timer), bdata(bname),
    &(object->thread_my_printf), FALSE);

  construct_my_timer(&(object->thread_busy_timer), bdata(bname),
    &(object->thread_my_printf), FALSE);

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

  bstring queue_name = bformat("thread-%p", self);

  construct_queue(&(object->thread_queue), bdata(queue_name),
                  &(object->thread_my_printf));

  bdestroy(queue_name);

  object->thread_board_id =
    create_board(&(object->thread_my_printf), object);

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

