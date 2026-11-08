// SCU REVISION 8.1260 zo  8 nov 2026 17:20:46 CET
#ifndef GlobalsH
#define GlobalsH

#define REVISION "8.1260"

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
#define MY_LINE_MAX 8192

typedef long long i64_t;
typedef unsigned long long ui64_t;

_Static_assert(sizeof(i64_t) == 8 && sizeof(ui64_t) == 8,
               "GWD requires 64-bit i64_t and ui64_t");

#include "compat.h"

// generic defines

#define INVALID (-1)
#define FALSE 0
#define TRUE 1

#define KBYTE 1024
#define MBYTE 1048576
#define GBYTE 1073741824LL
// #define TBYTE   1099511627776LL
#define S_MAX 32767
#define S_MIN (-S_MAX)
#define L_MAX 2147483647L
#define L_MIN (-L_MAX)
#define LL_MAX 9223372036854775807LL
#define LL_MIN (-LL_MAX)
#define UL_MAX 4294967296ULL
#define ULL_MAX 18446744073709551615ULL

#define EPS_REAL 1.0e-3
#define EPS_DOUBLE 1.0e-6

// specific defines

#define USE_HARDWARE_CRC32
#define USE_HARDWARE_RAND

#define NODE_MAX 1024

#define CJSON_PRAGMA_ID "pragma"
#define CJSON_PARAMETERS_ID "parameters"
#define CJSON_SHAPE_ID "shape"
#define CJSON_NETWORK2MATERIAL_SCORE_ID "network2material_score"
#define CJSON_MATERIALS_ID "materials"
#define CJSON_MATERIAL_NAME_ID "name"
#define CJSON_ARCHITECTURE_PATTERNS_ID "architecture_patterns"
#define CJSON_ARCHITECTURE_MATERIAL_ID "architecture_material"
#define CJSON_ACTIVATION_INPUTS_ID "activation_inputs"
#define CJSON_ACTIVATION_ID "activation"
#define CJSON_ACTIVATION_LAST_ID "activation_last"
#define CJSON_NMAN_MIN_ID "nman_min"
#define CJSON_NMAN_MAX_ID "nman_max"

#define CJSON_EVENT_ID "event"
#define CJSON_DATE_ID "date"
#define CJSON_WHITE_ID "white"
#define CJSON_BLACK_ID "black"
#define CJSON_RESULT_ID "result"
#define CJSON_STARTING_POSITION_ID "FEN"
#define CJSON_TIME_ID "time"
#define CJSON_DEPTH_ID "depth"

#define CJSON_MOVES_ID "moves"
#define CJSON_MOVE_STRING_ID "move_string"
#define CJSON_COMMENT_STRING_ID "comment_string"

#define CJSON_HUB_CLIENT_DIR "dir"
#define CJSON_HUB_CLIENT_EXE "exe"
#define CJSON_HUB_CLIENT_ARG "arg"

#define CJSON_HUB_PV_ID "hub_pv"

#define CJSON_CSV_ID "csv"
#define CJSON_CSV_PATH_ID "path"
#define CJSON_CSV_COLUMNS_ID "columns"

#define CJSON_DIRECTORIES_ID "directories"
#define CJSON_DIRECTORY_NAME_ID "name"
#define CJSON_FILES_ID "files"
#define CJSON_FILE_NAME_ID "name"
#define CJSON_FILE_ROWS_ID "rows"
#define CJSON_FILE_COLS_ID "cols"
#define CJSON_FILE_DATA_ID "data"

// generic macro's

#define local static
#define and &&
#define or ||
#define index DO_NOT_USE_INDEX

#define cat2(x, y) x##y
#define cat3(x, y, z) x##y##z

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
#define FSEEKO(F, N) HARDBUG(compat_fseeko(F, N, SEEK_SET) != 0)
#define FBOFO(F) HARDBUG(compat_fseeko(F, 0, SEEK_SET) != 0)
#define FEOFO(F) HARDBUG(compat_fseeko(F, 0, SEEK_END) != 0)
#define FTELLO(F, X) HARDBUG(((X) = ftello(F)) < 0)
#define FCLOSE(F)                        \
  {                                      \
    HARDBUG(fclose(F) == EOF)(F) = NULL; \
  }

// generic typedefs

typedef int8_t i8_t;
typedef uint8_t ui8_t;
typedef int16_t i16_t;
typedef uint16_t ui16_t;
typedef int32_t i32_t;
typedef uint32_t ui32_t;

// specific typedefs

typedef ui64_t hash_key_t;
typedef struct board board_t;

#define SCALED_DOUBLE_FLOAT 1
#define SCALED_DOUBLE_I32 2
#define SCALED_DOUBLE_T SCALED_DOUBLE_FLOAT

#if SCALED_DOUBLE_T == SCALED_DOUBLE_FLOAT
typedef float scaled_double_t;
#else
typedef i32_t scaled_double_t;
#endif

#define SCORE_T_INT 1
#define SCORE_T_FLOAT 2

#define SCORE_T SCORE_T_FLOAT

#if SCORE_T == SCORE_T_INT
typedef i32_t score_t;
#define SCANF_SCORE_T "%d"
#define PRINTF_SCORE_T "%d"
#define SCANF_SCORE_T_RAW "%d"
#define PRINTF_SCORE_T_RAW "%d"
#define ABS_SCORE_T abs
#define SCORE(S) ((score_t)(S))
#define SCORE2PRINTF(S) (S)
#elif SCORE_T == SCORE_T_FLOAT
typedef i32_t score_t;
#define SCANF_SCORE_T "%d"
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
