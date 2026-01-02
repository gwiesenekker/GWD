//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

void construct_my_bstream(my_bstream_t *object, char *arg_name)
{
  HARDBUG((object->BS_file = fopen(arg_name, "r")) == NULL)

  HARDBUG((object->BS_bfile = bsopen((bNread) fread, object->BS_file)) == NULL)

  HARDBUG((object->BS_bline = bfromcstr("")) == NULL)

  HARDBUG((object->BS_newline = bfromcstr("\n")) == NULL)

  HARDBUG((object->BS_empty = bfromcstr("")) == NULL)
}

int my_bstream_readln(my_bstream_t *object, int remove_newline)
{
  int result = bsreadln(object->BS_bline, object->BS_bfile, (char) '\n');

  if (result != BSTR_OK) return(result);

  if (remove_newline) bfindreplace(object->BS_bline,
                                   object->BS_newline, object->BS_empty, 0);

  return(result);
}

void destroy_my_bstream(my_bstream_t *object)
{
  HARDBUG(bdestroy(object->BS_empty) == BSTR_ERR)

  HARDBUG(bdestroy(object->BS_newline) == BSTR_ERR)

  HARDBUG(bdestroy(object->BS_bline) == BSTR_ERR)

  HARDBUG(bsclose(object->BS_bfile) == NULL)

  FCLOSE(object->BS_file)
}
