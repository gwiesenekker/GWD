// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef GlobalsH
#define GlobalsH

#define REVISION "8.1286"

// generic includes

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <math.h>
#include <stdarg.h>
#include <limits.h>

_Static_assert(CHAR_BIT == 8, "GWD requires 8-bit bytes");

// specific includes

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#include <sys/syscall.h>
#else
#define RUNNING_ON_VALGRIND 0
#define VALGRIND_CHECK_MEM_IS_DEFINED
#endif

#ifdef USE_OPENMPI
#include <mpi.h>
#endif

// forward

#undef LINE_MAX // dirent.h
#define LINE_MAX do_not_use_LINE_MAX

typedef long long my_int64_t;
typedef unsigned long long my_uint64_t;

_Static_assert((sizeof(my_int64_t) == 8) && (sizeof(my_uint64_t) == 8),
               "GWD requires 64-bit my_int64_t and my_uint64_t");

#include "compat.h"

// generic defines

#define INVALID (-1)
#define FALSE 0
#define TRUE 1

enum
{
  MBYTE = 1048576
};

// specific defines

#define USE_HARDWARE_CRC32
#define USE_HARDWARE_RAND

enum
{
  NODE_MAX = 1024
};

// generic macro's

#define local static
#define and &&
#define or ||
#define index DO_NOT_USE_INDEX

#define __FUNC__ __func__

#define BIT(X) (1U << (X))
#define BITULL(X) (1ULL << (X))

#define PRINTF(...) my_printf(STDOUT, __VA_ARGS__)

#ifdef __cppcheck__
#define FATAL(X, C)              \
  {                              \
    fprintf(stderr, "%s\n", #X); \
    exit(C);                     \
  }
#else
#define FATAL(X, C)                                  \
  {                                                  \
    zzzzzz_invocation++;                             \
    zzzzzz(__FILE__, __FUNC__, (int)__LINE__, X, C); \
  }
#endif

#define HARDBUG(X)            \
  {                           \
    if (X)                    \
      FATAL(#X, EXIT_FAILURE) \
  }

#ifdef DEBUG
#define SOFTBUG(X) HARDBUG(X)
#else
#define SOFTBUG(X)
#endif

#define FREAD(F, P, T, N) HARDBUG(fread(P, sizeof(T), N, F) != N)
#define FWRITE(F, P, T, N) HARDBUG(fwrite(P, sizeof(T), N, F) != N)
#define FCLOSE(F)                        \
  {                                      \
    HARDBUG(fclose(F) == EOF)(F) = NULL; \
  }

// specific typedefs

typedef my_uint64_t hash_key_t;
typedef struct board board_t;

#define SCALED_DOUBLE_FLOAT 1
#define SCALED_DOUBLE_I32 2
#define SCALED_DOUBLE_T SCALED_DOUBLE_FLOAT

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
typedef float scaled_double_t;
#else
typedef int32_t scaled_double_t;
#endif

#define SCORE_T_INT 1
#define SCORE_T_FLOAT 2

#define SCORE_T SCORE_T_FLOAT

#if SCORE_T == SCORE_T_INT
typedef int32_t score_t;
#define PRINTF_SCORE_T "%d"
#define SCANF_SCORE_T_RAW "%d"
#define PRINTF_SCORE_T_RAW "%d"
#define ABS_SCORE_T abs
#define SCORE(S) ((score_t)(S))
#define SCORE2PRINTF(S) (S)
#elif SCORE_T == SCORE_T_FLOAT
typedef int32_t score_t;
#define PRINTF_SCORE_T "%.3f"
#define SCANF_SCORE_T_RAW "%d"
#define PRINTF_SCORE_T_RAW "%d"
#define ABS_SCORE_T abs
#define SCORE(S) ((score_t)((S) * 1000))
#define SCORE2PRINTF(S) ((S) / 1000.0)
#else
#error unknown SCORE_T
#endif

// I have to get rid of this!

extern void *STDOUT;

#include "profile.h"

#include "bstrlib.h"
#include "cJSON.h"
#include "my_safec.h"
#include "classes.h"
#include "my_printf.h"
#include "my_bstreams.h"
#include "my_cjson.h"
#include "my_malloc.h"
#include "my_mpi.h"
#include "my_random.h"
#include "my_sqlite3.h"
#include "fbuffer.h"

#include "buckets.h"
#include "caches.h"
#include "compress.h"
#include "records.h"
#include "queues.h"
#include "states.h"
#include "stats.h"
#include "timers.h"
#include "utils.h"
#include "patterns.h"

#include "gwdegtb.h"

#include "networks.h"
#include "boards.h"
#include "moves.h"
#include "sort.h"
#include "search.h"
#include "my_threads.h"
#include "book.h"
#include "dxp.h"
#include "endgame.h"
#include "hub.h"
#include "main.h"
#include "mcts.h"
#include "pdn.h"
#include "score.h"

#endif
