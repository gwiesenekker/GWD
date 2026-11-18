// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "globals.h"

// fread adapter with the exact bstrlib callback signature.
size_t
my_bread(void *arg_buffer, size_t arg_size, size_t arg_count, void *arg_file)
{
  return (fread(arg_buffer, arg_size, arg_count, (FILE *)arg_file));
}

local int my_bgetc(void *arg_file) { return (fgetc((FILE *)arg_file)); }

void construct_my_bstream(my_bstream_t *arg_with_my_bstream,
                          const char *arg_name)
{
  HARDBUG((arg_with_my_bstream->BS_file = fopen(arg_name, "r")) == NULL)

  HARDBUG((arg_with_my_bstream->BS_bfile =
             bsopen(my_bread, arg_with_my_bstream->BS_file)) == NULL)

  HARDBUG((arg_with_my_bstream->BS_bline = bfromcstr("")) == NULL)

  HARDBUG((arg_with_my_bstream->BS_newline = bfromcstr("\n")) == NULL)

  HARDBUG((arg_with_my_bstream->BS_empty = bfromcstr("")) == NULL)
}

int my_bstream_readln(my_bstream_t *arg_with_my_bstream, int arg_remove_newline)
{
  int result = bsreadln(arg_with_my_bstream->BS_bline,
                        arg_with_my_bstream->BS_bfile,
                        (char)'\n');

  HARDBUG(ferror(arg_with_my_bstream->BS_file))

  if (result != BSTR_OK)
    return (result);

  if (arg_remove_newline)
    HARDBUG(bfindreplace(arg_with_my_bstream->BS_bline,
                         arg_with_my_bstream->BS_newline,
                         arg_with_my_bstream->BS_empty,
                         0) == BSTR_ERR)

  return (result);
}

void destroy_my_bstream(my_bstream_t *arg_with_my_bstream)
{
  HARDBUG(bdestroy(arg_with_my_bstream->BS_empty) == BSTR_ERR)

  HARDBUG(bdestroy(arg_with_my_bstream->BS_newline) == BSTR_ERR)

  HARDBUG(bdestroy(arg_with_my_bstream->BS_bline) == BSTR_ERR)

  HARDBUG(bsclose(arg_with_my_bstream->BS_bfile) == NULL)

  FCLOSE(arg_with_my_bstream->BS_file)
}

// Grow the destination while retaining bstring's length and trailing NUL.
void my_bstrftime(bstring arg_string, const char *arg_format, time_t arg_time)
{
  int size = 64;

  HARDBUG(*arg_format == '\0')

  while (TRUE)
  {
    HARDBUG(balloc(arg_string, size) == BSTR_ERR)

    size_t length =
      strftime(bdata(arg_string), size, arg_format, localtime(&arg_time));

    if (length > 0)
    {
      arg_string->slen = (int)length;

      return;
    }

    HARDBUG(size > INT_MAX / 2)

    size *= 2;
  }
}

int my_breadline(bstring arg_line, FILE *arg_file)
{
  int result = bassigngets(arg_line, my_bgetc, arg_file, '\n');

  HARDBUG(result == BSTR_ERR)
  HARDBUG(ferror(arg_file))

  return (result == 0);
}

void my_bcpu_flags(bstring arg_flags)
{
  // The compat API still requires its own documented output capacity.
  HARDBUG(balloc(arg_flags, COMPAT_LINE_MAX) == BSTR_ERR)
  HARDBUG(return_cpu_flags(bdata(arg_flags)) != 0)

  arg_flags->slen = (int)strlen(bdata(arg_flags));
}
