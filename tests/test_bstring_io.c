/* Exercise protocol framing and stream reads with the real DEBUG name stack. */
#include "../my_malloc.c"
#include "../my_printf.c"
#include "../hub.c"
#define secs2string dxp_secs2string
#define play_game dxp_play_game
#include "../dxp.c"
#undef secs2string
#undef play_game
#include <assert.h>

void *STDOUT;
int zzzzzz_invocation;
options_t options;
my_mpi_globals_t my_mpi_globals;
void zzzzzz(const char *arg_file,
            const char *arg_func,
            long arg_line,
            const char *arg_message,
            int arg_code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", arg_file, arg_line, arg_func, arg_message);
  exit(arg_code);
}

int main(int argc, char **argv)
{
  assert(argc == 2);
  assert(compat_mutex_init(&my_names_mutex) == 0);

  PUSH_NAME(__FUNC__)
  nlogs = 0;
  my_mpi_globals.MMG_is_single = TRUE;

  HARDBUG((my_printf_logs_dir = bfromcstr(argv[1])) == NULL)

  my_printf_t loggers[2];

  construct_my_printf(&loggers[0], "strings", FALSE);
  construct_my_printf(&loggers[1], "strings", FALSE);
  STDOUT = &loggers[0];

  BSTRING(expected)
  BSTRING(line)
  BSTRING(partial)

  for (int ichar = 0; ichar < 20000; ichar++)
    HARDBUG(bconchar(expected, 'a' + ichar % 26) == BSTR_ERR)
  HARDBUG(bconchar(expected, '\n') == BSTR_ERR)

  FILE *file = tmpfile();

  assert(file != NULL);
  assert(fwrite(bdata(expected), 1, blength(expected), file) ==
         (size_t)blength(expected));
  assert(fputs("last line", file) >= 0);

  rewind(file);

  assert(my_breadline(line, file));
  assert(biseq(line, expected));
  assert(my_breadline(line, file));
  assert(biseqcstr(line, "last line"));
  assert(!my_breadline(line, file));
  assert(blength(line) == 0);

  rewind(file);

  HARDBUG(bassigncstr(line, "prefix") == BSTR_ERR)
  assert(breada(line, my_bread, file) == BSTR_OK);
  assert(blength(line) == 6 + blength(expected) + 9);
  assert(memcmp(bdata(line), "prefix", 6) == 0);
  assert(bdata(line)[blength(line)] == '\0');
  assert(!ferror(file));
  assert(fclose(file) == 0);

  /* A fragmented Hub line must survive nonblocking polls without truncation. */
  int pipes[2];

  assert(pipe(pipes) == 0);

  int saved_stdin = dup(0);

  assert(saved_stdin >= 0);
  assert(dup2(pipes[0], 0) == 0);
  assert(close(pipes[0]) == 0);

  for (int ioffset = 0; ioffset < blength(expected);)
  {
    int count = blength(expected) - ioffset;

    if (count > 1000)
      count = 1000;

    assert(write(pipes[1], bdata(expected) + ioffset, count) == count);

    ioffset += count;

    assert(read_hub_line(line, partial, FALSE) ==
           ((ioffset == blength(expected)) ? TRUE : FALSE));
  }

  assert(biseq(line, expected));
  assert(blength(partial) == 0);
  assert(close(pipes[1]) == 0);
  assert(read_hub_line(line, partial, TRUE) == INVALID);
  assert(dup2(saved_stdin, 0) == 0);
  assert(close(saved_stdin) == 0);

  /* Long DXP chat frames are skipped, and the following frame remains intact.
   */
  int sockets[2];

  assert(socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) == 0);
  assert(write(sockets[0], "C", 1) == 1);
  assert(write(sockets[0], bdata(expected), blength(expected)) ==
         blength(expected));
  assert(write(sockets[0], "\0E20\0", 5) == 5);

  int nbuffer;

  assert(reads(sockets[1], line, &nbuffer) == 4);
  assert(nbuffer == 4);
  assert(biseqcstr(line, "E20"));
  assert(close(sockets[0]) == 0);
  assert(reads(sockets[1], line, &nbuffer) == INVALID);
  assert(close(sockets[1]) == 0);

  /* Force rotation without writing four megabytes first. */
  loggers[0].MP_fsize = MAX_SIZE + 1;
  my_printf(&loggers[0], "%s", bdata(expected));
  my_printf(&loggers[0], "last line\n");

  assert(blength(loggers[0].MP_bbuffer) == 0);

  for (int ilog = 0; ilog < 2; ilog++)
  {
    assert(fclose(loggers[ilog].MP_flog) == 0);
    HARDBUG(bdestroy(loggers[ilog].MP_bname) == BSTR_ERR)
    HARDBUG(bdestroy(loggers[ilog].MP_bformat) == BSTR_ERR)
    HARDBUG(bdestroy(loggers[ilog].MP_bbuffer) == BSTR_ERR)
    HARDBUG(bdestroy(loggers[ilog].MP_bline) == BSTR_ERR)
    HARDBUG(bdestroy(loggers[ilog].MP_btime_stamp) == BSTR_ERR)
    assert(compat_mutex_destroy(&loggers[ilog].MP_mutex) == 0);
  }
  STDOUT = NULL;

  for (int ilog = 0; ilog < nlogs; ilog++)
  {
    cJSON_Delete(logs[ilog].R_cjson);

    HARDBUG(bdestroy(logs[ilog].R_string) == BSTR_ERR)
  }

  HARDBUG(bdestroy(my_printf_logs_dir) == BSTR_ERR)

  BDESTROY(partial)

  BDESTROY(line)

  BDESTROY(expected)

  POP_NAME(__FUNC__)

  assert(print_all_name_stacks() == 0);

  puts("Bstring I/O tests passed: long lines, append, EOF, Hub fragments and "
       "DXP framing");

  return (0);
}
