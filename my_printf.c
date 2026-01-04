//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#include "globals.h"

#define LOG_PREFIX_ID "LOG_prefix_id"
#define LOG_OBJECT_ID "LOG_object_id"

#define NLOGS 2

#define NROTATE 9

#define MAX_SIZE (4 * MBYTE)

#define LOGS_DIR "logs"
#define WINEPREFIX "WINEPREFIX"

bstring my_printf_logs_dir;

// this is more a test for my dynamic cJSON record_t
local int nlogs = INVALID;
record_t logs[NLOGS];

local my_mutex_t MP_mutex;

void my_printf(my_printf_t *self, char *arg_format, ...)
{
  my_printf_t *object = self;

  if (object == NULL)
  {
    // stdout

    va_list ap;

    va_start(ap, arg_format);

    if (vprintf(arg_format, ap) < 0)
    {
      fprintf(stderr, "vfprintf(arg_format, ap) > 0\n");

      exit(EXIT_FAILURE);
    }

    va_end(ap);

    return;
  }

  if (object->MP_paused)
    return;

  HARDBUG(compat_mutex_lock(&(object->MP_mutex)) != 0)

  int flush = FALSE;

  // if (compat_strcasecmp(arg_format, "") == 0) flush = TRUE;

  HARDBUG(bassigncstr(object->MP_bformat, arg_format) == BSTR_ERR)

  if (bchar(object->MP_bformat, blength(object->MP_bformat) - 1) == '$')
  {
    flush = TRUE;

    bdelete(object->MP_bformat, blength(object->MP_bformat) - 1, 1);
  }

  int ret;

  bvformata(ret, object->MP_bbuffer, bdata(object->MP_bformat), arg_format);

  HARDBUG(ret == BSTR_ERR)

  HARDBUG(bassigncstr(object->MP_bline, "") == BSTR_ERR)

  int ichar = 0;

  while (ichar < blength(object->MP_bbuffer))
  {
    if (bchare(object->MP_bbuffer, ichar, '\0') != '\n')
    {
      ichar++;

      continue;
    }

    char time_stamp[MY_LINE_MAX];

    time_t t = time(NULL);

    HARDBUG(
        strftime(time_stamp, MY_LINE_MAX, "%H:%M:%S-%d/%m/%Y", localtime(&t)) ==
        0)

    i64_t pid = compat_getpid();

    if (object->MP_fsize > MAX_SIZE)
    {
      fprintf(object->MP_flog,
              "\n%s|%lld@ PTHREAD_SELF=%#lX"
              " LOG FILE EXCEEDS MAXIMUM SIZE AND WILL BE ROTATED\n",
              time_stamp,
              pid,
              compat_pthread_self());

      if (fclose(object->MP_flog) != 0)
      {
        fprintf(stderr, "fclose(%s) != 0\n", bdata(object->MP_bname));

        exit(EXIT_FAILURE);
      }

      bstring bold_path;

      HARDBUG((bold_path = bfromcstr("")) == NULL)

      bstring bnew_path;

      HARDBUG((bnew_path = bfromcstr("")) == NULL)

      for (int irotate = NROTATE; irotate >= 1; --irotate)
      {
        btrunc(bold_path, 0);

        if (irotate > 1)
        {
          HARDBUG(bformata(bold_path,
                           "%s-%d",
                           bdata(object->MP_bname),
                           irotate - 1) == BSTR_ERR)
        }
        else
        {
          HARDBUG(bformata(bold_path, "%s", bdata(object->MP_bname)) ==
                  BSTR_ERR)
        }

        btrunc(bnew_path, 0);

        HARDBUG(
            bformata(bnew_path, "%s-%d", bdata(object->MP_bname), irotate) ==
            BSTR_ERR)

        // ignore return value of remove as file might not exist

        (void)remove(bdata(bnew_path));

        // ignore return value of rename as file might not exist

        (void)rename(bdata(bold_path), bdata(bnew_path));
      }

      HARDBUG(bdestroy(bnew_path) == BSTR_ERR)

      HARDBUG(bdestroy(bold_path) == BSTR_ERR)

      if ((object->MP_flog = fopen(bdata(object->MP_bname), "w")) == NULL)
      {
        fprintf(stderr, "fopen(%s, 'w') == NULL\n", bdata(object->MP_bname));

        exit(EXIT_FAILURE);
      }

      int nchar = fprintf(object->MP_flog,
                          "%s|%lld@ PTHREAD_SELF=%#lX"
                          " LOG FILE ROTATED AND RE-OPENED\n\n",
                          time_stamp,
                          pid,
                          compat_pthread_self());

      if (nchar < 0)
      {
        fprintf(stderr, "fprintf(%s, ...) < 0\n", bdata(object->MP_bname));

        exit(EXIT_FAILURE);
      }

      object->MP_fsize = nchar;
    }

    HARDBUG(bassignmidstr(object->MP_bline, object->MP_bbuffer, 0, ichar + 1) ==
            BSTR_ERR)

    int nchar = fprintf(object->MP_flog,
                        "%s|%lld@ %s",
                        time_stamp,
                        pid,
                        bdata(object->MP_bline));

    if (nchar < 0)
    {
      fprintf(stderr, "fprintf(%s, ...) < 0\n", bdata(object->MP_bname));

      exit(EXIT_FAILURE);
    }

#ifdef DEBUG
    flush = TRUE;
#else
    if ((my_mpi_globals.MMG_id_global <= 0) or
        (my_mpi_globals.MMG_id_slave == 0))
      flush = TRUE;
#endif

    if (flush)
    {
      if (fflush(object->MP_flog) != 0)
      {
        fprintf(stderr, "fflush(%s) != 0\n", bdata(object->MP_bname));

        exit(EXIT_FAILURE);
      }
    }

    object->MP_fsize += nchar;

    // also print to stdout if needed

    if (!options.hub and object->MP2stdout and
        (my_mpi_globals.MMG_id_global <= 0))
    {
      nchar = fprintf(stdout, "%s", bdata(object->MP_bline));

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

    bdelete(object->MP_bbuffer, 0, ichar + 1);

    ichar = 0;
  }

  HARDBUG(compat_mutex_unlock(&(object->MP_mutex)) != 0)
}

void construct_my_printf(my_printf_t *self, char *arg_prefix, int arg2stdout)
{
  my_printf_t *object = self;

  object->MP2stdout = arg2stdout;

  record_t *with_record = NULL;

  int ilog;

  for (ilog = 0; ilog < nlogs; ilog++)
  {
    with_record = logs + ilog;

    bstring prefix = bfromcstr("");

    get_field(with_record, LOG_PREFIX_ID, prefix);

    if (compat_strcasecmp(bdata(prefix), arg_prefix) == 0)
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

  HARDBUG((object->MP_bname = bfromcstr("")) == NULL)

  if (my_mpi_globals.MMG_nglobal <= 1)
    HARDBUG(bformata(object->MP_bname,
                     "%s/%s%d.txt",
                     bdata(my_printf_logs_dir),
                     arg_prefix,
                     object_id) == BSTR_ERR)
  else
    HARDBUG(bformata(object->MP_bname,
                     "%s/%s%d-%d-%d.txt",
                     bdata(my_printf_logs_dir),
                     arg_prefix,
                     object_id,
                     my_mpi_globals.MMG_id_global,
                     my_mpi_globals.MMG_nglobal) == BSTR_ERR)

  if ((object->MP_flog = fopen(bdata(object->MP_bname), "w")) == NULL)
  {
    fprintf(stderr, "fopen(%s, 'w') == NULL\n", bdata(object->MP_bname));

    exit(EXIT_FAILURE);
  }

  char time_stamp[MY_LINE_MAX];

  time_t t = time(NULL);

  HARDBUG(
      strftime(time_stamp, MY_LINE_MAX, "%H:%M:%S-%d/%m/%Y", localtime(&t)) ==
      0)

  i64_t pid = compat_getpid();

  int nchar = fprintf(object->MP_flog,
                      "%s|%lld@ PTHREAD_SELF=%#lX LOG FILE OPENED\n\n",
                      time_stamp,
                      pid,
                      compat_pthread_self());

  if (nchar < 0)
  {
    fprintf(stderr, "fprintf(%s, ...) < 0\n", bdata(object->MP_bname));

    exit(EXIT_FAILURE);
  }

  object->MP_fsize = nchar;

  HARDBUG((object->MP_bformat = bfromcstr("")) == NULL)
  HARDBUG((object->MP_bbuffer = bfromcstr("")) == NULL)

  HARDBUG((object->MP_bline = bfromcstr("")) == NULL)
  HARDBUG((object->MP_bstamp = bfromcstr("")) == NULL)

  object_id++;

  set_field(with_record, LOG_OBJECT_ID, &object_id);

  HARDBUG(compat_mutex_init(&(object->MP_mutex)) != 0)

  object->MP_paused = FALSE;
}

void pause_my_printf(my_printf_t *self)
{
  my_printf_t *object = self;

  my_printf(object, "MY_PRINTF PAUSED..\n");

  HARDBUG(compat_mutex_lock(&(object->MP_mutex)) != 0)

  object->MP_paused = TRUE;

  HARDBUG(compat_mutex_unlock(&(object->MP_mutex)) != 0)
}

void resume_my_printf(my_printf_t *self)
{
  my_printf_t *object = self;

  HARDBUG(compat_mutex_lock(&(object->MP_mutex)) != 0)

  object->MP_paused = FALSE;

  HARDBUG(compat_mutex_unlock(&(object->MP_mutex)) != 0)

  my_printf(object, "..MY_PRINTF RESUMED\n");
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

  if (opendir(bdata(my_printf_logs_dir)) == NULL)
  {
    HARDBUG(compat_mkdir(bdata(my_printf_logs_dir)) != 0)
    HARDBUG(opendir(bdata(my_printf_logs_dir)) == NULL)
  }

  nlogs = 0;

  HARDBUG(compat_mutex_init(&MP_mutex) != 0)
}

#define NTEST 4
#define NLINES 1000

void test_my_printf(void)
{
  my_printf_t test[NTEST];

  for (int itest = 0; itest < NTEST; itest++)
    construct_my_printf(test + itest, "test", TRUE);

  for (int iline = 1; iline <= NLINES; iline++)
  {
    int nchar = (iline - 1) % 10 + 1;

    char line[MY_LINE_MAX];

    snprintf(line, MY_LINE_MAX, "%d:", iline);

    for (int itest = 0; itest < NTEST; itest++)
      my_printf(test + itest, "%s", line);

    for (int ichar = 1; ichar <= nchar; ichar++)
    {
      snprintf(line, MY_LINE_MAX, " %d", ichar);

      for (int itest = 0; itest < NTEST; itest++)
        my_printf(test + itest, "%s", line);
    }
    for (int itest = 0; itest < NTEST; itest++)
      my_printf(test + itest, "\n");
  }
  for (int itest = 0; itest < NTEST; itest++)
    my_printf(test + itest, "$");
}
