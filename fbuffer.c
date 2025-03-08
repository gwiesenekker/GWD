//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#include "globals.h"

#define NFBUFFER (4 * MBYTE)

void flush_fbuffer(fbuffer_t *self, int arg_nfbuffer)
{
  fbuffer_t *object = self;

  if (blength(object->FB_bbuffer) > arg_nfbuffer)
  {
    int fd = compat_lock_file(bdata(object->FB_bname));

    HARDBUG(fd == -1)
  
    HARDBUG(compat_write(fd, bdata(object->FB_bbuffer),
                         blength(object->FB_bbuffer)) !=
            blength(object->FB_bbuffer))
  
    compat_unlock_file(fd);
  
    HARDBUG(bassigncstr(object->FB_bbuffer, "") == BSTR_ERR)
  }
}

void append_fbuffer(fbuffer_t *self, const char *arg_fmt, ...)
{
  fbuffer_t *object = self;

  int ret;

  bvformata(ret, object->FB_bbuffer, arg_fmt, arg_fmt);

  HARDBUG(ret == BSTR_ERR)

  bstring bbuffer = object->FB_bbuffer;

  if (bchar(bbuffer, blength(bbuffer) - 1) == '\n')
    flush_fbuffer(object, NFBUFFER);
}

bstring return_fbuffer_bname(fbuffer_t *self)
{
  fbuffer_t *object = self;

  return(object->FB_bname);
}

void construct_fbuffer(fbuffer_t *self, bstring arg_bname, char *arg_suffix,
  int arg_remove)
{
  fbuffer_t *object = self;

  HARDBUG((object->FB_bname = bfromcstr("")) == NULL)

  if (arg_suffix == NULL)
  {
    HARDBUG(bassign(object->FB_bname, arg_bname) == BSTR_ERR)
  }
  else
  {
    int pdot;

    if ((pdot = bstrrchr(arg_bname, '.')) == BSTR_ERR) 
    {
      HARDBUG(bformata(object->FB_bname, "%s%s",
                       bdata(arg_bname), arg_suffix) == BSTR_ERR)
    }
    else
    {
      BSTRING(bprefix)
  
      HARDBUG(bassignmidstr(bprefix, arg_bname, 0, pdot) == BSTR_ERR)
    
      HARDBUG(bformata(object->FB_bname, "%s%s",
                       bdata(bprefix), arg_suffix) == BSTR_ERR)

      BDESTROY(bprefix)
    }
  }

  HARDBUG((object->FB_bbuffer = bfromcstr("")) == NULL)

  if (arg_remove) remove(bdata(object->FB_bname));
}

void test_fbuffer(void)
{
  fbuffer_t test;

  BSTRING(btest)
 
  HARDBUG(bassigncstr(btest, "fbuffer.txt"))

  construct_fbuffer(&test, btest, NULL, TRUE);
 
  for (int itest = 0; itest < 1000000; itest++)
  {
    append_fbuffer(&test, "itest=%d\n", itest);
  }

  flush_fbuffer(&test, 0);

  BDESTROY(btest)
}
