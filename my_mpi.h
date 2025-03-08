//SCU REVISION 7.809 za  8 mrt 2025  5:23:19 CET
#ifndef MpiH
#define MpiH

#ifdef USE_OPENMPI
#include <mpi.h>
#endif

typedef struct
{
  const int MY_MPIG_NGLOBAL_MAX;
  const int MY_MPIG_BARRIER_TAG;

  int MY_MPIG_init;
  int MY_MPIG_nglobal;
  int MY_MPIG_id_global;
  int MY_MPIG_nslaves;
  int MY_MPIG_id_slave;

#ifdef USE_OPENMPI
  MPI_Comm MY_MPIG_comm_global;
  MPI_Comm MY_MPIG_comm_slaves;
#endif
} my_mpi_globals_t;

extern my_mpi_globals_t my_mpi_globals;

#ifdef USE_OPENMPI
void my_mpi_barrier(char *, MPI_Comm, int);
void my_mpi_probe(MPI_Comm);

void my_mpi_acquire_semaphore(MPI_Comm, MPI_Win);
void my_mpi_release_semaphore(MPI_Comm, MPI_Win);

int my_mpi_allreduce(const void *sendbuf, void *recvbuf, int count,
                     MPI_Datatype datatype, MPI_Op op, MPI_Comm comm);

int my_mpi_win_allocate(MPI_Aint size, int disp_unit, MPI_Info info,
                        MPI_Comm comm, void *baseptr, MPI_Win *win);
int my_mpi_win_fence(MPI_Comm comm, int assert, MPI_Win win);

void test_mpi(void);
#endif

#endif
