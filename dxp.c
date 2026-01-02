//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

#define SA struct sockaddr

#define DXP_GAMEREQ 'R'
#define DXP_GAMEACC 'A'
#define DXP_MOVE    'M'
#define DXP_GAMEEND 'E'
#define DXP_CHAT    'C'
#define DXP_BACKREQ '?'
#define DXP_BACKACC '?'

#define DXP_UNKNOWN '0'
#define DXP_IGIVEUP '1'
#define DXP_DRAW    '2'
#define DXP_IWIN    '3'

#define MARGIN 5

typedef struct
{
  search_t DXP_search;

  colour_enum DXP_game_colour;
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

local void encode(char arg_type, dxp_t *object,
  char arg_buffer[MY_LINE_MAX], int *arg_nbuffer)
{
  *arg_nbuffer = 0;

  if (arg_type == DXP_GAMEREQ)
  {
    int i = 0;

    arg_buffer[i] = DXP_GAMEREQ;
    i++;

    strcpy(arg_buffer + i, "01");
    i += 2;

    //                  01234567890123456789012345678901

    strncpy(arg_buffer + i, my_name, 32);

    i += 32;

    if (IS_WHITE(object->DXP_game_colour))
    {
      arg_buffer[i] = 'Z';
    }
    else
    {
      arg_buffer[i] = 'W';
    }
    i++;

    char string[MY_LINE_MAX];

    snprintf(string, MY_LINE_MAX, "%03d", options.dxp_game_time);

    strcpy(arg_buffer + i, string); 
    i += 3;

    //dependency

    object->DXP_game_time = options.dxp_game_time * 60;

    snprintf(string, MY_LINE_MAX, "%03d", options.dxp_game_moves);

    object->DXP_game_moves = options.dxp_game_moves;

    strcpy(arg_buffer + i, string); 
    i += 3;

    strcpy(arg_buffer + i, "B");
    i++;

    if (IS_WHITE(object->DXP_search.S_board.B_colour2move))
    {
      strcpy(arg_buffer + i, "W");
      i++;
    }
    else
    {
      strcpy(arg_buffer + i, "Z");
      i++;
    }

    for (int j = 1; j <= 50; ++j)
    {
      int jfield = square2field[j];

      if (object->DXP_search.S_board.B_white_man_bb & BITULL(jfield))
      {
        strcpy(arg_buffer + i, "w");
        i++;
      }
      else if (object->DXP_search.S_board.B_white_king_bb & BITULL(jfield))
      {
        strcpy(arg_buffer + i, "W");
        i++;
      } 
      else if (object->DXP_search.S_board.B_black_man_bb & BITULL(jfield))
      {
        strcpy(arg_buffer + i, "z");
        i++;
      }
      else if (object->DXP_search.S_board.B_black_king_bb & BITULL(jfield))
      {
        strcpy(arg_buffer + i, "Z");
        i++;
      } 
      else
      {
        strcpy(arg_buffer + i, "e");
        i++;
      }  
    }

    arg_buffer[i] = '\0';
    i++;

    *arg_nbuffer = i;

    PRINTF("DXP_GAMEREQ buffer=%s nbuffer=%d\n", arg_buffer, *arg_nbuffer);
  } 
  else if (arg_type == DXP_GAMEACC)
  {
    int i = 0;

    arg_buffer[i] = DXP_GAMEACC;
    i++;

    //                  01234567890123456789012345678901
    strncpy(arg_buffer + i, my_name, 32);
    i += 32;

    arg_buffer[i] = '0';
    i++;

    arg_buffer[i] = '\0';
    i++;

    *arg_nbuffer = i;

    PRINTF("DXP_GAMEACC buffer=%s nbuffer=%d\n", arg_buffer, *arg_nbuffer);
  }
  else if (arg_type == DXP_MOVE)
  {
    int i = 0;

    arg_buffer[i] = DXP_MOVE;
    i++;

    char string[MY_LINE_MAX];

    snprintf(string, MY_LINE_MAX, "%04d", object->DXP_game_time_used);

    strcpy(arg_buffer + i, string); 
    i += 4;

    //ugly

    int j = 0;

    strncpy(arg_buffer + i, object->DXP_move_string + j, 2);
    j += 3;
    i += 2;

    strncpy(arg_buffer + i, object->DXP_move_string + j, 2);
    j += 2;
    i += 2;
    
    int n = 0;
    int k = j;
    while (object->DXP_move_string[k] == 'x')
    { 
      n++;
      k += 3;
    }

    snprintf(string, MY_LINE_MAX, "%02d", n);
    strcpy(arg_buffer + i, string); 
    i += 2;

    k = j;
    while (object->DXP_move_string[k] == 'x')
    { 
      k++;
      strncpy(arg_buffer + i, object->DXP_move_string + k, 2);
      k += 2;
      i += 2;
    }

    arg_buffer[i] = '\0';
    i++;

    *arg_nbuffer = i;

    PRINTF("DXP_MOVE buffer=%s nbuffer=%d\n", arg_buffer, *arg_nbuffer);
  }
  else if (arg_type == DXP_GAMEEND)
  {
    int i = 0;

    arg_buffer[i] = DXP_GAMEEND;
    i++;
  
    arg_buffer[i] = object->DXP_game_code;
    i++;

    arg_buffer[i] = '0';
    i++;

    arg_buffer[i] = '\0';
    i++;

    *arg_nbuffer = i;

    PRINTF("DXP_GAMEEND buffer=%s nbuffer=%d\n", arg_buffer, *arg_nbuffer);
  }
  else if (arg_type == DXP_CHAT)
  { 
    FATAL("DXP_CHAT not implemented", EXIT_FAILURE)
  }
  else if (arg_type == DXP_BACKREQ)
  {
    FATAL("DXP_BACKREQ not implemented", EXIT_FAILURE)
  }
  else if (arg_type == DXP_BACKACC)
  {
    FATAL("DXP_BACKACC not implemented", EXIT_FAILURE)
  }
  else
  {
    FATAL("Unknown message type", EXIT_FAILURE)
  }
}

char decode(int arg_nhuffer, char *arg_buffer, dxp_t *object)
{
  HARDBUG(arg_nhuffer < 1)

  //buffer[nbuffer] = '\0';
  PRINTF("decode nbuffer=%d buffer=%s\n", arg_nhuffer, arg_buffer);

  if (arg_buffer[0] == DXP_GAMEREQ)
  {
    //ignore version

    int i = 3;
 
    char string[MY_LINE_MAX];

    HARDBUG((i + 31) >= arg_nhuffer)

    strncpy(your_name, arg_buffer + i, 32);

    your_name[32] = '\0';

    PRINTF("your_name=%s\n", your_name);

    i += 32;

    HARDBUG(i >= arg_nhuffer)

    char game_colour = arg_buffer[i];
    i++;

    PRINTF("game_colour=%c\n", game_colour);

    if (game_colour == 'W')
      object->DXP_game_colour = WHITE_ENUM;
    else
      object->DXP_game_colour = BLACK_ENUM;

    int game_time;
 
    HARDBUG((i + 2) >= arg_nhuffer)
    HARDBUG(my_sscanf(arg_buffer + i, "%3d", &game_time) != 1)
    i += 3;

    PRINTF("game_time=%d\n", game_time);

    object->DXP_game_time = game_time * 60;

    int game_moves;

    HARDBUG((i + 2) >= arg_nhuffer)
    HARDBUG(my_sscanf(arg_buffer + i, "%3d", &game_moves) != 1)
    i += 3;

    PRINTF("game_moves=%d\n", game_moves);

    object->DXP_game_moves = game_moves;
 
    HARDBUG(i >= arg_nhuffer)
    if (arg_buffer[i] == 'A')
    {
      PRINTF("starting position\n");

      string2board(&(object->DXP_search.S_board), STARTING_POSITION);
    }
    else if (arg_buffer[i] == 'B')
    {
      i++;
      HARDBUG(i >= arg_nhuffer)

      if (arg_buffer[i] == 'W')
        string[0] = 'w';
      else if (arg_buffer[i] == 'Z')
        string[0] = 'b';
      else
        FATAL("error in B/colour2move", EXIT_FAILURE)
   
      for (int j = 1; j <= 50; j++)
      {
        i++;
        HARDBUG(i >= arg_nhuffer)
  
        if (arg_buffer[i] == 'e')
          string[j] = *nn;
        else if (arg_buffer[i] == 'w')
          string[j] = *wO;
        else if (arg_buffer[i] == 'z')
          string[j] = *bO;
        else if (arg_buffer[i] == 'W')
          string[j] = *wX;
        else if (arg_buffer[i] == 'Z')
          string[j] = *bX;
        else
          FATAL("error in B/position", EXIT_FAILURE)
      }

      string[51] = '\0';

      PRINTF("string=%s\n", string);

      string2board(&(object->DXP_search.S_board), string);

      PRINTF("starting position\n");

      print_board(&(object->DXP_search.S_board));
    }
    else
      FATAL("error in DXP", EXIT_FAILURE)

    return(DXP_GAMEREQ);
  } 
  else if (arg_buffer[0] == DXP_GAMEACC)
  {
    int i = 1;

    HARDBUG((i + 31) >= arg_nhuffer)

    strncpy(your_name, arg_buffer + i, 32);

    your_name[32] = '\0';

    PRINTF("your_name=%s\n", your_name);

    i += 32;

    HARDBUG(i >= arg_nhuffer)

    char code = arg_buffer[i];

    PRINTF("acceptance code=%c\n", code);

    return(DXP_GAMEACC);
  }
  else if (arg_buffer[0] == DXP_MOVE)
  {
    //ignore time
    
    int i = 5;

    int f;
    HARDBUG(i >= arg_nhuffer)
    HARDBUG(my_sscanf(arg_buffer + i, "%2d", &f) != 1)
    HARDBUG(f < 1)
    HARDBUG(f > 50)
    PRINTF("f=%d\n", f);
    f = square2field[f];

    i += 2;

    int t;
    HARDBUG(i >= arg_nhuffer)
    HARDBUG(my_sscanf(arg_buffer + i, "%2d", &t) != 1)
    HARDBUG(t < 1)
    HARDBUG(t > 50)
    PRINTF("t=%d\n", t);
    t = square2field[t];
    i += 2;

    int n;
    HARDBUG(i >= arg_nhuffer)
    HARDBUG(my_sscanf(arg_buffer + i, "%2d", &n) != 1)
    HARDBUG(n > NPIECES_MAX)
    PRINTF("n=%d\n", n);
    i += 2;

    int c[NPIECES_MAX];
    for (int j = 0; j < n; j++)
    { 
      HARDBUG(i >= arg_nhuffer)
      HARDBUG(my_sscanf(arg_buffer + i, "%2d", c + j) != 1)
      HARDBUG(c[j] < 1)
      HARDBUG(c[j] > 50) 
      PRINTF("j=%d c[j]=%d\n", j, c[j]);
      c[j] = square2field[c[j]];
      i += 2;
    }

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(object->DXP_search.S_board), &moves_list);

    HARDBUG(moves_list.ML_nmoves == 0)

    HARDBUG(n != moves_list.ML_ncaptx)

    int jmove = INVALID;

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      move_t *move = moves_list.ML_moves + imove;

      int ifield = move->M_from;
      int kfield = move->M_move_to;
      ui64_t captures_bb = move->M_captures_bb;

      if (ifield != f) continue;
      if (kfield != t) continue;

      if (captures_bb == 0)
      {
        jmove = imove;
      }
      else
      {
        int m = 0;

        while(captures_bb != 0)
        {
          int jfield = BIT_CTZ(captures_bb);

          m = 0;
          for (int j = 0; j < n; j++)
            if (c[j] == jfield) m++;

          if (m == 0) break;

          HARDBUG(m != 1)

          captures_bb &= ~BITULL(jfield);
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

    strcpy(object->DXP_move_string, bdata(move));

    BDESTROY(move)

    PRINTF("DXP_move_string=%s\n", object->DXP_move_string);

    return(DXP_MOVE);
  }
  else if (arg_buffer[0] == DXP_GAMEEND)
  {
    int i = 1;

    HARDBUG(i >= arg_nhuffer)

    PRINTF("reason=%c\n", arg_buffer[i]);

    object->DXP_game_code = arg_buffer[i];

    i++;

    HARDBUG(i >= arg_nhuffer)
    PRINTF("stop code=%c\n", arg_buffer[i]);

    return(DXP_GAMEEND);
  }
  else if (arg_buffer[0] == DXP_CHAT)
  {
    return(DXP_CHAT);
  }
  else if (arg_buffer[0] == DXP_BACKREQ)
  {
    FATAL("DXP_BACKREQ message not implemented", EXIT_FAILURE)
  }
  else if (arg_buffer[0] == DXP_BACKACC)
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
int reads(int arg_sockfd, char arg_buffer[MY_LINE_MAX], int *arg_nbuffer)
{
  *arg_nbuffer = 0;

  int i;

  while(TRUE)
  {
    i = 0;

    //read socket until '\0'

    while(TRUE)
    {
      int n;
      char c;
  
      if ((n = compat_socket_read(arg_sockfd, &c, 1)) == INVALID)
        return(INVALID);
  
      if (n != 1)
      {
        PRINTF("n=%d\n", n);

        return(INVALID);
      }
  
      arg_buffer[i++] = c;
  
      if (c == '\0') break;
    }

    if (arg_buffer[0] == DXP_CHAT)
    {
      PRINTF("ignoring DXP_CHAT %s\n", arg_buffer);
      continue;
    }

    break;
  }
  *arg_nbuffer = i;
  return(i);
}

local char *secs2string(int arg_t)
{
  local char string[MY_LINE_MAX];
  int hours;
  int minutes;
  int seconds;

  hours = arg_t / 3600;
  minutes = (arg_t - hours * 3600) / 60;
  seconds = arg_t - hours * 3600 - minutes * 60;
  HARDBUG((hours * 3600 + minutes * 60 + seconds) != arg_t)

  snprintf(string, MY_LINE_MAX, "%02d:%02d:%02d", hours, minutes, seconds);
  return(string);
}

local char *rtrim(char *arg_trim)
{
  static char strim[MY_LINE_MAX];

  strcpy(strim, arg_trim);

  int i;

  for (i = strlen(strim) - 1; i > 0; --i) if (!isspace(strim[i])) break;

  HARDBUG(i == 0)

  strim[i + 1] = '\0';

  return(strim);
}

local int play_game(dxp_t *object, int arg_sockfd)
{
  int result = INVALID;

  game_state_t game_state;

  construct_game_state(&game_state);

  game_state.set_event(&game_state, event);

  if (IS_WHITE(object->DXP_game_colour))
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

  board2fen(&(object->DXP_search.S_board), bfen, FALSE);

  game_state.set_starting_position(&game_state, bdata(bfen));

  BDESTROY(bfen)

  if (IS_BLACK(object->DXP_search.S_board.B_colour2move))
    game_state.push_move(&game_state, "...", NULL);

  int nbuffer;
  char buffer[MY_LINE_MAX];

  object->DXP_game_time_used = 0; 
  object->DXP_move_number = 0;

  time_control_t time_control;

  configure_time_control(object->DXP_game_time,
    object->DXP_game_moves, &time_control);

  int score_min = SCORE_PLUS_INFINITY;
  int score_max = SCORE_MINUS_INFINITY;

  while(TRUE)
  {
    print_board(&(object->DXP_search.S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(object->DXP_search.S_board), &moves_list);

    if (object->DXP_search.S_board.B_colour2move != object->DXP_game_colour)
    {
      if (moves_list.ML_nmoves == 0)
      {
        PRINTF("waiting for DXP_GAMEEND..\n");

        if (reads(arg_sockfd, buffer, &nbuffer) == INVALID) break;
  
        char type = decode(nbuffer, buffer, object);
    
        HARDBUG(type != DXP_GAMEEND)

        if (options.dxp_strict_gameend)
        {
          encode(DXP_GAMEEND, object, buffer, &nbuffer);

          PRINTF("sending DXP_GAMEEND..\n");
  
          if (compat_socket_write(arg_sockfd, buffer, nbuffer) == INVALID)
            break;
        }

        if (IS_WHITE(object->DXP_search.S_board.B_colour2move))
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
          MESSAGE_STATE, game_state.get_game_state(&game_state));

        enqueue(return_thread_queue(thread_alpha_beta_master),
          MESSAGE_GO, "dxp/play_game");
      }

      PRINTF("waiting for DXP_MOVE or DXP_GAMEEND..\n");

      int nread = reads(arg_sockfd, buffer, &nbuffer);

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

      char type = decode(nbuffer, buffer, object);
  
      if (type == DXP_GAMEEND)
      {
        if (options.dxp_strict_gameend)
        {
          encode(DXP_GAMEEND, object, buffer, &nbuffer);

          PRINTF("sending DXP_GAMEEND..\n");

          if (compat_socket_write(arg_sockfd, buffer, nbuffer) == INVALID)
            break;
        }
        if (object->DXP_game_code == DXP_IGIVEUP)
        {
          if (IS_WHITE(object->DXP_search.S_board.B_colour2move))
            result = 0;
          else
            result = 2;
        }
        else if (object->DXP_game_code == DXP_DRAW)
        {
          result = 1;

          PRINTF("DRAW score_min=%d score_max=%d\n",
                 score_min, score_max);
        }
        else if (object->DXP_game_code == DXP_IWIN)
        {
          if (IS_WHITE(object->DXP_search.S_board.B_colour2move))
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

      HARDBUG(bassigncstr(move, object->DXP_move_string) == BSTR_ERR)

      HARDBUG((imove = search_move(&moves_list, move)) == INVALID)

      BDESTROY(move)

      if (compat_strcasecmp(comment, "NULL") == 0)
        game_state.push_move(&game_state, object->DXP_move_string, NULL);
      else
        game_state.push_move(&game_state, object->DXP_move_string, comment);

      do_move(&(object->DXP_search.S_board), imove, &moves_list, FALSE);
    }
    else
    {  
      //my turn

      if (moves_list.ML_nmoves == 0)
      {
        PRINTF("DXP game lost (nmoves == 0)\n");
        object->DXP_game_code = DXP_IGIVEUP;
  
        encode(DXP_GAMEEND, object, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");

        int nwrite;

        if ((nwrite = compat_socket_write(arg_sockfd, buffer, nbuffer)) ==
            INVALID) break;
  
        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(arg_sockfd, buffer, &nbuffer) == INVALID) break;
  
          char type = decode(nbuffer, buffer, object);
      
          HARDBUG(type != DXP_GAMEEND)
        }
  
        if (IS_WHITE(object->DXP_search.S_board.B_colour2move))
          result = 0;
        else
          result = 2;

        break;
      }

      //check for known endgame

      int egtb_mate = read_endgame(&(object->DXP_search),
                                   object->DXP_search.S_board.B_colour2move,
                                   NULL);
    
      if (egtb_mate != ENDGAME_UNKNOWN)
      {
        PRINTF("known endgame egtb_mate=%d\n", egtb_mate);
  
        if (egtb_mate == INVALID)
        {
          PRINTF("DXP game draw (egtb_mate=%d)\n", egtb_mate);

          object->DXP_game_code = DXP_DRAW;

          result = 1;

          PRINTF("DRAW score_min=%d score_max=%d\n",
                 score_min, score_max);
        }
        else if (egtb_mate > 0)
        {
          PRINTF("DXP game won (egtb_mate=%d)\n", egtb_mate);

          object->DXP_game_code = DXP_IWIN;

          if (IS_WHITE(object->DXP_search.S_board.B_colour2move))
            result = 2;
          else
            result = 0;
        }
        else
        {
          PRINTF("DXP game lost (egtb_mate=%d)\n", egtb_mate);

          object->DXP_game_code = DXP_IGIVEUP;

          if (IS_WHITE(object->DXP_search.S_board.B_colour2move))
            result = 0;
          else
            result = 2;
        }

        encode(DXP_GAMEEND, object, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");
  
        int nwrite;

        if ((nwrite = compat_socket_write(arg_sockfd, buffer, nbuffer)) ==
            INVALID) break;
  
        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(arg_sockfd, buffer, &nbuffer) == INVALID) break;
    
          char type = decode(nbuffer, buffer, object);
      
          HARDBUG(type != DXP_GAMEEND)
        }
  
        break;
      }

      if (object->DXP_move_number >=
          (object->DXP_game_moves + MARGIN))
      {
        PRINTF("moves exceeded\n");

        object->DXP_game_code = DXP_UNKNOWN;

        encode(DXP_GAMEEND, object, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");
  
        int nwrite;

        if ((nwrite = compat_socket_write(arg_sockfd, buffer, nbuffer)) ==
            INVALID) break;
  
        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(arg_sockfd, buffer, &nbuffer) == INVALID) break;
    
          char type = decode(nbuffer, buffer, object);
      
          HARDBUG(type != DXP_GAMEEND)
        }

        result = INVALID;

        break;
      }

      BSTRING(bbest_move)

      HARDBUG(bassigncstr(bbest_move, "NULL") == BSTR_ERR)

      BSTRING(bcomment)

      //check for book

      if (options.use_book)
        return_book_move(&(object->DXP_search.S_board), &moves_list, bbest_move);
  
      if (compat_strcasecmp(bdata(bbest_move), "NULL") != 0)
      {
        HARDBUG(bassigncstr(bcomment, "book") == BSTR_ERR)

        PRINTF("\nbook_move=%s\n", bdata(bbest_move));
      }
      else
      {
        int best_score;

        int best_depth;

        PRINTF("\nthinking..\n");

        double t1 = compat_time();
  
        if (moves_list.ML_nmoves == 1)
        {
          move2bstring(&moves_list, 0, bbest_move);
  
          HARDBUG(bassigncstr(bcomment, "only move") == BSTR_ERR)

          best_score = 0;
  
          best_depth = 0;
        } 
        else
        {
          //configure thread for search
  
          set_time_limit(object->DXP_move_number, &time_control);
  
          enqueue(return_thread_queue(thread_alpha_beta_master),
            MESSAGE_STATE, game_state.get_game_state(&game_state));
  
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
  
                HARDBUG(my_sscanf(bdata(message.message_text), "%s%d%d",
                                  cbest_move, &best_score, &best_depth) != 3)
   
                HARDBUG(bassigncstr(bbest_move, cbest_move) == BSTR_ERR)
  
                CDESTROY(cbest_move)

                HARDBUG(bassign(bcomment, message.message_text) == BSTR_ERR)
  
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
  
        update_time_control(object->DXP_move_number, move_time,
          &time_control);
  
        object->DXP_game_time_used += move_time;
  
        object->DXP_move_number++;
  
        PRINTF("used   %s\n", secs2string(move_time));
  
        PRINTF("total  %s\n", secs2string(object->DXP_game_time_used));
  
        PRINTF("remain %s\n", secs2string(object->DXP_game_time -
                                          object->DXP_game_time_used));
  
        PRINTF("\n* * * * * * * * * * %s %d %d\n\n",
          bdata(bbest_move), best_score, best_depth);

        if (best_score < score_min) score_min = best_score;
        if (best_score > score_max) score_max = best_score;
      }

      strcpy(object->DXP_move_string, bdata(bbest_move));
  
      if (options.dxp_annotate_level == 0)
        game_state.push_move(&game_state, object->DXP_move_string, NULL);
      else
        game_state.push_move(&game_state, object->DXP_move_string,
                             bdata(bcomment));
  
      encode(DXP_MOVE, object, buffer, &nbuffer);

      PRINTF("sending DXP_MOVE..\n");
  
      int nwrite;

      if ((nwrite = compat_socket_write(arg_sockfd, buffer, nbuffer)) ==
          INVALID) break;

      HARDBUG(nwrite != nbuffer)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bbest_move)) == INVALID)

      do_move(&(object->DXP_search.S_board), imove, &moves_list, FALSE);

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

  destroy_game_state(&game_state);

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

        if (my_sscanf(line, "%[^\n]", fen) == 0)
        {
          PRINTF("skipping empty line..\n");

          continue;
        }

        if (strncmp(fen, "//", 2) == 0)
        {
          HARDBUG(my_sscanf(fen, "//%[^\n]", opening) != 1)

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

      fen2board(&(with->DXP_search.S_board), fen);

      PRINTF("The starting position for game %d is:\n", igame);

      print_board(&(with->DXP_search.S_board));

      if (icolour == 0)
        with->DXP_game_colour = WHITE_ENUM;
      else
        with->DXP_game_colour = BLACK_ENUM;

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
        with->DXP_game_colour = BLACK_ENUM;
      else
        with->DXP_game_colour = WHITE_ENUM;
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

