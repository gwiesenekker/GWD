//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

int threads[NTHREADS_MAX];

int thread_alpha_beta_master_id = INVALID;

local thread_t thread_objs[NTHREADS_MAX];

thread_t *return_with_thread(int arg_id)
{
  int ithread = arg_id - THREAD_ID_OFFSET;
  HARDBUG(ithread < 0)
  HARDBUG(ithread >= NTHREADS_MAX)

  thread_t *with = thread_objs + ithread;
  HARDBUG(with->thread_id != arg_id)

  return(with);
}

local void solve_problems(thread_t *with, char *arg_name)
{ 
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
        enqueue(return_thread_queue(threads[ithread]), MESSAGE_FEN, fen);

      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads[ithread]), MESSAGE_GO,
          "threads/solve_problems");
    }

    my_printf(with->thread_my_printf, "problem #%d\n", nproblems);

    fen2board(fen, with->thread_board_id);

    board_t *with_board = return_with_board(with->thread_board_id);

    print_board(with->thread_board_id);

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
        my_printf(with->thread_my_printf,
          "solved problem #%d\n", nproblems);
      }
      else
      {
        my_printf(with->thread_my_printf,
          "did not solve problem #%d\n", nproblems);
        HARDBUG(fprintf(ftmp, "%s\n", fen) < 0)
      }
    }

    if (options.nthreads > 1)
    {
      for (int ithread = 1; ithread < options.nthreads; ithread++)
        enqueue(return_thread_queue(threads[ithread]),
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

local void thread_alpha_beta_master(thread_t *with)
{
  //wait for message
    
  my_printf(with->thread_my_printf, "alpha_beta master thread\n");
  my_printf(with->thread_my_printf, "");
  while(TRUE)
  { 
    message_t message;
    
    if (dequeue(&(with->thread_queue), &message) != INVALID)
    {
      if (message.message_id == MESSAGE_SOLVE)
      {
        my_printf(with->thread_my_printf, "got SOLVE message %s\n",
          bdata(message.message_text));

        stop_my_timer(&(with->thread_idle_timer));
        start_my_timer(&(with->thread_busy_timer));

        solve_problems(with, bdata(message.message_text));

        stop_my_timer(&(with->thread_busy_timer));
        start_my_timer(&(with->thread_idle_timer));

        enqueue(&main_queue, MESSAGE_READY,
          "threads/thread_alpha_beta_master");
      }
      else if (message.message_id == MESSAGE_EXIT_THREAD)
      {
        my_printf(with->thread_my_printf, "got EXIT THREAD message %s\n",
          bdata(message.message_text));

        stop_my_timer(&(with->thread_idle_timer));

        my_printf(with->thread_my_printf,
          "%.2f seconds idle\n%.2f seconds busy\n",
          return_my_timer(&(with->thread_idle_timer), FALSE),
          return_my_timer(&(with->thread_busy_timer), FALSE));

        with->thread_idle =
          return_my_timer(&(with->thread_idle_timer), FALSE);

        break;
      }
      else if (message.message_id == MESSAGE_ABORT_SEARCH)
      {
        my_printf(with->thread_my_printf, "got ABORT SEARCH message %s\n",
          bdata(message.message_text));

        //send abort also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads[ithread]),
              MESSAGE_ABORT_SEARCH, bdata(message.message_text));
        }
      }
      else if (message.message_id == MESSAGE_BOARD)
      {
        FATAL("MESSAGE_BOARD SHOULD NOT BE SENT ANY MORE", EXIT_FAILURE)
        my_printf(with->thread_my_printf, "got BOARD message %s\n",
          bdata(message.message_text));

        //send board also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads[ithread]),
              MESSAGE_BOARD, bdata(message.message_text));
        }

        string2board(bdata(message.message_text), with->thread_board_id);
        print_board(with->thread_board_id);
      }
      else if (message.message_id == MESSAGE_STATE)
      {
        my_printf(with->thread_my_printf, "got STATE message %s\n",
          bdata(message.message_text));

        //send board also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads[ithread]),
              MESSAGE_STATE, bdata(message.message_text));
        }

        state_t *game = state_class->objects_ctor();

        game->set_state(game, bdata(message.message_text));

        board_t *with_board = return_with_board(with->thread_board_id);

        state2board(with_board, game);

        state_class->objects_dtor(game);
      }
      else if (message.message_id == MESSAGE_MOVE)
      {
        FATAL("MESSAGE_MOVE SHOULD NOT BE SENT ANY MORE", EXIT_FAILURE)

        my_printf(with->thread_my_printf, "got MOVE message %s\n",
          bdata(message.message_text));

        //send move also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads[ithread]),
              MESSAGE_MOVE, bdata(message.message_text));
        }


        moves_list_t moves_list;

        create_moves_list(&moves_list);
    
        board_t *with_board = return_with_board(with->thread_board_id);
      
        gen_moves(with_board, &moves_list, FALSE);

        HARDBUG(moves_list.nmoves == 0)

        check_moves(with_board, &moves_list);

        char move_string[MY_LINE_MAX];

        strncpy(move_string, bdata(message.message_text), MY_LINE_MAX);

        int imove;

        if ((imove = search_move(&moves_list, move_string)) == INVALID)
        {  
          print_board(with->thread_board_id);

          my_printf(with->thread_my_printf, "move=%s\n",
            bdata(message.message_text));

          moves_list.fprintf_moves(with->thread_my_printf,
            &moves_list, TRUE);

          FATAL("move not found", EXIT_FAILURE)
        }

        do_move(with_board, imove, &moves_list);
      }
      else if (message.message_id == MESSAGE_GO)
      {
        my_printf(with->thread_my_printf, "got GO message %s\n",
          bdata(message.message_text));

        //send GO also to other threads

        if (options.nthreads > 1)
        {
          for (int ithread = 1; ithread < options.nthreads; ithread++)
            enqueue(return_thread_queue(threads[ithread]),
              MESSAGE_GO, "threads/thread_alha_beta_master");
        }

        board_t *with_board = return_with_board(with->thread_board_id);

        print_board(with->thread_board_id);

        moves_list_t moves_list;

        create_moves_list(&moves_list);

        gen_moves(with_board, &moves_list, FALSE);

        HARDBUG(moves_list.nmoves == 0)

        check_moves(with_board, &moves_list);

        search(with_board, &moves_list,
               INVALID, INVALID, SCORE_MINUS_INFINITY, FALSE);

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
            enqueue(return_thread_queue(threads[ithread]),
              MESSAGE_ABORT_SEARCH, bdata(message.message_text));
        }
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }
    my_sleep(CENTI_SECOND);
  }
}

local void thread_alpha_beta_slave(thread_t *with)
{
  BEGIN_BLOCK(__FUNC__)

  my_printf(with->thread_my_printf, "alpha_beta slave thread\n");
  my_printf(with->thread_my_printf, "");

  while(TRUE)
  { 
    message_t message;
    
    if (dequeue(&(with->thread_queue), &message) != INVALID)
    {
      if (message.message_id == MESSAGE_FEN)
      {
        my_printf(with->thread_my_printf, "got FEN message %s\n",
          bdata(message.message_text));

        fen2board(bdata(message.message_text), with->thread_board_id);

        print_board(with->thread_board_id);
      }
      else if (message.message_id == MESSAGE_EXIT_THREAD)
      {
        my_printf(with->thread_my_printf, "got EXIT THREAD message %s\n",
          bdata(message.message_text));

        stop_my_timer(&(with->thread_idle_timer));

        my_printf(with->thread_my_printf,
          "%.2f seconds idle\n%.2f seconds busy\n",
          return_my_timer(&(with->thread_idle_timer), FALSE),
          return_my_timer(&(with->thread_busy_timer), FALSE));

        with->thread_idle =
          return_my_timer(&(with->thread_idle_timer), FALSE);

        break;
      }
      else if (message.message_id == MESSAGE_ABORT_SEARCH)
      {
        my_printf(with->thread_my_printf, "got ABORT SEARCH message %s\n",
          bdata(message.message_text));
      }
      else if (message.message_id == MESSAGE_BOARD)
      {
        FATAL("MESSAGE_BOARD SHOULD NOT BE SENT ANY MORE", EXIT_FAILURE)

        my_printf(with->thread_my_printf, "got BOARD message %s\n",
          bdata(message.message_text));

        string2board(bdata(message.message_text), with->thread_board_id);

        print_board(with->thread_board_id);
      }
      else if (message.message_id == MESSAGE_MOVE)
      {
        FATAL("MESSAGE_MOVE SHOULD NOT BE SENT ANY MORE", EXIT_FAILURE)

        my_printf(with->thread_my_printf, "got MOVE message %s\n",
          bdata(message.message_text));

        //this message should have only been sent to other threads

        HARDBUG(with->thread_id == thread_alpha_beta_master_id)
     
        moves_list_t moves_list;

        create_moves_list(&moves_list);

        board_t *with_board = return_with_board(with->thread_board_id);
     
        gen_moves(with_board, &moves_list, FALSE);

        HARDBUG(moves_list.nmoves == 0)
  
        check_moves(with_board, &moves_list);
  
        char move_string[MY_LINE_MAX];
     
        strncpy(move_string, bdata(message.message_text), MY_LINE_MAX);
  
        int imove;

        if ((imove = search_move(&moves_list, move_string)) == INVALID)
        {  
          print_board(with->thread_board_id);

          my_printf(with->thread_my_printf, "move=%s\n",
            bdata(message.message_text));

          moves_list.fprintf_moves(with->thread_my_printf,
            &moves_list, TRUE);

          FATAL("move not found", EXIT_FAILURE)
        }
  
        do_move(with_board, imove, &moves_list);
      }
      else if (message.message_id == MESSAGE_STATE)
      {
        my_printf(with->thread_my_printf, "got STATE message %s\n",
          bdata(message.message_text));

        state_t *game = state_class->objects_ctor();

        game->set_state(game, bdata(message.message_text));

        board_t *with_board = return_with_board(with->thread_board_id);

        state2board(with_board, game);

        state_class->objects_dtor(game);
      }
      else if (message.message_id == MESSAGE_GO)
      {
        my_printf(with->thread_my_printf, "got GO message %s\n",
          bdata(message.message_text));

        while(TRUE)
        {
          if (dequeue(&(with->thread_queue), &message) == INVALID)
          {
            my_sleep(CENTI_SECOND);
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
            
          board_t *with_board = return_with_board(with->thread_board_id);

          moves_list_t moves_list;

          create_moves_list(&moves_list);

          gen_moves(with_board, &moves_list, FALSE);

          HARDBUG(moves_list.nmoves == 0)
    
          check_moves(with_board, &moves_list);
    
          int imove;

          if ((imove = search_move(&moves_list, move_string)) ==
              INVALID)
          {  
            print_board(with->thread_board_id);
  
            my_printf(with->thread_my_printf, "move=%s\n",
              move_string);
    
            moves_list.fprintf_moves(with->thread_my_printf,
              &moves_list, TRUE);
    
            FATAL("move not found", EXIT_FAILURE)
          }
    
          if (message.message_id == MESSAGE_SEARCH_FIRST)
          {
            my_printf(with->thread_my_printf,
              "got SEARCH FIRST message %s\n", bdata(message.message_text));

            my_printf(with->thread_my_printf, "%s %d %d %d %d\n",
              move_string, depth_min, depth_max,
              root_score, minimal_window);

            clear_totals(with_board);

            search(with_board, &moves_list,
                   depth_min, depth_max, root_score, FALSE);
  
            print_totals(with_board);
          }
          else if (message.message_id == MESSAGE_SEARCH_AHEAD)
          {
            my_printf(with->thread_my_printf,
              "got SEARCH AHEAD message %s\n", bdata(message.message_text));

            my_printf(with->thread_my_printf, "%s %d %d %d %d\n",
              move_string, depth_min, depth_max,
              root_score, minimal_window);

            do_move(with_board, imove, &moves_list);

            moves_list_t your_moves_list;
  
            create_moves_list(&your_moves_list);
  
            gen_moves(with_board, &your_moves_list, FALSE);
  
            if (your_moves_list.nmoves > 0)
            {
              clear_totals(with_board);

              search(with_board, &your_moves_list,
                     depth_min, depth_max, -root_score, FALSE);
   
              print_totals(with_board);
            }

            undo_move(with_board, imove, &moves_list);
          }
          else if (message.message_id == MESSAGE_SEARCH_SECOND)
          {
            my_printf(with->thread_my_printf,
              "got SEARCH SECOND message %s\n", bdata(message.message_text));

            my_printf(with->thread_my_printf, "%s %d %d %d %d\n",
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

            search(with_board, &moves_list,
                   depth_min, depth_max, root_score, FALSE);
    
            print_totals(with_board);
          }
          else
            FATAL("unknown message_id", EXIT_FAILURE)
        }
      }
      else
        FATAL("message.message_id error", EXIT_FAILURE)
    }

    my_sleep(CENTI_SECOND);
  }

  END_BLOCK
}

local void *thread_func(void *arg_with)
{
  thread_t *with = arg_with;

#if COMPAT_CSTD == COMPAT_CSTD_C11
  i64_t tid = thrd_current();
#else
  i64_t tid = INVALID;
#endif

  my_printf(with->thread_my_printf, "thread id=%d tid=%lld\n",
    with->thread_id, tid);

  BEGIN_BLOCK("main-thread")

  bstring bname = bfromcstr("thread");

  HARDBUG(bname == NULL)

  HARDBUG(bformata(bname, "%d", with->thread_id) == BSTR_ERR)

  construct_my_timer(&(with->thread_idle_timer), bdata(bname),
    with->thread_my_printf, FALSE);

  construct_my_timer(&(with->thread_busy_timer), bdata(bname),
    with->thread_my_printf, FALSE);

  with->thread_idle = 0.0;

  reset_my_timer(&(with->thread_idle_timer));

  reset_my_timer(&(with->thread_busy_timer));

  stop_my_timer(&(with->thread_busy_timer));

  if (with->thread_role == THREAD_ALPHA_BETA_MASTER)
  {
    thread_alpha_beta_master(with);
  }
  else if (with->thread_role == THREAD_ALPHA_BETA_SLAVE)
  {
    thread_alpha_beta_slave(with);
  }
  else
    FATAL("unknown thread_role", EXIT_FAILURE);

  //solve_random(with);

  END_BLOCK

  DUMP_PROFILE(1)

  return(NULL);
}

local int create_thread(int arg_role)
{
  int ithread;
  thread_t *with;

  //search empty slot

  for (ithread = 0; ithread < NTHREADS_MAX; ithread++)
  {
    with = thread_objs + ithread;

    if (with->thread_id == INVALID) break;
  }

  HARDBUG(ithread >= NTHREADS_MAX)
  with->thread_id = THREAD_ID_OFFSET + ithread;

  with->thread_role = arg_role;

  with->thread_my_printf = construct_my_printf(ithread);

  char queue_name[MY_LINE_MAX];

  snprintf(queue_name, MY_LINE_MAX, "thread%d", ithread);

  construct_queue(&(with->thread_queue), queue_name, with->thread_my_printf);

  with->thread_board_id =
    create_board(with->thread_my_printf, with->thread_id);

  my_thread_create(&(with->thread), thread_func, with);

  randull_thread(1, with->thread_id);

  return(with->thread_id);
}

ui64_t randull_thread(int init, int arg_id)
{
  thread_t *with = return_with_thread(arg_id);

  if (init == 1)
  {
    with->thread_j = 24 - 1;
    with->thread_k = 55 - 1;

    for (int i = 0; i < 55; i++)
    {
//ui64_t unsigned long??
      unsigned long long r;

#ifdef USE_HARDWARE_RAND
      HARDBUG(!_rdrand64_step(&r))
#else
#error NOT IMPLEMENTED YET
      r = i;
#endif

      with->thread_y[i] = r;

      HARDBUG(with->thread_y[i] != r)
    }
  }

  ui64_t ul =
    (with->thread_y[with->thread_k] += with->thread_y[with->thread_j]);

  if (--(with->thread_j) < 0) with->thread_j = 55 - 1;
  if (--(with->thread_k) < 0) with->thread_k = 55 - 1;

  return(ul);
}

queue_t *return_thread_queue(int arg_thread_id)
{
  thread_t *with = return_with_thread(arg_thread_id);

  return(&(with->thread_queue));
}

void init_threads(void)
{
  for (int ithread = 0; ithread < NTHREADS_MAX; ithread++)
  {
    thread_t *with = thread_objs + ithread;

    with->thread_id = INVALID;
  }
}

void start_threads(void)
{
  int ithread = 0;

  if (options.nthreads_alpha_beta > 0)
  {
    HARDBUG(ithread >= NTHREADS_MAX)

    thread_alpha_beta_master_id = threads[ithread] =
      create_thread(THREAD_ALPHA_BETA_MASTER);

    ithread++;

    for (int jthread = 1; jthread < options.nthreads_alpha_beta; jthread++)
    {
      HARDBUG(ithread >= NTHREADS_MAX)

      threads[ithread] = create_thread(THREAD_ALPHA_BETA_SLAVE);

      ithread++;
    }
    PRINTF("thread_alpha_beta_master_id=%d\n", thread_alpha_beta_master_id);
  }

  HARDBUG(ithread != options.nthreads)
}

void join_threads(void)
{
  double idle = 0.0;

  for (int ithread = 0; ithread < options.nthreads; ithread++)
  {
    thread_t *with = return_with_thread(threads[ithread]);

    my_thread_join(with->thread);
    
    idle += with->thread_idle;
  }
  PRINTF("total idle time=%.2f seconds\n", idle);
}

#define NTEST 4

void test_threads(void)
{
  int test [NTEST];

  for (int itest = 0; itest < NTEST; itest++)
    test[itest] = create_thread(INVALID);

  for (int itest = 0; itest < NTEST; itest++)
  {
    thread_t *with = return_with_thread(test[itest]);

    my_thread_join(with->thread);
  }
}

