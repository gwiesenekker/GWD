//SCU REVISION 7.661 vr 11 okt 2024  2:21:18 CEST
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
void my_mpi_acquire_semaphore(MPI_Win);
void my_mpi_release_semaphore(MPI_Win);
void test_mpi(void);
#endif

#endif
