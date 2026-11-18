// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#include "utils.h"

#include "bstrlib.h"
#include "cJSON.h"
#include "common.h"
#include "main.h"
#include "my_bstreams.h"
#include "my_malloc.h"
#include "my_mpi.h"
#include "my_printf.h"
#include "profile.h"
#include "revision.h"

#if COMPAT_ARCH == COMPAT_ARCH_X86_64
#define POLYNOMIAL_CRC32 0x82F63B78UL
#else
#define POLYNOMIAL_CRC32 0xEDB88320UL
#endif

int zzzzzz_invocation = 0;

_Noreturn void zzzzzz(const char *arg_file,
                      const char *arg_func,
                      long arg_line,
                      const char *arg_error,
                      int arg_code)
{
  fprintf(stderr,
          "** %s **\n"
          " my_mpi_globals.MMG_global_id=%d\n"
          " my_mpi_globals.MMG_nglobal=%d\n"
          " pthread_self=%#lX\n"
          " file=%s\n"
          " func=%s\n"
          " line=%ld\n"
          " error=%s\n"
          " code=%d\n"
          " version=%s\n",
          arg_code == 0 ? "OK" : "FATAL",
          my_mpi_globals.MMG_global_id,
          my_mpi_globals.MMG_nglobal,
          compat_pthread_self(),
          arg_file,
          arg_func,
          arg_line,
          arg_error,
          arg_code,
          REVISION);

  if (zzzzzz_invocation > 1)
  {
    fprintf(stderr, "WARNING: zzzzzz_invocation=%d\n", zzzzzz_invocation);
  }
  else if (arg_code != EXIT_SUCCESS)
  {
    print_my_names(FALSE);

    PRINTF("** %s **\n"
           " my_mpi_globals.MMG_global_id=%d\n"
           " my_mpi_globals.MMG_nglobal=%d\n"
           " pthread_self=%#lX\n"
           " file=%s\n"
           " func=%s\n"
           " line=%ld\n"
           " error=%s\n"
           " code=%d\n"
           " version=%s\n$",
           arg_code == 0 ? "OK" : "FATAL",
           my_mpi_globals.MMG_global_id,
           my_mpi_globals.MMG_nglobal,
           compat_pthread_self(),
           arg_file,
           arg_func,
           arg_line,
           arg_error,
           arg_code,
           REVISION);

    if (options.hub)
      PRINTF("error message=%s$\n", arg_error);
  }

  if (my_mpi_globals.MMG_init)
  {
    if (arg_code == EXIT_SUCCESS)
    {
      my_mpi_finalize();
    }
    else
    {
      // issue when multiple processes call my_mpi_abort

      my_mpi_abort(my_mpi_globals.MMG_global_comm, arg_code);
    }
  }
  exit(arg_code);
}

int fexists(const char *arg_name)
{
  if (compat_access(arg_name, F_OK) == 0)
    return (TRUE);
  else
    return (FALSE);
}

local int is_prime(my_int64_t arg_number)
{
  if (arg_number < 4)
    return (TRUE);

  if ((arg_number % 2) == 0)
    return (FALSE);

  for (my_int64_t divisor = 3; (divisor * divisor) <= arg_number; divisor += 2)
    if ((arg_number % divisor) == 0)
      return (FALSE);

  return (TRUE);
}

my_int64_t first_prime_below(my_int64_t arg_number)
{
  if (arg_number < 4)
    return (arg_number);

  if ((arg_number % 2) == 0)
    --arg_number;

  my_int64_t candidate;

  for (candidate = arg_number; candidate > 3; candidate -= 2)
    if (is_prime(candidate))
      return (candidate);

  return (candidate);
}

static uint32_t crc_table32[256];
static my_uint64_t crc_table64[256];

void init_crc32(uint32_t *arg_crc) { *arg_crc = 0xFFFFFFFF; }

void update_crc32(uint32_t *arg_crc, const void *arg_data, uint32_t arg_nbytes)
{
  BEGIN_BLOCK(__FUNC__)

  const int8_t *bytes = (const int8_t *)arg_data;

  for (uint32_t ibyte = 0; ibyte < arg_nbytes; ibyte++)
    *arg_crc = crc_table32[(*arg_crc ^ bytes[ibyte]) & 0xFF] ^ (*arg_crc >> 8);

  END_BLOCK
}

uint32_t return_crc32(const void *arg_data, uint32_t arg_nbytes)
{
  const int8_t *bytes = (const int8_t *)arg_data;

  uint32_t crc = 0xFFFFFFFF;

  for (uint32_t ibyte = 0; ibyte < arg_nbytes; ibyte++)
    crc = crc_table32[(crc ^ bytes[ibyte]) & 0xFF] ^ (crc >> 8);

  return (~crc);
}

my_uint64_t
return_crc64(const void *arg_data, my_uint64_t arg_nbytes, int arg_flip)
{
  my_uint64_t crc = 0xFFFFFFFFFFFFFFFFULL;

  const int8_t *bytes = (const int8_t *)arg_data;

  for (my_uint64_t ibyte = 0; ibyte < arg_nbytes; ibyte++)
    crc = crc_table64[(crc ^ bytes[ibyte]) & 0xFF] ^ (crc >> 8);

  // unfortanely all database checksums have used crc

  if (arg_flip)
    return (~crc);
  else
    return (crc);
}

void init_utils(void)
{
  for (uint32_t ibyte = 0; ibyte < 256; ibyte++)
  {
    uint32_t part = ibyte;

    for (uint32_t ibit = 0; ibit < 8; ibit++)
    {
      if (part & 1)
        part = (part >> 1) ^ POLYNOMIAL_CRC32;
      else
        part >>= 1;
    }
    crc_table32[ibyte] = part;
  }

  for (uint32_t ibyte = 0; ibyte < 256; ibyte++)
  {
    my_uint64_t part = ibyte;

    for (uint32_t ibit = 0; ibit < 8; ibit++)
    {
      if (part & 1)
        part = (part >> 1) ^ 0xC96C5795D7870F42ULL;
      else
        part >>= 1;
    }
    crc_table64[ibyte] = part;
  }
}

void test_utils(void)
{
  uint32_t checksum;
  char *text = "The quick brown fox jumps over the lazy dog";
  BSTRING(bchecksum)

  init_crc32(&checksum);

  update_crc32(&checksum, (int8_t *)text, strlen(text));

  checksum = ~checksum;

  HARDBUG(checksum != return_crc32(text, strlen(text)))

  HARDBUG(bassignformat(bchecksum, "%#X", checksum) == BSTR_ERR)

  PRINTF("the crc32 of '%s' is %s hex\n", text, bdata(bchecksum));

#if POLYNOMIAL_CRC32 == 0x82F63B78UL
  HARDBUG(strcmp(bdata(bchecksum), "0X22620404") != 0)
#else
  HARDBUG(strcmp(bdata(bchecksum), "0X414FA339") != 0)
#endif
  BDESTROY(bchecksum)
}

void file2bstring(const char *arg_name, bstring arg_bstring)
{
  my_int64_t size;

  HARDBUG((size = compat_size(arg_name)) == -1)

  HARDBUG(btrunc(arg_bstring, 0) == BSTR_ERR)

  HARDBUG((size < 0) or (size >= INT_MAX))
  HARDBUG(ballocmin(arg_bstring, (int)size + 1) == BSTR_ERR)

  FILE *farg_name;

  HARDBUG((farg_name = fopen(arg_name, "r")) == NULL)

  struct bStream *barg_name;

  HARDBUG((barg_name = bsopen(my_bread, farg_name)) == NULL)

  while (bsreadlna(arg_bstring, barg_name, (char)'\n') == BSTR_OK)
    ;

  HARDBUG(ferror(farg_name))
  HARDBUG(bseof(barg_name) != TRUE)
  HARDBUG(bsclose(barg_name) == NULL)

  FCLOSE(farg_name)

  BSTRING(newline)

  HARDBUG(bassigncstr(newline, "\n") == BSTR_ERR)

  BSTRING(space)

  HARDBUG(bassigncstr(space, " ") == BSTR_ERR)

  HARDBUG(bfindreplace(arg_bstring, newline, space, 0) == BSTR_ERR)

  BDESTROY(space)

  BDESTROY(newline)
}

void file2cjson(const char *arg_name, cJSON **arg_cjson)
{
  my_int64_t fsize;

  HARDBUG((fsize = compat_size(arg_name)) == -1)

  CSTRING(cstring, fsize + 1)

  FILE *fcjson;

  HARDBUG((fcjson = fopen(arg_name, "rb")) == NULL)

  FREAD(fcjson, cstring, char, fsize)

  FCLOSE(fcjson)

  cstring[fsize] = '\0';

  if ((*arg_cjson = cJSON_Parse(cstring)) == NULL)
  {
    const char *error = cJSON_GetErrorPtr();

    PRINTF("error %s loading %s\n", error, arg_name);

    FATAL("cJSON error", EXIT_FAILURE)
  }

  CDESTROY(cstring)
}

cJSON *cJSON_FindItemInObject(const cJSON *arg_cjson, const char *arg_name)
{
  cJSON *result = NULL;

  cJSON *entry;

  cJSON_ArrayForEach(entry, arg_cjson)
  {
    if (compat_strcasecmp(entry->string, arg_name) == 0)
    {
      result = entry;

      break;
    }
  }

  return (result);
}
