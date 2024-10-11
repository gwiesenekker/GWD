//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
#ifndef CompatH
#define CompatH

#define COMPAT_OS_LINUX     1
#define COMPAT_OS_WINDOWS   2
#define COMPAT_CSTD_C11     3
#define COMPAT_CSTD_WIN     4
#define COMPAT_CSTD_PTHREAD 5

#ifdef _WIN32
#define COMPAT_OS   COMPAT_OS_WINDOWS

#define COMPAT_CSTD COMPAT_CSTD_WIN
#else
#define COMPAT_OS   COMPAT_OS_LINUX

#define COMPAT_CSTD COMPAT_CSTD_PTHREAD
#endif

#if COMPAT_OS == COMPAT_OS_LINUX
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <poll.h>
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <dirent.h>
#include <mm_malloc.h>
#include <smmintrin.h>
#include <immintrin.h>
#include <fcntl.h>

#define ALIGN64(X) X __attribute__((aligned(64)))

#define MALLOC(P, T, N) HARDBUG(((P) = _mm_malloc(sizeof(T) * (N), 64)) == NULL)
#define FREE_AND_NULL(P) {_mm_free(P); (P) = NULL;}

#define BIT_COUNT(ULL)   (int) __builtin_popcountll(ULL)
#define BIT_CTZ(ULL)     __builtin_ctzll(ULL)
#define BIT_CLZ(ULL)     __builtin_clzll(ULL)

typedef int pipe_t;

#else

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#include <conio.h>
#include <io.h>
#include <fcntl.h>
#include <process.h>
#include <direct.h>
#include <intrin.h>

#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <WinSock2.h>
#include <Ws2tcpip.h>

#include "dirent.h"

#pragma comment(lib, "Ws2_32.lib")
#pragma comment(lib, "libzstd.lib")
#pragma comment(linker, "/STACK:16777216")

#define F_OK 0

#define ALIGN64(X) __declspec(align(64)) X

#define MALLOC(P, T, N)\
  HARDBUG(((P) = (T *) _aligned_malloc(sizeof(T) * (N), 64)) == NULL)
#define FREE_AND_NULL(P) {_aligned_free(P); (P) = NULL;}

#define BIT_COUNT(ULL)   (int) __popcnt64(ULL)
#define BIT_CTZ(ULL)     _tzcnt_u64(ULL)
#define BIT_CLZ(ULL)     _lzcnt_u64(ULL)

typedef int pid_t;
typedef HANDLE pipe_t;

#endif

#if COMPAT_CSTD == COMPAT_CSTD_C11
#include <threads.h>
typedef mtx_t my_mutex_t;
typedef thrd_t my_thread_t;
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
typedef SRWLOCK my_mutex_t;
typedef HANDLE my_thread_t;
#else
#include <pthread.h>
typedef pthread_mutex_t my_mutex_t;
typedef pthread_t my_thread_t;
#endif

typedef void *(*my_thread_func_t)(void *);

#define read do_not_use_read_but_my_read
#define write do_not_use_write_but_my_write
#define close do_not_use_close_but_my_close

#define fork       do_not_use_fork_but_my_fork
#define pipe       do_not_use_pipe_but_my_pipe
#define fseeko     do_not_use_fseeko_but_my_fseeko
#define strcasecmp do_not_use_strcasecmp_but_my_strcasecmp
#define access     do_not_use_access_but_my_access
#define dup2       do_not_use_dup2_but_my_dup2
#define chdir      do_not_use_chdir_but_my_chdir
#define mkdir      do_not_use_mkdir_but_my_mkdir
#define clock      do_not_use_clock_but_return_my_clock
#define sleep      do_not_use_sleep_but_my_sleep

i64_t my_read(int, void *, i64_t);
i64_t my_write(int, void *, i64_t);
int my_close(int);
int my_lock_file(char *);
void my_unlock_file(int);
void my_fdprintf(int, char *, ...);

//socket

i64_t my_socket_read(int, void *, i64_t);
i64_t my_socket_write(int, void *, i64_t);

int my_socket_startup(void);
int my_socket_cleanup(void);
int my_socket_close(int);

int my_fork(void);

int my_pipe(pipe_t [2]);

i64_t my_pipe_read(pipe_t, void *, i64_t);
i64_t my_pipe_write(pipe_t, void *, i64_t);
int my_pipe_close(pipe_t);

int my_fseeko(FILE *, i64_t, int);

int my_strcasecmp(char *, char *);

int my_poll(void);

int my_access(char *, int);

int my_dup2(int, int);

int my_chdir(char *);

int my_mkdir(char *);

double return_my_clock(void);

int return_physical_memory(void);

int return_physical_cpus(void);

void return_cpu_flags(char [MY_LINE_MAX]);

void my_sleep(double);

double my_time(void);

//threads

int my_mutex_init(my_mutex_t *);
int my_mutex_lock(my_mutex_t *);
int my_mutex_trylock(my_mutex_t *);
int my_mutex_unlock(my_mutex_t *);
int my_mutex_destroy(my_mutex_t *);

void my_thread_create(my_thread_t *, my_thread_func_t, void *);
void my_thread_join(my_thread_t);

#endif

