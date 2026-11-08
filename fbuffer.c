// SCU REVISION 8.1260 zo  8 nov 2026 17:20:46 CET
#include "globals.h"

#define NFBUFFER (4 * MBYTE)

void flush_fbuffer(fbuffer_t *arg_with_fbuffer, int arg_nfbuffer)
{
  if (arg_with_fbuffer->FB_nfbuffer > arg_nfbuffer)
  {
    int fd = compat_lock_file(bdata(arg_with_fbuffer->FB_bname), TRUE);

    HARDBUG(fd == -1)

    HARDBUG(compat_write(fd,
                         arg_with_fbuffer->FB_fbuffer,
                         arg_with_fbuffer->FB_nfbuffer) !=
            arg_with_fbuffer->FB_nfbuffer)

    HARDBUG(compat_unlock_file(fd) != 0)

    arg_with_fbuffer->FB_nfbuffer = 0;
  }
}

void append_fbuffer_fmt(fbuffer_t *arg_with_fbuffer, const char *arg_fmt, ...)
{
  int ret;

  bvformata(ret, arg_with_fbuffer->FB_bstring, arg_fmt, arg_fmt);

  HARDBUG(ret == BSTR_ERR)

  bstring b = arg_with_fbuffer->FB_bstring;

  if (bchar(b, blength(b) - 1) == '\n')
  {
    HARDBUG((arg_with_fbuffer->FB_nfbuffer + blength(b)) >= (2 * NFBUFFER))

    memcpy((i8_t *)arg_with_fbuffer->FB_fbuffer + arg_with_fbuffer->FB_nfbuffer,
           bdata(b),
           blength(b));

    arg_with_fbuffer->FB_nfbuffer += blength(b);

    HARDBUG(bassigncstr(arg_with_fbuffer->FB_bstring, "") == BSTR_ERR)

    flush_fbuffer(arg_with_fbuffer, NFBUFFER);
  }
}

void append_fbuffer_bin(fbuffer_t *arg_with_fbuffer,
                        void *arg_data,
                        size_t ldata)
{
  if (arg_data == NULL)
  {
    flush_fbuffer(arg_with_fbuffer, NFBUFFER);

    return;
  }

  HARDBUG((arg_with_fbuffer->FB_nfbuffer + ldata) >= (2 * NFBUFFER))

  memcpy((i8_t *)arg_with_fbuffer->FB_fbuffer + arg_with_fbuffer->FB_nfbuffer,
         arg_data,
         ldata);

  arg_with_fbuffer->FB_nfbuffer += ldata;
}

bstring return_fbuffer_bname(fbuffer_t *arg_with_fbuffer)
{
  return (arg_with_fbuffer->FB_bname);
}

void construct_fbuffer(fbuffer_t *arg_with_fbuffer,
                       bstring arg_bname,
                       char *arg_suffix,
                       int arg_remove)
{
  HARDBUG((arg_with_fbuffer->FB_bname = bfromcstr("")) == NULL)

  if (arg_suffix == NULL)
  {
    HARDBUG(bassign(arg_with_fbuffer->FB_bname, arg_bname) == BSTR_ERR)
  }
  else
  {
    int pdot;

    if ((pdot = bstrrchr(arg_bname, '.')) == BSTR_ERR)
    {
      HARDBUG(bformata(arg_with_fbuffer->FB_bname,
                       "%s%s",
                       bdata(arg_bname),
                       arg_suffix) == BSTR_ERR)
    }
    else
    {
      BSTRING(bprefix)

      HARDBUG(bassignmidstr(bprefix, arg_bname, 0, pdot) == BSTR_ERR)

      HARDBUG(bformata(arg_with_fbuffer->FB_bname,
                       "%s%s",
                       bdata(bprefix),
                       arg_suffix) == BSTR_ERR)

      BDESTROY(bprefix)
    }
  }

  arg_with_fbuffer->FB_nfbuffer = 0;
  MY_MALLOC_VOID(arg_with_fbuffer->FB_fbuffer, 2 * NFBUFFER)

  HARDBUG((arg_with_fbuffer->FB_bstring = bfromcstr("")) == NULL)

  PRINTF("debug=%s\n", bdata(arg_with_fbuffer->FB_bname));

  if (arg_remove)
    remove(bdata(arg_with_fbuffer->FB_bname));
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
