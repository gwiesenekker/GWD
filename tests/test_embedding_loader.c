/* Exercise the actual matrix loader with ordinary allocations in place of MPI.
 */
#include "../networks.c"
#include <assert.h>

int zzzzzz_invocation;
my_mpi_globals_t my_mpi_globals;

_Noreturn void zzzzzz(const char *arg_file,
                      const char *arg_func,
                      long arg_line,
                      const char *arg_message,
                      int arg_code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", arg_file, arg_line, arg_func, arg_message);
  exit(arg_code);
}

void *my_malloc(const char *arg_name,
                const char *arg_file,
                const char *arg_func,
                int arg_line,
                size_t arg_size)
{
  (void)arg_name;
  (void)arg_file;
  (void)arg_func;
  (void)arg_line;

  return (malloc(arg_size));
}
void my_free(void *arg_pointer) { free(arg_pointer); }
void register_heap(const char *arg_name,
                   const char *arg_file,
                   const char *arg_func,
                   int arg_line,
                   void *arg_pointer,
                   size_t arg_size,
                   int arg_allocated,
                   int arg_readonly)
{
  (void)arg_name;
  (void)arg_file;
  (void)arg_func;
  (void)arg_line;
  (void)arg_pointer;
  (void)arg_size;
  (void)arg_allocated;
  (void)arg_readonly;
}
void my_mpi_allocate_shared(MPI_Comm arg_comm,
                            size_t arg_size,
                            size_t arg_count,
                            void **arg_pointer,
                            MPI_Win *arg_win)
{
  (void)arg_comm;
  *arg_pointer = malloc(arg_size * arg_count);

  assert(*arg_pointer != NULL);

  memset(*arg_pointer,
         0xa5,
         arg_size * arg_count); /* Detect missing zero filling. */
  *arg_win = 0;
}
void my_mpi_sync_shared(MPI_Comm arg_comm,
                        MPI_Win arg_win,
                        const char *arg_name)
{
  (void)arg_comm;
  (void)arg_win;
  (void)arg_name;
}

int main(int argc, char **argv)
{
  (void)argc;
  (void)argv;
  init_base64_table();

  cJSON *csv = cJSON_CreateObject();

  cJSON_AddNumberToObject(csv, CJSON_FILE_ROWS_ID, 2);
  cJSON_AddNumberToObject(csv, CJSON_FILE_COLS_ID, 3);
  /* Two binary32 rows: [1, 2, 3], [4, 5, 6]. */
  cJSON_AddStringToObject(csv,
                          CJSON_FILE_DATA_ID,
                          "AACAPwAAAEAAAEBAAACAQAAAoEAAAMBA");

  for (int iorientation = 1; iorientation <= 3; iorientation++)
  {
    int nrows = INVALID;
    int ncols = INVALID;
    scaled_double_t *rows = NULL, *columns = NULL;

    read_2d_csv_base64(csv,
                       &nrows,
                       &ncols,
                       iorientation & 1 ? &rows : NULL,
                       iorientation & 2 ? &columns : NULL);

    assert((nrows == 2) and (ncols == 3));

    for (int irow = 0; irow < nrows; irow++)
    {
      for (int icol = 0; icol < ncols; icol++)
      {
        float expected = 1 + 3 * irow + icol;

        if (rows != NULL)
          assert(rows[irow * ncols + icol] == expected);

        if (columns != NULL)
          assert(columns[icol * nrows + irow] == expected);
      }
    }
    free(rows);
    free(columns);
  }
  cJSON_Delete(csv);
  puts("Contiguous rows, transposed columns and both orientations PASS");

  return (0);
}
