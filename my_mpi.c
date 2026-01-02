//SCU REVISION 8.0098 vr  2 jan 2026 13:41:25 CET
#include "globals.h"

enum
{
  SEM_MUTEX      = 0,
  SEM_COUNT      = 1,
  SEM_ENTRY_GATE = 2,
  SEM_EXIT_GATE  = 3,
  NSEMS
};

#define TAG_REQUEST_LOCK 1
#define TAG_LOCK_GRANTED 2
#define TAG_RELEASE_LOCK 3
#define TAG_FLUSH_LOCK   4

const char *tag2string[] =
{
  "TAG_INVALID",
  "TAG_REQUEST_LOCK",
  "TAG_LOCK_GRANTED",
  "TAG_RELEASE_LOCK",
  "TAG_FLUSH_LOCK"
};

my_mpi_globals_t my_mpi_globals =
{
  .MMG_init = FALSE,
  .MMG_nglobal = 0,
  .MMG_id_global = INVALID,
  .MMG_nslaves = 0,
  .MMG_id_slave = INVALID,
  .MMG_comm_global = MPI_COMM_NULL,
  .MMG_comm_slaves = MPI_COMM_NULL
};

local void my_sem_setval(int arg_semid, int arg_sem_num, int arg_value)
{
#ifdef USE_OPENMPI
  union semun
  {
    int val;
    struct semid_ds *buf;
    unsigned short *array;
  };

  union semun value;

  value.val = arg_value;

  HARDBUG(semctl(arg_semid, arg_sem_num, SETVAL, value) != 0)
#endif
}

local int my_sem_getval(int arg_semid, int arg_sem_num)
{
#ifdef USE_OPENMPI
  int value;

  HARDBUG((value = semctl(arg_semid, arg_sem_num, GETVAL)) == -1)

  return(value);
#endif
}

local void my_create_semaphore(int arg_semkey)
{
#ifdef USE_OPENMPI
  int semid = semget(arg_semkey, NSEMS, IPC_CREAT | IPC_EXCL | 0600);

  if (semid == -1)
  {
    HARDBUG(errno != EEXIST)

    if ((semid = semget(arg_semkey, NSEMS, 0600)) == -1)
    {
      perror("semget failed");

      FATAL("semget", EXIT_FAILURE)
    }
  
    if (semctl(semid, 0, IPC_RMID) == -1)
    { 
      perror("semctl failed");
  
      FATAL("semctl", EXIT_FAILURE)
    }
  
    if ((semid = semget(arg_semkey, NSEMS,
                        IPC_CREAT | IPC_EXCL | 0600)) == -1)
    {
      perror("semget failed");
 
      FATAL("semget", EXIT_FAILURE)
    }
  }

  my_sem_setval(semid, SEM_MUTEX, 1);
  my_sem_setval(semid, SEM_COUNT, 0);
  my_sem_setval(semid, SEM_ENTRY_GATE, 0);
  my_sem_setval(semid, SEM_EXIT_GATE, 1);
#endif
}

void my_mpi_init(int *narg, char ***argv, int arg_physical_memory)
{
#ifdef USE_OPENMPI

  MPI_Init(narg, argv);

  my_mpi_globals.MMG_init = TRUE;

  int mpi_nworld;
  
  MPI_Comm_size(MPI_COMM_WORLD, &mpi_nworld);
  
  HARDBUG(mpi_nworld < 1)

  if (mpi_nworld > 1) MPI_Barrier(MPI_COMM_WORLD);

  int mpi_id_world;

  MPI_Comm_rank(MPI_COMM_WORLD, &mpi_id_world);

  if (mpi_id_world == 0)
  {
    my_create_semaphore(SEMKEY_GEN_BOOK_BARRIER);
    my_create_semaphore(SEMKEY_INIT_ENDGAME_BARRIER);
    my_create_semaphore(SEMKEY_GEN_RANDOM_BARRIER);
    my_create_semaphore(SEMKEY_MAIN_BARRIER);
    my_create_semaphore(SEMKEY_TEST_MY_MPI);
    my_create_semaphore(SEMKEY_TEST_MY_MPI_BARRIER);
    my_create_semaphore(SEMKEY_FLUSH_SQL_BUFFER);
    my_create_semaphore(SEMKEY_CONSTRUCT_MY_SQLITE3_BARRIER);
    my_create_semaphore(SEMKEY_CLOSE_MY_SQLITE3);
    my_create_semaphore(SEMKEY_CLOSE_MY_SQLITE3_BARRIER);
    my_create_semaphore(SEMKEY_GEN_DB_BARRIER);
    my_create_semaphore(SEMKEY_UPDATE_DB_BARRIER);
    my_create_semaphore(SEMKEY_ADD_POSITIONS_BARRIER);
    my_create_semaphore(SEMKEY_QUERY_POSITIONS_BARRIER);
  }

  if (mpi_nworld > 1) MPI_Barrier(MPI_COMM_WORLD);

  my_mpi_globals.MMG_comm_global = MPI_COMM_WORLD;
  my_mpi_globals.MMG_comm_slaves = MPI_COMM_NULL;
  
  int slave = (mpi_id_world == 0) ? 0 : 1;
  
  MPI_Comm_split(MPI_COMM_WORLD, slave, mpi_id_world,
                 &my_mpi_globals.MMG_comm_slaves);
  
  if (!slave)
  {
    my_mpi_globals.MMG_comm_slaves = MPI_COMM_NULL;
  
    my_mpi_globals.MMG_id_slave = INVALID;
  }
  else
  {
    MPI_Comm_rank(my_mpi_globals.MMG_comm_slaves,
                  &my_mpi_globals.MMG_id_slave);
  }

  MPI_Allreduce(&slave, &my_mpi_globals.MMG_nslaves, 1, MPI_INT, MPI_SUM,
                MPI_COMM_WORLD);
  
  HARDBUG(my_mpi_globals.MMG_nslaves != (mpi_nworld - 1))

  MPI_Comm_size(my_mpi_globals.MMG_comm_global,
                &my_mpi_globals.MMG_nglobal);

  HARDBUG(my_mpi_globals.MMG_nglobal > MY_MPI_NGLOBAL_MAX)
  
  HARDBUG(my_mpi_globals.MMG_nglobal !=
          (my_mpi_globals.MMG_nslaves + 1))
  
  MPI_Comm_rank(my_mpi_globals.MMG_comm_global,
                &my_mpi_globals.MMG_id_global);
  
  if (my_mpi_globals.MMG_id_global == 0)
  {
    HARDBUG(my_mpi_globals.MMG_id_slave != INVALID)
  }
  else
  {
    HARDBUG(my_mpi_globals.MMG_id_slave == INVALID)
  }

  MPI_Bcast(&arg_physical_memory, 1, MPI_INT, 0,
            my_mpi_globals.MMG_comm_global);
#else
  my_mpi_globals.MMG_init = FALSE;

  my_mpi_globals.MMG_nglobal = 0;
  my_mpi_globals.MMG_id_global = INVALID;

  my_mpi_globals.MMG_nslaves = 0;
  my_mpi_globals.MMG_id_slave = INVALID;
#endif
}

int my_mpi_comm_size(MPI_Comm comm, int *size)
{
#ifdef USE_OPENMPI
  return(MPI_Comm_size(comm, size));
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_comm_rank(MPI_Comm comm, int *rank)
{
#ifdef USE_OPENMPI
  return(MPI_Comm_rank(comm, rank));
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_allreduce(const void *arg_sendbuf, void *arg_recvbuf, int arg_count,
  MPI_Datatype arg_datatype, MPI_Op arg_op, MPI_Comm arg_comm)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return(MPI_SUCCESS);

  return(MPI_Allreduce(arg_sendbuf, arg_recvbuf, arg_count, arg_datatype,
                       arg_op, arg_comm));
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_bcast(void *buffer, int count, MPI_Datatype datatype, int root,
  MPI_Comm comm)
{
#ifdef USE_OPENMPI
  return(MPI_Bcast(buffer, count, datatype, root, comm));
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_win_allocate_shared(MPI_Aint size, int disp_unit, MPI_Info info,
  MPI_Comm comm, void *baseptr, MPI_Win *win)
{
#ifdef USE_OPENMPI
  return(MPI_Win_allocate_shared(size, disp_unit, info, comm, baseptr, win));
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_win_shared_query(MPI_Win win, int rank, MPI_Aint *size,
  int *disp_unit, void *baseptr)
{
#ifdef USE_OPENMPI
  return(MPI_Win_shared_query(win, rank, size, disp_unit, baseptr));
#else
  return(MPI_SUCCESS);
#endif
}

void my_mpi_win_allocate(MPI_Aint arg_size, int arg_disp_unit,
  MPI_Info arg_info, MPI_Comm arg_comm, void *arg_baseptr, MPI_Win *arg_win)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  MPI_Win_allocate(arg_size, arg_disp_unit, arg_info, arg_comm, arg_baseptr,
                   arg_win);

  int rank;

  my_mpi_comm_rank(arg_comm, &rank);

  if (rank == 0)
  {
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, *arg_win);

    int zero = 0;

    MPI_Put(&zero, 1, MPI_INT, 0, 0, 1, MPI_INT, *arg_win);

    MPI_Win_flush(0, *arg_win);

    MPI_Win_unlock(0, *arg_win);
  }

  my_mpi_win_fence(arg_comm, 0, *arg_win);
#endif
}

int my_mpi_win_fence(MPI_Comm arg_comm, int arg_assert, MPI_Win arg_win)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return(MPI_SUCCESS);

  return(MPI_Win_fence(arg_assert, arg_win));
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_win_free(MPI_Win *win)
{
#ifdef USE_OPENMPI
  return(MPI_Win_free(win));
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_finalize(void)
{
#ifdef USE_OPENMPI
  return(MPI_Finalize());
#else
  return(MPI_SUCCESS);
#endif
}

int my_mpi_abort(MPI_Comm comm, int errorcode)
{
#ifdef USE_OPENMPI
  return(MPI_Abort(comm, errorcode));
#else
  return(MPI_SUCCESS);
#endif
}

void my_mpi_barrier(char *arg_info, MPI_Comm arg_comm, int arg_verbose)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int ncomm;

  MPI_Comm_size(arg_comm, &ncomm);

  HARDBUG(ncomm < 1)

  if (ncomm == 1) return;

  int comm_id;

  MPI_Comm_rank(arg_comm, &comm_id);

  if (arg_verbose) PRINTF("entering my_mpi_barrier %s ncomm=%d comm_id=%d\n$",
    arg_info, ncomm, comm_id);

  double t0 = compat_time();

  MPI_Request request;

  MPI_Ibarrier(arg_comm, &request);

  MPI_Wait(&request, MPI_STATUS_IGNORE);

  double t1 = compat_time();

  double wall = t1 - t0;

  if (arg_verbose)
    PRINTF("leaving my_mpi_barrier %s wall=%.0f\n$", arg_info, wall);

  my_mpi_allreduce(MPI_IN_PLACE, &wall, 1, MPI_DOUBLE, MPI_MAX, arg_comm);

  if (arg_verbose)
    PRINTF("my_mpi_barrier %s wall(MPI_MAX)=%.0f\n$", arg_info, wall);
#endif
}

#define NTRIES_LIMIT 10000

void my_mpi_acquire_semaphore(MPI_Comm arg_comm, MPI_Win arg_win)
{
#ifdef USE_OPENMPI
  static int ntries_max = 0;

  if (arg_comm == MPI_COMM_NULL) return;

  int zero = 0;

  int one = 1;

  int lock_acquired = 0;

  int value = INVALID;

  int ntries = 0;

  while(TRUE)
  {
    HARDBUG(ntries > NTRIES_LIMIT)

    if (ntries >= NTRIES_LIMIT) 
      PRINTF("%s::MPI_Win_lock..\n$", __FUNC__);

    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, arg_win);

    if (ntries >= NTRIES_LIMIT) 
      PRINTF("%s::MPI_Fetch_and_op..\n$", __FUNC__);

    MPI_Fetch_and_op(&zero, &lock_acquired, MPI_INT, 0, 0, MPI_SUM, arg_win);

    if (ntries >= NTRIES_LIMIT) 
      PRINTF("%s::lock_acquired=%d\n$", __FUNC__, lock_acquired);

    if (ntries >= NTRIES_LIMIT) 
      PRINTF("%s::MPI_Win_flush..\n$", __FUNC__);

    MPI_Win_flush(0, arg_win);

    if (lock_acquired == 0)
    {
      if (ntries >= NTRIES_LIMIT) 
        PRINTF("%s::MPI_Accumulate..\n$", __FUNC__);

      MPI_Accumulate(&one, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, arg_win);

      if (ntries >= NTRIES_LIMIT) 
        PRINTF("%s::MPI_Win_flush..\n$", __FUNC__);

      MPI_Win_flush(0, arg_win);
  
      if (ntries >= NTRIES_LIMIT) 
        PRINTF("%s::MPI_Get..\n$", __FUNC__);

      MPI_Get(&value, 1, MPI_INT, 0, 0, 1, MPI_INT, arg_win);

      if (ntries >= NTRIES_LIMIT) 
        PRINTF("%s::MPI_Win_flush..\n$", __FUNC__);

      MPI_Win_flush(0, arg_win);

      HARDBUG(value != 1)

      if (ntries >= NTRIES_LIMIT) 
        PRINTF("%s::MPI_Win_unlock..\n$", __FUNC__);

      MPI_Win_unlock(0, arg_win);

      if (ntries > ntries_max) ntries_max = ntries;

      PRINTF("lock acquired ntries=%d ntries_max=%d\n",
             ntries, ntries_max);

      break;
    }

    if (ntries >= NTRIES_LIMIT) 
      PRINTF("%s::MPI_Win_unlock..\n$", __FUNC__);

    MPI_Win_unlock(0, arg_win);

    ++ntries;

    compat_sleep(MILLI_SECOND * (ntries / 100 + 1));
  }
#endif
}

void my_mpi_release_semaphore(MPI_Comm arg_comm, MPI_Win arg_win)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int zero = 0;

  int value = INVALID;

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, arg_win);

  MPI_Get(&value, 1, MPI_INT, 0, 0, 1, MPI_INT, arg_win);

  MPI_Win_flush(0, arg_win);

  HARDBUG(value != 1)

  MPI_Accumulate(&zero, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, arg_win);

  MPI_Win_flush(0, arg_win);
  
  MPI_Get(&value, 1, MPI_INT, 0, 0, 1, MPI_INT, arg_win);

  MPI_Win_flush(0, arg_win);

  HARDBUG(value != 0)

  MPI_Win_unlock(0, arg_win);
#endif
}

void my_mpi_acquire_semaphore_v2(MPI_Comm arg_comm)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int nprocs;

  MPI_Comm_size(arg_comm, &nprocs);

  if (nprocs <= 1) return;

  int iproc;

  MPI_Comm_rank(arg_comm, &iproc);

  HARDBUG(iproc == 0)

  int tag = TAG_REQUEST_LOCK;

  PRINTF("%s::SENDING %s\n$", __FUNC__, tag2string[tag]);

  MPI_Send(NULL, 0, MPI_INT, 0, tag, arg_comm);

  PRINTF("%s::SENT %s\n$", __FUNC__, tag2string[tag]);

  tag = TAG_LOCK_GRANTED;

  PRINTF("%s::RECEIVING %s\n$", __FUNC__, tag2string[tag]);

  progress_t progress;

  construct_progress(&progress, 0, 10);

  while (TRUE)
  {
    int flag = 0;
    MPI_Status status;
   
    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, arg_comm, &flag, &status);
  
    update_progress(&progress);

    if (flag)
    {
      PRINTF("%s::PROBED source=%d tag=%d\n$", __FUNC__,
            status.MPI_TAG, status.MPI_SOURCE);

      break;
    }

    compat_sleep(MILLI_SECOND);
  }

  finalize_progress(&progress);

  MPI_Recv(NULL, 0, MPI_INT, 0, tag, arg_comm, MPI_STATUS_IGNORE);

  PRINTF("%s::RECEIVED %s\n$", __FUNC__, tag2string[tag]);
#endif
}

void my_mpi_release_semaphore_v2(MPI_Comm arg_comm)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int nprocs;

  MPI_Comm_size(arg_comm, &nprocs);

  if (nprocs <= 1) return;

  int iproc;

  MPI_Comm_rank(arg_comm, &iproc);

  HARDBUG(iproc == 0)

  int tag = TAG_RELEASE_LOCK;

  PRINTF("%s::SENDING %s\n$", __FUNC__, tag2string[tag]);

  MPI_Send(NULL, 0, MPI_INT, 0, TAG_RELEASE_LOCK, arg_comm);

  PRINTF("%s::SENT %s\n$", __FUNC__, tag2string[tag]);
#endif
}

void my_mpi_flush_semaphore_v2(MPI_Comm arg_comm)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int nprocs;

  MPI_Comm_size(arg_comm, &nprocs);

  if (nprocs <= 1) return;

  int iproc;

  MPI_Comm_rank(arg_comm, &iproc);

  HARDBUG(iproc == 0)

  MPI_Send(NULL, 0, MPI_INT, 0, TAG_FLUSH_LOCK, arg_comm);
#endif
}

#define LENGTH_MAX (2 * MY_MPI_NGLOBAL_MAX)

void my_mpi_semaphore_server_v2(MPI_Comm arg_comm)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int nprocs;

  MPI_Comm_size(arg_comm, &nprocs);

  if (nprocs <= 1) return;
 
  HARDBUG(nprocs > LENGTH_MAX)

  PRINTF("%s::nprocs=%d\n$", __FUNC__, nprocs);

  int iproc;

  MPI_Comm_rank(arg_comm, &iproc);

  HARDBUG(iproc != 0)

  int ndone = 0;
   
  int queue[LENGTH_MAX];
 
  double wall[MY_MPI_NGLOBAL_MAX];

  for (iproc = 0; iproc < nprocs; iproc++)
   wall[iproc] = compat_time();

  i64_t nread = 0;
  i64_t nwrite = -1;
  int length_max = 0;

  i64_t nprobes = 0;
  i64_t nmessages = 0;

  int locked = FALSE;

  while(TRUE)
  {
    MPI_Status status;

    int flag;

    MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, arg_comm, &flag, &status);

    ++nprobes;

    if ((nprobes % 1000) == 0) PRINTF("%s::nprobes=%lld\n$", __FUNC__, nprobes);

    if (!flag)
    {
      compat_sleep(MILLI_SECOND);

      continue;
    }

    ++nmessages;

    int source = status.MPI_SOURCE;
    int tag = status.MPI_TAG;

    PRINTF("%s::nmessages=%lld RECEIVING FROM source=%d tag=%s\n$",
           __FUNC__, nmessages, source, tag2string[tag]);

    MPI_Recv(NULL, 0, MPI_INT, status.MPI_SOURCE, status.MPI_TAG, arg_comm,
             MPI_STATUS_IGNORE);

    PRINTF("%s::nmessages=%lld RECEIVED tag=%s FROM source=%d\n$",
           __FUNC__, nmessages, tag2string[tag], source);

    double now = compat_time();

    wall[source] = now;

    if (tag == TAG_REQUEST_LOCK)
    {
      int length = nwrite - nread + 1;

      HARDBUG(length >= LENGTH_MAX)

      nwrite++;
      
      queue[nwrite % LENGTH_MAX] = source;

      length++;

      if (length > length_max) length_max = length;

      PRINTF("%s::nmessages=%lld enqueue source=%d"
             " nread=%lld nwrite=%lld length=%d length_max=%d\n$",
             __FUNC__, nmessages, source, nread, nwrite, length, length_max);
    }
    else if (tag == TAG_RELEASE_LOCK)
    {
      HARDBUG(!locked)
   
      locked = FALSE;

      PRINTF("%s::RELEASED LOCK\n$", __FUNC__);
    }
    else if (tag == TAG_FLUSH_LOCK)
    {
      ++ndone;

      PRINTF("%s::FLUSHED LOCK ndone=%d\n$", __FUNC__, ndone);

      if (ndone == (nprocs - 1))
      {
        PRINTF("%s::nmessages=%lld nread=%lld nwrite=%lld length_max=%d\n$",
               __FUNC__, nmessages, nread, nwrite, length_max);

        HARDBUG(nread <= nwrite)
   
        break;
      }
    }
    else
    {
      PRINTF("tag=%d\n$", tag);   
 
      FATAL("unknown tag", EXIT_FAILURE)
    }

    if ((nread <= nwrite) and !locked)
    {
      source = queue[nread % LENGTH_MAX];

      tag = TAG_LOCK_GRANTED;

      PRINTF("%s::nmessages=%lld SENDING tag=%s TO source=%d\n$",
             __FUNC__, nmessages, tag2string[tag], source);

      MPI_Send(NULL, 0, MPI_INT, source, tag, arg_comm);

      PRINTF("%s::nmessages=%lld SENT tag=%s TO source=%d\n$",
             __FUNC__, nmessages, tag2string[tag], source);

      locked = TRUE;

      PRINTF("%s::LOCKED\n$", __FUNC__);

      nread++;

      int length = nwrite - nread + 1;

      PRINTF("%s::nmessages=%lld dequeue"
             " nread=%lld nwrite=%lld length=%d\n$",
             __FUNC__, nmessages, nread, nwrite, length);
    }

    int ilatency_max = INVALID;

    double latency_max = 0.0;

    for (iproc = 0; iproc < nprocs; iproc++)
    {
      double latency = now - wall[iproc];
      if (latency > latency_max) ilatency_max = iproc;
    }

    if (ilatency_max != INVALID)
      PRINTF("max latency=%.0f for process %d\n", latency_max, ilatency_max);
  }
#endif
}

local int my_semop(int arg_semid, int arg_sem_num, short arg_op,
  short arg_flg)
{
#ifdef USE_OPENMPI
  struct sembuf sops;

  sops.sem_num = arg_sem_num;
  sops.sem_op = arg_op;
  sops.sem_flg = arg_flg;

  return(semop(arg_semid, &sops, 1));
#else
  return(0);
#endif
}

void my_mpi_acquire_semaphore_v3(MPI_Comm arg_comm, int arg_semkey)
{
#ifdef USE_OPENMPI

  if (arg_comm == MPI_COMM_NULL) return;

  int nprocs;

  MPI_Comm_size(arg_comm, &nprocs);

  if (nprocs <= 1) return;

  int iproc;

  MPI_Comm_rank(arg_comm, &iproc);

  progress_t progress;

  construct_progress(&progress, 0, 10);

  int semid;

  HARDBUG((semid = semget(arg_semkey, NSEMS, 0600)) == -1);

  PRINTF("%s::acquiring lock iproc=%d arg_semkey=%d semid=%d\n$",
         __FUNC__, iproc, arg_semkey, semid);

  while (TRUE)
  {
    if (my_semop(semid, 0, -1, IPC_NOWAIT) == 0) break;
 
    update_progress(&progress);

    compat_sleep(MILLI_SECOND);
  }

  finalize_progress(&progress);

  PRINTF("%s::acquired lock iproc=%d\n$", __FUNC__, iproc);
#endif
}

void my_mpi_release_semaphore_v3(MPI_Comm arg_comm, int arg_semkey)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int nprocs;

  MPI_Comm_size(arg_comm, &nprocs);

  if (nprocs <= 1) return;

  int iproc;

  MPI_Comm_rank(arg_comm, &iproc);

  int semid;

  HARDBUG((semid = semget(arg_semkey, NSEMS, 0600)) == -1);

  HARDBUG(my_semop(semid, 0, 1, 0) != 0)

  PRINTF("%s::released lock iproc=%d arg_semkey=%d semid=%d\n$",
         __FUNC__, iproc, arg_semkey, semid);
#endif
}

void my_mpi_barrier_v3(char *arg_info, MPI_Comm arg_comm, 
  int arg_semkey, int arg_verbose)
{
#ifdef USE_OPENMPI
  if (arg_comm == MPI_COMM_NULL) return;

  int nprocs;

  MPI_Comm_size(arg_comm, &nprocs);

  HARDBUG(nprocs < 1)

  if (nprocs == 1) return;

  int iproc;

  MPI_Comm_rank(arg_comm, &iproc);

  int semid;

  HARDBUG((semid = semget(arg_semkey, NSEMS, 0600)) == -1)

  if (arg_verbose)
    PRINTF("%s::entering barrier %s nprocs=%d iproc=%d"
           " arg_semkey=%d semid=%d..\n$",
           __FUNC__, arg_info, nprocs, iproc, arg_semkey, semid);

  double t0 = compat_time();

  //arrival

  HARDBUG(my_semop(semid, SEM_MUTEX, -1, 0) != 0)

  int count = my_sem_getval(semid, SEM_COUNT);

  ++count;

  my_sem_setval(semid, SEM_COUNT, count);

  if (count == nprocs)
  {
    //close second gate

    HARDBUG(my_semop(semid, SEM_EXIT_GATE, -1, 0) != 0)

    //open first gate

    HARDBUG(my_semop(semid, SEM_ENTRY_GATE, +1, 0) != 0)
  }

  HARDBUG(my_semop(semid, SEM_MUTEX, +1, 0) != 0)

  //pass the baton

  HARDBUG(my_semop(semid, SEM_ENTRY_GATE, -1, 0) != 0)

  HARDBUG(my_semop(semid, SEM_ENTRY_GATE, +1, 0) != 0)

  //departure

  HARDBUG(my_semop(semid, SEM_MUTEX, -1, 0) != 0)

  count = my_sem_getval(semid, SEM_COUNT);

  --count;

  my_sem_setval(semid, SEM_COUNT, count);

  if (count == 0)
  {
    //close the first gate

    HARDBUG(my_semop(semid, SEM_ENTRY_GATE, -1, 0) != 0)

    //open the second gate

    HARDBUG(my_semop(semid, SEM_EXIT_GATE, +1, 0) != 0)
  }

  HARDBUG(my_semop(semid, SEM_MUTEX, +1, 0) != 0)

  //pass the baton

  HARDBUG(my_semop(semid, SEM_EXIT_GATE, -1, 0) != 0)

  HARDBUG(my_semop(semid, SEM_EXIT_GATE, +1, 0) != 0)

  double t1 = compat_time();

  double wall = t1 - t0;

  if (arg_verbose)
    PRINTF("%s::..leaving barrier %s wall=%.0f seconds\n$",
           __FUNC__, arg_info, wall);

#endif
}

#define SIZE 1024

#define NMESSAGES 10

void test_my_mpi(void)
{
  my_random_t test_random;

  construct_my_random(&test_random, INVALID);

  ui64_t seed = return_my_random(&test_random);

  PRINTF("seed=%llX\n", seed);

  MPI_Win win;
  int *shared_data;

  my_mpi_win_allocate_shared(my_mpi_globals.MMG_id_global == 0 ?
    SIZE * sizeof(int) : 0, sizeof(int), MPI_INFO_NULL,
    my_mpi_globals.MMG_comm_global, &shared_data, &win);

  my_mpi_win_fence(my_mpi_globals.MMG_comm_global, 0, win);

  int disp_unit;

  MPI_Aint ssize;

  void *baseptr;

  my_mpi_win_shared_query(win, 0, &ssize, &disp_unit, &baseptr);

  shared_data = (int *)baseptr;

  my_mpi_win_fence(my_mpi_globals.MMG_comm_global, 0, win);

  if (my_mpi_globals.MMG_id_global == 0)
  {
    for (int i = 0; i < SIZE; i++) shared_data[i] = i;
  }

  my_mpi_win_fence(my_mpi_globals.MMG_comm_global, 0, win);

  for (int i = 0; i < SIZE; i++) HARDBUG(shared_data[i] != i)

  my_mpi_win_free(&win);

  my_mpi_barrier("after sharded query", my_mpi_globals.MMG_comm_global,
                 TRUE);

  PRINTF("Process %d is testing semaphores..\n$",
         my_mpi_globals.MMG_id_global);

  for (i64_t imessage = 1; imessage <= NMESSAGES; ++imessage)
  {
    compat_sleep(MILLI_SECOND);
    
    my_mpi_acquire_semaphore_v3(my_mpi_globals.MMG_comm_global,
                                SEMKEY_TEST_MY_MPI);
    
    PRINTF("Process %d acquired semaphore\n$", my_mpi_globals.MMG_id_global);
  
    compat_sleep(MILLI_SECOND);
    
    my_mpi_release_semaphore_v3(my_mpi_globals.MMG_comm_global,
                                SEMKEY_TEST_MY_MPI);
 
    PRINTF("Process %d released semaphore\n$", my_mpi_globals.MMG_id_global);
  }

  //compat_sleep(my_mpi_globals.MMG_id_global + MILLI_SECOND);

  my_mpi_barrier_v3("after semaphores", my_mpi_globals.MMG_comm_global,
                    SEMKEY_TEST_MY_MPI_BARRIER, TRUE);
}

