// SCU REVISION 8.1262 di 10 nov 2026 15:00:45 CET
#include "globals.h"

void construct_my_bstream(my_bstream_t *arg_with_my_bstream, char *arg_name)
{
  HARDBUG((arg_with_my_bstream->BS_file = fopen(arg_name, "r")) == NULL)

  HARDBUG((arg_with_my_bstream->BS_bfile =
             bsopen((bNread)fread, arg_with_my_bstream->BS_file)) == NULL)

  HARDBUG((arg_with_my_bstream->BS_bline = bfromcstr("")) == NULL)

  HARDBUG((arg_with_my_bstream->BS_newline = bfromcstr("\n")) == NULL)

  HARDBUG((arg_with_my_bstream->BS_empty = bfromcstr("")) == NULL)
}

int my_bstream_readln(my_bstream_t *arg_with_my_bstream, int remove_newline)
{
  int result = bsreadln(arg_with_my_bstream->BS_bline,
                        arg_with_my_bstream->BS_bfile,
                        (char)'\n');

  if (result != BSTR_OK)
    return (result);

  if (remove_newline)
    bfindreplace(arg_with_my_bstream->BS_bline,
                 arg_with_my_bstream->BS_newline,
                 arg_with_my_bstream->BS_empty,
                 0);

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
