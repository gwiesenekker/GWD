//SCU REVISION 7.851 di  8 apr 2025  7:23:10 CEST
#ifndef EndgameH
#define EndgameH

#define ARRAY_PAGE_SIZE 1024
#define NPAGE_ENTRIES 256

#define NENDGAME_MAX 7
#define MATE_MAX     512
#define MATE_DRAW    (-1)
#define MATE_INVALID (-32768)

#define ENDGAME_UNKNOWN 32767

typedef struct
{
  i8_t entry_white_mate;
  i8_t entry_black_mate;
} entry_i8_t;

typedef struct
{
  i16_t entry_white_mate;
  i16_t entry_black_mate;
} entry_i16_t;

void init_endgame(void);
int read_endgame(search_t *, int, int *);
void fin_endgame(void);
void recompress_endgame(char *, int, int);

#endif

