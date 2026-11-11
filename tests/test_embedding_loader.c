/* Exercise the actual matrix loader with ordinary allocations in place of MPI. */
#include "../networks.c"
#include <assert.h>

int zzzzzz_invocation;
my_mpi_globals_t my_mpi_globals;

_Noreturn void zzzzzz(char *file, const char *func, long line,
                     char *message, int code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", file, line, func, message);
  exit(code);
}

void *my_malloc(char *name, char *file, const char *func, int line, size_t size)
{
  (void)name; (void)file; (void)func; (void)line;
  return malloc(size);
}
void my_free(void *pointer) { free(pointer); }
void register_heap(char *name, char *file, const char *func, int line,
                   void *pointer, size_t size, int allocated, int readonly)
{
  (void)name; (void)file; (void)func; (void)line;
  (void)pointer; (void)size; (void)allocated; (void)readonly;
}
void my_mpi_allocate_shared(MPI_Comm comm, size_t size, size_t count,
                            void **pointer, MPI_Win *win)
{
  (void)comm;
  *pointer = malloc(size * count);
  assert(*pointer != NULL);
  memset(*pointer, 0xa5, size * count); /* Detect missing zero filling. */
  *win = 0;
}
void my_mpi_sync_shared(MPI_Comm comm, MPI_Win win, char *name)
{
  (void)comm; (void)win; (void)name;
}

int main(int argc, char **argv)
{
  (void)argc; (void)argv;
  init_base64_table();
  cJSON *csv = cJSON_CreateObject();
  cJSON_AddNumberToObject(csv, CJSON_FILE_ROWS_ID, 2);
  cJSON_AddNumberToObject(csv, CJSON_FILE_COLS_ID, 3);
  /* Two binary32 rows: [1, 2, 3], [4, 5, 6]. */
  cJSON_AddStringToObject(csv, CJSON_FILE_DATA_ID,
                         "AACAPwAAAEAAAEBAAACAQAAAoEAAAMBA");
  for (int iorientation = 1; iorientation <= 3; iorientation++)
  {
    int nrows = INVALID;
    int ncols = INVALID;
    scaled_double_t *rows = NULL, *columns = NULL;
    read_2d_csv_base64(csv, &nrows, &ncols,
                       iorientation & 1 ? &rows : NULL,
                       iorientation & 2 ? &columns : NULL);
    assert(nrows == 2 && ncols == 3);
    for (int irow = 0; irow < nrows; irow++)
    {
      for (int icol = 0; icol < ncols; icol++)
      {
        float expected = 1 + 3 * irow + icol;
        if (rows != NULL) assert(rows[irow * ncols + icol] == expected);
        if (columns != NULL) assert(columns[icol * nrows + irow] == expected);
      }
    }
    free(rows);
    free(columns);
  }
  cJSON_Delete(csv);
  puts("Contiguous rows, transposed columns and both orientations PASS");
  return 0;
}
