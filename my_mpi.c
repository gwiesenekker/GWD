#include "globals.h"

//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST

my_mpi_globals_t my_mpi_globals =
{
  .MY_MPIG_NGLOBAL_MAX = 32,
  .MY_MPIG_BARRIER_TAG = 1,

  .MY_MPIG_init = FALSE,
  .MY_MPIG_nglobal = 0,
  .MY_MPIG_id_global = INVALID,
  .MY_MPIG_nslaves = 0,
#ifdef USE_OPENMPI
  .MY_MPIG_id_slave = INVALID,
  .MY_MPIG_comm_global = MPI_COMM_NULL,
  .MY_MPIG_comm_slaves = MPI_COMM_NULL
#else
  .MY_MPIG_mpi_id_slave = INVALID
#endif
};

#ifdef USE_OPENMPI

local const double BARRIER_POLL = DECI_SECOND;
local const double PROBE_POLL = DECI_SECOND;

void my_mpi_barrier(char *info, MPI_Comm comm, int verbose)
{
  int ncomm;
  MPI_Comm_size(comm, &ncomm);
  HARDBUG(ncomm < 1)

  if (ncomm == 1) return;

  int comm_id;
  MPI_Comm_rank(comm, &comm_id);

  if (verbose) PRINTF("entering my_mpi_barrier %s ncomm=%d comm_id=%d\n",
    info, ncomm, comm_id);

  int done[my_mpi_globals.MY_MPIG_NGLOBAL_MAX];
  double wall[my_mpi_globals.MY_MPIG_NGLOBAL_MAX];

  for (int icomm = 0; icomm < ncomm; icomm++)
  {
    done[icomm] = FALSE;
    wall[icomm] = 0.0;
  }

  double t0 = my_time();

  for (int icomm = 0; icomm < ncomm; icomm++)
  {
    MPI_Request request;
    int flag;
    MPI_Status status;

    if (icomm == comm_id) continue;

    if (verbose)
      PRINTF("my_mpi_barrier sending from %d to %d\n",
        comm_id, icomm);

    MPI_Isend(&comm_id, 1, MPI_INT, icomm, my_mpi_globals.MY_MPIG_BARRIER_TAG,
              comm, &request);

    while(TRUE)
    {
      MPI_Test(&request, &flag, &status);
      if (flag) break;
    }

    MPI_Test_cancelled(&status, &flag);
    HARDBUG(flag)
  }

  while(TRUE)
  {
    int icomm;
    for (icomm = 0; icomm < ncomm; icomm++)
    {
      if (icomm == comm_id) continue;
      if (!done[icomm]) break;
    }
    if (icomm == ncomm) break;

    int flag;
    MPI_Status status;
    MPI_Iprobe(MPI_ANY_SOURCE, my_mpi_globals.MY_MPIG_BARRIER_TAG,
               comm, &flag, &status);

    if (!flag)
    {
      my_sleep(BARRIER_POLL);
      continue;
    }

    HARDBUG(status.MPI_TAG != my_mpi_globals.MY_MPIG_BARRIER_TAG)

    int source = status.MPI_SOURCE;

    HARDBUG(source < 0)
    HARDBUG(source >= ncomm)
    HARDBUG(source == comm_id)

    HARDBUG(done[source])

    int receive;
    MPI_Recv(&receive, 1, MPI_INT, source, my_mpi_globals.MY_MPIG_BARRIER_TAG,
             comm, &status);

    HARDBUG(status.MPI_SOURCE != source)
    HARDBUG(status.MPI_TAG != my_mpi_globals.MY_MPIG_BARRIER_TAG)
    HARDBUG(receive != source)

    if (verbose)
      PRINTF("my_mpi_barrier receiving from %d\n", source);

    double t1 = my_time();

    done[source] = TRUE;
    wall[source] = t1 - t0;
  }

  if (verbose) PRINTF("leaving my_mpi_barrier %s\n", info);

  double wall_max = 0.0;

  for (int icomm = 0; icomm < ncomm; icomm++)
  {  
    if (verbose)
    {
      PRINTF("my_mpi_barrier icomm=%d time=%.0f%s\n",
        icomm, wall[icomm], icomm == comm_id ? " *" : "");
    }
    if (wall[icomm] > wall_max) wall_max = wall[icomm];
  }

  //to be sure

  MPI_Barrier(comm);

  MPI_Allreduce(MPI_IN_PLACE, &wall_max, 1, MPI_DOUBLE, MPI_MAX, comm);
  
  if (verbose) PRINTF("my_mpi_barrier wall_max=%.0f\n", wall_max);
}

void my_mpi_probe(MPI_Comm comm)
{
  while(TRUE)
  {
     int flag;
     MPI_Status status;

     MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &flag, &status);

    if (flag) break;

     my_sleep(PROBE_POLL);
  }
}

void my_mpi_acquire_semaphore(MPI_Win win)
{
  int one = 1;
  int lock_acquired = 0;

  while(TRUE)
  {
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);

    MPI_Fetch_and_op(&one, &lock_acquired, MPI_INT, 0, 0, MPI_NO_OP, win);

    if (lock_acquired == 0)
    {
      MPI_Accumulate(&one, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, win);

      MPI_Win_unlock(0, win);

      break;
    }

    MPI_Win_unlock(0, win);

    my_sleep(0.1);
  }
}

void my_mpi_release_semaphore(MPI_Win win)
{
  int zero = 0;

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, win);

  MPI_Put(&zero, 1, MPI_INT, 0, 0, 1, MPI_INT, win);

  MPI_Win_unlock(0, win);
}

#define SIZE 1024

void test_mpi(void)
{
  ui64_t seed = randull(INVALID);

  PRINTF("seed=%llX\n", seed);

  MPI_Win win;
  int *shared_data;

  MPI_Win_allocate_shared(my_mpi_globals.MY_MPIG_id_global == 0 ?
    SIZE * sizeof(int) : 0, sizeof(int), MPI_INFO_NULL,
    my_mpi_globals.MY_MPIG_comm_global, &shared_data, &win);

  MPI_Win_fence(0, win);

  int disp_unit;

  MPI_Aint ssize;

  void *baseptr;

  MPI_Win_shared_query(win, 0, &ssize, &disp_unit, &baseptr);

  shared_data = (int *)baseptr;

  MPI_Win_fence(0, win);

  if (my_mpi_globals.MY_MPIG_id_global == 0)
  {
    for (int i = 0; i < SIZE; i++) shared_data[i] = i;
  }

  MPI_Win_fence(0, win);

  for (int i = 0; i < SIZE; i++) HARDBUG(shared_data[i] != i)

  MPI_Win_free(&win);

  my_mpi_barrier("after sharded query", my_mpi_globals.MY_MPIG_comm_global,
                 TRUE);

  int *semaphore;

  MPI_Win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL,
                   my_mpi_globals.MY_MPIG_comm_global, &semaphore, &win);

  if (my_mpi_globals.MY_MPIG_id_global == 0) *semaphore = 0;

  MPI_Win_fence(0, win);

  PRINTF("Process %d is testing semaphores\n",
         my_mpi_globals.MY_MPIG_id_global);

  my_sleep(0.1 + 0.1 * (randull(0) % my_mpi_globals.MY_MPIG_nglobal));

  my_mpi_acquire_semaphore(win);

  PRINTF("Process %d acquired semaphore\n", my_mpi_globals.MY_MPIG_id_global);

  my_sleep(0.1 + 0.1 * (randull(0) % my_mpi_globals.MY_MPIG_nglobal));

  my_mpi_release_semaphore(win);

  PRINTF("Process %d released semaphore\n", my_mpi_globals.MY_MPIG_id_global);

  MPI_Win_fence(0, win);

  MPI_Win_free(&win);

  my_mpi_barrier("after semaphores", my_mpi_globals.MY_MPIG_comm_global, TRUE);
}

#endif

