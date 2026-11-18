/* Exercise the real DEBUG name stack as well as game-state ownership. */
#include "../states.h"
#include "../my_malloc.c"
#include <assert.h>

void *STDOUT;
int zzzzzz_invocation;
void my_printf(my_printf_t *arg_output, const char *arg_format, ...)
{
  (void)arg_output;
  (void)arg_format;
}
void zzzzzz(const char *arg_file,
            const char *arg_func,
            long arg_line,
            const char *arg_message,
            int arg_code)
{
  fprintf(stderr, "%s:%ld %s: %s\n", arg_file, arg_line, arg_func, arg_message);
  exit(arg_code);
}

int main(int argc, char **argv)
{
  assert(argc == 2);
  assert(compat_mutex_init(&my_names_mutex) == 0);

  PUSH_NAME(__FUNC__)

  game_state_t original, loaded;

  construct_game_state(&original);
  construct_game_state(&loaded);

  BSTRING(bexpected)
  BSTRING(bcomment)

  /* Ordinary small states still round-trip. */
  original.push_move(&original, "32-28", NULL);
  original.save(&original, argv[1]);
  loaded.load(&loaded, argv[1]);

  assert(strcmp(original.get_game_state(&original),
                loaded.get_game_state(&loaded)) == 0);

  for (int imove = 0; imove < 600; imove++)
    original.push_move(&original, "28-23", "score=0 depth=20");

  assert(strlen(original.get_game_state(&original)) > 8192);

  /* One JSON string crosses multiple old fgets chunk boundaries. */
  for (int ichar = 0; ichar < 20000; ichar++)
    HARDBUG(bconchar(bcomment, 'a' + ichar % 26) == BSTR_ERR)
  HARDBUG(bcatcstr(bcomment, "\"\\\n\t") == BSTR_ERR)

  original.push_move(&original, "23-18", bdata(bcomment));

  HARDBUG(bassigncstr(bexpected, original.get_game_state(&original)) ==
          BSTR_ERR)

  printf("Serialized game: %d bytes, 602 moves\n", blength(bexpected));

  for (int iload = 0; iload < 3; iload++)
  {
    original.save(&original, argv[1]);
    loaded.load(&loaded, argv[1]);

    assert(strcmp(bdata(bexpected), loaded.get_game_state(&loaded)) == 0);
    assert(cJSON_GetArraySize(loaded.get_moves(&loaded)) == 602);
  }

  /* Also accept formatted multi-line JSON, CRLF and no final newline. */
  char *formatted = cJSON_Print(original.cjson_object);

  assert(formatted != NULL);

  FILE *file = fopen(argv[1], "wb");

  assert(file != NULL);

  for (int ichar = 0; formatted[ichar] != '\0'; ichar++)
  {
    if (formatted[ichar] == '\n')
      assert(fputc('\r', file) != EOF);
    assert(fputc(formatted[ichar], file) != EOF);
  }

  assert(fclose(file) == 0);

  cJSON_free(formatted);
  loaded.load(&loaded, argv[1]);

  assert(strcmp(bdata(bexpected), loaded.get_game_state(&loaded)) == 0);

  /* Parsing a state's own serialization must preserve its lifetime. */
  loaded.set_game_state(&loaded, loaded.get_game_state(&loaded));

  assert(strcmp(bdata(bexpected), loaded.get_game_state(&loaded)) == 0);

  loaded.clear_moves(&loaded);

  assert(strlen(loaded.get_game_state(&loaded)) < 8192);

  BDESTROY(bcomment)

  BDESTROY(bexpected)

  destroy_game_state(&loaded);

  destroy_game_state(&original);

  POP_NAME(__FUNC__)

  assert(print_all_name_stacks() == 0);
  assert(remove(argv[1]) == 0);

  puts("Game-state tests passed; DEBUG temporary stack empty");

  return (0);
}
