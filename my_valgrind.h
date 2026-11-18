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
