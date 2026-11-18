// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "my_printf.h"

#include "bstrlib.h"
#include "common.h"
#include "main.h"
#include "my_bstreams.h"
#include "my_malloc.h"
#include "my_mpi.h"
#include "records.h"

#define LOG_PREFIX_ID "LOG_prefix_id"
#define LOG_OBJECT_ID "LOG_object_id"

enum
{
  NLOGS = 2,
  NROTATE = 9,
  MAX_SIZE = (4 * MBYTE)
};

#define LOGS_DIR "logs"
#define WINEPREFIX "WINEPREFIX"

bstring my_printf_logs_dir;

// this is more a test for my dynamic cJSON record_t
local int nlogs = INVALID;
record_t logs[NLOGS];

local my_mutex_t MP_mutex;

void my_printf(my_printf_t *arg_with_my_printf, const char *arg_format, ...)
{
  if (arg_with_my_printf == NULL)
  {
    // stdout

    va_list arguments;

    va_start(arguments, arg_format);

    if (vprintf(arg_format, arguments) < 0)
    {
      fprintf(stderr, "vfprintf(arg_format, ap) > 0\n");

      exit(EXIT_FAILURE);
    }

    va_end(arguments);

    return;
  }

  if (arg_with_my_printf->MP_paused)
    return;

  HARDBUG(compat_mutex_lock(&(arg_with_my_printf->MP_mutex)) != 0)

  int flush = FALSE;

  // if (compat_strcasecmp(arg_format, "") == 0) flush = TRUE;

  HARDBUG(bassigncstr(arg_with_my_printf->MP_bformat, arg_format) == BSTR_ERR)

  if (bchar(arg_with_my_printf->MP_bformat,
            blength(arg_with_my_printf->MP_bformat) - 1) == '$')
  {
    flush = TRUE;

    HARDBUG(bdelete(arg_with_my_printf->MP_bformat,
                    blength(arg_with_my_printf->MP_bformat) - 1,
                    1) == BSTR_ERR)
  }

  int result;

  bvformata(result,
            arg_with_my_printf->MP_bbuffer,
            bdata(arg_with_my_printf->MP_bformat),
            arg_format);

  HARDBUG(result == BSTR_ERR)

  HARDBUG(bassigncstr(arg_with_my_printf->MP_bline, "") == BSTR_ERR)

  int ichar = 0;

  while (ichar < blength(arg_with_my_printf->MP_bbuffer))
  {
    if (bchare(arg_with_my_printf->MP_bbuffer, ichar, '\0') != '\n')
    {
      ichar++;

      continue;
    }

    bstring time_stamp = arg_with_my_printf->MP_btime_stamp;

    time_t current_time = time(NULL);

    my_bstrftime(time_stamp, "%H:%M:%S-%d/%m/%Y", current_time);

    my_int64_t pid = compat_getpid();

    if (arg_with_my_printf->MP_fsize > MAX_SIZE)
    {
      fprintf(arg_with_my_printf->MP_flog,
              "\n%s|%lld@ PTHREAD_SELF=%#lX"
              " LOG FILE EXCEEDS MAXIMUM SIZE AND WILL BE ROTATED\n",
              bdata(time_stamp),
              pid,
              compat_pthread_self());

      if (fclose(arg_with_my_printf->MP_flog) != 0)
      {
        fprintf(stderr,
                "fclose(%s) != 0\n",
                bdata(arg_with_my_printf->MP_bname));

        exit(EXIT_FAILURE);
      }

      bstring bold_path;

      HARDBUG((bold_path = bfromcstr("")) == NULL)

      bstring bnew_path;

      HARDBUG((bnew_path = bfromcstr("")) == NULL)

      for (int irotate = NROTATE; irotate >= 1; --irotate)
      {
        HARDBUG(btrunc(bold_path, 0) == BSTR_ERR)

        if (irotate > 1)
        {
          HARDBUG(bformata(bold_path,
                           "%s-%d",
                           bdata(arg_with_my_printf->MP_bname),
                           irotate - 1) == BSTR_ERR)
        }
        else
        {
          HARDBUG(bformata(bold_path,
                           "%s",
                           bdata(arg_with_my_printf->MP_bname)) == BSTR_ERR)
        }

        HARDBUG(btrunc(bnew_path, 0) == BSTR_ERR)

        HARDBUG(bformata(bnew_path,
                         "%s-%d",
                         bdata(arg_with_my_printf->MP_bname),
                         irotate) == BSTR_ERR)

        // ignore return value of remove as file might not exist

        (void)remove(bdata(bnew_path));

        // ignore return value of rename as file might not exist

        (void)rename(bdata(bold_path), bdata(bnew_path));
      }

      HARDBUG(bdestroy(bnew_path) == BSTR_ERR)

      HARDBUG(bdestroy(bold_path) == BSTR_ERR)

      if ((arg_with_my_printf->MP_flog =
             fopen(bdata(arg_with_my_printf->MP_bname), "w")) == NULL)
      {
        fprintf(stderr,
                "fopen(%s, 'w') == NULL\n",
                bdata(arg_with_my_printf->MP_bname));

        exit(EXIT_FAILURE);
      }

      int nchar = fprintf(arg_with_my_printf->MP_flog,
                          "%s|%lld@ PTHREAD_SELF=%#lX"
                          " LOG FILE ROTATED AND RE-OPENED\n\n",
                          bdata(time_stamp),
                          pid,
                          compat_pthread_self());

      if (nchar < 0)
      {
        fprintf(stderr,
                "fprintf(%s, ...) < 0\n",
                bdata(arg_with_my_printf->MP_bname));

        exit(EXIT_FAILURE);
      }

      arg_with_my_printf->MP_fsize = nchar;
    }

    HARDBUG(bassignmidstr(arg_with_my_printf->MP_bline,
                          arg_with_my_printf->MP_bbuffer,
                          0,
                          ichar + 1) == BSTR_ERR)

    int nchar = fprintf(arg_with_my_printf->MP_flog,
                        "%s|%lld@ %s",
                        bdata(time_stamp),
                        pid,
                        bdata(arg_with_my_printf->MP_bline));

    if (nchar < 0)
    {
      fprintf(stderr,
              "fprintf(%s, ...) < 0\n",
              bdata(arg_with_my_printf->MP_bname));

      exit(EXIT_FAILURE);
    }

#ifdef DEBUG
    flush = TRUE;
#else
    if (my_mpi_globals.MMG_is_master or my_mpi_globals.MMG_is_lead_slave)
      flush = TRUE;
#endif

    if (flush)
    {
      if (fflush(arg_with_my_printf->MP_flog) != 0)
      {
        fprintf(stderr,
                "fflush(%s) != 0\n",
                bdata(arg_with_my_printf->MP_bname));

        exit(EXIT_FAILURE);
      }
    }

    arg_with_my_printf->MP_fsize += nchar;

    // also print to stdout if needed

    if (!options.hub and arg_with_my_printf->MP2stdout and
        my_mpi_globals.MMG_is_master)
    {
      nchar = fprintf(stdout, "%s", bdata(arg_with_my_printf->MP_bline));

      if (nchar < 0)
      {
        fprintf(stderr, "fprintf(stdout, ...) < 0\n");

        exit(EXIT_FAILURE);
      }

      if (fflush(stdout) != 0)
      {
        fprintf(stderr, "fflush(stdout) != 0\n");

        exit(EXIT_FAILURE);
      }
    }

    HARDBUG(bdelete(arg_with_my_printf->MP_bbuffer, 0, ichar + 1) == BSTR_ERR)

    ichar = 0;
  }

  HARDBUG(compat_mutex_unlock(&(arg_with_my_printf->MP_mutex)) != 0)
}

void construct_my_printf(my_printf_t *arg_with_my_printf,
                         const char *arg_prefix,
                         int arg_to_stdout)
{
  arg_with_my_printf->MP2stdout = arg_to_stdout;

  // Logging also serves allocation diagnostics; reuse an owned timestamp
  // string.
  HARDBUG((arg_with_my_printf->MP_btime_stamp = bfromcstr("")) == NULL)

  record_t *with_record = NULL;

  int ilog;

  for (ilog = 0; ilog < nlogs; ilog++)
  {
    with_record = logs + ilog;

    BSTRING(prefix)

    get_field(with_record, LOG_PREFIX_ID, prefix);

    int found = (compat_strcasecmp(bdata(prefix), arg_prefix) == 0);

    BDESTROY(prefix)

    if (found)
      break;
  }

  if (ilog == nlogs)
  {
    with_record = logs + nlogs++;

    construct_record(with_record);

    add_field(with_record, LOG_PREFIX_ID, FIELD_TYPE_STRING, arg_prefix);

    int object_id = 0;

    add_field(with_record, LOG_OBJECT_ID, FIELD_TYPE_INT, &object_id);
  }

  int object_id;

  get_field(with_record, LOG_OBJECT_ID, &object_id);

  HARDBUG((arg_with_my_printf->MP_bname = bfromcstr("")) == NULL)

  if (my_mpi_globals.MMG_is_single)
    HARDBUG(bformata(arg_with_my_printf->MP_bname,
                     "%s/%s%d.txt",
                     bdata(my_printf_logs_dir),
                     arg_prefix,
                     object_id) == BSTR_ERR)
  else
    HARDBUG(bformata(arg_with_my_printf->MP_bname,
                     "%s/%s%d-%d-%d.txt",
                     bdata(my_printf_logs_dir),
                     arg_prefix,
                     object_id,
                     my_mpi_globals.MMG_global_id,
                     my_mpi_globals.MMG_nglobal) == BSTR_ERR)

  if ((arg_with_my_printf->MP_flog =
         fopen(bdata(arg_with_my_printf->MP_bname), "w")) == NULL)
  {
    fprintf(stderr,
            "fopen(%s, 'w') == NULL\n",
            bdata(arg_with_my_printf->MP_bname));

    exit(EXIT_FAILURE);
  }

  bstring time_stamp = arg_with_my_printf->MP_btime_stamp;

  time_t current_time = time(NULL);

  my_bstrftime(time_stamp, "%H:%M:%S-%d/%m/%Y", current_time);

  my_int64_t pid = compat_getpid();

  int nchar = fprintf(arg_with_my_printf->MP_flog,
                      "%s|%lld@ PTHREAD_SELF=%#lX LOG FILE OPENED\n\n",
                      bdata(time_stamp),
                      pid,
                      compat_pthread_self());

  if (nchar < 0)
  {
    fprintf(stderr,
            "fprintf(%s, ...) < 0\n",
            bdata(arg_with_my_printf->MP_bname));

    exit(EXIT_FAILURE);
  }

  arg_with_my_printf->MP_fsize = nchar;

  HARDBUG((arg_with_my_printf->MP_bformat = bfromcstr("")) == NULL)
  HARDBUG((arg_with_my_printf->MP_bbuffer = bfromcstr("")) == NULL)

  HARDBUG((arg_with_my_printf->MP_bline = bfromcstr("")) == NULL)

  object_id++;

  set_field(with_record, LOG_OBJECT_ID, &object_id);

  HARDBUG(compat_mutex_init(&(arg_with_my_printf->MP_mutex)) != 0)

  arg_with_my_printf->MP_paused = FALSE;
}

void pause_my_printf(my_printf_t *arg_with_my_printf)
{
  my_printf(arg_with_my_printf, "MY_PRINTF PAUSED..\n");

  HARDBUG(compat_mutex_lock(&(arg_with_my_printf->MP_mutex)) != 0)

  arg_with_my_printf->MP_paused = TRUE;

  HARDBUG(compat_mutex_unlock(&(arg_with_my_printf->MP_mutex)) != 0)
}

void resume_my_printf(my_printf_t *arg_with_my_printf)
{
  HARDBUG(compat_mutex_lock(&(arg_with_my_printf->MP_mutex)) != 0)

  arg_with_my_printf->MP_paused = FALSE;

  HARDBUG(compat_mutex_unlock(&(arg_with_my_printf->MP_mutex)) != 0)

  my_printf(arg_with_my_printf, "..MY_PRINTF RESUMED\n");
}

void init_my_printf(void)
{
  HARDBUG((my_printf_logs_dir = bfromcstr("")) == NULL)

  if (getenv(WINEPREFIX) == NULL)
    HARDBUG(bformata(my_printf_logs_dir, "%s", LOGS_DIR) == BSTR_ERR)
  else
    HARDBUG(
      bformata(my_printf_logs_dir, "%s_%s", getenv(WINEPREFIX), LOGS_DIR) ==
      BSTR_ERR)

  CompatStat directory;

  if (compat_stat(bdata(my_printf_logs_dir), &directory) != 0)
  {
    HARDBUG(errno != ENOENT)
    HARDBUG(compat_mkdir(bdata(my_printf_logs_dir)) != 0)
    HARDBUG(compat_stat(bdata(my_printf_logs_dir), &directory) != 0)
  }

  HARDBUG(!directory.is_directory)

  nlogs = 0;

  HARDBUG(compat_mutex_init(&MP_mutex) != 0)
}

enum
{
  NTEST = 4,
  NLINES = 1000
};

void test_my_printf(void)
{
  my_printf_t test[NTEST];

  for (int itest = 0; itest < NTEST; itest++)
    construct_my_printf(test + itest, "test", TRUE);

  for (int iline = 1; iline <= NLINES; iline++)
  {
    int nchar = (iline - 1) % 10 + 1;

    BSTRING(line)

    HARDBUG(bassignformat(line, "%d:", iline) == BSTR_ERR)

    for (int itest = 0; itest < NTEST; itest++)
      my_printf(test + itest, "%s", bdata(line));

    for (int ichar = 1; ichar <= nchar; ichar++)
    {
      HARDBUG(bassignformat(line, " %d", ichar) == BSTR_ERR)

      for (int itest = 0; itest < NTEST; itest++)
        my_printf(test + itest, "%s", bdata(line));
    }

    for (int itest = 0; itest < NTEST; itest++)
      my_printf(test + itest, "\n");

    BDESTROY(line)
  }

  for (int itest = 0; itest < NTEST; itest++)
    my_printf(test + itest, "$");
}
