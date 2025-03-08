//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#include "globals.h"

#undef read
#undef write
#undef close
#undef fork
#undef pipe
#undef fseeko
#undef strcasecmp
#undef poll
#undef access
#undef dup2
#undef chdir
#undef mkdir
#undef clock

i64_t compat_read(int fd, void* buffer, i64_t nbuffer)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(read(fd, buffer, nbuffer));
#else
  return(_read(fd, buffer, nbuffer));
#endif
}

i64_t compat_write(int fd, void* buffer, i64_t nbuffer)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(write(fd, buffer, nbuffer));
#else
  return(_write(fd, buffer, nbuffer));
#endif
}

int compat_close(int fd)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(close(fd));
#else
  return(_close(fd));
#endif
}

int compat_lock_file(char *fname)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  int fd = open(fname, O_WRONLY | O_CREAT | O_APPEND, 0644);

  if (fd == -1) return(-1);

  struct flock fl;

  fl.l_type = F_WRLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;

  if (fcntl(fd, F_SETLKW, &fl) == -1) return(-1);
#else
  int fd = _open(fname,
                 _O_WRONLY | _O_CREAT | _O_APPEND, _S_IREAD | _S_IWRITE);

  if (fd == -1) return(-1);

  OVERLAPPED ol = {0};

  if (!LockFileEx((HANDLE)_get_osfhandle(fd),
                  LOCKFILE_EXCLUSIVE_LOCK, 0, MAXDWORD, MAXDWORD, &ol))
  {
    return(-1);
  }
#endif

  return(fd);
}

void compat_unlock_file(int fd)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  struct flock fl;

  fl.l_type = F_UNLCK;
  fl.l_whence = SEEK_SET;
  fl.l_start = 0;
  fl.l_len = 0;

  HARDBUG(fcntl(fd, F_SETLK, &fl) == -1)

  HARDBUG(compat_close(fd) == -1)
#else
  OVERLAPPED ol = {0};

  HARDBUG(!UnlockFileEx((HANDLE)_get_osfhandle(fd),
                        0, MAXDWORD, MAXDWORD, &ol))
#endif
}

void compat_fdprintf(int fd, char *format, ...)
{
  char buffer[MY_LINE_MAX];

  va_list args;

  va_start(args, format);

  int nbuffer = vsnprintf(buffer, sizeof(buffer), format, args);

  va_end(args);

  HARDBUG(nbuffer >= sizeof(buffer))

  HARDBUG(compat_write(fd, buffer, nbuffer) != nbuffer)
}

int compat_socket_startup(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(0);
#else
  WORD wVersionRequested = MAKEWORD(2, 2);

  WSADATA wsaData;

  return(WSAStartup(wVersionRequested, &wsaData));
#endif
}

int compat_socket_cleanup(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(0);
#else
  return(WSACleanup());
#endif
}

i64_t compat_socket_read(int fd, void* buffer, i64_t nbuffer)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(read(fd, buffer, nbuffer));
#else
  return(recv(fd, buffer, (int)nbuffer, 0));
#endif
}

i64_t compat_socket_write(int fd, void* buffer, i64_t nbuffer)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(write(fd, buffer, nbuffer));
#else
  return(send(fd, buffer, (int)nbuffer, 0));
#endif
}

int compat_socket_close(int fd)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(close(fd));
#else
  return(closesocket(fd));
#endif
}

int compat_fork(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(fork());
#else
  return(INVALID);
#endif
}

int compat_pipe(pipe_t pfd[2])
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(pipe(pfd));
#else
  SECURITY_ATTRIBUTES saAttr;

  saAttr.nLength = sizeof(SECURITY_ATTRIBUTES);
  saAttr.bInheritHandle = TRUE;
  saAttr.lpSecurityDescriptor = NULL;

  HANDLE readPipe, writePipe;

  if (!CreatePipe(&readPipe, &writePipe, &saAttr, 0)) return(-1);

  pfd[0] = readPipe;
  pfd[1] = writePipe;

  return(0);
#endif
}

i64_t compat_pipe_read(pipe_t fd, void* buffer, i64_t nbuffer)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(read(fd, buffer, nbuffer));
#else
  DWORD nread;

  if (!ReadFile(fd, buffer, nbuffer, &nread, NULL)) return(-1);

  return(nread);
#endif
}

i64_t compat_pipe_write(pipe_t fd, void* buffer, i64_t nbuffer)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(write(fd, buffer, nbuffer));
#else
  DWORD nwritten;

  if (!WriteFile(fd, buffer, nbuffer, &nwritten, NULL)) return(-1);

  return(nwritten);
#endif
}

int compat_pipe_close(pipe_t fd)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(close(fd));
#else
  return(CloseHandle(fd) ? 0 : -1);
#endif
}

int compat_fseeko(FILE* stream, i64_t offset, int whence)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(fseeko(stream, offset, whence));
#else
  return(_fseeki64(stream, offset, whence));
#endif
}

int compat_strcasecmp(char* s1, char* s2)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(strcasecmp(s1, s2));
#else
  return(_stricmp(s1, s2));
#endif
}

int compat_poll(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  struct pollfd mypoll = { STDIN_FILENO, POLLIN };

  if (poll(&mypoll, 1, 10) > 0)
    return(TRUE);
  else
    return(FALSE);
#else
  HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
  DWORD dwAvail = 0;

  if (GetFileType(hStdin) == FILE_TYPE_PIPE)
  {
    if (PeekNamedPipe(hStdin, NULL, 0, NULL, &dwAvail, NULL) && dwAvail > 0)
      return(TRUE);
  }
  else if (_kbhit()) // console
  {
    return(TRUE);
  }

  return(FALSE);
#endif
}

int compat_access(char* pathname, int mode)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(access(pathname, mode));
#else
  return(_access(pathname, mode));
#endif
}

int compat_dup2(int oldfd, int newfd)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(dup2(oldfd, newfd));
#else
  return(_dup2(oldfd, newfd));
#endif
}

int compat_chdir(char* path)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(chdir(path));
#else
  return(_chdir(path));
#endif
}

int compat_mkdir(char* path)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  return(mkdir(path, 0750));
#else
  return(_mkdir(path));
#endif
}

double return_my_clock(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  struct timespec tv;

  if (clock_gettime(CLOCK_THREAD_CPUTIME_ID, &tv) != 0)
  {
    PRINTF("WARNING: clock_gettime returned errno=%d\n", errno);
    HARDBUG(clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &tv) != 0)
  }

  return((double)tv.tv_sec + (double)tv.tv_nsec / NANO_SECONDS);
#else
  LARGE_INTEGER frequency, ticks;

  QueryPerformanceFrequency(&frequency);
  QueryPerformanceCounter(&ticks);

  return((double)ticks.QuadPart / frequency.QuadPart);
#endif
}

//returns physical memory in MB

int return_physical_memory(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  FILE* fproc;

  HARDBUG((fproc = fopen("/proc/meminfo", "r")) == NULL)

    char line[MY_LINE_MAX];

  i64_t result = 0;

  while (fgets(line, MY_LINE_MAX, fproc) != NULL)
  {
    i64_t lld;

    if (sscanf(line, "MemFree:%lld", &lld) == 1)
      result += lld * KBYTE;
    if (sscanf(line, "Cached:%lld", &lld) == 1)
      result += lld * KBYTE;
  }

  FCLOSE(fproc)

    HARDBUG(result == INVALID)

    return(result / MBYTE);
#else
  MEMORYSTATUSEX statex;

  statex.dwLength = sizeof(statex);

  GlobalMemoryStatusEx(&statex);

  return((int)(statex.ullAvailPhys / MBYTE));
#endif
}

int return_physical_cpus(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  FILE* fproc;

  HARDBUG((fproc = fopen("/proc/cpuinfo", "r")) == NULL)

    char line[MY_LINE_MAX];

  int result = INVALID;

  while (fgets(line, MY_LINE_MAX, fproc) != NULL)
  {
    int d;

    if (sscanf(line, "cpu cores%*s%d", &d) == 1)
    {
      result = d;
      break;
    }
  }

  FCLOSE(fproc)

    HARDBUG(result == INVALID)

    return(result);
#else
  PSYSTEM_LOGICAL_PROCESSOR_INFORMATION plpi = NULL;
  DWORD returnLength = 0;
  int physicalCores = 0;

  if (GetLogicalProcessorInformation(plpi, &returnLength) == FALSE)
  {
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
      plpi = (PSYSTEM_LOGICAL_PROCESSOR_INFORMATION)malloc(returnLength);

      if (plpi)
      {
        if (GetLogicalProcessorInformation(plpi, &returnLength))
        {
          PSYSTEM_LOGICAL_PROCESSOR_INFORMATION ptr = plpi;
          DWORD numSlots = returnLength /
                           sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);

          while (ptr < plpi + numSlots)
          {
            if (ptr->Relationship == RelationProcessorCore)
              physicalCores++;

            ptr++;
          }
        }

        free(plpi);
      }
    }
  }

  HARDBUG(physicalCores == 0); // detection failed

  return(physicalCores);
#endif
}

void return_cpu_flags(char flags[MY_LINE_MAX])
{
  strncpy(flags, "NULL", MY_LINE_MAX);

#if COMPAT_OS == COMPAT_OS_LINUX
  FILE* fproc;

  HARDBUG((fproc = fopen("/proc/cpuinfo", "r")) == NULL)

    char line[MY_LINE_MAX];

  while (fgets(line, MY_LINE_MAX, fproc) != NULL)
  {
    if (sscanf(line, "flags%*s%[^\n]", flags) == 1)
      break;
  }

  FCLOSE(fproc)
#else
  //no need to port as flags are informational only
#endif
}

void compat_sleep(double seconds)
{
  HARDBUG(seconds <= 0.0)

#if COMPAT_OS == COMPAT_OS_LINUX
    struct timespec req, rem;

  req.tv_sec = floor(seconds);
  req.tv_nsec = round((seconds - floor(seconds)) * NANO_SECONDS);
  HARDBUG(req.tv_nsec < 0)
    HARDBUG(req.tv_nsec >= NANO_SECONDS)
    nanosleep(&req, &rem);
#else
  Sleep((DWORD)(1000.0 * seconds));
#endif
}

double compat_time(void)
{
#if COMPAT_OS == COMPAT_OS_LINUX
  struct timespec tv;

  clock_gettime(CLOCK_REALTIME, &tv);

  return((double) tv.tv_sec + (double) tv.tv_nsec / NANO_SECONDS);
#else
   FILETIME ft;

   GetSystemTimePreciseAsFileTime(&ft);

   ULARGE_INTEGER uli;

   uli.LowPart = ft.dwLowDateTime;
   uli.HighPart = ft.dwHighDateTime;

   return((double) uli.QuadPart / (NANO_SECONDS / 100));
#endif
}

//threads

unsigned long compat_pthread_self(void)
{
#if COMPAT_CSTD == COMPAT_CSTD_C11
  return((unsigned long) thrd_current());
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
  DWORD dword = GetCurrentThreadId();
  return((unsigned long) dword);
#else
  return((unsigned long) pthread_self());
#endif
}

int compat_mutex_init(my_mutex_t* mutex)
{
#if COMPAT_CSTD == COMPAT_CSTD_C11
  return(mtx_init(mutex, mtx_plain));
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
  InitializeSRWLock(mutex);
  return(0);
#else
  return(pthread_mutex_init(mutex, NULL));
#endif
}

int compat_mutex_lock(my_mutex_t* mutex)
{
#if COMPAT_CSTD == COMPAT_CSTD_C11
  return(mtx_lock(mutex));
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
  AcquireSRWLockExclusive(mutex);
  return(0);
#else
  return(pthread_mutex_lock(mutex));
#endif
}

int compat_mutex_trylock(my_mutex_t* mutex)
{
#if COMPAT_CSTD == COMPAT_CSTD_C11
  return(mtx_trylock(mutex));
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
  TryAcquireSRWLockExclusive(mutex);
  return(0);
#else
  return(pthread_mutex_trylock(mutex));
#endif
}

int compat_mutex_unlock(my_mutex_t* mutex)
{
#if COMPAT_CSTD == COMPAT_CSTD_C11
  return(mtx_unlock(mutex));
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
  ReleaseSRWLockExclusive(mutex);
  return(0);
#else
  return(pthread_mutex_unlock(mutex));
#endif
}

int compat_mutex_destroy(my_mutex_t* mutex)
{
#if COMPAT_CSTD == COMPAT_CSTD_C11
  mtx_destroy(mutex);
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
#else
#endif
  return(0);
}

void compat_thread_create(my_thread_t* thread, my_thread_func_t thread_func,
                      void* arg)
{
  int ierr;

#if COMPAT_CSTD == COMPAT_CSTD_C11
  if ((ierr = thrd_create(thread, (thrd_start_t)thread_func, arg)) ==
      thrd_error)
  {
    fprintf(stderr, "thrd_create returned %d\n", ierr);

    exit(EXIT_FAILURE);
  }
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
  HANDLE hThread = CreateThread(NULL, 0, (unsigned long (*)(void*))thread_func,
                                arg, 0, NULL);

  if (hThread == NULL)
  {
    ierr = (int)GetLastError();

    fprintf(stderr, "thread_create returned %d\n", ierr);

    exit(EXIT_FAILURE);
  }

  *thread = hThread;
#else
  if ((ierr = pthread_create(thread, NULL, thread_func, arg)) != 0)
  {
    fprintf(stderr, "pthread_create returned %d\n", ierr);

    exit(EXIT_FAILURE);
  }
#endif
}

void compat_thread_join(my_thread_t thread)
{
  int ierr;
#if COMPAT_CSTD == COMPAT_CSTD_C11
  if ((ierr = thrd_join(thread, NULL)) == thrd_error)
  {
    fprintf(stderr, "thrd_join returned %d\n", ierr);

    exit(EXIT_FAILURE);
  }
#elif COMPAT_CSTD == COMPAT_CSTD_WIN
  ierr = (int)WaitForSingleObject(thread, INFINITE);

  if (ierr != WAIT_OBJECT_0)
  {
    fprintf(stderr, "thread_join returned %d\n", ierr);

    exit(EXIT_FAILURE);
  }

#else
  if ((ierr = pthread_join(thread, NULL)) != 0)
  {
    fprintf(stderr, "pthread_join returned %d\n", ierr);

    exit(EXIT_FAILURE);
  }
#endif
}
