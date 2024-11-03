//SCU REVISION 7.701 zo  3 nov 2024 10:59:01 CET
#include "globals.h"

#define LOG_PREFIX_ID "LOG_prefix_id"
#define LOG_OBJECT_ID "LOG_object_id"

#define NLOGS 2

#define NROTATE 9

#define MAX_SIZE (4 * MBYTE)

#define LOGS_DIR "logs"

local int nlogs = INVALID;
record_t logs[NLOGS];

local my_mutex_t my_printf_mutex;

void my_printf(void *self, char *arg_format, ...)
{
  my_printf_t *object = self;

  va_list ap;

  va_start(ap, arg_format);

  if (object == NULL)
  {
    vprintf(arg_format, ap);

    va_end(ap);

    return;
  }

  if (compat_strcasecmp(arg_format, "") == 0)
  {
    HARDBUG(compat_mutex_lock(&(object->my_printf_mutex)) != 0)

    if (fflush(object->my_printf_flog) != 0)
    {
      fprintf(stderr, "could not fflush %s\n", object->my_printf_fname);
      exit(EXIT_FAILURE);
    }

    HARDBUG(compat_mutex_unlock(&(object->my_printf_mutex)) != 0)

    return;
  }

  HARDBUG(compat_mutex_lock(&(object->my_printf_mutex)) != 0)

  char line[MY_LINE_MAX];

  (void) vsnprintf(line, MY_LINE_MAX, arg_format, ap);

  va_end(ap);
  
  int iline = 0;

  while(TRUE)
  {
    HARDBUG(iline >= MY_LINE_MAX)
    if (line[iline] == '\0') break;

    HARDBUG(object->my_printf_ibuffer >= MY_LINE_MAX)

    if ((object->my_printf_buffer[object->my_printf_ibuffer++] =
         line[iline++]) != '\n') continue;

    HARDBUG(object->my_printf_ibuffer >= MY_LINE_MAX)

    object->my_printf_buffer[object->my_printf_ibuffer] = '\0';

    //timestamp

    char stamp[MY_LINE_MAX];
  
    time_t t = time(NULL);
    (void) strftime(stamp, MY_LINE_MAX, "%H:%M:%S-%d/%m/%Y", localtime(&t));
   
    if (object->my_printf_fsize > MAX_SIZE)
    {
      fprintf(object->my_printf_flog,
        "\nlog file exceeds maximum size and will be rotated\n");
  
      if (fclose(object->my_printf_flog) == EOF)
      {
        fprintf(stderr, "could not fclose %s\n", object->my_printf_fname);
        exit(EXIT_FAILURE);
      }
  
      for (int irotate = NROTATE; irotate > 1; --irotate)
      {
        char oldpath[MY_LINE_MAX];
        char newpath[MY_LINE_MAX];

        snprintf(oldpath, MY_LINE_MAX, "%s-%d",
          object->my_printf_fname, irotate - 1);
        snprintf(newpath, MY_LINE_MAX, "%s-%d",
          object->my_printf_fname, irotate);

        remove(newpath);

        rename(oldpath, newpath);
      }
  
      char newpath[MY_LINE_MAX];
  
      snprintf(newpath, MY_LINE_MAX, "%s-1", object->my_printf_fname);

      remove(newpath);

      rename(object->my_printf_fname, newpath);
  
      if ((object->my_printf_flog = fopen(object->my_printf_fname, "w")) ==
          NULL)
      {
        fprintf(stderr, "could not fopen %s in mode 'w'\n",
          object->my_printf_fname);

        exit(EXIT_FAILURE);
      }

      t = time(NULL);

      int nchar = fprintf(object->my_printf_flog,
       "log file rotated and re-opened at %s\n", ctime(&t));
    
      if (nchar < 0)
      {
        fprintf(stderr, "could not fprintf %s\n", object->my_printf_fname);

        exit(EXIT_FAILURE);
      }
  
      object->my_printf_fsize = nchar;
    }

    int nchar = fprintf(object->my_printf_flog, "%s@ %s",
      stamp, object->my_printf_buffer);

    if (nchar < 0)
    {
      fprintf(stderr, "could not fprintf %s\n", object->my_printf_fname);
      exit(EXIT_FAILURE);
    }
#ifdef DEBUG
    if (fflush(object->my_printf_flog) != 0)
    {
      fprintf(stderr, "could not fflush object->my_printf_flog\n");
      exit(EXIT_FAILURE);
    }
#else
    if ((my_mpi_globals.MY_MPIG_id_global == 0) or
        (my_mpi_globals.MY_MPIG_id_global == INVALID) or 
        (my_mpi_globals.MY_MPIG_id_slave == 0))
    {
      if (fflush(object->my_printf_flog) != 0)
      {
        fprintf(stderr, "could not fflush object->my_printf_flog\n");
        exit(EXIT_FAILURE);
      }
    }
#endif
    object->my_printf_fsize += nchar;
  
    if (!options.hub and object->my_printf2stdout and
        ((my_mpi_globals.MY_MPIG_id_global == 0) or
         (my_mpi_globals.MY_MPIG_id_global == INVALID)))
    {
      nchar = fprintf(stdout, "%s", object->my_printf_buffer);

      if (nchar < 0)
      {
        fprintf(stderr, "could not fprintf stdout\n");

        exit(EXIT_FAILURE);
      }

      if (fflush(stdout) != 0)
      {
        fprintf(stderr, "could not fflush stdout\n");

        exit(EXIT_FAILURE);
      }
    }
  
    strcpy(object->my_printf_buffer, "");

    object->my_printf_ibuffer = 0;
  }

  HARDBUG(compat_mutex_unlock(&(object->my_printf_mutex)) != 0)
}

void construct_my_printf(void *self, char *arg_prefix, int arg2stdout)
{
  my_printf_t *object = self;

  object->my_printf2stdout = arg2stdout;

  record_t *with_record = NULL;

  int ilog;

  for (ilog = 0; ilog < nlogs; ilog++)
  {
    with_record = logs + ilog;

    bstring prefix = bfromcstr("");

    get_field(with_record, LOG_PREFIX_ID, prefix);
 
    if (compat_strcasecmp(bdata(prefix), arg_prefix) == 0) break;
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

  if (my_mpi_globals.MY_MPIG_nglobal <= 1)
    HARDBUG(snprintf(object->my_printf_fname, MY_LINE_MAX,
            LOGS_DIR "/%s%d.txt", arg_prefix, object_id) < 0)
  else
    HARDBUG(snprintf(object->my_printf_fname, MY_LINE_MAX,
            LOGS_DIR "/%s%d-%d-%d.txt", arg_prefix, object_id,
            my_mpi_globals.MY_MPIG_id_global,
            my_mpi_globals.MY_MPIG_nglobal) < 0)

  if ((object->my_printf_flog = fopen(object->my_printf_fname, "w")) == NULL)
  {
    fprintf(stderr, "could not fopen %s in mode 'w'\n",
            object->my_printf_fname);

    exit(EXIT_FAILURE);
  }

  time_t t = time(NULL);

  int nchar = fprintf(object->my_printf_flog,
    "log file opened at %s\n", ctime(&t));

  if (nchar < 0)
  {
    fprintf(stderr, "could not fprintf %s\n", object->my_printf_fname);

    exit(EXIT_FAILURE);
  }

  object->my_printf_fsize = nchar;

  strcpy(object->my_printf_buffer, "");

  object->my_printf_ibuffer = 0;

  object_id++;

  set_field(with_record, LOG_OBJECT_ID, &object_id);

  HARDBUG(compat_mutex_init(&(object->my_printf_mutex)) != 0)
}

void init_my_printf(void)
{
  if (opendir(LOGS_DIR) == NULL)
  {
    HARDBUG(compat_mkdir(LOGS_DIR) != 0)
    HARDBUG(opendir(LOGS_DIR) == NULL)
  }

  nlogs = 0;

  HARDBUG(compat_mutex_init(&my_printf_mutex) != 0)
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
      my_printf(test + itest, line);

    for (int ichar = 1; ichar <= nchar; ichar++)
    {
      snprintf(line, MY_LINE_MAX, " %d", ichar);

      for (int itest = 0; itest < NTEST; itest++)
        my_printf(test + itest, line);
    }
    for (int itest = 0; itest < NTEST; itest++)
      my_printf(test + itest, "\n");
  }
  for (int itest = 0; itest < NTEST; itest++)
    my_printf(test + itest, "");
}
