// SCU REVISION 8.1286 di 17 nov 2026 23:39:48 CET
#ifndef MpiH
#define MpiH

enum
{
  MY_MPI_NGLOBAL_MAX = 128
};

#ifdef USE_OPENMPI

#include <mpi.h>

#else

typedef void *MPI_Comm;
typedef void *MPI_Info;
typedef void *MPI_Datatype;
typedef void *MPI_Op;
typedef ptrdiff_t MPI_Aint;
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
#define MPI_MIN NULL

#define MPI_WIN_NULL NULL

#endif

typedef struct
{
  int MMG_init;
  int MMG_nglobal;
  int MMG_global_id;
  int MMG_nslaves;
  int MMG_slave_id;

  MPI_Comm MMG_global_comm;
  MPI_Comm MMG_slave_comm;

  int MMG_is_master;
  int MMG_is_single;
  int MMG_has_slaves;
  int MMG_is_slave;
  int MMG_is_lead_slave;

  int MMG_semkey_offset;

  int MMG_gen_book_barrier_semkey;
  int MMG_init_endgame_barrier_semkey;
  int MMG_gen_random_barrier_semkey;
  int MMG_main_barrier_semkey;
  int MMG_test_my_mpi_semkey;
  int MMG_test_my_mpi_barrier_semkey;
  int MMG_flush_sql_buffer_semkey;
  int MMG_construct_my_sqlite3_barrier_semkey;
  int MMG_close_my_sqlite3_semkey;
  int MMG_close_my_sqlite3_barrier_semkey;
  int MMG_auto_play_barrier_semkey;
  int MMG_gen_db_barrier_semkey;
  int MMG_update_db_barrier_semkey;
  int MMG_add_positions_barrier_semkey;
  int MMG_query_positions_barrier_semkey;
  int MMG_fen2csv_barrier_semkey;
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
void my_mpi_sync_shared(MPI_Comm, MPI_Win, const char *);

int my_mpi_win_free(MPI_Win *);

int my_mpi_finalize(void);
int my_mpi_abort(MPI_Comm, int);

void my_mpi_barrier(const char *, MPI_Comm, int);

void my_mpi_acquire_semaphore(MPI_Comm, MPI_Win);
void my_mpi_release_semaphore(MPI_Comm, MPI_Win);

void my_mpi_acquire_semaphore_v2(MPI_Comm);
void my_mpi_release_semaphore_v2(MPI_Comm);
void my_mpi_flush_semaphore_v2(MPI_Comm);
void my_mpi_semaphore_server_v2(MPI_Comm);

int my_mpi_construct_semaphore_v3(MPI_Comm, int);
void my_mpi_acquire_semaphore_v3(MPI_Comm, int);
void my_mpi_release_semaphore_v3(MPI_Comm, int);

void my_mpi_barrier_v3(const char *, MPI_Comm, int, int);
void my_mpi_barrier_v4(const char *, MPI_Comm, int, int);

void test_my_mpi(void);

#endif
