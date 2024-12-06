//SCU REVISION 7.750 vr  6 dec 2024  8:31:49 CET
#include "globals.h"

#ifdef USE_HARDWARE_CRC32
#define POLYNOMIAL_CRC32 0x82F63B78UL
#else
#define POLYNOMIAL_CRC32 0xEDB88320UL
#endif

int zzzzzz_invocation = 0;

#define NFRAMES_MAX 256

void zzzzzz(char *file, const char *func, long line, char *error, int code)
{
  fprintf(stderr, "** %s **\n"
    " my_mpi_globals.MY_MPIG_id_global=%d\n"
    " my_mpi_globals.MY_MPIG_nglobal=%d\n"
    " pthread_self=%#lX\n"
    " file=%s\n"
    " func=%s\n"
    " line=%ld\n"
    " error=%s\n"
    " code=%d\n"
    " version=%s\n",
   code == 0 ? "OK" : "FATAL",
   my_mpi_globals.MY_MPIG_id_global,
   my_mpi_globals.MY_MPIG_nglobal,
   compat_pthread_self(),
   file, func, line, error, code, REVISION);

  if (zzzzzz_invocation > 1)
  {
    fprintf(stderr, "WARNING: zzzzzz_invocation=%d\n", zzzzzz_invocation);
  }
  else if (code != EXIT_SUCCESS)
  { 
    PRINTF("** %s **\n"
      " my_mpi_globals.MY_MPIG_id_global=%d\n"
      " my_mpi_globals.MY_MPIG_nglobal=%d\n"
      " pthread_self=%#lX\n"
      " file=%s\n"
      " func=%s\n"
      " line=%ld\n"
      " error=%s\n"
      " code=%d\n"
      " version=%s\n",
     code == 0 ? "OK" : "FATAL",
     my_mpi_globals.MY_MPIG_id_global,
     my_mpi_globals.MY_MPIG_nglobal,
     compat_pthread_self(),
     file, func, line, error, code, REVISION);

     //flush

     PRINTF("", NULL);
  }

#ifdef USE_OPENMPI
  if (my_mpi_globals.MY_MPIG_init)
  {
    if (code == EXIT_SUCCESS)
    {
      MPI_Finalize();
    }
    else
    {
      //issue when multiple processes call MPI_Abort

      MPI_Abort(my_mpi_globals.MY_MPIG_comm_global, code);
    }
  }
#endif
  exit(code);
}

int fexists(char *name)
{
  if (compat_access(name, F_OK) == 0)
    return(TRUE);
  else
    return(FALSE);
}

local int is_prime(i64_t n)
{
  if (n < 4) return(TRUE);

  if ((n % 2) == 0) return(FALSE);
  for (i64_t i = 3; (i * i) <= n; i += 2)
    if ((n % i) == 0) return(FALSE);

  return(TRUE);
}

i64_t first_prime_below(i64_t n)
{
  if (n < 4) return(n);

  if ((n % 2) == 0) --n;

  i64_t i;

  for (i = n; i > 3; i -=2)
    if (is_prime(i)) return(i);

  return(i);
}

static ui32_t crc_table32[256];
static ui64_t crc_table64[256];

void init_crc32(ui32_t *crc)
{
  *crc = 0xFFFFFFFF;
}

#ifdef USE_HARDWARE_CRC32
void update_crc32(ui32_t *crc, void *p, ui32_t n)
{
  BEGIN_BLOCK(__FUNC__)

  i8_t *a = (i8_t *) p;
  for (ui32_t i = 0; i < n; i++)
    *crc = _mm_crc32_u8(*crc, a[i]);

  END_BLOCK
}
#else
void update_crc32(ui32_t *crc, void *p, ui32_t n)
{
  BEGIN_BLOCK(__FUNC__)

  i8_t *a = (i8_t *) p;
  for (ui32_t i = 0; i < n; i++)
    *crc = crc_table32[(*crc ^ a[i]) & 0xFF] ^ (*crc >> 8);

  END_BLOCK
}
#endif

ui32_t return_crc32(void *a, ui32_t n)
{
  i8_t *b = (i8_t *) a;

  ui32_t crc = 0xFFFFFFFF;

  for (ui32_t i = 0; i < n; i++)
    crc = crc_table32[(crc ^ b[i]) & 0xFF] ^ (crc >> 8);

  return(~crc);
}

ui64_t return_crc64(void *p, ui64_t n, int flip)
{
  ui64_t crc = 0xFFFFFFFFFFFFFFFFULL;

  i8_t *b = (i8_t *) p;

  for (ui64_t i = 0; i < n; i++)
    crc = crc_table64[(crc ^ b[i]) & 0xFF] ^ (crc >> 8);

  //unfortanely all database checksums have used crc

  if (flip)
    return(~crc);
  else
    return(crc);
}

void init_utils(void)
{
  for (ui32_t i = 0; i < 256; i++)
  {
    ui32_t part = i;
    for (ui32_t j = 0; j < 8; j++)
    {
      if (part & 1)
        part = (part >> 1) ^ POLYNOMIAL_CRC32;
      else
        part >>= 1;
    }
    crc_table32[i] = part;
  }

  for (ui32_t i = 0; i < 256; i++)
  {
    ui64_t part = i;
    for (ui32_t j = 0; j < 8; j++)
    {
      if (part & 1)
        part = (part >> 1) ^ 0xC96C5795D7870F42ULL;
      else
        part >>= 1;
    }
    crc_table64[i] = part;
  }
}

void test_utils(void)
{
  ui32_t c;
  char *f = "The quick brown fox jumps over the lazy dog";
  char s[9];

  init_crc32(&c);

  update_crc32(&c, (i8_t *) f, strlen(f));

  c = ~c;

  HARDBUG(c != return_crc32(f, strlen(f)))

  (void) snprintf(s, 9, "%X", c);

#ifdef USE_HARDWARE_CRC32
  PRINTF("the HW crc32 of '%s' is %s hex\n", f, s);
  HARDBUG(strcmp(s, "22620404") != 0)
#else
  PRINTF("the SW crc32 of '%s' is %s hex\n", f, s);
  HARDBUG(strcmp(s, "414FA339") != 0)
#endif
}

void file2cjson(char *arg_name, cJSON **arg_json)
{
  FILE *fjson;

  HARDBUG((fjson = fopen(arg_name, "r")) == NULL)

  struct bStream* bjson;

  HARDBUG((bjson = bsopen((bNread) fread, fjson)) == NULL)

  BSTRING(string)

  while (bsreadlna(string, bjson, (char) '\n') == BSTR_OK)
  ;
  
  HARDBUG(bsclose(bjson) == NULL)

  FCLOSE(fjson)

  BSTRING(newline)

  HARDBUG(bassigncstr(newline, "\n") != BSTR_OK)

  BSTRING(space)

  HARDBUG(bassigncstr(newline, " ") != BSTR_OK)

  bfindreplace(string, newline, space, 0);
  
  if ((*arg_json = cJSON_Parse(bdata(string))) == NULL)
  {
    const char *error = cJSON_GetErrorPtr();

    PRINTF("error %s loading %s\n", error, arg_name);

    FATAL("cJSON error", EXIT_FAILURE)
  }

  BDESTROY(space)

  BDESTROY(newline)

  BDESTROY(string)
}

cJSON *cJSON_FindItemInObject(cJSON *object, char *name)
{
  cJSON *result = NULL;

  cJSON *entry;

  cJSON_ArrayForEach(entry, object)
  {
    if (compat_strcasecmp(entry->string, name) == 0)
    {
      result = entry;
      break;
    }
  }

  return(result);
}
