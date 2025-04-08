//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#include "globals.h"

#define LOG_PREFIX_ID "LOG_prefix_id"
#define LOG_OBJECT_ID "LOG_object_id"

#define NLOGS 2

#define NROTATE 9

#define MAX_SIZE (4 * MBYTE)

#define LOGS_DIR "logs"

//this is more a test for my dynamic cJSON record_t
local int nlogs = INVALID;
record_t logs[NLOGS];

local my_mutex_t my_printf_mutex;

void my_printf(void *self, char *arg_format, ...)
{
  my_printf_t *object = self;

  if (object == NULL)
  {
    //stdout

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

  //flush 

  if (compat_strcasecmp(arg_format, "") == 0)
  {
    HARDBUG(compat_mutex_lock(&(object->my_printf_mutex)) != 0)

    if (fflush(object->my_printf_flog) != 0)
    {
      fprintf(stderr, "fflush(%s) != 0\n",
                      bdata(object->my_printf_bname));

      exit(EXIT_FAILURE);
    }

    HARDBUG(compat_mutex_unlock(&(object->my_printf_mutex)) != 0)

    return;
  }

  HARDBUG(compat_mutex_lock(&(object->my_printf_mutex)) != 0)

  int ret;

  bvformata(ret, object->my_printf_bbuffer, arg_format, arg_format);

  HARDBUG(ret == BSTR_ERR)

  bstring bline;

  HARDBUG((bline = bfromcstr("")) == NULL)

  int ichar = 0;

  while(ichar < blength(object->my_printf_bbuffer))
  {
    if (bchare(object->my_printf_bbuffer, ichar, '\0') != '\n')
    {
      ichar++;

      continue;
    }

    //timestamp

    char time_stamp[MY_LINE_MAX];
    
    time_t t = time(NULL);

    HARDBUG(strftime(time_stamp, MY_LINE_MAX, "%H:%M:%S-%d/%m/%Y",
                     localtime(&t)) == 0)
   
    if (object->my_printf_fsize > MAX_SIZE)
    {
      fprintf(object->my_printf_flog,
              "\n%s@ PTHREAD_SELF=%#lX"
              " LOG FILE EXCEEDS MAXIMUM SIZE AND WILL BE ROTATED\n",
              time_stamp, compat_pthread_self());
  
      if (fclose(object->my_printf_flog) != 0)
      {
        fprintf(stderr, "fclose(%s) != 0\n",
                        bdata(object->my_printf_bname));

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
          HARDBUG(bformata(bold_path, "%s-%d",
                           bdata(object->my_printf_bname),
                           irotate - 1) == BSTR_ERR)
        }
        else
        {
          HARDBUG(bformata(bold_path, "%s",
                           bdata(object->my_printf_bname)) == BSTR_ERR)
        }

        btrunc(bnew_path, 0);

        HARDBUG(bformata(bnew_path, "%s-%d",
                         bdata(object->my_printf_bname),
                         irotate) == BSTR_ERR)
  
        //ignore return value of remove as file might not exist

        (void) remove(bdata(bnew_path));
  
        //ignore return value of rename as file might not exist

        (void) rename(bdata(bold_path), bdata(bnew_path));
      }
    
      HARDBUG(bdestroy(bnew_path) == BSTR_ERR)

      HARDBUG(bdestroy(bold_path) == BSTR_ERR)
    
      if ((object->my_printf_flog =
           fopen(bdata(object->my_printf_bname), "w")) == NULL)
      {
        fprintf(stderr, "fopen(%s, 'w') == NULL\n",
                        bdata(object->my_printf_bname));
  
        exit(EXIT_FAILURE);
      }

      int nchar = fprintf(object->my_printf_flog,
                          "%s@ PTHREAD_SELF=%#lX"
                          " LOG FILE ROTATED AND RE-OPENED\n\n",
                          time_stamp, compat_pthread_self());
    
      if (nchar < 0)
      {
        fprintf(stderr, "fprintf(%s, ...) < 0\n",
                        bdata(object->my_printf_bname));

        exit(EXIT_FAILURE);
      }
  
      object->my_printf_fsize = nchar;
    }

    HARDBUG(bassignmidstr(bline, object->my_printf_bbuffer, 0, ichar + 1) ==
            BSTR_ERR)

    int nchar = fprintf(object->my_printf_flog, "%s@ %s",
                        time_stamp, bdata(bline));

    if (nchar < 0)
    {
      fprintf(stderr, "fprintf(%s, ...) < 0\n",
                      bdata(object->my_printf_bname));

      exit(EXIT_FAILURE);
    }

#ifdef DEBUG
    if (fflush(object->my_printf_flog) != 0)
    {
      fprintf(stderr, "fflush(%s) != 0\n", 
                      bdata(object->my_printf_bname));

      exit(EXIT_FAILURE);
    }
#else
    if ((my_mpi_globals.MY_MPIG_id_global == 0) or
        (my_mpi_globals.MY_MPIG_id_global == INVALID) or 
        (my_mpi_globals.MY_MPIG_id_slave == 0))
    {
      if (fflush(object->my_printf_flog) != 0)
      {
        fprintf(stderr, "fflush(%s) != 0\n", 
                        bdata(object->my_printf_bname));

        exit(EXIT_FAILURE);
      }
    }
#endif

    object->my_printf_fsize += nchar;
  
    //also print to stdout if needed

    if (!options.hub and object->my_printf2stdout and
        ((my_mpi_globals.MY_MPIG_id_global == 0) or
         (my_mpi_globals.MY_MPIG_id_global == INVALID)))
    {
      nchar = fprintf(stdout, "%s", bdata(bline));

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
  
    bdelete(object->my_printf_bbuffer, 0, ichar + 1);

    ichar = 0;
  }

  HARDBUG(bdestroy(bline) == BSTR_ERR)

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

  HARDBUG((object->my_printf_bname = bfromcstr("")) == NULL)

  if (my_mpi_globals.MY_MPIG_nglobal <= 1)
    HARDBUG(bformata(object->my_printf_bname,
                     LOGS_DIR "/%s%d.txt", arg_prefix, object_id) == BSTR_ERR)
  else
    HARDBUG(bformata(object->my_printf_bname,
                     LOGS_DIR "/%s%d-%d-%d.txt", arg_prefix, object_id,
                     my_mpi_globals.MY_MPIG_id_global,
                     my_mpi_globals.MY_MPIG_nglobal) == BSTR_ERR)

  if ((object->my_printf_flog =
       fopen(bdata(object->my_printf_bname), "w")) == NULL)
  {
    fprintf(stderr, "fopen(%s, 'w') == NULL\n",
                    bdata(object->my_printf_bname));

    exit(EXIT_FAILURE);
  }


  char time_stamp[MY_LINE_MAX];
    
  time_t t = time(NULL);

  HARDBUG(strftime(time_stamp, MY_LINE_MAX, "%H:%M:%S-%d/%m/%Y",
                   localtime(&t)) == 0)

  int nchar = fprintf(object->my_printf_flog,
                      "%s@ PTHREAD_SELF=%#lX LOG FILE OPENED\n\n",
                      time_stamp, compat_pthread_self());

  if (nchar < 0)
  {
    fprintf(stderr, "fprintf(%s, ...) < 0\n",
                    bdata(object->my_printf_bname));

    exit(EXIT_FAILURE);
  }

  object->my_printf_fsize = nchar;

  HARDBUG((object->my_printf_bbuffer = bfromcstr("")) == NULL)

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
