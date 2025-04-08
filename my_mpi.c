#include "globals.h"

//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST

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

local const double BARRIER_POLL = CENTI_SECOND;
local const double PROBE_POLL = CENTI_SECOND;

void my_mpi_barrier(char *arg_info, MPI_Comm arg_comm, int arg_verbose)
{
  if (arg_comm == MPI_COMM_NULL) return;

  int ncomm;

  MPI_Comm_size(arg_comm, &ncomm);

  HARDBUG(ncomm < 1)

  if (ncomm == 1) return;

  int comm_id;

  MPI_Comm_rank(arg_comm, &comm_id);

  if (arg_verbose) PRINTF("entering my_mpi_barrier %s ncomm=%d comm_id=%d\n",
    arg_info, ncomm, comm_id);

  int done[my_mpi_globals.MY_MPIG_NGLOBAL_MAX];

  double wall[my_mpi_globals.MY_MPIG_NGLOBAL_MAX];

  for (int icomm = 0; icomm < ncomm; icomm++)
  {
    done[icomm] = FALSE;

    wall[icomm] = 0.0;
  }

  double t0 = compat_time();

  for (int icomm = 0; icomm < ncomm; icomm++)
  {
    MPI_Request request;

    MPI_Status status;

    if (icomm == comm_id) continue;

    if (arg_verbose)
      PRINTF("my_mpi_barrier sending from %d to %d\n",
        comm_id, icomm);

    MPI_Isend(&comm_id, 1, MPI_INT, icomm, my_mpi_globals.MY_MPIG_BARRIER_TAG,
              arg_comm, &request);

    int error = MPI_Wait(&request, &status);

    if (error != MPI_SUCCESS)
    {
      char error_string[MPI_MAX_ERROR_STRING];

      int error_length;

      MPI_Error_string(error, error_string, &error_length);

      PRINTF("MPI_Wait error=%s\n", error_string);

      FATAL("MPI_Wait", EXIT_FAILURE)
    }
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
               arg_comm, &flag, &status);

    if (!flag)
    {
      compat_sleep(BARRIER_POLL);

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
             arg_comm, &status);

    HARDBUG(status.MPI_SOURCE != source)

    HARDBUG(status.MPI_TAG != my_mpi_globals.MY_MPIG_BARRIER_TAG)

    HARDBUG(receive != source)

    if (arg_verbose)
      PRINTF("my_mpi_barrier receiving from %d\n", source);

    double t1 = compat_time();

    done[source] = TRUE;

    wall[source] = t1 - t0;
  }

  if (arg_verbose) PRINTF("leaving my_mpi_barrier %s\n", arg_info);

  double wall_max = 0.0;

  for (int icomm = 0; icomm < ncomm; icomm++)
  {  
    if (arg_verbose)
    {
      PRINTF("my_mpi_barrier icomm=%d time=%.0f%s\n",
        icomm, wall[icomm], icomm == comm_id ? " *" : "");
    }
    if (wall[icomm] > wall_max) wall_max = wall[icomm];
  }

  //to be sure

  MPI_Barrier(arg_comm);

  my_mpi_allreduce(MPI_IN_PLACE, &wall_max, 1, MPI_DOUBLE, MPI_MAX, arg_comm);
  
  if (arg_verbose) PRINTF("my_mpi_barrier wall_max=%.0f\n", wall_max);
}

void my_mpi_probe(MPI_Comm arg_comm)
{
  while(TRUE)
  {
     int flag;

     MPI_Status status;

     MPI_Iprobe(MPI_ANY_SOURCE, MPI_ANY_TAG, arg_comm, &flag, &status);

    if (flag) break;

     compat_sleep(PROBE_POLL);
  }
}

void my_mpi_acquire_semaphore(MPI_Comm arg_comm, MPI_Win arg_win)
{
  if (arg_comm == MPI_COMM_NULL) return;

  int one = 1;

  int lock_acquired = 0;

  while(TRUE)
  {
    MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, arg_win);

    MPI_Fetch_and_op(&one, &lock_acquired, MPI_INT, 0, 0, MPI_NO_OP, arg_win);

    if (lock_acquired == 0)
    {
      MPI_Accumulate(&one, 1, MPI_INT, 0, 0, 1, MPI_INT, MPI_REPLACE, arg_win);

      MPI_Win_unlock(0, arg_win);

      break;
    }

    MPI_Win_unlock(0, arg_win);

    compat_sleep(MILLI_SECOND);
  }
}

void my_mpi_release_semaphore(MPI_Comm arg_comm, MPI_Win arg_win)
{
  if (arg_comm == MPI_COMM_NULL) return;

  int zero = 0;

  MPI_Win_lock(MPI_LOCK_EXCLUSIVE, 0, 0, arg_win);

  MPI_Put(&zero, 1, MPI_INT, 0, 0, 1, MPI_INT, arg_win);

  MPI_Win_unlock(0, arg_win);
}

int my_mpi_allreduce(const void *arg_sendbuf, void *arg_recvbuf, int arg_count,
                     MPI_Datatype arg_datatype, MPI_Op arg_op, MPI_Comm arg_comm)
{
  if (arg_comm == MPI_COMM_NULL) return(MPI_SUCCESS);

  return(MPI_Allreduce(arg_sendbuf, arg_recvbuf, arg_count, arg_datatype, arg_op, arg_comm));
}

int my_mpi_win_allocate(MPI_Aint arg_size, int arg_disp_unit, MPI_Info arg_info,
                        MPI_Comm arg_comm, void *arg_baseptr, MPI_Win *arg_win)
{
  if (arg_comm == MPI_COMM_NULL) return(MPI_SUCCESS);

  return(MPI_Win_allocate(arg_size, arg_disp_unit, arg_info, arg_comm, arg_baseptr, arg_win));
}

int my_mpi_win_fence(MPI_Comm arg_comm, int arg_assert, MPI_Win arg_win)
{
  if (arg_comm == MPI_COMM_NULL) return(MPI_SUCCESS);

  return(MPI_Win_fence(arg_assert, arg_win));
}

#define SIZE 1024

void test_mpi(void)
{
  my_random_t test_random;

  construct_my_random(&test_random, INVALID);

  ui64_t seed = return_my_random(&test_random);

  PRINTF("seed=%llX\n", seed);

  MPI_Win win;
  int *shared_data;

  MPI_Win_allocate_shared(my_mpi_globals.MY_MPIG_id_global == 0 ?
    SIZE * sizeof(int) : 0, sizeof(int), MPI_INFO_NULL,
    my_mpi_globals.MY_MPIG_comm_global, &shared_data, &win);

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global, 0, win);

  int disp_unit;

  MPI_Aint ssize;

  void *baseptr;

  MPI_Win_shared_query(win, 0, &ssize, &disp_unit, &baseptr);

  shared_data = (int *)baseptr;

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global, 0, win);

  if (my_mpi_globals.MY_MPIG_id_global == 0)
  {
    for (int i = 0; i < SIZE; i++) shared_data[i] = i;
  }

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global, 0, win);

  for (int i = 0; i < SIZE; i++) HARDBUG(shared_data[i] != i)

  MPI_Win_free(&win);

  my_mpi_barrier("after sharded query", my_mpi_globals.MY_MPIG_comm_global,
                 TRUE);

  int *semaphore;

  my_mpi_win_allocate(sizeof(int), sizeof(int), MPI_INFO_NULL,
                      my_mpi_globals.MY_MPIG_comm_global, &semaphore, &win);

  if (my_mpi_globals.MY_MPIG_id_global == 0) *semaphore = 0;

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global, 0, win);

  PRINTF("Process %d is testing semaphores\n",
         my_mpi_globals.MY_MPIG_id_global);

  compat_sleep(0.1 + 0.1 * (return_my_random(&test_random) %
                            my_mpi_globals.MY_MPIG_nglobal));

  my_mpi_acquire_semaphore(my_mpi_globals.MY_MPIG_comm_global, win);

  PRINTF("Process %d acquired semaphore\n", my_mpi_globals.MY_MPIG_id_global);

  compat_sleep(0.1 + 0.1 * (return_my_random(&test_random) %
                        my_mpi_globals.MY_MPIG_nglobal));

  my_mpi_release_semaphore(my_mpi_globals.MY_MPIG_comm_global, win);

  PRINTF("Process %d released semaphore\n", my_mpi_globals.MY_MPIG_id_global);

  my_mpi_win_fence(my_mpi_globals.MY_MPIG_comm_global, 0, win);

  MPI_Win_free(&win);

  my_mpi_barrier("after semaphores", my_mpi_globals.MY_MPIG_comm_global, TRUE);
}

#endif

