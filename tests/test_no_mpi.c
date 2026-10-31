#include "../my_mpi.c"
#include <assert.h>

#ifdef USE_OPENMPI
#error This test must be built without OpenMPI.
#endif

int zzzzzz_invocation;
_Noreturn void zzzzzz(char *file, const char *func, long line, char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

/* Isolate the wrapper from the tracked allocator; engine smoke tests use it. */
void *my_malloc(char *name, char *file, const char *func, int line, size_t size)
{
  (void)name; (void)file; (void)func; (void)line;
  return compat_aligned_alloc(64, size);
}

int main(int argc, char **argv)
{
  my_mpi_init(&argc, &argv, 0);
  assert(!my_mpi_globals.MMG_init);
  assert(my_mpi_globals.MMG_nglobal == 1);
  assert(my_mpi_globals.MMG_id_global == 0);
  assert(my_mpi_globals.MMG_is_single && my_mpi_globals.MMG_is_master);
  assert(!my_mpi_globals.MMG_has_slaves && !my_mpi_globals.MMG_is_slave);
  int size = -1, rank = -1;
  assert(my_mpi_comm_size(MPI_COMM_WORLD, &size) == MPI_SUCCESS && size == 1);
  assert(my_mpi_comm_rank(MPI_COMM_WORLD, &rank) == MPI_SUCCESS && rank == 0);
  void *data = NULL;
  MPI_Win win;
  my_mpi_allocate_shared(MPI_COMM_WORLD, sizeof(int), 257, &data, &win);
  assert(data != NULL && (uintptr_t)data % 64 == 0 && win == MPI_WIN_NULL);
  int *cells = data;
  for (int i = 0; i < 257; ++i)
    cells[i] = i;
  my_mpi_sync_shared(MPI_COMM_WORLD, win, "test");
  assert(my_mpi_allreduce(MPI_IN_PLACE, cells, 257, MPI_INT, MPI_SUM,
                         MPI_COMM_WORLD) == MPI_SUCCESS);
  for (int i = 0; i < 257; ++i)
    assert(cells[i] == i);
  compat_aligned_free(data);
  test_my_mpi(); /* Must safely do nothing without MPI. */
  assert(my_mpi_finalize() == MPI_SUCCESS);
  puts("no-MPI wrapper tests passed");
  return 0;
}
