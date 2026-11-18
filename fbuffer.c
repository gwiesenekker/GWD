// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "fbuffer.h"

#include "bstrlib.h"
#include "common.h"
#include "my_malloc.h"
#include "my_printf.h"

enum
{
  NFBUFFER = (4 * MBYTE)
};

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
  int result;

  bvformata(result, arg_with_fbuffer->FB_bstring, arg_fmt, arg_fmt);

  HARDBUG(result == BSTR_ERR)

  bstring bbuffer = arg_with_fbuffer->FB_bstring;

  if (bchar(bbuffer, blength(bbuffer) - 1) == '\n')
  {
    HARDBUG((arg_with_fbuffer->FB_nfbuffer + blength(bbuffer)) >=
            (2 * NFBUFFER))

    memcpy((int8_t *)arg_with_fbuffer->FB_fbuffer +
             arg_with_fbuffer->FB_nfbuffer,
           bdata(bbuffer),
           blength(bbuffer));

    arg_with_fbuffer->FB_nfbuffer += blength(bbuffer);

    HARDBUG(bassigncstr(arg_with_fbuffer->FB_bstring, "") == BSTR_ERR)

    flush_fbuffer(arg_with_fbuffer, NFBUFFER);
  }
}

void append_fbuffer_bin(fbuffer_t *arg_with_fbuffer,
                        const void *arg_data,
                        size_t arg_nbytes)
{
  if (arg_data == NULL)
  {
    flush_fbuffer(arg_with_fbuffer, NFBUFFER);

    return;
  }

  HARDBUG((arg_with_fbuffer->FB_nfbuffer + arg_nbytes) >= (2 * NFBUFFER))

  memcpy((int8_t *)arg_with_fbuffer->FB_fbuffer + arg_with_fbuffer->FB_nfbuffer,
         arg_data,
         arg_nbytes);

  arg_with_fbuffer->FB_nfbuffer += arg_nbytes;
}

bstring return_fbuffer_bname(const fbuffer_t *arg_with_fbuffer)
{
  return (arg_with_fbuffer->FB_bname);
}

void construct_fbuffer(fbuffer_t *arg_with_fbuffer,
                       const_bstring arg_bname,
                       const char *arg_suffix,
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
