//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#include "globals.h"

#ifdef USE_HARDWARE_CRC32
#define POLYNOMIAL_CRC32 0x82F63B78UL
#else
#define POLYNOMIAL_CRC32 0xEDB88320UL
#endif

local my_mutex_t mpi_abort_mutex;

#ifdef USE_OPENMPI
local int mpi_abort = FALSE;
#endif

local my_mutex_t randull_mutex;

int zzzzzz_invocation = 0;

#define NFRAMES_MAX 256

void zzzzzz(char *file, const char *func, long line, char *error, int code)
{
  char error_string[MY_LINE_MAX];
 
  snprintf(error_string, MY_LINE_MAX, 
    "** %s **\n"
    " my_mpi_globals.MY_MPIG_id_global=%d\n"
    " my_mpi_globals.MY_MPIG_nglobal=%d\n"
    " file=%s\n"
    " func=%s\n"
    " line=%ld\n"
    " error=%s\n"
    " code=%d\n"
    " version=%s\n",
   code == 0 ? "OK" : "FATAL",
   my_mpi_globals.MY_MPIG_id_global,
   my_mpi_globals.MY_MPIG_nglobal,
   file, func, line, error, code, REVISION);

  fprintf(stderr, "%s", error_string);

  if (zzzzzz_invocation > 1)
  {
    fprintf(stderr, "WARNING: zzzzzz_invocation=%d\n", zzzzzz_invocation);
  }
  else if (code != EXIT_SUCCESS)
  { 
    PRINTF("%s", error_string);
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
      HARDBUG(my_mutex_lock(&mpi_abort_mutex) != 0)

      if (mpi_abort == FALSE)
      {
        mpi_abort = TRUE;
        MPI_Abort(my_mpi_globals.MY_MPIG_comm_global, code);
      }

      HARDBUG(my_mutex_unlock(&mpi_abort_mutex) != 0)
    }
  }
#endif
  exit(code);
}

int fexists(char *name)
{
  if (my_access(name, F_OK) == 0)
    return(TRUE);
  else
    return(FALSE);
}

local char *x[55] = {
  "7695486691539278197",
  "17668290884826504057",
  "8217136619161190694",
  "9798145789352023136",
  "13533164559007451430",
  "11991545949957946907",
  "8108979246051854641",
  "4469134220914525539",
  "9803057183791134933",
  "11878025097534696099",
  "12705436919975675895",
  "3788215626329693944",
  "2788229817969439085",
  "896809635154914246",
  "7642979227627401218",
  "16415566446300674176",
  "17693797731780643904",
  "740343703055963834",
  "13163760304165995515",
  "13583424190793939054",
  "18157047174260536439",
  "10426639141240110889",
  "12577394375014380344",
  "1287267897397807597",
  "1238188445866482434",
  "6593406311695770544",
  "4360257755853189649",
  "9935866181755336076",
  "10763453978995970524",
  "17660290453874763156",
  "5474650546656110815",
  "855045942413422633",
  "14795454850486999302",
  "15005637393194842865",
  "6604271681962598632",
  "10901888371365892400",
  "16618911050944948120",
  "14108827758039928618",
  "8184664491189973222",
  "12933065998774769839",
  "12760171374446467927",
  "8756798036556576879",
  "8855962242306141418",
  "8573013844835606592",
  "13049868273895483103",
  "7553530932171241797",
  "14725311941001312690",
  "16509117984292574803",
  "16423365846456388573",
  "960124620475471253",
  "10365054185148123041",
  "15796623839392014534",
  "14883283904315193857",
  "10969950277165035521",
  "9502089656919400129"
};

ui64_t randull(int init)
{
  static ui64_t y[55];
  static int j, k;

  if (init == 1)
  {
    for (int i = 0; i < 55; i++)
    {
      (void) sscanf(x[i], "%llu", y + i);

      char z[64 + 1];

      (void) snprintf(z, 64, "%llu", y[i]);

      HARDBUG(strcmp(z, x[i]) != 0)
    }
    j = 24 - 1;
    k = 55 - 1;

    return(0);
  }
  else if (init == INVALID)
  {
    for (int i = 0; i < 55; i++)
    {
//ui64_t unsigned long??
      unsigned long long r;

#ifdef USE_HARDWARE_RAND
      HARDBUG(!_rdrand64_step(&r))
#else
      r = i;
#endif

      y[i] = r;

      HARDBUG(y[i] != r)
    }
    //PRINTF("local char *x[55] = {\n");
    //for (int i = 0; i < 54; i++)
      //PRINTF("  \"%llu\",\n", y[i]);
    //PRINTF("  \"%llu\"\n};\n", y[54]);
  }

//  HARDBUG(my_mutex_lock(&randull_mutex) != 0)

  ui64_t ul = (y[k] += y[j]);
  if (--j < 0) j = 55 - 1;
  if (--k < 0) k = 55 - 1;

//  HARDBUG(my_mutex_unlock(&randull_mutex) != 0)

  return(ul);
}

double randdouble(int n)
{
  HARDBUG(n < 1)

  i64_t m = 1;
  for (int i = 1; i <= n; ++i)
    m *= 10;
  
  return((randull(0) % m) / (m - 1.0));
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

  (void) randull(1);

  HARDBUG(my_mutex_init(&mpi_abort_mutex) != 0)
  HARDBUG(my_mutex_init(&randull_mutex) != 0)
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

  struct bStream* bjson = bsopen((bNread) fread, fjson);

  bstring string = bfromcstr("");

  while (bsreadlna(string, bjson, (char) '\n') == BSTR_OK);
  
  bsclose(bjson);

  FCLOSE(fjson);

  bstring newline = bfromcstr("\n");

  bstring space = bfromcstr(" ");

  bfindreplace(string, newline, space, 0);
  
  if ((*arg_json = cJSON_Parse(bdata(string))) == NULL)
  {
    const char *error = cJSON_GetErrorPtr();

    PRINTF("error %s loading %s\n", error, arg_name);

    FATAL("cJSON error", EXIT_FAILURE)
  }

  bdestroy(space);

  bdestroy(newline);

  bdestroy(string);
}

cJSON *cJSON_FindItemInObject(cJSON *object, char *name)
{
  cJSON *result = NULL;

  cJSON *entry;

  cJSON_ArrayForEach(entry, object)
  {
    if (my_strcasecmp(entry->string, name) == 0)
    {
      result = entry;
      break;
    }
  }

  return(result);
}
