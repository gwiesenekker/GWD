// SCU REVISION 8.1285 di 17 nov 2026 23:14:01 CET
#include "globals.h"

#define SA struct sockaddr

#define DXP_GAMEREQ 'R'
#define DXP_FISCHER 'F'
#define DXP_GAMEACC 'A'
#define DXP_MOVE 'M'
#define DXP_GAMEEND 'E'
#define DXP_CHAT 'C'
#define DXP_BACKREQ '?'
#define DXP_BACKACC '?'

#define DXP_UNKNOWN '0'
#define DXP_IGIVEUP '1'
#define DXP_DRAW '2'
#define DXP_IWIN '3'

#define MARGIN 5
#define DXP_TIME_RESERVE 0.050 // seconds for returning the move to the opponent

typedef struct
{
  search_t DXP_search;

  colour_enum DXP_game_colour;
  time_control_t DXP_time_control;
  bstring DXP_move_string;
  char DXP_game_code;
} dxp_t;

local bstring event;
local bstring my_name;
local bstring your_name;

// The Fischer request carries six characters, with up to three decimal places.
double format_dxp_increment(double arg_increment, char arg_field[7])
{
  int precision = 3;

  while (snprintf(arg_field, 7, "%06.*f", precision, arg_increment) != 6)
  {
    HARDBUG(precision == 0)

    --precision;
  }

  double increment;

  HARDBUG(my_sscanf(arg_field, "%lf", &increment) != 1)

  return increment;
}

local void
encode(char arg_type, dxp_t *arg_with_dxp, bstring arg_output, int *arg_nbuffer)
{
  // Fixed DXP fields fit in 128 bytes; captures need at most the move text
  // length.
  HARDBUG(blength(arg_with_dxp->DXP_move_string) > INT_MAX - 128)
  HARDBUG(balloc(arg_output, 128 + blength(arg_with_dxp->DXP_move_string)) ==
          BSTR_ERR)

  char *arg_buffer = bdata(arg_output);

  *arg_nbuffer = 0;

  if ((arg_type == DXP_GAMEREQ) or (arg_type == DXP_FISCHER))
  {
    int i = 0;

    arg_buffer[i] = arg_type;
    i++;

    strcpy(arg_buffer + i, "01");
    i += 2;

    //                  01234567890123456789012345678901

    strncpy(arg_buffer + i, bdata(my_name), 32);

    i += 32;

    if (IS_WHITE(arg_with_dxp->DXP_game_colour))
    {
      arg_buffer[i] = 'Z';
    }
    else
    {
      arg_buffer[i] = 'W';
    }
    i++;

    double game_time = options.dxp_game_time * 60.0;
    double increment = 0.0;
    int moves = options.dxp_game_moves;

    if (arg_type == DXP_FISCHER)
    {
      HARDBUG((game_time < 0.0) or (game_time > 999999.0))

      snprintf(arg_buffer + i, 7, "%06d", (int)game_time);
      i += 6;

      // Configure our clock with the same precision as the wire field.
      increment =
        format_dxp_increment(options.dxp_game_increment, arg_buffer + i);

      HARDBUG(increment == 0.0)

      i += 6;
      moves = INVALID;
    }
    else
    {
      HARDBUG((options.dxp_game_time < 0) or (options.dxp_game_time > 999))
      HARDBUG((moves < 0) or (moves > 999))

      snprintf(arg_buffer + i, 7, "%03d%03d", options.dxp_game_time, moves);
      i += 6;
    }
    configure_time_control(game_time,
                           moves,
                           increment,
                           TIME_HORIZON_PADDED,
                           &arg_with_dxp->DXP_time_control);

    strcpy(arg_buffer + i, "B");
    i++;

    if (IS_WHITE(arg_with_dxp->DXP_search.S_board.B_colour2move))
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

      if (arg_with_dxp->DXP_search.S_board.B_white_man_bb & BITULL(jfield))
      {
        strcpy(arg_buffer + i, "w");
        i++;
      }
      else if (arg_with_dxp->DXP_search.S_board.B_white_king_bb &
               BITULL(jfield))
      {
        strcpy(arg_buffer + i, "W");
        i++;
      }
      else if (arg_with_dxp->DXP_search.S_board.B_black_man_bb & BITULL(jfield))
      {
        strcpy(arg_buffer + i, "z");
        i++;
      }
      else if (arg_with_dxp->DXP_search.S_board.B_black_king_bb &
               BITULL(jfield))
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
    strncpy(arg_buffer + i, bdata(my_name), 32);
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

    BSTRING(string)

    HARDBUG(bassignformat(string,
                          "%04d",
                          (int)fmin(9999.0,
                                    round(return_time_used(
                                      &arg_with_dxp->DXP_time_control)))) ==
            BSTR_ERR)

    strcpy(arg_buffer + i, bdata(string));
    i += 4;

    // ugly

    int j = 0;

    strncpy(arg_buffer + i, bdata(arg_with_dxp->DXP_move_string) + j, 2);
    j += 3;
    i += 2;

    strncpy(arg_buffer + i, bdata(arg_with_dxp->DXP_move_string) + j, 2);
    j += 2;
    i += 2;

    int n = 0;
    int k = j;

    while (bdata(arg_with_dxp->DXP_move_string)[k] == 'x')
    {
      n++;
      k += 3;
    }

    HARDBUG(bassignformat(string, "%02d", n) == BSTR_ERR)

    strcpy(arg_buffer + i, bdata(string));
    i += 2;

    k = j;

    while (bdata(arg_with_dxp->DXP_move_string)[k] == 'x')
    {
      k++;
      strncpy(arg_buffer + i, bdata(arg_with_dxp->DXP_move_string) + k, 2);
      k += 2;
      i += 2;
    }

    arg_buffer[i] = '\0';
    i++;

    BDESTROY(string)

    *arg_nbuffer = i;

    PRINTF("DXP_MOVE buffer=%s nbuffer=%d\n", arg_buffer, *arg_nbuffer);
  }
  else if (arg_type == DXP_GAMEEND)
  {
    int i = 0;

    arg_buffer[i] = DXP_GAMEEND;
    i++;

    arg_buffer[i] = arg_with_dxp->DXP_game_code;
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
  arg_output->slen = *arg_nbuffer - 1;
}

char decode(int arg_nhuffer, char *arg_buffer, dxp_t *arg_with_dxp)
{
  HARDBUG(arg_nhuffer < 1)

  // buffer[nbuffer] = '\0';
  PRINTF("decode nbuffer=%d buffer=%s\n", arg_nhuffer, arg_buffer);

  if ((arg_buffer[0] == DXP_GAMEREQ) or (arg_buffer[0] == DXP_FISCHER))
  {
    // ignore version

    int i = 3;

    BSTRING(string)

    HARDBUG(bassigncstr(string, "?") == BSTR_ERR)
    HARDBUG(bpattern(string, 51) == BSTR_ERR)

    HARDBUG((i + 31) >= arg_nhuffer)

    HARDBUG(bassignblk(your_name, arg_buffer + i, 32) == BSTR_ERR)

    PRINTF("your_name=%s\n", bdata(your_name));

    i += 32;

    HARDBUG(i >= arg_nhuffer)

    char game_colour = arg_buffer[i];

    i++;

    PRINTF("game_colour=%c\n", game_colour);

    if (game_colour == 'W')
      arg_with_dxp->DXP_game_colour = WHITE_ENUM;
    else
      arg_with_dxp->DXP_game_colour = BLACK_ENUM;

    double game_time, increment;
    int moves;

    if (arg_buffer[0] == DXP_FISCHER)
    {
      HARDBUG((i + 12) >= arg_nhuffer)
      HARDBUG(my_sscanf(arg_buffer + i, "%6lf", &game_time) != 1)

      i += 6;

      HARDBUG(my_sscanf(arg_buffer + i, "%6lf", &increment) != 1)

      i += 6;
      moves = INVALID;
    }
    else
    {
      int minutes;

      HARDBUG((i + 6) >= arg_nhuffer)
      HARDBUG(my_sscanf(arg_buffer + i, "%3d", &minutes) != 1)

      i += 3;

      HARDBUG(my_sscanf(arg_buffer + i, "%3d", &moves) != 1)
      HARDBUG(moves < 0)

      i += 3;
      game_time = minutes * 60.0;
      increment = 0.0;
    }
    PRINTF("game_time=%.3f increment=%.3f moves=%d\n",
           game_time,
           increment,
           moves);
    configure_time_control(game_time,
                           moves,
                           increment,
                           TIME_HORIZON_PADDED,
                           &arg_with_dxp->DXP_time_control);

    HARDBUG(i >= arg_nhuffer)

    if (arg_buffer[i] == 'A')
    {
      PRINTF("starting position\n");

      string2board(&(arg_with_dxp->DXP_search.S_board), STARTING_POSITION);
    }
    else if (arg_buffer[i] == 'B')
    {
      i++;

      HARDBUG(i >= arg_nhuffer)

      if (arg_buffer[i] == 'W')
        bdata(string)[0] = 'w';
      else if (arg_buffer[i] == 'Z')
        bdata(string)[0] = 'b';
      else
        FATAL("error in B/colour2move", EXIT_FAILURE)

      for (int j = 1; j <= 50; j++)
      {
        i++;

        HARDBUG(i >= arg_nhuffer)

        if (arg_buffer[i] == 'e')
          bdata(string)[j] = *nn;
        else if (arg_buffer[i] == 'w')
          bdata(string)[j] = *wO;
        else if (arg_buffer[i] == 'z')
          bdata(string)[j] = *bO;
        else if (arg_buffer[i] == 'W')
          bdata(string)[j] = *wX;
        else if (arg_buffer[i] == 'Z')
          bdata(string)[j] = *bX;
        else
          FATAL("error in B/position", EXIT_FAILURE)
      }

      bdata(string)[51] = '\0';

      PRINTF("string=%s\n", bdata(string));

      string2board(&(arg_with_dxp->DXP_search.S_board), bdata(string));

      PRINTF("starting position\n");

      print_board(&(arg_with_dxp->DXP_search.S_board));
    }
    else
      FATAL("error in DXP", EXIT_FAILURE)

    BDESTROY(string)

    return arg_buffer[0];
  }
  else if (arg_buffer[0] == DXP_GAMEACC)
  {
    int i = 1;

    HARDBUG((i + 31) >= arg_nhuffer)

    HARDBUG(bassignblk(your_name, arg_buffer + i, 32) == BSTR_ERR)

    PRINTF("your_name=%s\n", bdata(your_name));

    i += 32;

    HARDBUG(i >= arg_nhuffer)

    char code = arg_buffer[i];

    PRINTF("acceptance code=%c\n", code);

    return (DXP_GAMEACC);
  }
  else if (arg_buffer[0] == DXP_MOVE)
  {
    // ignore time

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

    gen_moves(&(arg_with_dxp->DXP_search.S_board), &moves_list);

    HARDBUG(moves_list.ML_nmoves == 0)

    HARDBUG(n != moves_list.ML_ncaptx)

    int jmove = INVALID;

    for (int imove = 0; imove < moves_list.ML_nmoves; imove++)
    {
      move_t *move = moves_list.ML_moves + imove;

      int ifield = move->M_move_from;
      int kfield = move->M_move_to;
      ui64_t captures_bb = move->M_captures_bb;

      if (ifield != f)
        continue;

      if (kfield != t)
        continue;

      if (captures_bb == 0)
      {
        jmove = imove;
      }
      else
      {
        int m = 0;

        while (captures_bb != 0)
        {
          int jfield = BIT_CTZ(captures_bb);

          m = 0;

          for (int j = 0; j < n; j++)
            if (c[j] == jfield)
              m++;

          if (m == 0)
            break;

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

    HARDBUG(bassign(arg_with_dxp->DXP_move_string, move) == BSTR_ERR)

    BDESTROY(move)

    PRINTF("DXP_move_string=%s\n", bdata(arg_with_dxp->DXP_move_string));

    return (DXP_MOVE);
  }
  else if (arg_buffer[0] == DXP_GAMEEND)
  {
    int i = 1;

    HARDBUG(i >= arg_nhuffer)

    PRINTF("reason=%c\n", arg_buffer[i]);

    arg_with_dxp->DXP_game_code = arg_buffer[i];

    i++;

    HARDBUG(i >= arg_nhuffer)

    PRINTF("stop code=%c\n", arg_buffer[i]);

    return (DXP_GAMEEND);
  }
  else if (arg_buffer[0] == DXP_CHAT)
  {
    return (DXP_CHAT);
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

  return (INVALID);
}

// read from socket until \0
int reads(compat_socket_t arg_sockfd, bstring arg_buffer, int *arg_nbuffer)
{
  *arg_nbuffer = 0;

  while (TRUE)
  {
    HARDBUG(btrunc(arg_buffer, 0) == BSTR_ERR)

    while (TRUE)
    {
      char c;
      int n = compat_socket_read(arg_sockfd, &c, 1);

      if (n != 1)
        return (INVALID);

      if (c == '\0')
        break;

      HARDBUG(bconchar(arg_buffer, c) == BSTR_ERR)
    }

    if (bdata(arg_buffer)[0] != DXP_CHAT)
      break;
    PRINTF("ignoring DXP_CHAT %s\n", bdata(arg_buffer));
  }

  HARDBUG(blength(arg_buffer) == INT_MAX)

  *arg_nbuffer = blength(arg_buffer) + 1;

  return (*arg_nbuffer);
}

local char *secs2string(double arg_secs, bstring string)
{
  int hours;
  int minutes;
  int seconds;
  int centiseconds;

  i64_t secs = (i64_t)round(arg_secs * 100.0);

  centiseconds = secs % 100;

  secs /= 100;
  hours = secs / 3600;
  minutes = (secs - hours * 3600) / 60;
  seconds = secs - hours * 3600 - minutes * 60;

  HARDBUG(bassignformat(string,
                        "%02d:%02d:%02d.%02d",
                        hours,
                        minutes,
                        seconds,
                        centiseconds) == BSTR_ERR)

  return (bdata(string));
}

local char *rtrim(const char *arg_trim, bstring arg_string)
{
  HARDBUG(bassigncstr(arg_string, arg_trim) == BSTR_ERR)
  HARDBUG(brtrimws(arg_string) == BSTR_ERR)

  return (bdata(arg_string));
}

local int play_game(dxp_t *arg_with_dxp, compat_socket_t arg_sockfd)
{
  int result = INVALID;

  BSTRING(buffer)
  BSTRING(comment)
  BSTRING(game_result)
  BSTRING(bbest_move)
  BSTRING(bcomment)
  BSTRING(btime)
  BSTRING(btrim)

  game_state_t game_state;

  construct_game_state(&game_state);

  game_state.set_event(&game_state, bdata(event));

  if (IS_WHITE(arg_with_dxp->DXP_game_colour))
  {
    game_state.set_white(&game_state, rtrim(bdata(my_name), btrim));
    game_state.set_black(&game_state, rtrim(bdata(your_name), btrim));
  }
  else
  {
    game_state.set_white(&game_state, rtrim(bdata(your_name), btrim));
    game_state.set_black(&game_state, rtrim(bdata(my_name), btrim));
  }

  BSTRING(bfen)

  board2fen(&(arg_with_dxp->DXP_search.S_board), bfen, FALSE);

  game_state.set_starting_position(&game_state, bdata(bfen));

  BDESTROY(bfen)

  int nbuffer;

  time_control_t *with_time_control = &arg_with_dxp->DXP_time_control;

  score_t score_min = SCORE(SCORE_PLUS_INFINITY);
  score_t score_max = SCORE(SCORE_MINUS_INFINITY);

  while (TRUE)
  {
    print_board(&(arg_with_dxp->DXP_search.S_board));

    moves_list_t moves_list;

    construct_moves_list(&moves_list);

    gen_moves(&(arg_with_dxp->DXP_search.S_board), &moves_list);

    if (arg_with_dxp->DXP_search.S_board.B_colour2move !=
        arg_with_dxp->DXP_game_colour)
    {
      if (moves_list.ML_nmoves == 0)
      {
        PRINTF("waiting for DXP_GAMEEND..\n");

        if (reads(arg_sockfd, buffer, &nbuffer) == INVALID)
          break;

        char type = decode(nbuffer, bdata(buffer), arg_with_dxp);

        HARDBUG(type != DXP_GAMEEND)

        if (options.dxp_strict_gameend)
        {
          encode(DXP_GAMEEND, arg_with_dxp, buffer, &nbuffer);

          PRINTF("sending DXP_GAMEEND..\n");

          if (compat_socket_write(arg_sockfd, bdata(buffer), nbuffer) ==
              INVALID)
            break;
        }

        if (IS_WHITE(arg_with_dxp->DXP_search.S_board.B_colour2move))
          result = 0;
        else
          result = 2;

        break;
      }

      HARDBUG(bassigncstr(comment, "NULL") == BSTR_ERR)

      if (options.ponder)
      {
        // configure thread for pondering

        options.time_limit = 10000.0;
        options.time_ntrouble = 0;

        PRINTF("\nPondering..\n");

        enqueue(return_thread_queue(thread_alpha_beta_master),
                MESSAGE_STATE,
                game_state.get_game_state(&game_state));

        enqueue(return_thread_queue(thread_alpha_beta_master),
                MESSAGE_GO,
                "dxp/play_game");
      }

      PRINTF("waiting for DXP_MOVE or DXP_GAMEEND..\n");

      int nread = reads(arg_sockfd, buffer, &nbuffer);

      if (options.ponder)
      {
        // cancel pondering and dequeue messages

        enqueue(return_thread_queue(thread_alpha_beta_master),
                MESSAGE_ABORT_SEARCH,
                "main/play_game");

        message_t message;

        while (TRUE)
        {
          if (dequeue(&main_queue, &message) != INVALID)
          {
            if (message.message_id == MESSAGE_INFO)
            {
              // PRINTF("got info %s\n", bdata(message.message_text));
            }
            else if (message.message_id == MESSAGE_RESULT)
            {
              PRINTF("got hint %s\n", bdata(message.message_text));

              break;
            }
            else
              FATAL("message.message_id error", EXIT_FAILURE)
          }

          HARDBUG(compat_sleep(CENTI_SECOND) != 0)
        }

        if (options.dxp_annotate_level > 1)
          HARDBUG(bassign(comment, message.message_text) == BSTR_ERR)
      }

      if (nread == INVALID)
        break;

      char type = decode(nbuffer, bdata(buffer), arg_with_dxp);

      if (type == DXP_GAMEEND)
      {
        if (options.dxp_strict_gameend)
        {
          encode(DXP_GAMEEND, arg_with_dxp, buffer, &nbuffer);

          PRINTF("sending DXP_GAMEEND..\n");

          if (compat_socket_write(arg_sockfd, bdata(buffer), nbuffer) ==
              INVALID)
            break;
        }

        if (arg_with_dxp->DXP_game_code == DXP_IGIVEUP)
        {
          if (IS_WHITE(arg_with_dxp->DXP_search.S_board.B_colour2move))
            result = 0;
          else
            result = 2;
        }
        else if (arg_with_dxp->DXP_game_code == DXP_DRAW)
        {
          result = 1;

          PRINTF("DRAW score_min=" PRINTF_SCORE_T " score_max=" PRINTF_SCORE_T
                 "\n",
                 SCORE2PRINTF(score_min),
                 SCORE2PRINTF(score_max));
        }
        else if (arg_with_dxp->DXP_game_code == DXP_IWIN)
        {
          if (IS_WHITE(arg_with_dxp->DXP_search.S_board.B_colour2move))
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

      HARDBUG(bassign(move, arg_with_dxp->DXP_move_string) == BSTR_ERR)

      HARDBUG((imove = search_move(&moves_list, move)) == INVALID)

      BDESTROY(move)

      if (compat_strcasecmp(bdata(comment), "NULL") == 0)
        game_state.push_move(&game_state,
                             bdata(arg_with_dxp->DXP_move_string),
                             NULL);
      else
        game_state.push_move(&game_state,
                             bdata(arg_with_dxp->DXP_move_string),
                             bdata(comment));

      do_move(&(arg_with_dxp->DXP_search.S_board), imove, &moves_list);
    }
    else
    {
      // my turn

      if (moves_list.ML_nmoves == 0)
      {
        PRINTF("DXP game lost (nmoves == 0)\n");
        arg_with_dxp->DXP_game_code = DXP_IGIVEUP;

        encode(DXP_GAMEEND, arg_with_dxp, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");

        int nwrite;

        if ((nwrite =
               compat_socket_write(arg_sockfd, bdata(buffer), nbuffer)) ==
            INVALID)
          break;

        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(arg_sockfd, buffer, &nbuffer) == INVALID)
            break;

          char type = decode(nbuffer, bdata(buffer), arg_with_dxp);

          HARDBUG(type != DXP_GAMEEND)
        }

        if (IS_WHITE(arg_with_dxp->DXP_search.S_board.B_colour2move))
          result = 0;
        else
          result = 2;

        break;
      }

      // check for known endgame

      int egtb_mate = read_endgame(&(arg_with_dxp->DXP_search), NULL);

      if (egtb_mate != ENDGAME_UNKNOWN)
      {
        PRINTF("known endgame egtb_mate=%d\n", egtb_mate);

        if (egtb_mate == INVALID)
        {
          PRINTF("DXP game draw (egtb_mate=%d)\n", egtb_mate);

          arg_with_dxp->DXP_game_code = DXP_DRAW;

          result = 1;

          PRINTF("DRAW score_min=" PRINTF_SCORE_T " score_max=" PRINTF_SCORE_T
                 "\n",
                 SCORE2PRINTF(score_min),
                 SCORE2PRINTF(score_max));
        }
        else if (egtb_mate > 0)
        {
          PRINTF("DXP game won (egtb_mate=%d)\n", egtb_mate);

          arg_with_dxp->DXP_game_code = DXP_IWIN;

          if (IS_WHITE(arg_with_dxp->DXP_search.S_board.B_colour2move))
            result = 2;
          else
            result = 0;
        }
        else
        {
          PRINTF("DXP game lost (egtb_mate=%d)\n", egtb_mate);

          arg_with_dxp->DXP_game_code = DXP_IGIVEUP;

          if (IS_WHITE(arg_with_dxp->DXP_search.S_board.B_colour2move))
            result = 0;
          else
            result = 2;
        }

        encode(DXP_GAMEEND, arg_with_dxp, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");

        int nwrite;

        if ((nwrite =
               compat_socket_write(arg_sockfd, bdata(buffer), nbuffer)) ==
            INVALID)
          break;

        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(arg_sockfd, buffer, &nbuffer) == INVALID)
            break;

          char type = decode(nbuffer, bdata(buffer), arg_with_dxp);

          HARDBUG(type != DXP_GAMEEND)
        }

        break;
      }

      if (return_moves_left(with_time_control) <= -MARGIN)
      {
        PRINTF("moves exceeded\n");

        arg_with_dxp->DXP_game_code = DXP_UNKNOWN;

        encode(DXP_GAMEEND, arg_with_dxp, buffer, &nbuffer);

        PRINTF("sending DXP_GAMEEND..\n");

        int nwrite;

        if ((nwrite =
               compat_socket_write(arg_sockfd, bdata(buffer), nbuffer)) ==
            INVALID)
          break;

        HARDBUG(nwrite != nbuffer)

        if (options.dxp_strict_gameend)
        {
          PRINTF("waiting for DXP_GAMEEND..\n");

          if (reads(arg_sockfd, buffer, &nbuffer) == INVALID)
            break;

          char type = decode(nbuffer, bdata(buffer), arg_with_dxp);

          HARDBUG(type != DXP_GAMEEND)
        }

        result = INVALID;

        break;
      }

      HARDBUG(bassigncstr(bbest_move, "NULL") == BSTR_ERR)

      double book_start = compat_time();

      // check for book

      if (options.use_book)
        return_book_move(&(arg_with_dxp->DXP_search.S_board),
                         &moves_list,
                         bbest_move);

      if (compat_strcasecmp(bdata(bbest_move), "NULL") != 0)
      {
        HARDBUG(bassigncstr(bcomment, "book") == BSTR_ERR)

        if (is_rolling_time_control(with_time_control))
          update_time_control(compat_time() - book_start, with_time_control);

        PRINTF("\nbook_move=%s\n", bdata(bbest_move));
      }
      else
      {
        score_t best_score;

        int best_depth;

        PRINTF("\nthinking..\n");

        double t1 = compat_time();

        if (moves_list.ML_nmoves == 1)
        {
          move2bstring(&moves_list, 0, bbest_move);

          HARDBUG(bassigncstr(bcomment, "only move") == BSTR_ERR)

          best_score = SCORE(0);

          best_depth = 0;
        }
        else
        {
          // configure thread for search

          set_time_limit(with_time_control, DXP_TIME_RESERVE);

          enqueue(return_thread_queue(thread_alpha_beta_master),
                  MESSAGE_STATE,
                  game_state.get_game_state(&game_state));

          enqueue(return_thread_queue(thread_alpha_beta_master),
                  MESSAGE_GO,
                  "dxp/play_game");

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
                PRINTF("got result %s\n", bdata(message.message_text));

                CSTRING(cbest_move, blength(message.message_text))

                HARDBUG(my_sscanf(bdata(message.message_text),
                                  "%s" SCANF_SCORE_T_RAW "%d",
                                  cbest_move,
                                  &best_score,
                                  &best_depth) != 3)

                HARDBUG(bassigncstr(bbest_move, cbest_move) == BSTR_ERR)

                CDESTROY(cbest_move)

                HARDBUG(bassign(bcomment, message.message_text) == BSTR_ERR)

                break;
              }
              else
                FATAL("message.message_id error", EXIT_FAILURE)
            }

            HARDBUG(compat_sleep(CENTI_SECOND) != 0)
          }
        }

        double t2 = compat_time();

        double move_time = t2 - t1;

        update_time_control(move_time, with_time_control);

        PRINTF("used   %s\n", secs2string(move_time, btime));

        PRINTF("total  %s\n",
               secs2string(return_time_used(with_time_control), btime));

        PRINTF("remain %s\n",
               secs2string(return_time_left(with_time_control), btime));

        PRINTF("\n* * * * * * * * * * %s " PRINTF_SCORE_T " %d\n\n",
               bdata(bbest_move),
               SCORE2PRINTF(best_score),
               best_depth);

        if (best_score < score_min)
          score_min = best_score;

        if (best_score > score_max)
          score_max = best_score;
      }

      HARDBUG(bassign(arg_with_dxp->DXP_move_string, bbest_move) == BSTR_ERR)

      if (options.dxp_annotate_level == 0)
        game_state.push_move(&game_state,
                             bdata(arg_with_dxp->DXP_move_string),
                             NULL);
      else
        game_state.push_move(&game_state,
                             bdata(arg_with_dxp->DXP_move_string),
                             bdata(bcomment));

      encode(DXP_MOVE, arg_with_dxp, buffer, &nbuffer);

      PRINTF("sending DXP_MOVE..\n");

      int nwrite;

      if ((nwrite = compat_socket_write(arg_sockfd, bdata(buffer), nbuffer)) ==
          INVALID)
        break;

      HARDBUG(nwrite != nbuffer)

      int imove;

      HARDBUG((imove = search_move(&moves_list, bbest_move)) == INVALID)

      do_move(&(arg_with_dxp->DXP_search.S_board), imove, &moves_list);
    }
  }

  HARDBUG(bassigncstr(game_result, "*") == BSTR_ERR)

  if (result == 2)
  {
    HARDBUG(bassigncstr(game_result, "2-0") == BSTR_ERR)
  }
  else if (result == 1)
  {
    HARDBUG(bassigncstr(game_result, "1-1") == BSTR_ERR)
  }
  else if (result == 0)
  {
    HARDBUG(bassigncstr(game_result, "0-2") == BSTR_ERR)
  }

  PRINTF("game_result=%s\n", bdata(game_result));

  game_state.set_result(&game_state, bdata(game_result));

  game_state.save2pdn(&game_state, "dxp.pdn");

  destroy_game_state(&game_state);

  BDESTROY(btrim)

  BDESTROY(btime)

  BDESTROY(bcomment)

  BDESTROY(bbest_move)

  BDESTROY(game_result)

  BDESTROY(comment)

  BDESTROY(buffer)

  return (result);
}

local void set_my_name(void)
{
  if (compat_strcasecmp(bdata(options.dxp_tag), "NULL") == 0)
    HARDBUG(
      bassignformat(my_name, "GWD %s %s", REVISION, bdata(options.network0)) ==
      BSTR_ERR)
  else
    HARDBUG(bassignformat(my_name,
                          "GWD %s %s %s",
                          REVISION,
                          bdata(options.dxp_tag),
                          bdata(options.network0)) == BSTR_ERR)

  HARDBUG(btrunc(my_name, 32) == BSTR_ERR)

  while (blength(my_name) < 32)
    HARDBUG(bconchar(my_name, ' ') == BSTR_ERR)

  PRINTF("my_name=%s\n", bdata(my_name));
}

local void dxp_game_initiator(compat_socket_t arg_fd)
{
  HARDBUG((event = bfromcstr("")) == NULL)
  HARDBUG((my_name = bfromcstr("")) == NULL)
  HARDBUG((your_name = bfromcstr("")) == NULL)

  set_my_name();

  dxp_t dxp;

  dxp_t *with_dxp = &dxp;

  HARDBUG((with_dxp->DXP_move_string = bfromcstr("")) == NULL)

  construct_time_control(&with_dxp->DXP_time_control);

  construct_search(&(with_dxp->DXP_search), STDOUT, NULL, NETWORK0_ID);

  int nwon = 0;
  int ndraw = 0;
  int nlost = 0;
  int nunknown = 0;

  BSTRING(fen)
  BSTRING(opening)
  BSTRING(line)

  FILE *fballot = NULL;

  if (fexists(bdata(options.dxp_ballot)))
  {
    HARDBUG((fballot = fopen(bdata(options.dxp_ballot), "r")) == NULL)

    PRINTF("Reading ballots from file %s\n", bdata(options.dxp_ballot));
  }

  BSTRING(buffer)

  int igame = 0;

  while (TRUE)
  {
    // either select the starting position
    // or a fen position

    HARDBUG(bassigncstr(fen, "NULL") == BSTR_ERR)
    HARDBUG(bassigncstr(opening, "") == BSTR_ERR)

    if (fballot != NULL)
    {
      while (TRUE)
      {

        if (!my_breadline(line, fballot))
          goto label_break;

        HARDBUG(bassign(fen, line) == BSTR_ERR)
        HARDBUG(brtrimws(fen) == BSTR_ERR)

        if (blength(fen) == 0)
        {
          PRINTF("skipping empty line..\n");

          continue;
        }

        if (strncmp(bdata(fen), "//", 2) == 0)
        {
          HARDBUG(bassignmidstr(opening, fen, 2, blength(fen) - 2) == BSTR_ERR)

          PRINTF("opening=%s\n", bdata(opening));

          continue;
        }

        break;
      }
      PRINTF("Ballot fen is: %s\n", bdata(fen));
    }
    else
    {
      HARDBUG(bassigncstr(fen, STARTING_POSITION2FEN) == BSTR_ERR)

      HARDBUG(bassigncstr(opening, "(none)") == BSTR_ERR)
    }

    for (int icolour = 0; icolour < 2; icolour++)
    {
      ++igame;

      if (igame > options.dxp_games)
        goto label_break;

      HARDBUG(
        bassignformat(event, "Game %d, Opening %s", igame, bdata(opening)) ==
        BSTR_ERR)

      PRINTF("event=%s\n", bdata(event));

      fen2board(&(with_dxp->DXP_search.S_board), bdata(fen));

      PRINTF("The starting position for game %d is:\n", igame);

      print_board(&(with_dxp->DXP_search.S_board));

      if (icolour == 0)
        with_dxp->DXP_game_colour = WHITE_ENUM;
      else
        with_dxp->DXP_game_colour = BLACK_ENUM;

      PRINTF("sending DXP_GAMEREQ..\n");

      int nbuffer;

      encode((options.dxp_game_increment > 0.0) ? DXP_FISCHER : DXP_GAMEREQ,
             with_dxp,
             buffer,
             &nbuffer);

      int nwrite;

      if ((nwrite = compat_socket_write(arg_fd, bdata(buffer), nbuffer)) ==
          INVALID)
        break;

      HARDBUG(nwrite != nbuffer)

      PRINTF("waiting for DXP_GAMEACC..\n");

      while (TRUE)
      {
        if (reads(arg_fd, buffer, &nbuffer) == INVALID)
          goto label_break;

        char type = decode(nbuffer, bdata(buffer), with_dxp);

        HARDBUG(type != DXP_GAMEACC)

        break;
      }

      int result = play_game(with_dxp, arg_fd);

      if (result == 2)
      {
        if (IS_WHITE(with_dxp->DXP_game_colour))
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
        if (IS_WHITE(with_dxp->DXP_game_colour))
          ++nlost;
        else
          ++nwon;
      }
      else
        ++nunknown;

      PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
             nwon,
             ndraw,
             nlost,
             nunknown);

      if (IS_WHITE(with_dxp->DXP_game_colour))
        with_dxp->DXP_game_colour = BLACK_ENUM;
      else
        with_dxp->DXP_game_colour = WHITE_ENUM;
    }
  }

label_break:

  PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
         nwon,
         ndraw,
         nlost,
         nunknown);

  results2csv(nwon, ndraw, nlost, nunknown);

  if (fballot != NULL)
    FCLOSE(fballot)

  BDESTROY(buffer)

  BDESTROY(line)

  BDESTROY(opening)

  BDESTROY(fen)

  HARDBUG(bdestroy(with_dxp->DXP_move_string) == BSTR_ERR)
  HARDBUG(bdestroy(your_name) == BSTR_ERR)
  HARDBUG(bdestroy(my_name) == BSTR_ERR)
  HARDBUG(bdestroy(event) == BSTR_ERR)
}

local void dxp_game_follower(compat_socket_t arg_fd)
{
  HARDBUG((event = bfromcstr("")) == NULL)
  HARDBUG((my_name = bfromcstr("")) == NULL)
  HARDBUG((your_name = bfromcstr("")) == NULL)

  set_my_name();

  dxp_t dxp;

  dxp_t *with_dxp = &dxp;

  HARDBUG((with_dxp->DXP_move_string = bfromcstr("")) == NULL)

  construct_time_control(&with_dxp->DXP_time_control);

  construct_search(&(with_dxp->DXP_search), STDOUT, NULL, NETWORK0_ID);

  int nwon = 0;
  int ndraw = 0;
  int nlost = 0;
  int nunknown = 0;

  BSTRING(buffer)

  int igame = 0;

  while (TRUE)
  {
    PRINTF("waiting for DXP_GAMEREQ..\n");

    int nbuffer;

    if (reads(arg_fd, buffer, &nbuffer) == INVALID)
      break;

    char type = decode(nbuffer, bdata(buffer), with_dxp);

    HARDBUG((type != DXP_GAMEREQ) and (type != DXP_FISCHER))

    PRINTF("received DXP_GAMEREQ\n");

    encode(DXP_GAMEACC, with_dxp, buffer, &nbuffer);

    PRINTF("sending DXP_GAMEACC..\n");

    int nwrite;

    if ((nwrite = compat_socket_write(arg_fd, bdata(buffer), nbuffer)) ==
        INVALID)
      break;

    HARDBUG(nwrite != nbuffer)

    ++igame;

    HARDBUG(bassignformat(event, "Game %d", igame) == BSTR_ERR)

    int result = play_game(with_dxp, arg_fd);

    if (result == 2)
    {
      if (IS_WHITE(with_dxp->DXP_game_colour))
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
      if (IS_WHITE(with_dxp->DXP_game_colour))
        ++nlost;
      else
        ++nwon;
    }
    else
      ++nunknown;

    PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
           nwon,
           ndraw,
           nlost,
           nunknown);
  }

  PRINTF("DXP nwon=%d ndraw=%d nlost=%d nunknown=%d\n",
         nwon,
         ndraw,
         nlost,
         nunknown);

  results2csv(nwon, ndraw, nlost, nunknown);

  BDESTROY(buffer)

  HARDBUG(bdestroy(with_dxp->DXP_move_string) == BSTR_ERR)
  HARDBUG(bdestroy(your_name) == BSTR_ERR)
  HARDBUG(bdestroy(my_name) == BSTR_ERR)
  HARDBUG(bdestroy(event) == BSTR_ERR)
}

void dxp_server(void)
{
  compat_socket_t sockfd;
  int yes = 1;

  HARDBUG(compat_socket_startup() != 0)

  HARDBUG((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == COMPAT_INVALID_SOCKET)

  HARDBUG(setsockopt(sockfd,
                     SOL_SOCKET,
                     SO_REUSEADDR,
                     (const char *)&yes,
                     sizeof(yes)) == INVALID)

  struct sockaddr_in server_address;

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = htonl(INADDR_ANY);
  server_address.sin_port = htons(options.dxp_port);

  HARDBUG((bind(sockfd, (SA *)&server_address, sizeof(server_address))) ==
          INVALID)

  HARDBUG((listen(sockfd, 5)) == INVALID)

  struct sockaddr_in client_address;

  compat_socklen_t len = sizeof(client_address);

  compat_socket_t connfd;

  PRINTF("server waiting for connection..\n");

  HARDBUG((connfd = accept(sockfd, (SA *)&client_address, &len)) ==
          COMPAT_INVALID_SOCKET)

  HARDBUG(len > sizeof(client_address))

  PRINTF("server accepted connection..\n");

  if (options.dxp_initiator)
    dxp_game_initiator(connfd);
  else
    dxp_game_follower(connfd);

  HARDBUG(compat_socket_close(connfd) == INVALID)
  HARDBUG(compat_socket_close(sockfd) == INVALID)

  HARDBUG(compat_socket_cleanup() != 0)
}

void dxp_client(void)
{
  compat_socket_t sockfd;

  HARDBUG(compat_socket_startup() != 0)

  HARDBUG((sockfd = socket(AF_INET, SOCK_STREAM, 0)) == COMPAT_INVALID_SOCKET)

  struct sockaddr_in server_address;

  memset(&server_address, 0, sizeof(server_address));

  server_address.sin_family = AF_INET;
  server_address.sin_addr.s_addr = inet_addr(bdata(options.dxp_server_ip));
  server_address.sin_port = htons(options.dxp_port);

  HARDBUG(connect(sockfd, (SA *)&server_address, sizeof(server_address)) ==
          INVALID)

  if (options.dxp_initiator)
    dxp_game_initiator(sockfd);
  else
    dxp_game_follower(sockfd);

  HARDBUG(compat_socket_close(sockfd) == INVALID)

  HARDBUG(compat_socket_cleanup() != 0)
}
