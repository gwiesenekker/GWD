//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"


#define MY_PRINTF_MAX       64

#define NROTATE 9
#define MAX_SIZE (4 * MBYTE)

char log_infix[MY_LINE_MAX];

#define LOGS_DIR "logs"

class_t *my_printf_class;

void my_printf(my_printf_t *object, char *arg_format, ...)
{
  va_list ap;

  va_start(ap, arg_format);

  if (object == NULL)
  {
    vprintf(arg_format, ap);

    return;
  }

  if (my_strcasecmp(arg_format, "") == 0)
  {
    HARDBUG(my_mutex_lock(&(object->my_printf_mutex)) != 0)

    if (fflush(object->my_printf_flog) != 0)
    {
      fprintf(stderr, "could not fflush %s\n", object->my_printf_fname);
      exit(EXIT_FAILURE);
    }

    HARDBUG(my_mutex_unlock(&(object->my_printf_mutex)) != 0)

    return;
  }

  HARDBUG(my_mutex_lock(&(object->my_printf_mutex)) != 0)

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
  
    if (!options.hub and (object->object_id == 0) and
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

  HARDBUG(my_mutex_unlock(&(object->my_printf_mutex)) != 0)
}

my_printf_t *construct_my_printf(int arg_id)
{
  my_printf_t *object;

  MALLOC(object, my_printf_t, 1)

  object->object_id =
    my_printf_class->register_object(my_printf_class, object);

  if (arg_id == INVALID)
  {
    if (my_mpi_globals.MY_MPIG_nglobal <= 1)
      HARDBUG(snprintf(object->my_printf_fname, MY_LINE_MAX, 
              LOGS_DIR "/stdout%s.txt", log_infix) < 0)
    else
      HARDBUG(snprintf(object->my_printf_fname, MY_LINE_MAX,
              LOGS_DIR "/stdout%s;%d;%d.txt", log_infix,
              my_mpi_globals.MY_MPIG_id_global,
              my_mpi_globals.MY_MPIG_nglobal) < 0)
  }
  else
  {
    if (my_mpi_globals.MY_MPIG_nglobal <= 1)
      HARDBUG(snprintf(object->my_printf_fname, MY_LINE_MAX,
              LOGS_DIR "/log%s-%d.txt", log_infix, arg_id) < 0)
    else
      HARDBUG(snprintf(object->my_printf_fname, MY_LINE_MAX,
              LOGS_DIR "/log%s-%d;%d;%d.txt", log_infix, arg_id,
              my_mpi_globals.MY_MPIG_id_global,
              my_mpi_globals.MY_MPIG_nglobal) < 0)
  }

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

  HARDBUG(my_mutex_init(&(object->my_printf_mutex)) != 0)

  return(object);
}

local void destroy_my_printf(void *self)
{
  my_printf_t *object = self;

  //flush my_printf_buffer

  if (object->my_printf_ibuffer != 0)
  {
    char stamp[MY_LINE_MAX];
  
    HARDBUG(object->my_printf_ibuffer >= MY_LINE_MAX)

    object->my_printf_buffer[object->my_printf_ibuffer] = '\0';

    time_t t = time(NULL);
    (void) strftime(stamp, MY_LINE_MAX, "%H:%M:%S-%d/%m/%Y", localtime(&t));

    int nchar = fprintf(object->my_printf_flog, "%s@ %s\n",
      stamp, object->my_printf_buffer);

    if (nchar < 0)
    {
      fprintf(stderr, "could not fprintf %s\n", object->my_printf_fname);
      exit(EXIT_FAILURE);
    }
    if (!options.hub and (object->object_id == 0) and
        ((my_mpi_globals.MY_MPIG_id_global == 0) or
         (my_mpi_globals.MY_MPIG_id_global == INVALID)))
    {
      nchar = fprintf(stdout, "%s\n", object->my_printf_buffer);
      if (nchar < 0)
      {
        fprintf(stderr, "could not fprintf stdout\n");
        exit(EXIT_FAILURE);
      }
    }
    strcpy(object->my_printf_buffer, "");
    object->my_printf_ibuffer = 0;
  }

  HARDBUG(object->my_printf_ibuffer != 0)

  if (fclose(object->my_printf_flog) == EOF)
  {
    fprintf(stderr, "could not fclose %s\n", object->my_printf_fname);
    exit(EXIT_FAILURE);
  }

  my_printf_class->deregister_object(my_printf_class, object);

  FREE_AND_NULL(object)
}

void init_my_printf_class(void)
{
  if (opendir(LOGS_DIR) == NULL)
  {
    HARDBUG(my_mkdir(LOGS_DIR) != 0)
    HARDBUG(opendir(LOGS_DIR) == NULL)
  }

  my_printf_class = init_class(MY_PRINTF_MAX, NULL, destroy_my_printf, NULL);
}

void fin_my_printf_class(void)
{
}

#define NTEST 4
#define NLINES 1000

void test_my_printf_class(void)
{
  my_printf_t *test[NTEST];

  for (int itest = 0; itest < NTEST; itest++)
    test[itest] = construct_my_printf(itest);

  for (int iline = 1; iline <= NLINES; iline++)
  {
    int nchar = (iline - 1) % 10 + 1;

    char line[MY_LINE_MAX];

    snprintf(line, MY_LINE_MAX, "%d:", iline);

    for (int itest = 0; itest < NTEST; itest++)
      my_printf(test[itest], line);

    for (int ichar = 1; ichar <= nchar; ichar++)
    {
      snprintf(line, MY_LINE_MAX, " %d", ichar);

      for (int itest = 0; itest < NTEST; itest++)
        my_printf(test[itest], line);
    }
    for (int itest = 0; itest < NTEST; itest++)
      my_printf(test[itest], "\n");
  }
  for (int itest = 0; itest < NTEST; itest++)
    my_printf_class->objects_dtor(test[itest]);
}
