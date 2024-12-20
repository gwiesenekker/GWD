//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#include "globals.h"

#define SA struct sockaddr

#define GAME_MOVES 40

#define DXP_GAMEREQ 'R'
#define DXP_GAMEACC 'A'
#define DXP_MOVE    'M'
#define DXP_GAMEEND 'E'
#define DXP_CHAT    'C'
#define DXP_BACKREQ '?'
#define DXP_BACKACC '?'
#define DXP_INVALID '?'

#define DXP_UNKNOWN '0'
#define DXP_IGIVEUP '1'
#define DXP_DRAW    '2'
#define DXP_IWIN    '3'

#define SQR(X) ((X) * (X))

#define MARGIN 5

typedef struct
{
  search_t DXP_search;

  int DXP_game_colour;
  int DXP_game_time;
  int DXP_game_moves;
  char DXP_move_string[MY_LINE_MAX];
  int DXP_game_time_used;
  int DXP_move_number;
  char DXP_game_code;
} dxp_t;

local char event[MY_LINE_MAX];
local char my_name[MY_LINE_MAX];
local char your_name[MY_LINE_MAX];

local void encode(char type, dxp_t *with,
  char buffer[MY_LINE_MAX], int *nbuffer)
{
  *nbuffer = 0;

  if (type == DXP_GAMEREQ)
  {
    int i = 0;

    buffer[i] = DXP_GAMEREQ;
    i++;

    strcpy(buffer + i, "01");
    i += 2;

    //                  01234567890123456789012345678901

    strncpy(buffer + i, my_name, 32);

    i += 32;

    if (IS_WHITE(with->DXP_game_colour))
    {
      buffer[i] = 'Z';
    }
    else
    {
      buffer[i] = 'W';
    }
    i++;

    char string[MY_LINE_MAX];

    snprintf(string, MY_LINE_MAX, "%03d", options.dxp_game_time);

    strcpy(buffer + i, string); 
    i += 3;

    //dependency

    with->DXP_game_time = options.dxp_game_time * 60;

    snprintf(string, MY_LINE_MAX, "%03d", options.dxp_game_moves);

    with->DXP_game_moves = options.dxp_game_moves;

    strcpy(buffer + i, string); 
    i += 3;

    strcpy(buffer + i, "B");
    i++;

    if (IS_WHITE(with->DXP_search.S_board.board_colour2move))
    {
      strcpy(buffer + i, "W");
      i++;
    }
    else
    {
      strcpy(buffer + i, "Z");
      i++;
    }

    for (int j = 1; j <= 50; ++j)
    {
      int jboard = map[j];

      if (with->DXP_search.S_board.board_white_man_bb & BITULL(jboard))
      {
        strcpy(buffer + i, "w");
        i++;
      }
      else if (with->DXP_search.S_board.board_white_king_bb & BITULL(jboard))
      {
        strcpy(buffer + i, "W");
        i++;
      } 
      else if (with->DXP_search.S_board.board_black_man_bb & BITULL(jboard))
      {
        strcpy(buffer + i, "z");
        i++;
      }
      else if (with->DXP_search.S_board.board_black_king_bb & BITULL(jboard))
      {
        strcpy(buffer + i, "Z");
        i++;
      } 
      else
      {
        strcpy(buffer + i, "e");
        i++;
      }  
    }

    buffer[i] = '\0';
    i++;

    *nbuffer = i;

    PRINTF("DXP_GAMEREQ buffer=%s nbuffer=%d\n", buffer, *nbuffer);
  } 
  else if (type == DXP_GAMEACC)
  {
    int i = 0;

    buffer[i] = DXP_GAMEACC;
    i++;

    //                  01234567890123456789012345678901
    strncpy(buffer + i, my_name, 32);
    i += 32;

    buffer[i] = '0';
    i++;

    buffer[i] = '\0';
    i++;

    *nbuffer = i;

    PRINTF("DXP_GAMEACC buffer=%s nbuffer=%d\n", buffer, *nbuffer);
  }
  else if (type == DXP_MOVE)
  {
    int i = 0;

    buffer[i] = DXP_MOVE;
    i++;

    char string[MY_LINE_MAX];

    snprintf(string, MY_LINE_MAX, "%04d", with->DXP_game_time_used);

    strcpy(buffer + i, string); 
    i += 4;

    //ugly

    int j = 0;

    strncpy(buffer + i, with->DXP_move_string + j, 2);
    j += 3;
    i += 2;

    strncpy(buffer + i, with->DXP_move_string + j, 2);
    j += 2;
    i += 2;
    
    int n = 0;
    int k = j;
    while (with->DXP_move_string[k] == 'x')
    { 
      n++;
      k += 3;
    }

    snprintf(string, MY_LINE_MAX, "%02d", n);
    strcpy(buffer + i, string); 
    i += 2;

    k = j;
    while (with->DXP_move_string[k] == 'x')
    { 
      k++;
      strncpy(buffer + i, with->DXP_move_string + k, 2);
      k += 2;
      i += 2;
    }

    buffer[i] = '\0';
    i++;

    *nbuffer = i;

    PRINTF("DXP_MOVE buffer=%s nbuffer=%d\n", buffer, *nbuffer);
  }
  else if (type == DXP_GAMEEND)
  {
    int i = 0;

    buffer[i] = DXP_GAMEEND;
    i++;
  
    buffer[i] = with->DXP_game_code;
    i++;

    buffer[i] = '0';
    i++;

    buffer[i] = '\0';
    i++;

    *nbuffer = i;

    PRINTF("DXP_GAMEEND buffer=%s nbuffer=%d\n", buffer, *nbuffer);
  }
  else if (type == DXP_CHAT)
  { 
    FATAL("DXP_CHAT not implemented", EXIT_FAILURE)
  }
  else if (type == DXP_BACKREQ)
  {
    FATAL("DXP_BACKREQ not implemented", EXIT_FAILURE)
  }
  else if (type == DXP_BACKACC)
  {
    FATAL("DXP_BACKACC not implemented", EXIT_FAILURE)
  }
  else
  {
    FATAL("Unknown message type", EXIT_FAILURE)
  }
}

char decode(int nbuffer, char *buffer, dxp_t *with)
{
  HARDBUG(nbuffer < 1)

  //buffer[nbuffer] = '\0';
  PRINTF("decode nbuffer=%d buffer=%s\n", nbuffer, buffer);

  if (buffer[0] == DXP_GAMEREQ)
  {
    //ignore version

    int i = 3;
 
    char string[MY_LINE_MAX];

    HARDBUG((i + 31) >= nbuffer)

    strncpy(your_name, buffer + i, 32);

    your_name[32] = '\0';

    PRINTF("your_name=%s\n", your_name);

    i += 32;

    HARDBUG(i >= nbuffer)

    char game_colour = buffer[i];
    i++;

    PRINTF("game_colour=%c\n", game_colour);

    if (game_colour == 'W')
      with->DXP_game_colour = WHITE_BIT;
    else
      with->DXP_game_colour = BLACK_BIT;

    int game_time;
 
    HARDBUG((i + 2) >= nbuffer)
    HARDBUG(sscanf(buffer + i, "%3d", &game_time) != 1)
    i += 3;

    PRINTF("game_time=%d\n", game_time);

    with->DXP_game_time = game_time * 60;

    int game_moves;

    HARDBUG((i + 2) >= nbuffer)
    HARDBUG(sscanf(buffer + i, "%3d", &game_moves) != 1)
    i += 3;

    PRINTF("game_moves=%d\n", game_moves);

    with->DXP_game_moves = game_moves;
 
    HARDBUG(i >= nbuffer)
    if (buffer[i] == 'A')
    {
      PRINTF("starting position\n");

      string2board(&(with->DXP_search.S_board), STARTING_POSITION);
    }
    else if (buffer[i] == 'B')
    {
      i++;
      HARDBUG(i >= nbuffer)

      if (buffer[i] == 'W')
        string[0] = 'w';
      else if (buffer[i] == 'Z')
        string[0] = 'b';
      else
        FATAL("error in B/colour2move", EXIT_FAILURE)
   
      for (int j = 1; j <= 50; j++)
      {
        i++;
        HARDBUG(i >= nbuffer)
  
        if (buffer[i] == 'e')
          string[j] = *nn;
        else if (buffer[i] == 'w')
          string[j] = *wO;
        else if (buffer[i] == 'z')
          string[j] = *bO;
        else if (buffer[i] == 'W')
          string[j] = *wX;
        else if (buffer[i] == 'Z')
          string[j] = *bX;
        else
          FATAL("error in B/position", EXIT_FAILURE)
      }

      string[51] = '\0';

      PRINTF("string=%s\n", string);

      string2board(&(with->DXP_search.S_board), string);

      PRINTF("starting position\n");

      print_board(&(with->DXP_search.S_board));
    }
    else
      FATAL("error in DXP", EXIT_FAILURE)

    return(DXP_GAMEREQ);
  } 
  else if (buffer[0] == DXP_GAMEACC)
  {
    int i = 1;

    HARDBUG((i + 31) >= nbuffer)

    strncpy(your_name, buffer + i, 32);

    your_name[32] = '\0';

    PRINTF("your_name=%s\n", your_name);

    i += 32;

    HARDBUG(i >= nbuffer)

    char code = buffer[i];

    PRINTF("acceptance code=%c\n", code);

    return(DXP_GAMEACC);
  }
  else if (buffer[0] == DXP_MOVE)
  {
    //ignore time
    
    int i = 5;

    int f;
    HARDBUG(i >= nbuffer)
    HARDBUG(sscanf(buffer + i, "%2d", &f) != 1)
    HARDBUG(f < 1)
    HARDBUG(f > 50)
    PRINTF("f=%d\n", f);
    f = map[f];

    i += 2;

    int t;
    HARDBUG(i >= nbuffer)
    HARDBUG(sscanf(buffer + i, "%2d", &t) != 1)
    HARDBUG(t < 1)
    HARDBUG(t > 50)
    PRINTF("t=%d\n", t);
    t = map[t];
    i += 2;

    int n;
    HARDBUG(i >= nbuffer)
    HARDBUG(sscanf(buffer + i, "%2d", &n) != 1)
    HARDBUG(n > NPIECES_MAX)
    PRINTF("n=%d\n", n);
    i += 2;

    int c[NPIECES_MAX];
    for (int j = 0; j < n; j++)
    { 
      HARDBUG(i >= nbuffer)
      HARDBUG(sscanf(buffer + i, "%2d", c + j) != 1)
      HARDBUG(c[j] < 1)
      HARDBUG(c[j] > 50) 
      PRINTF("j=%d c[j]=%d\n", j, c[j]);
      c[j] = map[c[j]];
      i += 2;
    }

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with->DXP_search.S_board), &moves_list, FALSE);

    HARDBUG(moves_list.nmoves == 0)

    HARDBUG(n != moves_list.ncaptx)

    int jmove = INVALID;

    for (int imove = 0; imove < moves_list.nmoves; imove++)
    {
      move_t *move = moves_list.moves + imove;

      int iboard = move->move_from;
      int kboard = move->move_to;
      ui64_t captures_bb = move->move_captures_bb;

      if (iboard != f) continue;
      if (kboard != t) continue;

      if (captures_bb == 0)
      {
        jmove = imove;
      }
      else
      {
        int m = 0;

        while(captures_bb != 0)
        {
          int jboard = BIT_CTZ(captures_bb);

          m = 0;
          for (int j = 0; j < n; j++)
            if (c[j] == jboard) m++;

          if (m == 0) break;

          HARDBUG(m != 1)

          captures_bb &= ~BITULL(jboard);
        } 
        if (m == 1)
        {
          jmove = imove;
        }
      }
    }
    HARDBUG(jmove == INVALID)

    BSTRING(move)

    move2bstring(&moves_list, jmove, move);

    strcpy(with->DXP_move_string, bdata(move));

    BDESTROY(move)

    PRINTF("DXP_move_string=%s\n", with->DXP_move_string);

    return(DXP_MOVE);
  }
  else if (buffer[0] == DXP_GAMEEND)
  {
    int i = 1;

    HARDBUG(i >= nbuffer)

    PRINTF("reason=%c\n", buffer[i]);

    with->DXP_game_code = buffer[i];

    i++;

    HARDBUG(i >= nbuffer)
    PRINTF("stop code=%c\n", buffer[i]);

    return(DXP_GAMEEND);
  }
  else if (buffer[0] == DXP_CHAT)
  {
    return(DXP_CHAT);
  }
  else if (buffer[0] == DXP_BACKREQ)
  {
    FATAL("DXP_BACKREQ message not implemented", EXIT_FAILURE)
  }
  else if (buffer[0] == DXP_BACKACC)
  {
    FATAL("DXP_BACKACC message not implemented", EXIT_FAILURE)
  }
  else
  {
    FATAL("Unknown message type", EXIT_FAILURE)
  }
  return(INVALID);
}

//read from socket until \0
int reads(int sockfd, char buffer[MY_LINE_MAX], int *nbuffer)
{
  *nbuffer = 0;

  int i;

  while(TRUE)
  {
    i = 0;

    //read socket until '\0'

    while(TRUE)
    {
      int n;
      char c;
  
      if ((n = compat_socket_read(sockfd, &c, 1)) == INVALID)
        return(INVALID);
  
      if (n != 1)
      {
        PRINTF("n=%d\n", n);

        return(INVALID);
      }
  
      buffer[i++] = c;
  
      if (c == '\0') break;
    }

    if (buffer[0] == DXP_CHAT)
    {
      PRINTF("ignoring DXP_CHAT %s\n", buffer);
      continue;
    }

    break;
  }
  *nbuffer = i;
  return(i);
}

local char *secs2string(int t)
{
  local char string[MY_LINE_MAX];
  int hours;
  int minutes;
  int seconds;

  hours = t / 3600;
  minutes = (t - hours * 3600) / 60;
  seconds = t - hours * 3600 - minutes * 60;
  HARDBUG((hours * 3600 + minutes * 60 + seconds) != t)

  snprintf(string, MY_LINE_MAX, "%02d:%02d:%02d", hours, minutes, seconds);
  return(string);
}

local char *rtrim(char *trim)
{
  static char strim[MY_LINE_MAX];

  strcpy(strim, trim);

  int i;

  for (i = strlen(strim) - 1; i > 0; --i) if (!isspace(strim[i])) break;

  HARDBUG(i == 0)

  strim[i + 1] = '\0';

  return(strim);
}

local int play_game(dxp_t *with, int sockfd)
{
  int result = INVALID;

  state_t game_state;

  construct_state(&game_state);

  game_state.set_event(&game_state, event);

  if (IS_WHITE(with->DXP_game_colour))
  {
    game_state.set_white(&game_state, rtrim(my_name));
    game_state.set_black(&game_state, rtrim(your_name));
  }
  else
  {
    game_state.set_white(&game_state, rtrim(your_name));
    game_state.set_black(&game_state, rtrim(my_name));
  }

  BSTRING(bfen)

  board2fen(with->DXP_search.S_board.board_colour2move,
    with->DXP_search.S_board.board_white_man_bb,
    with->DXP_search.S_board.board_black_man_bb,
    with->DXP_search.S_board.board_white_king_bb,
    with->DXP_search.S_board.board_black_king_bb,
    bfen, FALSE);

  game_state.set_starting_position(&game_state, bdata(bfen));

  BDESTROY(bfen)

  if (IS_BLACK(with->DXP_search.S_board.board_colour2move))
    game_state.push_move(&game_state, "...", NULL);

  int nbuffer;
  char buffer[MY_LINE_MAX];

  with->DXP_game_time_used = 0; 
  with->DXP_move_number = 0;

  time_control_t time_control;

  configure_time_control(with->DXP_game_time,
    with->DXP_game_moves, &time_control);

  while(TRUE)
  {
    print_board(&(with->DXP_search.S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(with->DXP_search.S_board), &moves_list, FALSE);

    if (with->DXP_search.S_board.board_colour2move != with->DXP_game_colour)
    {
      if (moves_list.nmoves == 0)
      {
        PRINTF("waiting for DXP_GAMEEND..\n");

        if (reads(sockfd, buffer, &nbuffer) == INVALID) break;
  
        char type = decode(nbuffer, buffer, with);
    
        HARDBUG(type != DXP_GAMEEND)

        if (options.dxp_strict_gameend)
        {
          encode(DXP_GAMEEND, with, buffer, &nbuffer);

          PRINTF("sending DXP_GAMEEND..\n");
  
          int nwrite;

          if ((nwrite = compat_socket_write(sockfd, buffer, nbuffer)) ==
              INVALID) break;
        }

        if (IS_WHITE(with->DXP_search.S_board.board_colour2move))
          result = 0;
        else
          result = 2;
 
        break;
      }

      char comment[MY_LINE_MAX];
 
      strcpy(comment, "NULL");

      if (options.ponder)
      {
        //configure thread for pondering

        options.time_limit = 10000.0;
        options.time_ntrouble = 0;
  
        PRINTF("\nPondering..\n");
  
        enqueue(return_thread_queue(thread_alpha_beta_master),
          MESSAGE_STATE, game_state.get_state(&game_state));

        enqueue(return_thread_queue(thread_alpha_beta_master),
          MESSAGE_GO, "dxp/play_game");
      }

      PRINTF("waiting for DXP_MOVE or DXP_GAMEEND..\n");

      int nread = reads(sockfd, buffer, &nbuffer);

      if (options.ponder)
      {
        //cancel pondering and dequeue messages

        enqueue(return_thread_queue(thread_alpha_beta_master),
          MESSAGE_ABORT_SEARCH, "main/play_game");
  
        message_t message;
         
        while(TRUE)
        {
          if (dequeue(&main_queue, &message) != INVALID)
          {
            if (message.message_id == MESSAGE_INFO)
            {
              //PRINTF("got info %s\n", bdata(message.message_text));
            }
            else if (message.message_id == MESSAGE_RESULT)
            {
              PRINTF("got hint %s\n", bdata(message.message_text));
              break;
            }
            else
              FATAL("message.message_id error", EXIT_FAILURE)
          }
          compat_sleep(CENTI_SECOND);
        }
        if (options.dxp_annotate_level > 1)
          sprintf(comment, "%s", bdata(message.message_text));
      }

      if (nread == INVALID) break;

      char type = decode(nbuffer, buffer, with);
  
      if (type == DXP_GAMEEND)
      {
        if (options.dxp_strict_gameend)
        {
          encode(DXP_GAMEEND, with, buffer, &nbuffer);

          PRINTF("sending DXP_GAMEEND..\n");

          int nwrite;

          if ((nwrite = compat_socket_write(sockfd, buffer, nbuffer)) ==
              INVALID) break;
        }
        if (with->DXP_game_code == DXP_IGIVEUP)
        {
          if (IS_WHITE(with->DXP_search.S_board.board_colour2move))
            result = 0;
          else
            result = 2;
        }
        else if (with->DXP_game_code == DXP_DRAW)
        {
          result = 1;
        }
        else if (with->DXP_game_code == DXP_IWIN)
        {
          if (IS_WHITE(with->DXP_search.S_board.board_colour2move))
            result = 2;
          else
            result = 0;
        }
        else
        {
          result = INVALID;
        }

        break;
      }
    
      HARDBUG(type != DXP_MOVE)
  
      int imove;

      BSTRING(move)

      HARDBUG(bassigncstr(move, with->DXP_move_string) != BSTR_OK)

      HARDBUG((imove = search_move(&moves_list, move)) == INVALID)

      BDESTROY(move)

      if (compat_strcasecmp(comment, "NULL") == 0)
        game_state.push_move(&game_state, with->DXP_move_string, NULL);
      else
        game_state.push_move(&game_state, with->DXP_move_string, comment);

      do_move(&(with->DXP_search.S_board), imove, &moves_list);
    }
    else
    {  
      //my turn

      if (moves_list.nmoves == 0)
      {
        PRINTF("DXP game lost (nmoves == 0)\n");
        with->DXP_game_code = DXP_IGIVEUP;
  
        encode(DXP_GAMEEND, with, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");

        int nwrite;

        if ((nwrite = compat_socket_write(sockfd, buffer, nbuffer)) ==
            INVALID) break;
  
        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(sockfd, buffer, &nbuffer) == INVALID) break;
  
          char type = decode(nbuffer, buffer, with);
      
          HARDBUG(type != DXP_GAMEEND)
        }
  
        if (IS_WHITE(with->DXP_search.S_board.board_colour2move))
          result = 0;
        else
          result = 2;

        break;
      }

      //check for known endgame

      int egtb_mate = read_endgame(&(with->DXP_search),
                                   with->DXP_search.S_board.board_colour2move,
                                   TRUE);
    
      if (egtb_mate != ENDGAME_UNKNOWN)
      {
        PRINTF("known endgame egtb_mate=%d\n", egtb_mate);
  
        if (egtb_mate == INVALID)
        {
          PRINTF("DXP game draw (egtb_mate=%d)\n", egtb_mate);

          with->DXP_game_code = DXP_DRAW;

          result = 1;
        }
        else if (egtb_mate > 0)
        {
          PRINTF("DXP game won (egtb_mate=%d)\n", egtb_mate);

          with->DXP_game_code = DXP_IWIN;

          if (IS_WHITE(with->DXP_search.S_board.board_colour2move))
            result = 2;
          else
            result = 0;
        }
        else
        {
          PRINTF("DXP game lost (egtb_mate=%d)\n", egtb_mate);

          with->DXP_game_code = DXP_IGIVEUP;

          if (IS_WHITE(with->DXP_search.S_board.board_colour2move))
            result = 0;
          else
            result = 2;
        }

        encode(DXP_GAMEEND, with, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");
  
        int nwrite;

        if ((nwrite = compat_socket_write(sockfd, buffer, nbuffer)) ==
            INVALID) break;
  
        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(sockfd, buffer, &nbuffer) == INVALID) break;
    
          char type = decode(nbuffer, buffer, with);
      
          HARDBUG(type != DXP_GAMEEND)
        }
  
        break;
      }

      if (with->DXP_move_number >=
          (with->DXP_game_moves + MARGIN))
      {
        PRINTF("moves exceeded\n");

        with->DXP_game_code = DXP_UNKNOWN;

        encode(DXP_GAMEEND, with, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");
  
        int nwrite;

        if ((nwrite = compat_socket_write(sockfd, buffer, nbuffer)) ==
            INVALID) break;
  
        HARDBUG(nwrite != nbuffer)

        result = INVALID;

        break;
      }

      BSTRING(bbest_move)

      HARDBUG(bassigncstr(bbest_move, "NULL") != BSTR_OK)

      int best_score;
      int best_depth;

      BSTRING(bcomment)

      //check for book

      if (options.dxp_book)
        return_book_move(&(with->DXP_search.S_board), &moves_list, bbest_move);
  
      if (compat_strcasecmp(bdata(bbest_move), "NULL") != 0)
      {
        HARDBUG(bassigncstr(bcomment, "book") != BSTR_OK)

        PRINTF("\nbook_move=%s\n", bdata(bbest_move));

        best_score = 0;

        best_depth = 0;
      }
      else
      {
        PRINTF("\nthinking..\n");

        double t1 = compat_time();
  
        if (moves_list.nmoves == 1)
        {
          move2bstring(&moves_list, 0, bbest_move);
  
          HARDBUG(bassigncstr(bcomment, "only move") != BSTR_OK)

          best_score = 0;
  
          best_depth = 0;
        } 
        else
        {
          //configure thread for search
  
          set_time_limit(with->DXP_move_number, &time_control);
  
          enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_STATE, game_state.get_state(&game_state));
  
          enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_GO, "dxp/play_game");
    
          while(TRUE)
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
                PRINTF("got result %s\n", bdata(message.message_text));
    
                CSTRING(cbest_move, blength(message.message_text))
  
                HARDBUG(sscanf(bdata(message.message_text), "%s%d%d",
                               cbest_move, &best_score, &best_depth) != 3)
   
                HARDBUG(bassigncstr(bbest_move, cbest_move) != BSTR_OK)
  
                CDESTROY(cbest_move)

                HARDBUG(bassign(bcomment, message.message_text) != BSTR_OK)
  
                break;
              }
              else
                FATAL("message.message_id error", EXIT_FAILURE)
            } 
            compat_sleep(CENTI_SECOND);
          }
        }

        double t2 = compat_time();
  
        double move_time = t2 - t1;
  
        update_time_control(with->DXP_move_number, move_time,
          &time_control);
  
        with->DXP_game_time_used += move_time;
  
        with->DXP_move_number++;
  
        PRINTF("used   %s\n", secs2string(move_time));
  
        PRINTF("total  %s\n", secs2string(with->DXP_game_time_used));
  
        PRINTF("remain %s\n", secs2string(with->DXP_game_time -
                                          with->DXP_game_time_used));
  
        PRINTF("\n* * * * * * * * * * %s %d %d\n\n",
          bdata(bbest_move), best_score, best_depth);
      }

      strcpy(with->DXP_move_string, bdata(bbest_move));
  
      if (options.dxp_annotate_level == 0)
        game_state.push_move(&game_state, with->DXP_move_string, NULL);
      else
        game_state.push_move(&game_state, with->DXP_move_string,
                             bdata(bcomment));
  
      encode(DXP_MOVE, with, buffer, &nbuffer);

      PRINTF("sending DXP_MOVE..\n");
  
      int nwrite;

      if ((nwrite = compat_socket_write(sockfd, buffer, nbuffer)) ==
          INVALID) break;

      HARDBUG(nwrite != nbuffer)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bbest_move)) == INVALID)

      do_move(&(with->DXP_search.S_board), imove, &moves_list);

      BDESTROY(bcomment)

      BDESTROY(bbest_move)
    }
  }

  char game_result[MY_LINE_MAX];

  strcpy(game_result, "*");

  if (result == 2)
  {
    strcpy(game_result, "2-0");
  }
  else if (result == 1)
  {
    strcpy(game_result, "1-1");
  }
  else if (result == 0)
  {
    strcpy(game_result, "0-2");
  }

  PRINTF("game_result=%s\n", game_result);

  game_state.set_result(&game_state, game_result);
 
  game_state.save2pdn(&game_state, "dxp.pdn");

  destroy_state(&game_state);

  return(result);
}

local void set_my_name(void)
{
  if (compat_strcasecmp(options.dxp_tag, "NULL") == 0)
    snprintf(my_name, MY_LINE_MAX, "GWD %s %s", REVISION,
             options.network_name);
  else
    snprintf(my_name, MY_LINE_MAX, "GWD %s %s %s", REVISION, options.dxp_tag,
             options.network_name);

  for (int j = strlen(my_name); j < 32; j++) my_name[j] = ' ';

  my_name[32] = '\0';

  PRINTF("my_name=%s\n", my_name);
}

local void dxp_game_initiator(int arg_fd)
{
  set_my_name();

  dxp_t dxp;

  dxp_t *with = &dxp;

  construct_search(&(with->DXP_search), STDOUT, NULL);

  int nwon = 0;
  int ndraw = 0;
  int nlost = 0;
  int nunknown = 0;

  FILE *fballot = NULL;

  if (fexists(options.dxp_ballot))
  {
    HARDBUG((fballot = fopen(options.dxp_ballot, "r")) == NULL)

    PRINTF("Reading ballots from file %s\n", options.dxp_ballot);
  }

  int igame = 0;
  
  while(TRUE)
  {
    //either select the starting position
    //or a fen position

    char fen[MY_LINE_MAX];
    char opening[MY_LINE_MAX];

    strcpy(fen, "NULL");
    strcpy(opening, "");

    if (fballot != NULL)
    {
      while(TRUE)
      {
        char line[MY_LINE_MAX];

        if (fgets(line, MY_LINE_MAX, fballot) == NULL) goto label_break;

        if (sscanf(line, "%[^\n]", fen) == 0)
        {
          PRINTF("skipping empty line..\n");

          continue;
        }

        if (strncmp(fen, "//", 2) == 0)
        {
          HARDBUG(sscanf(fen, "//%[^\n]", opening) != 1)

          PRINTF("opening=%s\n", opening);

          continue;
        }

        break;
      }
      PRINTF("Ballot fen is: %s\n", fen);
    }
    else
    { 
      strncpy(fen, STARTING_POSITION2FEN, MY_LINE_MAX);

      strcpy(opening, "(none)");
    }

    for (int icolour = 0; icolour < 2; icolour++)
    {
      ++igame;
      if (igame > options.dxp_games) goto label_break;

      snprintf(event, MY_LINE_MAX, "Game %d, Opening %s", igame, rtrim(opening));

      PRINTF("event=%s\n", event);

      fen2board(with, fen);

      PRINTF("The starting position for game %d is:\n", igame);

      print_board(&(with->DXP_search.S_board));

      if (icolour == 0)
        with->DXP_game_colour = WHITE_BIT;
      else
        with->DXP_game_colour = BLACK_BIT;

      PRINTF("sending DXP_GAMEREQ..\n");
  
      int nbuffer;
      char buffer[MY_LINE_MAX];

      encode(DXP_GAMEREQ, with, buffer, &nbuffer);

      int nwrite;

      if ((nwrite = compat_socket_write(arg_fd, buffer, nbuffer)) ==
          INVALID) break;

      HARDBUG(nwrite != nbuffer)
  
      PRINTF("waiting for DXP_GAMEACC..\n");
  
      while(TRUE)
      {
        if (reads(arg_fd, buffer, &nbuffer) == INVALID) 
          goto label_break;
  
        char type = decode(nbuffer, buffer, with);
  
        HARDBUG(type != DXP_GAMEACC)

        break;
      }
  
      int result = play_game(with, arg_fd);
  
      if (result == 2)
      {
         if (IS_WHITE(with->DXP_game_colour))
           ++nwon;
         else
           ++nlost;
      }
      else if (result == 1)
      {
        ++ndraw;
      } 
      else if (result == 0)
      {
         if (IS_WHITE(with->DXP_game_colour))
           ++nlost;
         else
           ++nwon;
      }
      else
        ++nunknown;
  
      PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
        nwon, ndraw, nlost, nunknown);
  
      if (IS_WHITE(with->DXP_game_colour))
        with->DXP_game_colour = BLACK_BIT;
      else
        with->DXP_game_colour = WHITE_BIT;
    }
  }

  label_break:

  PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
    nwon, ndraw, nlost, nunknown);

  results2csv(nwon, ndraw, nlost, nunknown);
}

local void dxp_game_follower(int arg_fd)
{
  set_my_name();

  dxp_t dxp;

  dxp_t *with = &dxp;

  construct_search(&(with->DXP_search), STDOUT, NULL);

  int nwon = 0;
  int ndraw = 0;
  int nlost = 0;
  int nunknown = 0;

  int igame = 0;

  while(TRUE)
  {
    PRINTF("waiting for DXP_GAMEREQ..\n");

    int nbuffer;
    char buffer[MY_LINE_MAX];

    if (reads(arg_fd, buffer, &nbuffer) == INVALID) break;

    char type = decode(nbuffer, buffer, with);

    HARDBUG(type != DXP_GAMEREQ)

    PRINTF("received DXP_GAMEREQ\n");

    encode(DXP_GAMEACC, with, buffer, &nbuffer);

    PRINTF("sending DXP_GAMEACC..\n");

    int nwrite;

    if ((nwrite = compat_socket_write(arg_fd, buffer, nbuffer)) == INVALID) break;

    HARDBUG(nwrite != nbuffer)

    ++igame;

    snprintf(event, MY_LINE_MAX, "Game %d", igame);

    int result = play_game(with, arg_fd);

    if (result == 2)
    {
      if (IS_WHITE(with->DXP_game_colour))
        ++nwon;
      else
        ++nlost;
    }
    else if (result == 1)
    {
      ++ndraw;
    } 
    else if (result == 0)
    {
      if (IS_WHITE(with->DXP_game_colour))
        ++nlost;
      else
        ++nwon;
    }
    else
      ++nunknown;

    PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
      nwon, ndraw, nlost, nunknown);
  }

  PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
    nwon, ndraw, nlost, nunknown);

  results2csv(nwon, ndraw, nlost, nunknown);
}

void dxp_server(void)
{
  int sockfd;

  HARDBUG(compat_socket_startup() != 0)

  HARDBUG((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID)

  struct sockaddr_in server_address;

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(options.dxp_port);

  HARDBUG((bind(sockfd, (SA *) &server_address,
            sizeof(server_address))) == INVALID)

  HARDBUG((listen(sockfd, 5)) == INVALID)

  struct sockaddr_in client_address;
  int len = sizeof(client_address);

  int connfd;

  PRINTF("server waiting for connection..\n");

  HARDBUG((connfd = accept(sockfd, (SA *) &client_address,
                       (socklen_t *) &len)) == INVALID)

  HARDBUG(len > sizeof(client_address))

  PRINTF("server accepted connection..\n");
 
  if (options.dxp_initiator) 
    dxp_game_initiator(connfd);
  else
    dxp_game_follower(connfd);

  HARDBUG(compat_socket_close(sockfd) == INVALID)

  HARDBUG(compat_socket_cleanup() != 0)
}

void dxp_client(void)
{
  int sockfd;

  HARDBUG(compat_socket_startup() != 0)

  HARDBUG((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == INVALID)

  struct sockaddr_in server_address;

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr(options.dxp_server_ip);
  server_address.sin_port = htons(options.dxp_port);

  HARDBUG(connect(sockfd, (SA *) &server_address,
              sizeof(server_address)) == INVALID)

  if (options.dxp_initiator) 
    dxp_game_initiator(sockfd);
  else
    dxp_game_follower(sockfd);

  HARDBUG(compat_socket_close(sockfd) == INVALID)

  HARDBUG(compat_socket_cleanup() != 0)
}

