//SCU REVISION 7.902 di 26 aug 2025  4:15:00 CEST
#include "globals.h"

#define NFBUFFER (4 * MBYTE)

void flush_fbuffer(fbuffer_t *self, int arg_nfbuffer)
{
  fbuffer_t *object = self;

  if (object->FB_nfbuffer > arg_nfbuffer)
  {
    int fd = compat_lock_file(bdata(object->FB_bname));

    HARDBUG(fd == -1)
  
    HARDBUG(compat_write(fd, object->FB_fbuffer, object->FB_nfbuffer) !=
            object->FB_nfbuffer)
  
    compat_unlock_file(fd);
  
    object->FB_nfbuffer = 0;
  }
}

void append_fbuffer_fmt(fbuffer_t *self, const char *arg_fmt, ...)
{
  fbuffer_t *object = self;

  int ret;

  bvformata(ret, object->FB_bstring, arg_fmt, arg_fmt);

  HARDBUG(ret == BSTR_ERR)

  bstring bstring = object->FB_bstring;

  if (bchar(bstring, blength(bstring) - 1) == '\n')
  {
    HARDBUG((object->FB_nfbuffer + blength(bstring)) >= (2 * NFBUFFER))

    memcpy((i8_t *) object->FB_fbuffer + object->FB_nfbuffer,
           bdata(bstring), blength(bstring));

    object->FB_nfbuffer += blength(bstring);

    HARDBUG(bassigncstr(object->FB_bstring, "") == BSTR_ERR)

    flush_fbuffer(object, NFBUFFER);
  }
}

void append_fbuffer_bin(fbuffer_t *self, void *arg_data, size_t ldata)
{
  fbuffer_t *object = self;

  if (arg_data == NULL)
  {
    flush_fbuffer(object, NFBUFFER);

    return;
  }

  HARDBUG((object->FB_nfbuffer + ldata) >= (2 * NFBUFFER))

  memcpy((i8_t *) object->FB_fbuffer + object->FB_nfbuffer, arg_data, ldata);

  object->FB_nfbuffer += ldata;
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

  object->FB_nfbuffer = 0;
  MY_MALLOC_VOID(object->FB_fbuffer, 2 * NFBUFFER)

  HARDBUG((object->FB_bstring = bfromcstr("")) == NULL)

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
    append_fbuffer_fmt(&test, "itest=%d\n", itest);
  }

  flush_fbuffer(&test, 0);

  BDESTROY(btest)
}
