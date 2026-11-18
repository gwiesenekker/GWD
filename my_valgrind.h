// SCU REVISION 8.1299 wo 18 nov 2026 18:02:29 CET
#ifndef MyValgrindH
#define MyValgrindH

#ifdef USE_VALGRIND
#include <valgrind/valgrind.h>
#include <valgrind/memcheck.h>
#include <sys/syscall.h>
#else
#define RUNNING_ON_VALGRIND 0
#define VALGRIND_CHECK_MEM_IS_DEFINED
#endif

#endif
