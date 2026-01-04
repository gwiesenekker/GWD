//SCU REVISION 8.100 zo  4 jan 2026 13:50:23 CET
// SCU REVISION 8.0108 zo  4 jan 2026 10:07:27 CET
#ifndef MpiH
#define MpiH

#define MY_MPI_NGLOBAL_MAX 128

#ifdef USE_OPENMPI

#include <mpi.h>
// #include <sys/ipc.h>
#include <sys/sem.h>

#define BARRIER_POLL CENTI_SECOND

#else

typedef void *MPI_Comm;
typedef void *MPI_Info;
typedef void *MPI_Datatype;
typedef void *MPI_Op;
typedef int MPI_Aint;
typedef void *MPI_Win;

#define MPI_SUCCESS 0
#define MPI_ERRCODES_IGNORE NULL

#define MPI_COMM_WORLD NULL
#define MPI_COMM_NULL NULL
#define MPI_COMM_SELF NULL

#define MPI_INFO_NULL NULL

#define MPI_IN_PLACE NULL
#define MPI_INT NULL
#define MPI_DOUBLE NULL
#define MPI_LONG_LONG_INT NULL
#define MPI_SUM NULL
#define MPI_MAX NULL

#define MPI_WIN_NULL NULL

#endif

typedef struct
{
  const int MMG_NGLOBAL_MAX;
  const int MMG_BARRIER_TAG;

  int MMG_init;
  int MMG_nglobal;
  int MMG_id_global;
  int MMG_nslaves;
  int MMG_id_slave;

  MPI_Comm MMG_comm_global;
  MPI_Comm MMG_comm_slaves;

  int MMG_semkey_offset;

  int MMG_semkey_gen_book_barrier;
  int MMG_semkey_init_endgame_barrier;
  int MMG_semkey_gen_random_barrier;
  int MMG_semkey_main_barrier;
  int MMG_semkey_test_my_mpi;
  int MMG_semkey_test_my_mpi_barrier;
  int MMG_semkey_flush_sql_buffer;
  int MMG_semkey_construct_my_sqlite3_barrier;
  int MMG_semkey_close_my_sqlite3;
  int MMG_semkey_close_my_sqlite3_barrier;
  int MMG_semkey_gen_db_barrier;
  int MMG_semkey_update_db_barrier;
  int MMG_semkey_add_positions_barrier;
  int MMG_semkey_query_positions_barrier;
} my_mpi_globals_t;

extern my_mpi_globals_t my_mpi_globals;

void my_mpi_init(int *, char ***, int);
int my_mpi_comm_size(MPI_Comm, int *);
int my_mpi_comm_rank(MPI_Comm, int *);

int my_mpi_allreduce(const void *, void *, int, MPI_Datatype, MPI_Op, MPI_Comm);

int my_mpi_win_allocate_shared(MPI_Aint,
                               int,
                               MPI_Info,
                               MPI_Comm,
                               void *,
                               MPI_Win *);
int my_mpi_win_shared_query(MPI_Win, int, MPI_Aint *, int *, void *);
int my_mpi_win_fence(MPI_Comm, int, MPI_Win);
void my_mpi_allocate_shared(MPI_Comm, size_t, size_t, void **, MPI_Win *);
void my_mpi_sync_shared(MPI_Comm, MPI_Win, char *);

int my_mpi_win_free(MPI_Win *);

int my_mpi_finalize(void);
int my_mpi_abort(MPI_Comm, int);

void my_mpi_barrier(char *, MPI_Comm, int);

void my_mpi_acquire_semaphore(MPI_Comm, MPI_Win);
void my_mpi_release_semaphore(MPI_Comm, MPI_Win);

void my_mpi_acquire_semaphore_v2(MPI_Comm);
void my_mpi_release_semaphore_v2(MPI_Comm);
void my_mpi_flush_semaphore_v2(MPI_Comm);
void my_mpi_semaphore_server_v2(MPI_Comm);

int my_mpi_construct_semaphore_v3(MPI_Comm, int);
void my_mpi_acquire_semaphore_v3(MPI_Comm, int);
void my_mpi_release_semaphore_v3(MPI_Comm, int);

void my_mpi_barrier_v3(char *, MPI_Comm, int, int);
void my_mpi_barrier_v4(char *, MPI_Comm, int, int);

void test_my_mpi(void);

#endif
